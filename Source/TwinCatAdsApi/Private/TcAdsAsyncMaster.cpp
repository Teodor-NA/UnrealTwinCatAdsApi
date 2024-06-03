// Fill out your copyright notice in the Description page of Project Settings.

#include "TcAdsAsyncMaster.h"
#include "TcAdsDef.h"
#include "TcAdsAPI.h"
#include "TcAdsAsyncVariable.h"
#include "TcAdsAsyncVariable_Private.h"
#include "Misc/DefaultValueHelper.h"


FTcAdsAsyncWorker* FTcAdsAsyncWorker::_Instance = nullptr;


FTcAdsAsyncWorker::FTcAdsAsyncWorker(const FString& remoteAmsNetId, int32 remoteAmsPort, float cycleTime)
	: _thread(nullptr)
	, _adsPort(0)
	, _cycleTime(cycleTime)
	, _error(0)
{
	_remoteAmsAddr = ParseAmsAddress(remoteAmsNetId, remoteAmsPort);
	if (_remoteAmsAddr.port == 0)
	{
		LOG_TCADS_ERROR(
			"Could not parse '%s:%d' into an ams address. Aborting thread creation",
			*remoteAmsNetId, remoteAmsPort
		);
		return;
	}
	
	_thread = FRunnableThread::Create(this, TEXT("TcAdsAsyncMaster"), 0, TPri_Normal);
	if (_thread)
	{
		LOG_TCADS_DISPLAY("Created thread %u: %s", _thread->GetThreadID(), *_thread->GetThreadName());
	}
	else
	{
		LOG_TCADS_ERROR("Thread creation failed");
	}
}

FTcAdsAsyncWorker::~FTcAdsAsyncWorker()
{
	if (!_thread)
		return;

	TArray<FTcAdsAsyncVariable*> varList;
	// Copy list to avoid locking bug
	{
		std::lock_guard<std::mutex> lock(_mutexLock);
		varList = _variableList;
		_variableList.Empty();
	}
	
	for (auto var : varList)
	{
		if (var)
		{
			delete var;
			var = nullptr;
		}
	}
	
	LOG_TCADS_DISPLAY("Destroying thread %u: %s", _thread->GetThreadID(), *_thread->GetThreadName());
	delete _thread;
	_thread = nullptr;

}

AmsAddr FTcAdsAsyncWorker::ParseAmsAddress(const FString& netId, int32 port)
{
	AmsAddr tempAddress = {{0, 0,0 ,0, 0,0}, 0};

	if ((port > 65535) || (port < 0))
		return tempAddress;

	constexpr TCHAR delim = TEXT('.');
	
	TArray<FString> tmp;
	netId.ParseIntoArray(tmp, &delim);

	if (tmp.Num() != 6)
		return tempAddress;

	int32 idx = 0;
	for (auto& str : tmp)
	{
		int32 val;
		bool success = FDefaultValueHelper::ParseInt(str, val);
		if ((val > 255) || (val < 0))
			success = false;

		if (!success)
			return tempAddress;

		tempAddress.netId.b[idx] = static_cast<uint8>(val);
		++idx;
	}

	tempAddress.port = static_cast<uint16>(port);

	return tempAddress;
}

AmsAddr FTcAdsAsyncWorker::_getRemoteAmsAddr()
{
	std::lock_guard<std::mutex> lock(_mutexLock);
	return _remoteAmsAddr;
}

LONG FTcAdsAsyncWorker::_getLocalAdsPort()
{	
	std::lock_guard<std::mutex> lock(_mutexLock);
	return _adsPort;
}

LONG FTcAdsAsyncWorker::_closePort()
{
	if (_adsPort <= 0)
		return 0;
	
	_error = AdsPortCloseEx(_adsPort);

	if (_error)
	{
		LOG_TCADS_DISPLAY("Local ADS port %d close failed with error code 0x%x", _adsPort, _error);
	}
	else
	{
		LOG_TCADS_DISPLAY("Local ADS port %d closed", _adsPort);
	}

	_adsPort = 0;

	return _error;
}

void FTcAdsAsyncWorker::ReadCallback(AmsAddr* pAddr, AdsNotificationHeader* pNotification, ULONG hUser)
{
	if (!_Instance)
		return;

	FTcAdsAsyncVariable* adsVar;
		
	auto index = static_cast<int32>(hUser);
	if (index > _Instance->_variableList.Num())
	{
		LOG_TCADS_DISPLAY("Callback failed, index %d is out of range'", index);
		return;
	}
	
	{
		std::lock_guard<std::mutex> lock(_Instance->_mutexLock);
		adsVar = _Instance->_variableList[index];
	}

	adsVar->unpack(pNotification->data, true);

	// ToDo: Remove this, it is just for testing
	auto mode = adsVar->getAdsMode();
	LOG_TCADS_DISPLAY("Updated callback state of '%s(%d)' variable(%d) with value: %f'"
		, *GetAdsAccessTypeName(mode)
		, mode
		, adsVar->getIndex()
		, adsVar->getValue()
	);

}

AmsAddr FTcAdsAsyncWorker::GetRemoteAmsAddr()
{
	if (!_Instance)
		return {0, 0, 0, 0, 0, 0, 0};

	return _Instance->_getRemoteAmsAddr();
}

LONG FTcAdsAsyncWorker::GetLocalAdsPort()
{
	if (!_Instance)
		return 0;

	return _Instance->_getLocalAdsPort();
}

FTcAdsAsyncWorker* FTcAdsAsyncWorker::Create(const FString& remoteAmsNetId, LONG remoteAmsPort, float refreshRate)
{
	//Create new instance of thread if it does not exist
	//		and the platform supports multi threading!
	if (!_Instance && FPlatformProcess::SupportsMultithreading())
	{
		_Instance = new FTcAdsAsyncWorker(remoteAmsNetId, remoteAmsPort, refreshRate);			
	}
	return _Instance;
}

bool FTcAdsAsyncWorker::Init()
{
	return true;
}

uint32 FTcAdsAsyncWorker::Run()
{
	// Wait for thread to be ready
	int timeoutCount = 0;
	while (!_thread)
	{
		static constexpr int timeout = 10;

		if (++timeoutCount > timeout)
		{
			LOG_TCADS_ERROR("Thread creation failed: Timeout waiting for thread");
			return (_error = 2);
		}
		
		LOG_TCADS_DISPLAY("Waiting for thread, attempt %d/%d", timeoutCount, timeout);
		
		FPlatformProcess::Sleep(0.1f);
	}

	TArray<FTcAdsAsyncVariable*> readList;
	TArray<FTcAdsAsyncVariable*> writeList;
	
	while (_stopTaskCounter.GetValue() == 0)
	{
		double loopTick = FPlatformTime::Seconds();

		// --------------------v Worker loop v-------------------------

		size_t readBufferSize = 0;
		size_t writeBufferSize = 0;

		LONG adsPort;
		AmsAddr amsAddr;
		
		{
			std::lock_guard<std::mutex> lock(_mutexLock);
			// Copy out the pointers for the variables that are ready for update in a separate read/write lists
			for (auto adsVar : _variableList)
			{
				auto ready = adsVar->readyForUpdate();

				if (CheckAdsUpdateFlag(ready, EAdsRemoteReadFlag))
				{
					readList.Add(adsVar);
					readBufferSize += adsVar->readSize();
				}
				if (CheckAdsUpdateFlag(ready, EAdsRemoteWriteFlag))
				{
					writeList.Add(adsVar);
					writeBufferSize += adsVar->writeSize();
				}
			}
		
			// copy out what we need for communication so that we can release the mutex lock
			adsPort = _adsPort;
			amsAddr = _remoteAmsAddr;
		}

		_ReadValues(adsPort, &amsAddr, readList, readBufferSize);
		_WriteValues(adsPort, &amsAddr, writeList, writeBufferSize);

		// ToDo: remove this, it is just for testing
		for  (auto adsVar : readList)
		{
			auto mode = adsVar->getAdsMode();
			LOG_TCADS_DISPLAY("Updated read state of '%s(%d)' variable(%d) with value: %f'"
				, *GetAdsAccessTypeName(mode)
				, mode
				, adsVar->getIndex()
				, adsVar->getValue()
			);
		}
		for  (auto adsVar : writeList)
		{
			auto mode = adsVar->getAdsMode();
			LOG_TCADS_DISPLAY("Updated write state of '%s(%d)' variable(%d) with value: %f'"
				, *GetAdsAccessTypeName(mode)
				, mode
				, adsVar->getIndex()
				, adsVar->getValue()
			);
		}
		
		// Reset lists without changing allocation so that hopefully the arrays grow to a certain size and then stays there
		readList.Reset();
		writeList.Reset();

		// --------------------^ Worker loop ^-------------------------

		double loopTime = FPlatformTime::Seconds() - loopTick;
		double sleepTime = _cycleTime - loopTime;

		if (sleepTime < 0.0f)
		{
			LOG_TCADS_WARNING("Looptime measured at %f [s], which is an overrun by %f [s]"
				, loopTime
				, -sleepTime
			);
		}
		else
		{
			FPlatformProcess::Sleep(sleepTime);
		}
	}

	{
		std::lock_guard<std::mutex> lock(_mutexLock);
		LOG_TCADS_DISPLAY("Thread %u %s finished with return code %u",
			_thread->GetThreadID(),
			*_thread->GetThreadName(),
			_error
		);
	
		return _error;
	}
}

void FTcAdsAsyncWorker::Stop()
{
	_stopTaskCounter.Increment();
}

LONG FTcAdsAsyncWorker::openPort()
{
	std::lock_guard<std::mutex> lock(_mutexLock);
	
	// Port is already open or failed
	if (_adsPort != 0)
	{
		auto tmp = _adsPort;
		return tmp;
	}

	if (_remoteAmsAddr.port == 0)
	{
		LOG_TCADS_ERROR("Invalid AMS address. Aborting ADS communication");
		_adsPort = -1;
		return -1;
	}
	
	auto tmp = _adsPort = AdsPortOpenEx();
	if (_adsPort == 0)
	{
		LOG_TCADS_ERROR("Local ADS port open failed")
		_adsPort = -1;
		return -1;
	}

	
	LOG_TCADS_DISPLAY("Local ADS port %d opened. Attempting communication", _adsPort)

	char pDevName[50];
	AdsVersion remoteVersion;
	
	_error = AdsSyncReadDeviceInfoReqEx(
		_adsPort,
		&_remoteAmsAddr,
		pDevName,
		&remoteVersion
	);

	if (_error)
	{
		LOG_TCADS_ERROR("Failed to get device info from remote device. Error code: 0x%x. Aborting communication", _error);
		_closePort();
		_adsPort = -1;
		return -1;
	}
	
	LOG_TCADS_DISPLAY("Connected to remote device %s running version %u.%u.%u"
		, *FString(pDevName)
		, remoteVersion.version
		, remoteVersion.revision
		, remoteVersion.build
	);

	return tmp;
}

LONG FTcAdsAsyncWorker::closePort()
{
	std::lock_guard<std::mutex> lock(_mutexLock);
	_releaseHandles();
	return _closePort();
}

// LONG FTcAdsAsyncWorker::_Update(LONG port, AmsAddr amsAddr, TArray<UTcAdsAsyncVariable*>& readList,
// 	TArray<UTcAdsAsyncVariable*>& writeList)
// {
// 	LONG err = _WriteValues(port, &amsAddr, writeList);
//
// 	if (err)
// 		return err;
//
// 	err = _ReadValues(port, &amsAddr, readList);
//
// 	return err;
// }

LONG FTcAdsAsyncWorker::_ReadValues(LONG port, AmsAddr* amsAddr,
	TArray<FTcAdsAsyncVariable*>& readList, size_t readBufferSize)
{
	if (readList.Num() <= 0)
		return 0;

	TArray<char> readBuffer;
	readBuffer.SetNumUninitialized(readBufferSize);
	TArray<FDataPar> readReqBuffer;
	readReqBuffer.Reserve(readList.Num());

	for (auto adsVar : readList)
	{
		readReqBuffer.Emplace(adsVar->getDataPar());
	}

	unsigned long bytesRead;
	long err = AdsSyncReadWriteReqEx2(
		port,
		amsAddr,
		ADSIGRP_SUMUP_READ,
		readReqBuffer.Num(),
		readBuffer.Num(),
		readBuffer.GetData(),
		readReqBuffer.Num()*readReqBuffer.GetTypeSize(),
		readReqBuffer.GetData(),
		&bytesRead
	);

	// Unpack received data
	char* pError = readBuffer.GetData();
	char* pData = pError + readList.Num()*sizeof(LONG);
		
	for (auto adsVar : readList)
	{
		pData += adsVar->unpack(pData, false, err, pError);
		pError += sizeof(LONG);
	}

	return err;
}

LONG FTcAdsAsyncWorker::_WriteValues(LONG port, AmsAddr* amsAddr,
	TArray<FTcAdsAsyncVariable*>& writeList, size_t writeBufferSize)
{
	if (writeList.Num() <= 0)
		return 0;

	size_t reqSize = writeList.Num()*sizeof(FDataPar);

	// Create a buffer for write data [ reqData[0], reqData[1], ..., reqData[N], value[0], value[1], ..., value[n] ]
	TArray<char> writeBuffer;
	writeBuffer.SetNumUninitialized(writeBufferSize);
	
	// Copy request info and values
	FDataPar* pReqBuffer = reinterpret_cast<FDataPar*>(writeBuffer.GetData());
	char* pValueBuffer = writeBuffer.GetData() + reqSize;
	for (auto adsVar : writeList)
	{
		pValueBuffer += adsVar->pack(pValueBuffer, pReqBuffer++);
	}
	
	// Make a buffer for the ADS return codes
	TArray<int32> errorBuffer;
	errorBuffer.SetNumUninitialized(writeList.Num());
	
	// Get data from ADS
	unsigned long bytesRead;
	long err = AdsSyncReadWriteReqEx2(
		port,
		amsAddr,
		ADSIGRP_SUMUP_WRITE,
		writeList.Num(),
		errorBuffer.Num()*errorBuffer.GetTypeSize(),
		errorBuffer.GetData(),
		writeBuffer.Num(),
		writeBuffer.GetData(),
		&bytesRead
	);

	if (err)
	{
		for (auto adsVar : writeList)
			adsVar->setError(err);

		return err;
	}
	
	// Unpack errors
	auto pErrorPos = errorBuffer.GetData();
	for (auto adsVar : writeList)
	{
		adsVar->setError(*(pErrorPos++));
	}
	
	return 0;
}

void FTcAdsAsyncWorker::_releaseHandles()
{
	for (auto adsVar : _variableList)
	{
		adsVar->releaseCallback(_adsPort, &_remoteAmsAddr);
	}
}

void FTcAdsAsyncWorker::EnsureCompletion()
{
	if (!_thread)
		return;
	
	Stop();
	_thread->WaitForCompletion();
}

void FTcAdsAsyncWorker::Shutdown()
{
	
	if (_Instance)
	{
		if (_Instance->_thread)
		{
			LOG_TCADS_DISPLAY("Attempting to shut down thread %u %s",
				_Instance->_thread->GetThreadID(), *_Instance->_thread->GetThreadName());
		}
		_Instance->EnsureCompletion();

		delete _Instance;
		_Instance = nullptr;
	}
}

// bool FTcAdsAsyncWorker::IsThreadFinished()
// {
// 	if (_Instance) return _Instance->isFinished();
// 	return true;
// }

FTcAdsAsyncVariable* FTcAdsAsyncWorker::CreateVariable(UTcAdsAsyncVariable* variable)
{
	if (!_Instance)
	{
		LOG_TCADS_WARNING("Cannot create ADS '%s(%d)' variable '%s'. Thread instance is not running"
			, *GetAdsAccessTypeName(variable->adsSetupInfo.adsMode)
			, variable->adsSetupInfo.adsMode
			, *variable->adsSetupInfo.adsName
		);
		
		variable->adsError = ADSERR_DEVICE_NOTREADY;
		return nullptr;
	}
	
	if (_Instance->openPort() <= 0)
	{
		LOG_TCADS_ERROR("Cannot create ADS '%s(%d)' variable '%s'. Port could not be opened"
			, *GetAdsAccessTypeName(variable->adsSetupInfo.adsMode)
			, variable->adsSetupInfo.adsMode
			, *variable->adsSetupInfo.adsName
		);

		variable->adsError = ADSERR_DEVICE_NOTREADY;
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(_Instance->_mutexLock);
	auto obj = new FTcAdsAsyncVariable(variable, _Instance->_variableList.Num());
	
	if (!obj)
	{
		LOG_TCADS_WARNING("Cannot create ADS '%s(%d)' variable '%s'. Object allocation failed"
			, *GetAdsAccessTypeName(variable->adsSetupInfo.adsMode)
			, variable->adsSetupInfo.adsMode
			, *variable->adsSetupInfo.adsName
		);

		variable->adsError = ADSERR_DEVICE_NOMEMORY;
		return nullptr;
	}

	variable->adsError = obj->fetchSymbolEntry(variable->adsSetupInfo.adsName, _Instance->_adsPort, &_Instance->_remoteAmsAddr);

	if (variable->adsError)
	{
		LOG_TCADS_ERROR("Fetching symbol info for '%s(%d)' variable '%s' failed with error code: 0x%x"
			, *GetAdsAccessTypeName(variable->adsSetupInfo.adsMode)
			, variable->adsSetupInfo.adsMode
			, *variable->adsSetupInfo.adsName
			, variable->adsError
		);
		
		delete obj;
		return nullptr;
	}

	if (CheckAdsUpdateFlag(variable->adsSetupInfo.adsMode, EAdsCallbackFlag))
	{
		variable->adsError = obj->setCallback(_Instance->_adsPort, &_Instance->_remoteAmsAddr,
			static_cast<int32>(_Instance->_cycleTime * 10000000.0f) * variable->adsSetupInfo.cycleMultiplier);

		if (variable->adsError)
		{
			LOG_TCADS_ERROR("Setting up callback for '%s(%d)' variable '%s' failed with error code: 0x%x"
			   , *GetAdsAccessTypeName(variable->adsSetupInfo.adsMode)
			   , variable->adsSetupInfo.adsMode
			   , *variable->adsSetupInfo.adsName
			   , variable->adsError
		   );

			delete obj;
			return nullptr;
		}
		
		LOG_TCADS_DISPLAY("Successfully set up callback for '%s(%d)' variable '%s'"
		   , *GetAdsAccessTypeName(variable->adsSetupInfo.adsMode)
		   , variable->adsSetupInfo.adsMode
		   , *variable->adsSetupInfo.adsName
	   );
	}

	_Instance->_variableList.Add(obj);
	
	LOG_TCADS_DISPLAY("Created new '%s(%d)' variable '%s'"
		, *GetAdsAccessTypeName(variable->adsSetupInfo.adsMode)
		, variable->adsSetupInfo.adsMode
		, *variable->adsSetupInfo.adsName
	);

	return obj;
}

// Sets default values
ATcAdsAsyncMaster::ATcAdsAsyncMaster()
	: baseTime(0.1f)
	, remoteAmsNetId(TEXT("127.0.0.1.1.1"))
	, remoteAmsPort(851)
	, _asyncWorker(nullptr)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

FTcAdsAsyncVariable* ATcAdsAsyncMaster::createVariable(UTcAdsAsyncVariable* variable)
{
	FTcAdsAsyncVariable* tmp = nullptr;
	if (_asyncWorker->IsValid())
		tmp = _asyncWorker->CreateVariable(variable);
	else
		_variableQueue.Add(variable);

	return tmp;
}
// Called when the game starts or when spawned
void ATcAdsAsyncMaster::BeginPlay()
{
	Super::BeginPlay();

	_asyncWorker = FTcAdsAsyncWorker::Create(remoteAmsNetId, remoteAmsPort, baseTime);

	// Check variable queue
	for (auto var : _variableQueue)
	{
		auto tmp = _asyncWorker->CreateVariable(var);
		var->_setVariableReference(tmp);
	}

	_variableQueue.Empty();
}

void ATcAdsAsyncMaster::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	_asyncWorker->closePort();
	
	FTcAdsAsyncWorker::Shutdown();
}


