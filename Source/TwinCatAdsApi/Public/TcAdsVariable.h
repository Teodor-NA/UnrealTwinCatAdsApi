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

	int32 getHandle(int32 adsPort, AmsAddr* pAmsAddr, ULONG& handle);
	int32 releaseHandle(int32 adsPort, AmsAddr* pAmsAddr, ULONG& handle);
	int32 setCallback(int32 adsPort, AmsAddr* amsAddr, ADSTRANSMODE adsTransMode, TcAdsCallbackStruct& callbackStruct);
	int32 releaseCallback(int32 adsPort, AmsAddr* amsAddr, TcAdsCallbackStruct& callbackStruct);

public:
	virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

	size_t transferSize() const;
	size_t readSize() const { return (Size_ + sizeof(uint32)); }
	size_t writeSize() const { return (Size_ + sizeof(FDataPar)); }

	void release();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ads Info")
	FString AdsName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ads Info")
	EAdsAccessType Access;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ads Info")
	float Value;
	UPROPERTY(VisibleAnywhere, Category = "Ads Info")
	int32 Error;
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
	 * @param adsPort [in] An open ads port
	 * @param amsAddress [in] Ams address to remote machine
	 * @param dataEntries [opt][in/out] Array of data entries for ADS-SUM commands
	 * @param bufferSize [opt][in/out] Buffer size for ADS-SUM commands
	 * @return Ads error code
	 */
	uint32 getSymbolEntryFromAds(int32 adsPort, AmsAddr* amsAddress, TArray<FDataPar>* dataEntries = nullptr, size_t* bufferSize = nullptr);
	int32 setupCallbackVariable(int32 adsPort, AmsAddr* amsAddr, ADSTRANSMODE adsTransMode, TcAdsCallbackStruct& callbackStruct);
	int32 terminateCallbackVariable(int32 adsPort, AmsAddr* amsAddr, TcAdsCallbackStruct& callbackStruct);
	
	template <class TDst, class TSrc>
	static void CopyCast(void* pDst, const void* pSrc);

	size_t unpackValue(const char* errorSrc, const char* valueSrc, uint32 errorIn);
	size_t unpackValue(const char* valueSrc);
	size_t packValue(char* valueDst) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ads Info")
	ATcAdsMaster* AdsMaster;
	
private:
	
	UPROPERTY(VisibleAnywhere, Category = "Ads Info")
	EAdsDataTypeId DataType_;

	UPROPERTY(VisibleAnywhere, Category = "Ads Info")
	uint32 Size_;
};

template <class TDst, class TSrc>
void UTcAdsVariable::CopyCast(void* pDst, const void* pSrc)
{
	// TDst* dst = pDst;
	// TSrc* src = pSrc;
	// *dst = static_cast<TDst>(*src);

	*static_cast<TDst*>(pDst) = static_cast<TDst>(*static_cast<const TSrc*>(pSrc));
}
