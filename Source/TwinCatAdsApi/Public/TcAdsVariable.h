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
	explicit UTcAdsVariable(ATcAdsMaster* master) : UTcAdsVariable() { AdsMaster = master; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

public:
	virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

	size_t transferSize() const;
	size_t readSize() const { return (Size_ + sizeof(uint32)); }
	size_t writeSize() const { return (Size_ + sizeof(FDataPar)); }

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
	 * unsigned variables. If you are using c++ use \c size instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ads Info")
	int64 GetSize() const { return Size_; }
	
	/*!
	 * Get ADS variable size
	 */
	uint32 size() const { return Size_; }
	/*!
	 * Contact the plc to get variable information for ADS (this is done automatically by TcAdsMaster)
	 */
	uint32 getSymbolEntryFromAds(int32 adsPort, AmsAddr& amsAddress, TArray<FDataPar>& out);

	bool newVar() const { return _newVar; }

	template <class TDst, class TSrc>
	static void copyCast(void* dst, const void* src);

	size_t unpackValues(const char* errorSrc, const char* valueSrc, uint32 errorIn);
	size_t packValues(char* valueDst) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ads Info")
	ATcAdsMaster* AdsMaster;
	
private:
	
	UPROPERTY(VisibleAnywhere, Category = "Ads Info")
	EAdsDataTypeId DataType_;

	UPROPERTY(VisibleAnywhere, Category = "Ads Info")
	uint32 Size_;

	bool _newVar;
};

template <class TDst, class TSrc>
void UTcAdsVariable::copyCast(void* dst, const void* src)
{
	// TDst* dst = Dst;
	// TSrc* src = Src;
	// *dst = static_cast<TDst>(*src);

	*static_cast<TDst*>(dst) = static_cast<TDst>(*static_cast<const TSrc*>(src));
}
