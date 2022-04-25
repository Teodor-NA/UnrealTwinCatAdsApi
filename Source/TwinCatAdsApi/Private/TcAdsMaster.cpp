// Fill out your copyright notice in the Description page of Project Settings.


#include "TcAdsMaster.h"
#include "TcAdsVariable.h"
#include "Misc/DefaultValueHelper.h"
#include "TcAdsAPI.h"

// Sets default values
ATcAdsMaster::ATcAdsMaster() :
	_remoteAmsAddress({{0, 0, 0, 0, 0, 0}, 0}),
	_readBufferSize(0),
	_writeBufferSize(0),
	ReadValuesInterval(0.1f),
	WriteValuesInterval(0.1f),
	ReadDataRoundTripTime(0.0f),
	RemoteAmsNetId(TEXT("127.0.0.1.1.1")),
	RemoteAmsPort(851),
	RemoteAmsAddressValid(false)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green,TEXT("Ads master loaded"));

}

// Called when the game starts or when spawned
void ATcAdsMaster::BeginPlay()
{
	Super::BeginPlay();
	
	union {uint32 raw; AdsVersion version;}localVersion;
	localVersion.raw = AdsGetDllVersion();
	LocalDllVersion = FString::Printf(TEXT("%u.%u.%u"),
		localVersion.version.version,
		localVersion.version.revision,
		localVersion.version.build
	);

	RemoteAmsAddressValid = parseAmsAddress(RemoteAmsNetId, RemoteAmsPort, _remoteAmsAddress);

	if (RemoteAmsAddressValid)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green,
				FString::Printf(TEXT("Attempting communication with remote AMS Net Id %u.%u.%u.%u.%u.%u on port %u"),
					_remoteAmsAddress.netId.b[0],
					_remoteAmsAddress.netId.b[1],
					_remoteAmsAddress.netId.b[2],
					_remoteAmsAddress.netId.b[3],
					_remoteAmsAddress.netId.b[4],
					_remoteAmsAddress.netId.b[5],
					_remoteAmsAddress.port
					));
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red,
				FString::Printf(TEXT("Failed to parse '%s' into an AMS Net Id\nAborting ADS communication"), *RemoteAmsNetId));
		}
		
		return;
	}
	
	// Open port
	AdsPort = AdsPortOpenEx();
	
	if (AdsPort)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green, FString::Printf(TEXT("Local port: %d opened"), AdsPort));
	}
	else
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, TEXT("Port opening failed\nAborting ADS communication"));
	
		return;
	}

	AmsAddr localAddress;
	AdsGetLocalAddressEx(AdsPort, &localAddress);
	LocalAmsAddress = FString::Printf(TEXT("%u.%u.%u.%u.%u.%u:%u"),
		localAddress.netId.b[0],
		localAddress.netId.b[1],
		localAddress.netId.b[2],
		localAddress.netId.b[3],
		localAddress.netId.b[4],
		localAddress.netId.b[5],
		localAddress.port
	); 
	
	char remoteAsciiDevName[50];
	AdsVersion remoteVersion;

	auto err = AdsSyncReadDeviceInfoReqEx(AdsPort, &_remoteAmsAddress, remoteAsciiDevName, &remoteVersion);
	
	if (err)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red,
				 FString::Printf( TEXT("Failed to get device info from '%s'\nAborting ADS communication"), *RemoteAmsNetId));
		}
		return;
	}

	RemoteDeviceName = FString(remoteAsciiDevName);
	RemoteAdsVersion = FString::Printf(TEXT("%u.%u.%u"), remoteVersion.version, remoteVersion.revision, remoteVersion.build);
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green,
			 FString::Printf( TEXT("Connected to '%s' using version %u.%u.%u \nStarting ADS communication"),
			 	*RemoteDeviceName,
			 	remoteVersion.version,
			 	remoteVersion.revision,
			 	remoteVersion.build
			 	));
	}

	// Start timers
	if (ReadValuesInterval > 0.0f)
		GetWorldTimerManager().SetTimer(_readValuesTimerHandle,this, &ATcAdsMaster::readValues, ReadValuesInterval, true);

	if (WriteValuesInterval > 0.0f)
		GetWorldTimerManager().SetTimer(_writeValuesTimerHandle,this, &ATcAdsMaster::writeValues, WriteValuesInterval, true);
}

void ATcAdsMaster::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);

	// Stop timers
	GetWorldTimerManager().ClearTimer(_readValuesTimerHandle);
	GetWorldTimerManager().ClearTimer(_writeValuesTimerHandle);

	/// Seems like we don't need to release handles, since we are actually using the index group and offset to access
	/// variables, and therefore handles are not created for us. Might be wrong though, the documentation isn't exactly
	/// clear on this...

	// Clear lists. Need to do this since these are uproperties exposed to the editor and therefore have a weird ass lifespan
	ReadVariableList.Empty();
	WriteVariableList.Empty();
	
	if (AdsPort)
	{
		AdsPortCloseEx(AdsPort);
		AdsPort = 0;
	}
	
}

void ATcAdsMaster::removeVariable(const UTcAdsVariable* variable) // const UTcAdsVariable* Variable, TArray<UTcAdsVariable*>& VarList, TArray<FDataPar>& ReqList, size_t& BufferSize) //, EAdsAccessType Access)
{
	if (!variable)
		return;
	if (!variable->ValidAds)
		return;
	
	switch (variable->Access)
	{
	case EAdsAccessType::None: break;
	case EAdsAccessType::Read:
		removeVariablePrivate(variable, ReadVariableList, _readReqBuffer, _readBufferSize);
		break;
	case EAdsAccessType::Write:
		removeVariablePrivate(variable, WriteVariableList, _writeReqBuffer, _writeBufferSize);
		break;
	case EAdsAccessType::ReadCyclic: break;
	case EAdsAccessType::ReadOnChange: break;
	case EAdsAccessType::WriteOnChange: break;
	default: ;
	}
}

void ATcAdsMaster::checkForNewVars(TArray<UTcAdsVariable*>& vars, TArray<FDataPar>& reqBuffer, size_t& bufferSize) //, EAdsAccessType Access)
{
	// Check for new variables in the list. Since new variables are always added at the end, we check in reverse
	// (so that normally we will check the first entry and immediately step out).
	bool newVar = false;
	int32 newIndex = 0;
	for (auto i = vars.Num(); i--;)
	{
		// Skip variables that are pending kill (this shouldn't happen, but you never know with unreal...)
		if (!IsValid(vars[i]))
			continue;
		
		if (vars[i]->newVar())
		{
			newVar = true;
			newIndex = i;
		}
		else
			break;
	}

	// Nothing to do
	if (!newVar)
		return;

	// If there are any new variables then we must ask ADS for them in forward order, so that the
	// arrays xxxVariableList and xxxReqBuffer_ are synchronized
	for (auto i = newIndex; i < vars.Num(); ++i)
	{
		// Skip variables that are pending kill again
		if (!IsValid(vars[i]))
			continue;

		auto err = vars[i]->getSymbolEntryFromAds(AdsPort, _remoteAmsAddress, reqBuffer);
		if (!err)
		{
			bufferSize += vars[i]->transferSize();
		}
		
		if (GEngine)
		{
			if (err)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Yellow,
					FString::Printf(TEXT("Failed to subscribe to '%s' variable '%s'. Error code 0x%x"),
						AdsAccessTypeName(vars[i]->Access),
						*vars[i]->AdsName,
						err
						));
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green,
					FString::Printf(TEXT("Successfully subscribed to '%s' variable '%s'"),
						AdsAccessTypeName(vars[i]->Access),
						*vars[i]->AdsName
						));
			}
		}
	}
}

bool ATcAdsMaster::parseAmsAddress(const FString& netId, const int32 port, AmsAddr& out)
{
	if ((port > 65535) || (port < 0))
		return false;
	
	AmsAddr tempAddress = {{0, 0,0 ,0, 0,0}, static_cast<uint16>(port)};

	constexpr TCHAR delim = TEXT('.');
	
	TArray<FString> tmp;
	netId.ParseIntoArray(tmp, &delim);

	if (tmp.Num() != 6)
		return false;

	int32 idx = 0;
	for (auto& str : tmp)
	{
		int32 val;
		bool success = FDefaultValueHelper::ParseInt(str, val);
		if ((val > 255) || (val < 0))
			success = false;

		if (!success)
			return false;

		tempAddress.netId.b[idx] = static_cast<uint8>(val);
		++idx;
	}

	out = tempAddress;

	return true;
}

void ATcAdsMaster::removeVariablePrivate(const UTcAdsVariable* variable, TArray<UTcAdsVariable*>& variableList,
	TArray<FDataPar> reqBuffer, size_t& bufferSize)
{
	auto idx = variableList.IndexOfByKey(variable);
	if (idx == INDEX_NONE)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow,
			   FString::Printf(TEXT("No variable '%s' found in list. No action taken"), *variable->AdsName));
		}
		return;
	}

	variableList.RemoveAt(idx);
	reqBuffer.RemoveAt(idx);
	bufferSize -= variable->transferSize();
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green,
		   FString::Printf(TEXT("Unsubscribed variable '%s'"), *variable->AdsName));
	}
}

// Called every frame
void ATcAdsMaster::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

}

void ATcAdsMaster::addVariable(UTcAdsVariable* variable)
{
	if (!variable)
		return;

	switch (variable->Access)
	{
	case EAdsAccessType::None: break;
	case EAdsAccessType::Read:
		ReadVariableList.Add(variable);
		break;
	case EAdsAccessType::Write:
		WriteVariableList.Add(variable);
		break;
	case EAdsAccessType::ReadCyclic: break;
	case EAdsAccessType::ReadOnChange: break;
	case EAdsAccessType::WriteOnChange: break;
	default: ;
	}
	
}

// void ATcAdsMaster::AddReadVariable(UTcAdsVariable* Variable)
// {
// 	if (Variable)
// 		ReadVariableList.Add(Variable);
// }
//
// void ATcAdsMaster::AddWriteVariable(UTcAdsVariable* Variable)
// {
// 	if (Variable)
// 		WriteVariableList.Add(Variable);
// }

void ATcAdsMaster::readValues()
{
	checkForNewVars(ReadVariableList, _readReqBuffer, _readBufferSize); //, EAdsAccessType::Read);
	
	// Nothing to do
	if (_readReqBuffer.Num() == 0)
		return;
	
	TSimpleBuffer<char> readBuffer(_readBufferSize);
	
	// Get data from ADS
	unsigned long bytesRead;

	double readTimeTick = FPlatformTime::Seconds();
	long err = AdsSyncReadWriteReqEx2(
		AdsPort,
		&_remoteAmsAddress,
		ADSIGRP_SUMUP_READ,
		_readReqBuffer.Num(),
		readBuffer.byteSize(),
		readBuffer.getData(),
		_readReqBuffer.Num()*sizeof(FDataPar),
		_readReqBuffer.GetData(),
		&bytesRead
	);
	double readTimeTock = FPlatformTime::Seconds();

	ReadDataRoundTripTime = static_cast<float>((readTimeTock - readTimeTick)*1000.0);
	
	if (err && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 5, FColor::Red,
		   FString::Printf(TEXT("Failed to get data from ADS, error code 0x%x"), err));
	}

	// Unpack data
	char* pErrorPos = readBuffer.getData();
	char* pValuePos = pErrorPos + _readReqBuffer.Num()*sizeof(uint32);
	for (const auto adsVar : ReadVariableList)
	{
		if (IsValid(adsVar) && adsVar->ValidAds)
		{
			pValuePos += adsVar->unpackValues(pErrorPos, pValuePos, err);
			pErrorPos += sizeof(uint32);
		}
	}
}

void ATcAdsMaster::writeValues()
{
	checkForNewVars(WriteVariableList, _writeReqBuffer, _writeBufferSize); //, EAdsAccessType::Write);

	// Nothing to do
	if (_writeReqBuffer.Num() == 0)
		return;

	const size_t reqSize = _writeReqBuffer.Num()*sizeof(FDataPar);
	
	TSimpleBuffer<char> writeBuffer(_writeBufferSize);
	// Copy data from Write request buffer
	memcpy_s(writeBuffer.getData(), writeBuffer.byteSize(), _writeReqBuffer.GetData(), reqSize);

	char* bufferPos = writeBuffer.getData() + reqSize;
	
	// Get data from variables
	for (const auto adsVar : WriteVariableList)
	{
		if (IsValid(adsVar) && adsVar->ValidAds)
		{
			bufferPos += adsVar->packValues(bufferPos);
		}
	}

	// Make a buffer for the ADS return codes
	TSimpleBuffer<uint32> errorBuffer(_writeReqBuffer.Num());
	
	// Get data from ADS
	unsigned long bytesRead;
	long err = AdsSyncReadWriteReqEx2(
		AdsPort,
		&_remoteAmsAddress,
		ADSIGRP_SUMUP_WRITE,
		_writeReqBuffer.Num(),
		errorBuffer.byteSize(),
		errorBuffer.getData(),
		writeBuffer.byteSize(),
		writeBuffer.getData(),
		&bytesRead
	);

	if (err && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2, 5, FColor::Red,
		   FString::Printf(TEXT("Failed to write data to ADS, error code 0x%x"), err));
	}

	// Unpack errors
	uint32* pErrorPos = errorBuffer.getData();
	for (auto adsVar : WriteVariableList)
	{
		if (IsValid(adsVar) && adsVar->ValidAds)
		{
			adsVar->Error = *pErrorPos;
			++pErrorPos;
		}
	}
}
