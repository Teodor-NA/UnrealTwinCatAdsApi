#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TcAdsApiTypes.h"
#include "TcAdsAsyncVariable.generated.h"

UCLASS(ClassGroup=(Custom), meta = (BlueprintSpawnableComponent))
class TWINCATADSAPI_API UTcAdsAsyncVariable : public UActorComponent
{
	GENERATED_BODY()

	friend FTcAdsAsyncVariable;
	friend ATcAdsAsyncMaster;
	
public:
	UTcAdsAsyncVariable();

	virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

private:
	FTcAdsAsyncVariable* _variable;

	void _setVariableReference(FTcAdsAsyncVariable* variable) { _variable = variable; }

	float _prevVal;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "ADS Info"))
	FTcAdsVariableInfo adsSetupInfo;
	/**
	 * @brief Current value
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "ADS Value"))
	float adsValue;
	/**
	 * @brief ADS Return Code
	 * @link https://infosys.beckhoff.com/english.php?content=../content/1033/devicemanager/374277003.html&id= @endlink
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ADS Info", meta = (DisplayName = "ADS Error"))
	int32 adsError;
	
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "ADS Name"))
	// FString adsName;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "ADS Access Mode"))
	// EAdsAccessMode adsMode;
	// /**
	//  * @brief Multiple of the connected master's base time interval to update the variable.
	//  * example: if update interval is 3 and the master's base time is 0.01 s the variable will be updated every 0.03 s.
	//  * if value is <= 1 the value will update with every tick
	//  */
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "Cyckle Multiplier"))
	// int32 cycleMultiplier;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "ADS Value"))
	// float adsValue;
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ADS Info", meta = (DisplayName = "ADS Error"))
	// int32 adsError;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "ADS Master"))
	// ATcAdsAsyncMaster* adsMaster;
	//
};

