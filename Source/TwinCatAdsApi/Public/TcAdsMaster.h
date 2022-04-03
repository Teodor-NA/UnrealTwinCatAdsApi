// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ThirdParty/TwinCatAdsApiLibrary/Include/TcAdsDef.h"
#include "TcAdsApiTypes.h"
// #include "TcAdsModule.h"
#include "TcAdsMaster.generated.h"

UCLASS()
class TWINCATADSAPI_API ATcAdsMaster : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATcAdsMaster();
	
	static size_t UnpackAdsValues(const char* BufferPos, FSubscriberInputData& Out);

	// Timer delegates
	void UpdateValues();
	void UpdateVars();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	AmsAddr TargetAmsAddress_;
	FTimerHandle GetDataTimerHandle_;
	// FTimerHandle CheckVarsTimerHandle_;

	TSimpleBuffer<FDataPar> ReqBuffer_;
	TSimpleBuffer<char> ReceiveBuffer_;
	// unsigned long ValidVariableCount_;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// float* GetReadVariableReferenceByName(const FString& Name);
	
	AdsVersion DllVersion;
	AmsAddr LocalAmsAddr;
	int32 AdsPort;

	UFUNCTION(BlueprintCallable)
	float GetReadVariable(int Index) const;
	UFUNCTION(BlueprintCallable)
	void SetWriteVariable(int Index, float Val);
	
	/*!
	 * Intervals between calls to the plc to refresh data [s]
	 */
	UPROPERTY(EditAnywhere, Category = "Time")
	float GetDataInterval;
	/*!
	 * Intervals between when to check for new variables [s]
	 */
	// UPROPERTY(EditAnywhere, Category = "Time")
	// float CheckForVariablesDelay;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, EditFixedSize, Category = "Target")
	TArray<uint8> TargetAmsNetId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	int32 TargetAmsPort;
	
	UPROPERTY(EditAnywhere, Category = "Subscribed Variables")
	TArray<FSubscriberInputData> ReadVariableList;
	UPROPERTY(EditAnywhere, Category = "Subscribed Variables")
	TArray<FSubscriberInputData> WriteVariableList;

};
