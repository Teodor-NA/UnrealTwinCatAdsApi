// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
// #include "ThirdParty/TwinCatAdsApiLibrary/Include/TcAdsDef.h"
#include "TcAdsApiTypes.h"
// #include "TcAdsMaster.h"
#include "TcAdsModule.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TWINCATADSAPI_API UTcAdsModule : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTcAdsModule();

	// virtual void OnComponentCreated() override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	// virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// AmsAddr TargetAmsAddress_;

	// TSimpleBuffer<char> ReceiveBuffer_;
	// unsigned long ValidVariableCount_;

	// FTimerHandle TimerHandle;

	// static size_t UnpackAdsValues(const char* BufferPos, FSubscriberInputData& Out);
	//
	// void UpdateValues();
	
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// virtual void Tick(float DeltaSeconds) override;

	// AdsVersion DllVersion;
	// AmsAddr LocalAmsAddr;
	// int32 AdsPort;
	//
	// UPROPERTY(EditAnywhere, Category = "Time")
	// float StepTime;
	//
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, EditFixedSize, Category = "Target")
	// TArray<uint8> TargetAmsNetId;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	// int32 TargetAmsPort;

	UPROPERTY(EditAnywhere, Category = "Ads Master")
	ATcAdsMaster* AdsMaster;

	/*!
	 *	Variables to subscribe to on this module. Do not edit while game is running!
	 */
	UPROPERTY(EditAnywhere, Category = "Subscibed variables")
	TArray<FSubscriberInputData> Variables;
	
};
