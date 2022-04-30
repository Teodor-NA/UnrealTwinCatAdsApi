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
	size_t readSize() const { return (_symbolEntry.size + sizeof(uint32)); }
	size_t writeSize() const { return (_symbolEntry.size + sizeof(FDataPar)); }

	void release();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ads Info")
	FString AdsName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ads Info")
	EAdsAccessType Access;
	UPROPERTY(VisibleAnywhere, Category = "Ads Info")
	int32 Error;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ads Info")
	bool ValidAds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ads Info")
	float Value;
	
	UFUNCTION(BlueprintCallable, Category = "Ads Info")
	int64 GetError() const { return Error; }
	UFUNCTION(BlueprintCallable, Category = "Ads Info")
	EAdsDataTypeId GetDataType() const { return static_cast<EAdsDataTypeId>(_symbolEntry.dataType); }
	/*!
	 * Blueprint friendly way to get ADS variable size. Incurs an additional cast since blueprint does not support
	 * unsigned variables. If you are using c++ use \c symbolSize instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ads Info")
	int64 SymbolSize() const { return _symbolEntry.size; }
	
	/*!
	 * Get ADS variable size
	 */
	uint32 symbolSize() const { return _symbolEntry.size; }
	/*!
	 * Contact the plc to get variable information for ADS (this is done automatically by TcAdsMaster)
	 * @param adsPort [in] An open ads port
	 * @param amsAddress [in] Ams address to remote machine
	 * @return Ads error code
	 */
	uint32 getSymbolEntryFromAds(int32 adsPort, AmsAddr* amsAddress);
	int32 setupCallbackVariable(int32 adsPort, AmsAddr* amsAddr, ADSTRANSMODE adsTransMode, TcAdsCallbackStruct& callbackStruct);
	int32 terminateCallbackVariable(int32 adsPort, AmsAddr* amsAddr, TcAdsCallbackStruct& callbackStruct);
	
	template <class TDst, class TSrc>
	static void CopyCast(void* pDst, const void* pSrc);

	size_t unpackValue(const char* errorSrc, const char* valueSrc, uint32 errorIn);
	size_t unpackValue(const char* valueSrc);
	size_t packValue(char* valueDst) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ads Info")
	ATcAdsMaster* AdsMaster;

	// For internal use. Don't use this
	bool valueChanged();
	// For internal use. Don't use this
	bool readyToWrite() { return (((Access == EAdsAccessType::WriteOnChange) || (Access == EAdsAccessType::ReadWriteOnChange)) ? valueChanged() : true); }

	static bool ValidAdsVariable(const UTcAdsVariable* variable);

	const AdsSymbolEntry& symbolEntry() const { return _symbolEntry; }
	const FDataPar& reqData() const { return *reinterpret_cast<const FDataPar*>(&_symbolEntry.iGroup); }
	
private:
	AdsSymbolEntry _symbolEntry;
	
	float _prevVal;
};

template <class TDst, class TSrc>
void UTcAdsVariable::CopyCast(void* pDst, const void* pSrc)
{
	// TDst* dst = pDst;
	// TSrc* src = pSrc;
	// *dst = static_cast<TDst>(*src);

	*static_cast<TDst*>(pDst) = static_cast<TDst>(*static_cast<const TSrc*>(pSrc));
}
