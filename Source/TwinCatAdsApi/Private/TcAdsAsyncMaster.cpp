// Fill out your copyright notice in the Description page of Project Settings.


#include "TcAdsAsyncMaster.h"


FTcAdsAsyncWorker* FTcAdsAsyncWorker::_Instance = nullptr;


FTcAdsAsyncWorker::FTcAdsAsyncWorker(ATcAdsAsyncMaster* master, int32 numberToCount) :
	_thread(nullptr),
	_master(master),
	_counter(0),
	_numberToCount(numberToCount),
	_error(0)
{
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
	
	LOG_TCADS_DISPLAY("Destroying thread %u: %s", _thread->GetThreadID(), *_thread->GetThreadName());
	delete _thread;
	_thread = nullptr;
}

FTcAdsAsyncWorker* FTcAdsAsyncWorker::Create(ATcAdsAsyncMaster* master, int32 numberToCount)
{
	//Create new instance of thread if it does not exist
	//		and the platform supports multi threading!
	if (!_Instance && FPlatformProcess::SupportsMultithreading())
	{
		_Instance = new FTcAdsAsyncWorker(master, numberToCount);			
	}
	return _Instance;
}

bool FTcAdsAsyncWorker::Init()
{
	return true;
}

uint32 FTcAdsAsyncWorker::Run()
{
	FPlatformProcess::Sleep(0.3f);
	
	while ((_stopTaskCounter.GetValue() == 0) && !isFinished())
	{
		if (_thread)
		{
			LOG_TCADS_DISPLAY("Thread %u %s ticked and has counted to %d/%d",
			   _thread->GetThreadID(),
			   *_thread->GetThreadName(),
			   ++_counter,
			   _numberToCount
		   );
		}
		else
		{
			LOG_TCADS_WARNING("Thread doesen't exist, but we still ticked and counted to %d/%d",
			   ++_counter,
			   _numberToCount
		   );
		}

		// LOG_TCADS_DISPLAY("Thread %u %s ticked and has counted to %d/%d",
		// 	_thread->GetThreadID(),
		// 	*_thread->GetThreadName(),
		// 	++_counter,
		// 	_numberToCount
		// );
		FPlatformProcess::Sleep(1.0f);
	}

	if (!isFinished())
		_error = 1;

	if (_thread)
	{
		LOG_TCADS_DISPLAY("Thread %u %s finished with return code %u",
			_thread->GetThreadID(),
			*_thread->GetThreadName(),
			_error
		);
	}
	
	return _error;
}

void FTcAdsAsyncWorker::Stop()
{
	_stopTaskCounter.Increment();
}

void FTcAdsAsyncWorker::EnsureCompletion()
{
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
		if (_Instance->_thread)
		{
			if (_Instance->isFinished())
			{
				LOG_TCADS_DISPLAY("Thread %u %s was shut down successfully", _Instance->_thread->GetThreadID(), *_Instance->_thread->GetThreadName());
			}
			else
			{
				LOG_TCADS_DISPLAY("Thread %u %s was shut down prematurely", _Instance->_thread->GetThreadID(), *_Instance->_thread->GetThreadName());
			}
		}
		delete _Instance;
		_Instance = nullptr;
	}
}

bool FTcAdsAsyncWorker::IsThreadFinished()
{
	if (_Instance) return _Instance->isFinished();
	return true;
}

// Sets default values
ATcAdsAsyncMaster::ATcAdsAsyncMaster() :
	_asyncWorker(nullptr)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ATcAdsAsyncMaster::BeginPlay()
{
	Super::BeginPlay();

	_asyncWorker = FTcAdsAsyncWorker::Create(this, 10);
}

void ATcAdsAsyncMaster::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	FTcAdsAsyncWorker::Shutdown();
}

// Called every frame
// void ATcAdsAsyncMaster::Tick(float DeltaTime)
// {
// 	Super::Tick(DeltaTime);
//
// }

