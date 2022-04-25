// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TcAdsDef.h"
#include "TcAdsApiTypes.h"
#include "TcAdsVariable.h"
#include "TcAdsMaster.generated.h"

UCLASS()
class TWINCATADSAPI_API ATcAdsMaster : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATcAdsMaster();

	void readValues();
	void writeValues();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	
	// static void RemoveVariable(const UTcAdsVariable* Variable, TArray<UTcAdsVariable*>& VarList,
	// 	TArray<FDataPar>& ReqList, size_t& BufferSize); //, EAdsAccessType Access);
	
	AmsAddr _remoteAmsAddress;
	FTimerHandle _readValuesTimerHandle;
	FTimerHandle _writeValuesTimerHandle;
	FTimerHandle _updateListsTimerHandle;

	TArray<FDataPar> _readReqBuffer;
	size_t _readBufferSize;
	TArray<FDataPar> _writeReqBuffer;
	size_t _writeBufferSize;

	void updateVarLists();
	void checkForNewVars(TArray<UTcAdsVariable*>& vars, TArray<FDataPar>& reqBuffer, size_t& bufferSize);
	void checkForCallbackVars();
	static bool parseAmsAddress(const FString& netId, int32 port, AmsAddr& out);

	static void removeVariablePrivate(const UTcAdsVariable* variable, TArray<UTcAdsVariable*>& variableList,
		TArray<FDataPar> reqBuffer, size_t& bufferSize);

	static void removeCallbackVariable(const UTcAdsVariable* variable);
	
	static void __stdcall ReadCallback(AmsAddr* pAddr, AdsNotificationHeader* pNotification, ULONG hUser);

	static TArray<TcAdsCallbackStruct> _CallbackList;
	
public:	
	// Called every frame
	virtual void Tick(float deltaTime) override;
	
	void addVariable(UTcAdsVariable* variable);
	void removeVariable(const UTcAdsVariable* variable);
	
	// Intervals between calls to the plc to read data [s]. A value of <=0 disables communication
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time",
		meta = (DisplayName = "Read Values Interval [s]"))
	float ReadValuesInterval;
	
	// Intervals between calls to the plc to write data [s]. A value of <=0 disables communication
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time",
		meta = (DisplayName = "Write Values Interval [s]"))
	float WriteValuesInterval;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time",
		meta = (DisplayName = "Read Data Round Trip Time [ms]"))
	float ReadDataRoundTripTime;
	// Interval between checking for new variables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time",
		meta = (DisplayName = "Update Variable Lists Interval [s]"))
	float UpdateListsInterval;
	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, EditFixedSize, Category = "Remote ADS Info")
	FString RemoteAmsNetId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Remote ADS Info")
	int32 RemoteAmsPort;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Remote ADS Info")
	bool RemoteAmsAddressValid;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Remote ADS Info")
	FString RemoteDeviceName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Remote ADS Info")
	FString RemoteAdsVersion;

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

