// Fill out your copyright notice in the Description page of Project Settings.


#include "TcAdsMaster.h"
#include "TcAdsVariable.h"
#include "Misc/DefaultValueHelper.h"
#include "TcAdsAPI.h"


TArray<TcAdsCallbackStruct> ATcAdsMaster::_CallbackList = TArray<TcAdsCallbackStruct>();
uint32 ATcAdsMaster::_ActiveMasters = 0;

// Sets default values
ATcAdsMaster::ATcAdsMaster() :
	_remoteAmsAddress({{0, 0, 0, 0, 0, 0}, 0}),
	_readBufferSize(0),
	_writeBufferSize(0),
	// _firstTick(true),
	ReadValuesInterval(0.1f),
	WriteValuesInterval(0.1f),
	ReadDataRoundTripTime(0.0f),
	// UpdateListsInterval(10.0f),
	RemoteAmsNetId(TEXT("127.0.0.1.1.1")),
	RemoteAmsPort(851),
	RemoteAmsAddressValid(false)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	union {uint32 raw; AdsVersion version;}localVersion;
	localVersion.raw = AdsGetDllVersion();
	LocalDllVersion = FString::Printf(TEXT("%u.%u.%u"),
		localVersion.version.version,
		localVersion.version.revision,
		localVersion.version.build
	);

	UE_LOG(LogTcAds, Display,
		TEXT("Ads master created using dll version %s"),
		*LocalDllVersion
	);
}

int32 ATcAdsMaster::openPort()
{
	// Port is already open or failed
	if (AdsPort != 0)
		return AdsPort;
	
	RemoteAmsAddressValid = parseAmsAddress(RemoteAmsNetId, RemoteAmsPort, _remoteAmsAddress);

	if (RemoteAmsAddressValid)
	{
			UE_LOG(LogTcAds, Display,
				TEXT("Attempting communication with remote AMS Net Id %u.%u.%u.%u.%u.%u:%u"),
				_remoteAmsAddress.netId.b[0],
				_remoteAmsAddress.netId.b[1],
				_remoteAmsAddress.netId.b[2],
				_remoteAmsAddress.netId.b[3],
				_remoteAmsAddress.netId.b[4],
				_remoteAmsAddress.netId.b[5],
				_remoteAmsAddress.port
			);
		
	}
	else
	{
		UE_LOG(LogTcAds, Error,
			TEXT("Failed to parse '%s' into an AMS Net Id\nAborting ADS communication"),
			*RemoteAmsNetId
		);
		
		return (AdsPort = -1);
	}
	
	// Open port
	AdsPort = AdsPortOpenEx();
	
	if (AdsPort > 0)
	{
		UE_LOG(LogTcAds, Display, TEXT("Local port: %d opened"), AdsPort);
	}
	else
	{
		UE_LOG(LogTcAds, Display, TEXT("Port opening failed\nAborting ADS communication"));
	
		return (AdsPort = -1);
	}
	
	char remoteAsciiDevName[50];
	AdsVersion remoteVersion;

	auto err = AdsSyncReadDeviceInfoReqEx(AdsPort, &_remoteAmsAddress, remoteAsciiDevName, &remoteVersion);
	
	if (err)
	{
		UE_LOG(LogTcAds, Error,
			TEXT("Failed to get device info from '%s'. Error code: 0x%x\nAborting ADS communication"),
			*RemoteAmsNetId,
			err
		);

		closePort();
		return (AdsPort = -1);
	}

	RemoteDeviceName = FString(remoteAsciiDevName);
	RemoteAdsVersion = FString::Printf(TEXT("%u.%u.%u"), remoteVersion.version, remoteVersion.revision, remoteVersion.build);

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
	
	UE_LOG(LogTcAds, Display,
		TEXT("Connected to '%s' using version %u.%u.%u \nStarting ADS communication"),
		*RemoteDeviceName,
		remoteVersion.version,
		remoteVersion.revision,
		remoteVersion.build
	);

	return AdsPort;
}

void ATcAdsMaster::closePort()
{
	if (AdsPort > 0)
	{
		AdsPortCloseEx(AdsPort);
		
		UE_LOG(LogTcAds, Display, TEXT("ADS port %d closed"), AdsPort);
		AdsPort = 0;
	}
}

// Called when the game starts or when spawned
void ATcAdsMaster::BeginPlay()
{
	Super::BeginPlay();

	++_ActiveMasters;
	
	// union {uint32 raw; AdsVersion version;}localVersion;
	// localVersion.raw = AdsGetDllVersion();
	// LocalDllVersion = FString::Printf(TEXT("%u.%u.%u"),
	// 	localVersion.version.version,
	// 	localVersion.version.revision,
	// 	localVersion.version.build
	// );
	//
	// AmsAddr localAddress;
	// AdsGetLocalAddressEx(AdsPort, &localAddress);
	// LocalAmsAddress = FString::Printf(TEXT("%u.%u.%u.%u.%u.%u:%u"),
	// 	localAddress.netId.b[0],
	// 	localAddress.netId.b[1],
	// 	localAddress.netId.b[2],
	// 	localAddress.netId.b[3],
	// 	localAddress.netId.b[4],
	// 	localAddress.netId.b[5],
	// 	localAddress.port
	// );
	
	// Start timers
	if (ReadValuesInterval > 0.0f)
		GetWorldTimerManager().SetTimer(_readValuesTimerHandle,this, &ATcAdsMaster::readValues, ReadValuesInterval, true);

	if (WriteValuesInterval > 0.0f)
		GetWorldTimerManager().SetTimer(_writeValuesTimerHandle,this, &ATcAdsMaster::writeValues, WriteValuesInterval, true);

	// if (UpdateListsInterval > 0.0f)
	// 	GetWorldTimerManager().SetTimer(_updateListsTimerHandle, this, &ATcAdsMaster::updateVarLists, UpdateListsInterval, true);
}

void ATcAdsMaster::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);
	
	// Stop timers
	GetWorldTimerManager().ClearTimer(_readValuesTimerHandle);
	GetWorldTimerManager().ClearTimer(_writeValuesTimerHandle);

	// Stop callback notifications and release handles
	TArray<TcAdsCallbackStruct> temp = _CallbackList;
	for (auto adsVar : temp)
	{
		if (IsValid(adsVar.variable))
			if (adsVar.variable->AdsMaster == this)
				adsVar.variable->release();
	}

	// If we are the last master to end play, then we clean up the callback array
	if (--_ActiveMasters == 0)
		_CallbackList.Empty();
	
	// Clear lists. Need to do this since these are uproperties exposed to the editor and therefore have a weird ass lifespan
	ReadVariableList.Empty();
	WriteVariableList.Empty();
	
	closePort();
	
}

void ATcAdsMaster::removeVariable(UTcAdsVariable* variable) // const UTcAdsVariable* Variable, TArray<UTcAdsVariable*>& VarList, TArray<FDataPar>& ReqList, size_t& BufferSize) //, EAdsAccessType Access)
{
	if (!IsValid(variable))
	{
		return;
	}
	if (!variable->ValidAds)
	{
		return;
	}
	
	switch (variable->Access)
	{
	case EAdsAccessType::None: break;
	case EAdsAccessType::Read:
		removeVariablePrivate(variable, ReadVariableList, _readReqBuffer, _readBufferSize);
		break;
	case EAdsAccessType::Write:
		removeVariablePrivate(variable, WriteVariableList, _writeReqBuffer, _writeBufferSize);
		break;
	case EAdsAccessType::ReadCyclic:
		removeCallbackVariable(variable);
		break;
	case EAdsAccessType::ReadOnChange:
		removeCallbackVariable(variable);
		break;
	case EAdsAccessType::WriteOnChange: break;
	default: ;
	}
}

// void ATcAdsMaster::updateVarLists()
// {
// 	// // Update read list
// 	// checkForNewVars(ReadVariableList, _readReqBuffer, _readBufferSize);
// 	//
// 	// // Update write list
// 	// checkForNewVars(WriteVariableList, _writeReqBuffer, _writeBufferSize);
// 	//
// 	// // Update callback list
// 	// checkForCallbackVars();
// }

// void ATcAdsMaster::checkForNewVars(TArray<UTcAdsVariable*>& vars, TArray<FDataPar>& reqBuffer, size_t& bufferSize) //, EAdsAccessType Access)
// {
// 	// Check for new variables in the list. Since new variables are always added at the end, we check in reverse
// 	// (so that normally we will check the first entry and immediately step out).
// 	bool newVar = false;
// 	int32 newIndex = 0;
// 	for (auto i = vars.Num(); i--;)
// 	{
// 		// Skip variables that are pending kill (this shouldn't happen, but you never know with unreal...)
// 		if (!IsValid(vars[i]))
// 			continue;
// 		
// 		if (vars[i]->newVar())
// 		{
// 			newVar = true;
// 			newIndex = i;
// 		}
// 		else
// 			break;
// 	}
//
// 	// Nothing to do
// 	if (!newVar)
// 		return;
//
// 	// If there are any new variables then we must ask ADS for them in forward order, so that the
// 	// arrays xxxVariableList and xxxReqBuffer_ are synchronized
// 	for (auto i = newIndex; i < vars.Num(); ++i)
// 	{
// 		// Skip variables that are pending kill again
// 		if (!IsValid(vars[i]))
// 			continue;
//
// 		auto err = vars[i]->getSymbolEntryFromAds(AdsPort, _remoteAmsAddress, reqBuffer);
// 		if (!err)
// 		{
// 			bufferSize += vars[i]->transferSize();
// 		}
// 		
// 		if (GEngine)
// 		{
// 			if (err)
// 			{
// 				GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Yellow,
// 					FString::Printf(TEXT("Failed to subscribe to '%s' variable '%s'. Error code 0x%x"),
// 						AdsAccessTypeName(vars[i]->Access),
// 						*vars[i]->AdsName,
// 						err
// 						));
// 			}
// 			else
// 			{
// 				GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green,
// 					FString::Printf(TEXT("Successfully subscribed to '%s' variable '%s'"),
// 						AdsAccessTypeName(vars[i]->Access),
// 						*vars[i]->AdsName
// 						));
// 			}
// 		}
// 	}
// }

// void ATcAdsMaster::checkForCallbackVars()
// {
// 	// Check for new variables in the list. Since new variables are always added at the end, we check in reverse
// 	// (so that normally we will check the first entry and immediately step out).
// 	bool newVar = false;
// 	int32 newIndex = 0;
// 	for (auto i = _CallbackList.Num(); i--;)
// 	{
// 		// Skip variables that are pending kill (this shouldn't happen, but you never know with unreal...)
// 		if (!IsValid(_CallbackList[i].variable))
// 			continue;
// 		
// 		if (_CallbackList[i].variable->newVar())
// 		{
// 			newVar = true;
// 			newIndex = i;
// 		}
// 		else
// 			break;
// 	}
//
// 	// Nothing to do
// 	if (!newVar)
// 		return;
// 	
// 	for (auto i = newIndex; i < _CallbackList.Num(); ++i)
// 	{
// 		// Skip variables that are pending kill again
// 		if (!IsValid(_CallbackList[i].variable))
// 			continue;
//
// 		auto err = _CallbackList[i].variable->setupCallback(AdsPort, _remoteAmsAddress, _CallbackList[i].handle);
// 		if (GEngine)
// 		{
// 			if (err)
// 			{
// 				GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Yellow,
// 					FString::Printf(TEXT("Failed to add callback for '%s' variable '%s'. Error code 0x%x"),
// 						AdsAccessTypeName(_CallbackList[i].variable->Access),
// 						*_CallbackList[i].variable->AdsName,
// 						err
// 					)
// 				);
// 			}
// 			else
// 			{
// 				GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green,
// 					FString::Printf(TEXT("Successfully added callback for '%s' variable '%s'"),
// 						AdsAccessTypeName(_CallbackList[i].variable->Access),
// 						*_CallbackList[i].variable->AdsName
// 					)
// 				);
// 			}
// 		}
// 	}
// }

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
	if (variableList.Num() == 0)
		return;
	
	auto idx = variableList.IndexOfByKey(variable);
	if (idx == INDEX_NONE)
	{
		UE_LOG(LogTcAds, Warning,
			TEXT("No variable '%s' found in %s list. No action taken"),
			*variable->AdsName,
			AdsAccessTypeName(variable->Access)
		);
		
		return;
	}

	variableList.RemoveAt(idx);
	reqBuffer.RemoveAt(idx);
	bufferSize -= variable->transferSize();
	
	UE_LOG(LogTcAds, Display,
		TEXT("Unsubscribed variable '%s'"),
		*variable->AdsName
	);
}

void ATcAdsMaster::removeCallbackVariable(UTcAdsVariable* variable)
{
	auto idx = _CallbackList.IndexOfByKey(variable);

	if (idx == INDEX_NONE)
	{
		UE_LOG(LogTcAds, Warning,
			   TEXT("No variable '%s' found in callback list. No action taken"),
			   *variable->AdsName
		);
		
		return;
	}

	variable->terminateCallbackVariable(AdsPort, &_remoteAmsAddress, _CallbackList[idx]);
}

void ATcAdsMaster::ReadCallback(AmsAddr* pAddr, AdsNotificationHeader* pNotification, ULONG hUser)
{
	for (auto& callback : _CallbackList)
	{
		if (!IsValid(callback.variable))
			continue;
		
		if (callback.hUser == hUser)
		{
			callback.variable->unpackValue(reinterpret_cast<char*>(pNotification->data));
			return;
		}
	}
}

// Called every frame
void ATcAdsMaster::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

}

void ATcAdsMaster::addVariable(UTcAdsVariable* variable)
{
	// Variable is invalid
	if (!IsValid(variable))
		return;

	// Check if port is open, and attempt to open if not
	if (openPort() <= 0)
		return;

	uint32 err = ADSERR_DEVICE_INVALIDPARM;
	
	switch (variable->Access)
	{
//	case EAdsAccessType::None: break;
	case EAdsAccessType::Read:
		err = variable->getSymbolEntryFromAds(AdsPort, &_remoteAmsAddress, &_readReqBuffer, &_readBufferSize);
		if (!err)
			ReadVariableList.Add(variable);
		break;
	case EAdsAccessType::Write:
		err = variable->getSymbolEntryFromAds(AdsPort, &_remoteAmsAddress, &_writeReqBuffer, &_writeBufferSize);
		if (!err)
			WriteVariableList.Add(variable);
		break;
	case EAdsAccessType::ReadCyclic:
		err = variable->getSymbolEntryFromAds(AdsPort, &_remoteAmsAddress);
		if (!err)
		{
			TcAdsCallbackStruct temp(variable);
			err = variable->setupCallbackVariable(AdsPort, &_remoteAmsAddress, ADSTRANS_SERVERCYCLE, temp);
			if (!err)
				_CallbackList.Emplace(temp);
		}
		break;
	case EAdsAccessType::ReadOnChange:
		err = variable->getSymbolEntryFromAds(AdsPort, &_remoteAmsAddress);
		if (!err)
		{
			TcAdsCallbackStruct temp(variable);
			err = variable->setupCallbackVariable(AdsPort, &_remoteAmsAddress, ADSTRANS_SERVERONCHA, temp);
			if (!err)
				_CallbackList.Emplace(temp);
		}
		break;
//	case EAdsAccessType::WriteOnChange: break;
	default: ;
	}
	
	if (err)
	{
		UE_LOG(LogTcAds, Error,
			TEXT("Failed to subscribe to '%s' variable '%s'. Error code 0x%x"),
			AdsAccessTypeName(variable->Access),
			*variable->AdsName,
			err
		);

		return;
	}

	UE_LOG(LogTcAds, Display,
		TEXT("Successfully subscribed to '%s' variable '%s'"),
		AdsAccessTypeName(variable->Access),
		*variable->AdsName
	);
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
	// Port is not yet open or has failed to open
	if (AdsPort <= 0)
		return;
	
	// checkForNewVars(ReadVariableList, _readReqBuffer, _readBufferSize); //, EAdsAccessType::Read);
	
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
	
	// Unpack data
	char* pErrorPos = readBuffer.getData();
	char* pValuePos = pErrorPos + _readReqBuffer.Num()*sizeof(uint32);
	for (const auto adsVar : ReadVariableList)
	{
		if (IsValid(adsVar) && adsVar->ValidAds)
		{
			pValuePos += adsVar->unpackValue(pErrorPos, pValuePos, err);
			pErrorPos += sizeof(uint32);
		}
	}
}

void ATcAdsMaster::writeValues()
{
	// Port is not yet open or has failed to open
	if (AdsPort <= 0)
		return;

	// checkForNewVars(WriteVariableList, _writeReqBuffer, _writeBufferSize); //, EAdsAccessType::Write);

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
			bufferPos += adsVar->packValue(bufferPos);
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
