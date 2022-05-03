#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TcAdsApiTypes.h"
#include "TcAdsAsyncVariable_Private.h"
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "Initial value"))
	float initialValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "ADS Master"))
	ATcAdsAsyncMaster* adsMaster;
	
};

