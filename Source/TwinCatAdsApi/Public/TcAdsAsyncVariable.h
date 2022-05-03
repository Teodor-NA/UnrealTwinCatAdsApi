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
	
public:
	UTcAdsAsyncVariable();

	virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;
	
	UFUNCTION(BlueprintCallable)
	float getValue() const;

	UFUNCTION(BlueprintCallable)
	float setValue(float val) const;

	void setVariableReference(FTcAdsAsyncVariable* variable) { _variable = variable; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

private:
	FTcAdsAsyncVariable* _variable;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "ADS Name"))
	FString adsName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "ADS Access Mode"))
	EAdsAccessMode adsMode;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "Initial Value"))
	float initialValue;
	/**
	 * @brief Multiple of the connected master's base time interval to update the variable.
	 * example: if update interval is 3 and the master's base time is 0.01 s the variable will be updated every 0.03 s.
	 * if value is <= 1 the value will update with every tick
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "Update Interval"))
	int32 updateInterval;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "ADS Master"))
	ATcAdsAsyncMaster* adsMaster;
	
};

