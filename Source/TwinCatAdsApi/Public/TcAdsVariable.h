// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TcAdsApiTypes.h"
#include "TcAdsDef.h"
#include "TcAdsVariable.generated.h"

/**
 * 
 */
UCLASS(ClassGroup=(Custom), meta = (BlueprintSpawnableComponent))
class TWINCATADSAPI_API UTcAdsVariable : public UActorComponent // public USceneComponent // public UActorComponent // public UObject
{
	GENERATED_BODY()

public:
	UTcAdsVariable();
	explicit UTcAdsVariable(ATcAdsMaster* Master) : UTcAdsVariable() { AdsMaster = Master; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	size_t TransferSize() const;
	size_t ReadSize() const { return (Size_ + sizeof(uint32)); }
	size_t WriteSize() const { return (Size_ + sizeof(FDataPar)); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ads Info")
	FString AdsName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ads Info")
	EAdsAccessType Access;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ads Info")
	float Value;
	UPROPERTY(VisibleAnywhere, Category = "Ads Info")
	uint32 Error;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ads Info")
	bool ValidAds;
	
	UFUNCTION(BlueprintCallable, Category = "Ads Info")
	int64 GetError() const { return Error; }
	UFUNCTION(BlueprintCallable, Category = "Ads Info")
	EAdsDataTypeId GetDataType() const { return DataType_; }
	/*!
	 * Blueprint friendly way to get ADS variable size. Incurs an additional cast since blueprint does not support
	 * unsigned variables. If you are using c++ use \c Size instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ads Info")
	int64 GetSize() const { return Size_; }
	
	/*!
	 * Get ADS variable size
	 */
	uint32 Size() const { return Size_; }
	/*!
	 * Contact the plc to get variable information for ADS (this is done automatically by TcAdsMaster)
	 */
	uint32 GetSymbolEntryFromAds(int32 AdsPort, AmsAddr& AmsAddress, TArray<FDataPar>& Out);

	bool NewVar() const { return NewVar_; }

	template <class TDst, class TSrc>
	static void CopyCast(void* Dst, const void* Src);

	size_t UnpackValues(const char* ErrorSrc, const char* ValueSrc, uint32 ErrorIn);
	size_t PackValues(char* ValueDst) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ads Info")
	ATcAdsMaster* AdsMaster;
	
private:
	
	UPROPERTY(VisibleAnywhere, Category = "Ads Info")
	EAdsDataTypeId DataType_;

	UPROPERTY(VisibleAnywhere, Category = "Ads Info")
	uint32 Size_;

	bool NewVar_;
};

template <class TDst, class TSrc>
void UTcAdsVariable::CopyCast(void* Dst, const void* Src)
{
	// TDst* dst = Dst;
	// TSrc* src = Src;
	// *dst = static_cast<TDst>(*src);

	*static_cast<TDst*>(Dst) = static_cast<TDst>(*static_cast<const TSrc*>(Src));
}
