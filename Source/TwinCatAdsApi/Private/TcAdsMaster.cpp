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
	ReadValuesInterval(0.1f),
	WriteValuesInterval(0.1f),
	Timeout(1000),
	ReadDataRoundTripTime(0.0f),
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
				TEXT("Attempting communication with remote AMS Address %u.%u.%u.%u.%u.%u:%u"),
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

	AdsSyncSetTimeoutEx(AdsPort, Timeout);
	
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

	// Stop callback notifications and release handles
	TArray<TcAdsCallbackStruct> temp = _CallbackList;
	for (auto adsVar : temp)
	{
		if (IsValid(adsVar.variable))
			if (adsVar.variable->AdsMaster == this)
				adsVar.variable->release();
	}

	// If we are the last master to end play then we clean up the callback array
	if (--_ActiveMasters == 0)
		_CallbackList.Empty();
	
	// Clear lists. Need to do this since these are uproperties exposed to the editor and therefore have a weird ass lifespan
	ReadVariableList.Empty();
	WriteVariableList.Empty();
	
	closePort();
	
}

void ATcAdsMaster::removeVariable(UTcAdsVariable* variable)
{
	if (!UTcAdsVariable::ValidAdsVariable(variable))
		return;
	
	switch (variable->Access)
	{
	case EAdsAccessMode::None: break;
	case EAdsAccessMode::Read:
		removeVariablePrivate(variable, ReadVariableList);
		break;
	case EAdsAccessMode::ReadCyclic:
		// Fallthrough
	case EAdsAccessMode::ReadOnChange:
		removeCallbackVariable(variable);
		break;
	case EAdsAccessMode::ReadWriteOnChange:
		removeCallbackVariable(variable);
		// Fallthrough
	case EAdsAccessMode::Write:
		// Fallthrough
	case EAdsAccessMode::WriteOnChange:
		removeVariablePrivate(variable, WriteVariableList);
		break;
	default: ;
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

void ATcAdsMaster::removeVariablePrivate(const UTcAdsVariable* variable, TArray<UTcAdsVariable*>& variableList)
{
	if (variableList.Num() == 0)
		return;
	
	auto idx = variableList.IndexOfByKey(variable);
	if (idx == INDEX_NONE)
	{
		UE_LOG(LogTcAds, Warning,
			TEXT("No variable '%s' found in %s list. No action taken"),
			*variable->AdsName,
			*GetAdsAccessTypeName(variable->Access)
//			AdsAccessTypeName(variable->Access)
		);
		
		return;
	}

	variableList.RemoveAt(idx);
	
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
	if  (
			!(
				((static_cast<int32>(hUser) < _CallbackList.Num()) || (static_cast<int32>(hUser) < 0))
				&& UTcAdsVariable::ValidAdsVariable(_CallbackList[hUser].variable)
				&& (_CallbackList[hUser].index == hUser)
			)
		)
		return;

	_CallbackList[hUser].variable->unpackValue(reinterpret_cast<char*>(pNotification->data));
	
	// for (auto& callback : _CallbackList)
	// {
	// 	if (!IsValid(callback.variable))
	// 		continue;
	// 	
	// 	if (callback.hUser == hUser)
	// 	{
	// 		callback.variable->unpackValue(reinterpret_cast<char*>(pNotification->data));
	// 		return;
	// 	}
	// }
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

	variable->getSymbolEntryFromAds(AdsPort, &_remoteAmsAddress);
	if (!variable->Error)
	{
		switch (variable->Access)
		{
		//	case EAdsAccessType::None: break; // Invalid
		case EAdsAccessMode::Read:
			ReadVariableList.Add(variable);
			break;
		case EAdsAccessMode::ReadCyclic:
		{
			TcAdsCallbackStruct temp(variable, _CallbackList.Num());
			variable->setupCallbackVariable(AdsPort, &_remoteAmsAddress, ADSTRANS_SERVERCYCLE, temp);
			if (!variable->Error)
				_CallbackList.Emplace(temp);
		} break;
		case EAdsAccessMode::ReadOnChange:
		{
			TcAdsCallbackStruct temp(variable, _CallbackList.Num());
			variable->setupCallbackVariable(AdsPort, &_remoteAmsAddress, ADSTRANS_SERVERONCHA, temp);
			if (!variable->Error)
				_CallbackList.Emplace(temp);
		} break;
		case EAdsAccessMode::ReadWriteOnChange:
		{
			TcAdsCallbackStruct temp(variable, _CallbackList.Num());
			variable->setupCallbackVariable(AdsPort, &_remoteAmsAddress, ADSTRANS_SERVERONCHA, temp);
			if (!variable->Error)
				_CallbackList.Emplace(temp);
		}
			// Fallthrough
		case EAdsAccessMode::Write:
			// Fallthrough
		case EAdsAccessMode::WriteOnChange:
			if (!variable->Error)
				WriteVariableList.Add(variable);
			break;
		default: variable->Error = ADSERR_DEVICE_INVALIDPARM;
		}
	}
	
	if (variable->Error)
	{
		UE_LOG(LogTcAds, Error,
			TEXT("Failed to subscribe to '%s' variable '%s'. Error code 0x%x"),
			*GetAdsAccessTypeName(variable->Access),
//			AdsAccessTypeName(variable->Access),
			*variable->AdsName,
			variable->Error
		);

		return;
	}

	UE_LOG(LogTcAds, Display,
		TEXT("Successfully subscribed to '%s' variable '%s'"),
		*GetAdsAccessTypeName(variable->Access),
//		AdsAccessTypeName(variable->Access),
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

	size_t bufferSize = 0;

	TArray<UTcAdsVariable*> readyList;
	readyList.Reserve(ReadVariableList.Num());
	
	// Check what variables are ready for read
	for (auto adsVar : ReadVariableList)
	{
		if (UTcAdsVariable::ValidAdsVariable(adsVar))
		{
			readyList.Add(adsVar);
			bufferSize += adsVar->transferSize();
		}
	}
	
	// Nothing to do
	if (readyList.Num() == 0)
		return;

	// Create the request buffer and the receive buffer
	TArray<FDataPar> reqBuffer;
	reqBuffer.Reserve(readyList.Num());
	TSimpleBuffer<char> readBuffer(bufferSize);
	
	for (auto adsVar : readyList)
	{
		reqBuffer.Emplace(adsVar->reqData());
	}
	
	// Get data from ADS
	unsigned long bytesRead;
	double readTimeTick = FPlatformTime::Seconds();
	long err = AdsSyncReadWriteReqEx2(
		AdsPort,
		&_remoteAmsAddress,
		ADSIGRP_SUMUP_READ,
		reqBuffer.Num(),
		readBuffer.byteSize(),
		readBuffer.getData(),
		reqBuffer.Num()*sizeof(FDataPar),
		reqBuffer.GetData(),
		&bytesRead
	);
	double readTimeTock = FPlatformTime::Seconds();

	ReadDataRoundTripTime = static_cast<float>((readTimeTock - readTimeTick)*1000.0);
	
	// Unpack data
	char* pErrorPos = readBuffer.getData();
	char* pValuePos = pErrorPos + reqBuffer.Num()*sizeof(uint32);
	for (const auto adsVar : readyList)
	{
		pValuePos += adsVar->unpackValue(pErrorPos, pValuePos, err);
		pErrorPos += sizeof(uint32);
	}
}

void ATcAdsMaster::writeValues()
{
	// Port is not yet open or has failed to open
	if (AdsPort <= 0)
		return;

	size_t bufferSize = 0;
	
	TArray<UTcAdsVariable*> readyList;
	readyList.Reserve(WriteVariableList.Num());
	
	// Check what variables are ready for write
	for (auto adsVar : WriteVariableList)
	{
		if (UTcAdsVariable::ValidAdsVariable(adsVar) && adsVar->readyToWrite())
		{
			readyList.Add(adsVar);
			bufferSize += adsVar->transferSize();
		}
	}
	
	// Nothing to do
	if (readyList.Num() == 0)
		return;

	size_t reqSize = readyList.Num()*sizeof(FDataPar);

	// Create a buffer for write data [ reqData[0], reqData[1], ..., reqData[N], value[0], value[1], ..., value[n] ]
	TSimpleBuffer<char> writeBuffer(bufferSize);
	
	// Copy request info and values
	auto pReqBuffer = writeBuffer.getData();
	auto pValueBuffer = pReqBuffer + reqSize;
	for (auto adsVar : readyList)
	{
		*reinterpret_cast<FDataPar*>(pReqBuffer) = adsVar->reqData();
		pReqBuffer += sizeof(FDataPar);
		pValueBuffer += adsVar->packValue(pValueBuffer);
	}
	
	// Make a buffer for the ADS return codes
	TSimpleBuffer<int32> errorBuffer(readyList.Num());
	
	// Get data from ADS
	unsigned long bytesRead;
	long err = AdsSyncReadWriteReqEx2(
		AdsPort,
		&_remoteAmsAddress,
		ADSIGRP_SUMUP_WRITE,
		readyList.Num(),
		errorBuffer.byteSize(),
		errorBuffer.getData(),
		writeBuffer.byteSize(),
		writeBuffer.getData(),
		&bytesRead
	);

	if (err)
	{
		for (auto adsVar : readyList)
			adsVar->Error = err;

		return;
	}
	
	// Unpack errors
	auto pErrorPos = errorBuffer.getData();
	for (auto adsVar : readyList)
	{
		adsVar->Error = *pErrorPos;
		++pErrorPos;
	}
}
