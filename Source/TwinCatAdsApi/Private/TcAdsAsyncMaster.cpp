// Fill out your copyright notice in the Description page of Project Settings.

#include "TcAdsDef.h"
#include "TcAdsAPI.h"
#include "TcAdsAsyncMaster.h"
#include "TcAdsAsyncVariable.h"
#include "TcAdsAsyncVariable_Private.h"
#include "Misc/DefaultValueHelper.h"


FTcAdsAsyncWorker* FTcAdsAsyncWorker::_Instance = nullptr;


FTcAdsAsyncWorker::FTcAdsAsyncWorker(const FString& remoteAmsNetId, int32 remoteAmsPort, float refreshRate)
	: _thread(nullptr)
	, _adsPort(0)
	, _refreshRate(refreshRate)
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
	
	_mutexLock.lock();
	for (auto var : _variableList)
	{
		if (var)
		{
			delete var;
			var = nullptr;
		}
	}
	_mutexLock.unlock();
	
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

LONG FTcAdsAsyncWorker::_closePort()
{
	if (_adsPort <= 0)
		return 0;
	
	auto tmpPort = _adsPort;
	_error = AdsPortCloseEx(_adsPort);
	_adsPort = 0;

	if (_error)
	{
		LOG_TCADS_DISPLAY("Local ADS port %d close failed with error code 0x%x", tmpPort, _error);
	}
	else
	{
		LOG_TCADS_DISPLAY("Local ADS port %d closed", tmpPort);
	}

	return _error;
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
		
		_mutexLock.lock();
		for (auto adsVar : _variableList)
		{
			auto ready = adsVar->readyForUpdate();

			if (static_cast<uint8>(ready) & EAdsReadFlag)
			{
				readList.Add(adsVar);
				readBufferSize += adsVar->readSize();
			}
			if (static_cast<uint8>(ready) & EAdsWriteFlag)
			{
				writeList.Add(adsVar);
				writeBufferSize += adsVar->writeSize();
			}
		}

		// copy out what we need for communication so that we can release the mutex lock
		auto adsPort = _adsPort;
		auto amsAddress = _remoteAmsAddr;
		
		_mutexLock.unlock();

		if (readList.Num() > 0)
		{
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
				adsPort,
				&amsAddress,
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
				pData += adsVar->unpack(pData, err, pError);
				pError += sizeof(LONG);
			}
		}
		
		// Reset lists without changing allocation so that hopefully the arrays grow to a certain size and then stays there
		readList.Reset();
		writeList.Reset();

		// --------------------^ Worker loop ^-------------------------

		double loopTime = FPlatformTime::Seconds() - loopTick;
		double sleepTime = _refreshRate - loopTime;

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
	
	LOG_TCADS_DISPLAY("Thread %u %s finished with return code %u",
		_thread->GetThreadID(),
		*_thread->GetThreadName(),
		_error
	);
	
	return _error;
}

void FTcAdsAsyncWorker::Stop()
{
	_stopTaskCounter.Increment();
}

LONG FTcAdsAsyncWorker::openPort()
{
	_mutexLock.lock();
	
	// Port is already open or failed
	if (_adsPort != 0)
	{
		auto tmp = _adsPort;
		_mutexLock.unlock();
		return tmp;
	}

	if (_remoteAmsAddr.port == 0)
	{
		LOG_TCADS_ERROR("Invalid AMS address. Aborting ADS communication");
		_adsPort = -1;
		_mutexLock.unlock();
		return -1;
	}
	
	auto tmp = _adsPort = AdsPortOpenEx();
	if (_adsPort == 0)
	{
		LOG_TCADS_ERROR("Local ADS port open failed")
		_adsPort = -1;
		_mutexLock.unlock();
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
		_mutexLock.unlock();
		return -1;
	}
	
	_mutexLock.unlock();
	
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
	_mutexLock.lock();
	auto tmpError = _closePort();
	_mutexLock.unlock();
	
	return tmpError;
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
			LOG_TCADS_DISPLAY("Attempting to shut down thread %u %s", _Instance->_thread->GetThreadID(), *_Instance->_thread->GetThreadName());
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
			, *GetAdsAccessTypeName(variable->adsMode)
			, variable->adsMode
			, *variable->adsName
		);
		return nullptr;
	}
	
	if (_Instance->openPort() <= 0)
	{
		LOG_TCADS_ERROR("Cannot create ADS '%s(%d)' variable '%s'. Port could not be opened"
			, *GetAdsAccessTypeName(variable->adsMode)
			, variable->adsMode
			, *variable->adsName
		);

		return nullptr;
	}
	
	auto obj = new FTcAdsAsyncVariable(variable);

	if (!obj)
	{
		LOG_TCADS_WARNING("Cannot create ADS '%s(%d)' variable '%s'. Object allocation failed"
			, *GetAdsAccessTypeName(variable->adsMode)
			, variable->adsMode
			, *variable->adsName
		);

		return nullptr;
	}

	auto err = obj->fetchSymbolEntry(variable->adsName, _Instance->_adsPort, &_Instance->_remoteAmsAddr);

	if (err)
	{
		LOG_TCADS_ERROR("Adding new '%s(%d)' variable '%s' failed with error code: 0x%x"
			, *GetAdsAccessTypeName(variable->adsMode)
			, variable->adsMode
			, *variable->adsName
			, err
		);
		
		delete obj;
		return nullptr;
	}

	// Lock while working on the variable list array since this can be accessed by the main thread and the worker
	_Instance->_mutexLock.lock();
	_Instance->_variableList.Add(obj);
	_Instance->_mutexLock.unlock();

	LOG_TCADS_DISPLAY("Created new '%s(%d)' variable '%s'"
		, *GetAdsAccessTypeName(variable->adsMode)
		, variable->adsMode
		, *variable->adsName
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
		var->setVariableReference(tmp);
	}

	_variableQueue.Empty();
}

void ATcAdsAsyncMaster::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	_asyncWorker->closePort();
	
	FTcAdsAsyncWorker::Shutdown();
}

// Called every frame
// void ATcAdsAsyncMaster::Tick(float DeltaTime)
// {
// 	Super::Tick(DeltaTime);
//
// }

