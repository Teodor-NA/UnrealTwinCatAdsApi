// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <mutex>

#include "CoreMinimal.h"
#include "TcAdsApiTypes.h"
#include "TcAdsDef.h"
#include "GameFramework/Actor.h"
#include "TcAdsAsyncMaster.generated.h"

class ATcAdsAsyncMaster;

class TWINCATADSAPI_API FTcAdsAsyncWorker : public FRunnable
{
public:
	// Singleton constructor
	static FTcAdsAsyncWorker* Create(const FString& remoteAmsNetId, LONG remoteAmsPort, float refreshRate);
	
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;

	LONG openPort();
	LONG closePort();
	
	/** Makes sure this thread has stopped properly */
	void EnsureCompletion();
	
	/** Shuts down the thread. Static so it can easily be called from outside the thread context */
	static void Shutdown();
	
	static bool IsValid() { return (_Instance != nullptr); }
	
//	static bool IsThreadFinished();

	static FTcAdsAsyncVariable* CreateVariable(UTcAdsAsyncVariable* variable);

	static void __stdcall ReadCallback(AmsAddr* pAddr, AdsNotificationHeader* pNotification, ULONG hUser);

	static AmsAddr GetRemoteAmsAddr();
	static LONG GetLocalAdsPort();

private:
	// Private constructor/destructor
	explicit FTcAdsAsyncWorker(const FString& remoteAmsNetId, int32 remoteAmsPort, float cycleTime);
	virtual ~FTcAdsAsyncWorker() override;

	// bool isFinished() const { return (_counter >= _numberToCount); }

	static AmsAddr ParseAmsAddress(const FString& netId, int32 port);

	// static LONG _Update(LONG port, AmsAddr amsAddr, TArray<UTcAdsAsyncVariable*>& readList,
	// 	TArray<UTcAdsAsyncVariable*>& writeList);

	AmsAddr _getRemoteAmsAddr();
	LONG _getLocalAdsPort();
	
	static LONG _ReadValues(LONG port, AmsAddr* amsAddr,
		TArray<FTcAdsAsyncVariable*>& readList, size_t readBufferSize);

	static LONG _WriteValues(LONG port, AmsAddr* amsAddr,
		TArray<FTcAdsAsyncVariable*>& writeList, size_t writeBufferSize);

	void _releaseHandles();
	LONG _closePort();
	
	TArray<FTcAdsAsyncVariable*> _variableList;
	
	/** Singleton instance, can access the thread any time via static accessor, if it is active! */
	static  FTcAdsAsyncWorker* _Instance;
	
	/** Thread to run the worker FRunnable on */
	FRunnableThread* _thread;
	/** Stop this thread? Uses Thread Safe Counter */
	FThreadSafeCounter _stopTaskCounter;

	LONG _adsPort;
	AmsAddr _remoteAmsAddr;

	float _cycleTime;
	
	// int32 _counter;
	// int32 _numberToCount;

	LONG _error;

	std::mutex _mutexLock;
};


UCLASS()
class TWINCATADSAPI_API ATcAdsAsyncMaster : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATcAdsAsyncMaster();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (DisplayName = "Base Time [s]"))
	float baseTime;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Remote ADS Info", meta = (DisplayName = "Remote AMS Net Id"))
	FString remoteAmsNetId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Remote ADS Info", meta = (DisplayName = "Remote AMS Port"))
	int32 remoteAmsPort;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Local ADS Info", meta = (DisplayName = "Local AMS Address"))
	FString localAmsAddress;

	FTcAdsAsyncVariable* createVariable(UTcAdsAsyncVariable* variable);

	AmsAddr getRemoteAmsAddr() const { return _asyncWorker->GetRemoteAmsAddr(); }
	LONG getLocalAdsPort() const { return _asyncWorker->GetLocalAdsPort(); }
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FTcAdsAsyncWorker* _asyncWorker;
	TArray<UTcAdsAsyncVariable*> _variableQueue;

};
