// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TcAdsApiTypes.h"
#include "GameFramework/Actor.h"
#include "TcAdsAsyncMaster.generated.h"

class ATcAdsAsyncMaster;

class TWINCATADSAPI_API FTcAdsAsyncWorker : public FRunnable
{
public:
	// Singleton constructor
	static FTcAdsAsyncWorker* Create(ATcAdsAsyncMaster* master, int32 numberToCount);
	
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;

	/** Makes sure this thread has stopped properly */
	void EnsureCompletion();
	
	/** Shuts down the thread. Static so it can easily be called from outside the thread context */
	static void Shutdown();

	static bool IsThreadFinished();

private:
	// Private constructor/destructor
	explicit FTcAdsAsyncWorker(ATcAdsAsyncMaster* master, int32 numberToCount);
	virtual ~FTcAdsAsyncWorker() override;

	bool isFinished() const { return (_counter >= _numberToCount); }
	
	/** Singleton instance, can access the thread any time via static accessor, if it is active! */
	static  FTcAdsAsyncWorker* _Instance;
	
	/** Thread to run the worker FRunnable on */
	FRunnableThread* _thread;
	/** Stop this thread? Uses Thread Safe Counter */
	FThreadSafeCounter _stopTaskCounter;
	ATcAdsAsyncMaster* _master;

	int32 _counter;
	int32 _numberToCount;

	uint32 _error;

};


UCLASS()
class TWINCATADSAPI_API ATcAdsAsyncMaster : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATcAdsAsyncMaster();

	static void LogMessage(const FString& message) { LOG_TCADS_DISPLAY("%s", *message); }
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FTcAdsAsyncWorker* _asyncWorker;

public:	
	// Called every frame
	// virtual void Tick(float DeltaTime) override;
	
};
