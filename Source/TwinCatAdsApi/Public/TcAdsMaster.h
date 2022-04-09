// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ThirdParty/TwinCatAdsApiLibrary/Include/TcAdsDef.h"
#include "TcAdsApiTypes.h"
// #include "TcAdsVariable.h"
#include "TcAdsMaster.generated.h"

UCLASS()
class TWINCATADSAPI_API ATcAdsMaster : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATcAdsMaster();

	void ReadValues();
	void WriteValues();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	static void RemoveVariable(const UTcAdsVariable* Variable, TArray<UTcAdsVariable*>& VarList,
		TArray<FDataPar>& ReqList, size_t& BufferSize); //, EAdsAccessType Access);
	
	AmsAddr RemoteAmsAddress_;
	FTimerHandle ReadValuesTimerHandle_;
	FTimerHandle WriteValuesTimerHandle_;

	TArray<FDataPar> ReadReqBuffer_;
	size_t ReadBufferSize_;
	TArray<FDataPar> WriteReqBuffer_;
	size_t WriteBufferSize_;
	
	// AdsVersion DllVersion_;
	// AmsAddr LocalAmsAddr_;

	void CheckForNewVars(TArray<UTcAdsVariable*>& Vars, TArray<FDataPar>& ReqBuffer, size_t& BufferSize); //, EAdsAccessType Acess);
	static bool ParseAmsAddress(const FString& NetId, const int32 Port, AmsAddr& Out);

	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	void AddReadVariable(UTcAdsVariable* Variable);
	void AddWriteVariable(UTcAdsVariable* Variable);
	void RemoveReadVariable(const UTcAdsVariable* Variable)
	{ RemoveVariable(Variable, ReadVariableList, ReadReqBuffer_, ReadBufferSize_); } //, EAdsAccessType::Read); }
	void RemoveWriteVariable(const UTcAdsVariable* Variable)
	{ RemoveVariable(Variable, WriteVariableList, WriteReqBuffer_, WriteBufferSize_); } //, EAdsAccessType::Write); }
	
	/*!
	 * Intervals between calls to the plc to refresh data [s]
	 */
	UPROPERTY(EditAnywhere, Category = "Time")
	float ReadValuesInterval;
	UPROPERTY(EditAnywhere, Category = "Time")
	float WriteValuesInterval;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, EditFixedSize, Category = "Remote ADS Info")
	FString RemoteAmsNetId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Remote ADS Info")
	int32 RemoteAmsPort;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Local ADS Info")
	int32 AdsPort;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Local ADS Info")
	FString LocalAmsAddress;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Local ADS Info")
	FString LocalDllVersion;
	
	UPROPERTY(VisibleAnywhere, Category = "Subscribed Variables")
	TArray<UTcAdsVariable*> ReadVariableList;
	UPROPERTY(VisibleAnywhere, Category = "Subscribed Variables")
	TArray<UTcAdsVariable*> WriteVariableList;

};

