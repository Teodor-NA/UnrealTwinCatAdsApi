// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TcAdsApiTypes.h"
#include "ThirdParty/TwinCatAdsApiLibrary/Include/TcAdsDef.h"
#include "TcAdsVariable.generated.h"

/**
 * 
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class TWINCATADSAPI_API UTcAdsVariable : public UActorComponent // public UObject
{
	GENERATED_BODY()

public:
	UTcAdsVariable();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, Category = "Ads Variable")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ads Variable")
	float Value;
	UPROPERTY(VisibleAnywhere, Category = "Ads Variable")
	uint32 Error;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ads Variable")
	bool Valid;

	UFUNCTION(BlueprintCallable, Category = "Ads Variable")
	int64 GetError() const { return Error; }
	UFUNCTION(BlueprintCallable, Category = "Ads Variable")
	EAdsDataTypeId GetDataType() const { return static_cast<EAdsDataTypeId>(SymbolEntry_.dataType); }
	/*!
	 * Blueprint friendly way to get ADS variable size. Incurs an additional cast since blueprint does not support
	 * unsigned variables. If you are using c++ use \c Size() instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ads Variable")
	int64 GetSize() const { return SymbolEntry_.size; }

	/*!
	 * Get ADS variable size
	 */
	uint32 Size() const { return SymbolEntry_.size; }
	/*!
	 * Contact the plc to get variable information for ADS (this is done automatically by TcAdsMaster)
	 */
	uint32 GetSymbolEntryFromAds(int32 AdsPort, AmsAddr& AmsAddress);

	const FDataPar* GetRequestData() const { return reinterpret_cast<const FDataPar*>(&SymbolEntry_.iGroup); }
	void SetExternalRequestData(FDataPar& Out) const { Out = *GetRequestData(); }

	size_t UnpackValues(const char* ErrorSrc, const char* ValueSrc);
	
private:
	AdsSymbolEntry SymbolEntry_;
//	FDataPar DataPar_;
	
	// UPROPERTY(VisibleAnywhere, Category = "Variable")
	// EAdsDataTypeId DataType;

	// UPROPERTY(VisibleAnywhere, Category = "Variable")
	// uint32 Size;
	
	// uint32 IndexGroup;
	// uint32 IndexOffset;
};
