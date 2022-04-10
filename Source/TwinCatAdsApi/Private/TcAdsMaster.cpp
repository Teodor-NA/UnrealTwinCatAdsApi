// Fill out your copyright notice in the Description page of Project Settings.


#include "TcAdsMaster.h"
#include "TcAdsVariable.h"
#include "Misc/DefaultValueHelper.h"
#include "TcAdsAPI.h"

// Sets default values
ATcAdsMaster::ATcAdsMaster() :
	RemoteAmsAddress_({{0, 0, 0, 0, 0, 0}, 0}),
	ReadBufferSize_(0),
	WriteBufferSize_(0),
	ReadValuesInterval(0.1),
	WriteValuesInterval(0.1),
	RemoteAmsNetId(TEXT("127.0.0.1.1.1")),
	RemoteAmsPort(851)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green,TEXT("Ads master loaded"));

}

// Called when the game starts or when spawned
void ATcAdsMaster::BeginPlay()
{
	Super::BeginPlay();

	bool AmsAddrParsed = ParseAmsAddress(RemoteAmsNetId, RemoteAmsPort, RemoteAmsAddress_);

	if (AmsAddrParsed)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green,
				FString::Printf(TEXT("Attempting communication with remote AMS Net Id %u.%u.%u.%u.%u.%u on port %u"),
					RemoteAmsAddress_.netId.b[0],
					RemoteAmsAddress_.netId.b[1],
					RemoteAmsAddress_.netId.b[2],
					RemoteAmsAddress_.netId.b[3],
					RemoteAmsAddress_.netId.b[4],
					RemoteAmsAddress_.netId.b[5],
					RemoteAmsAddress_.port
					));
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red,
				FString::Printf(TEXT("Failed to parse '%s' into an AMS Net Id\nAborting ADS communication"), *RemoteAmsNetId));
		}
		
		return;
	}
	
	// Open port
	AdsPort = AdsPortOpenEx();
	
	if (AdsPort)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green, FString::Printf(TEXT("Local port: %d opened"), AdsPort));
	}
	else
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, TEXT("Port opening failed\nAborting ADS communication"));
	
		return;
	}

	char RemoteAsciiDevName[50];
	AdsVersion RemoteVersion;
	
	auto Err = AdsSyncReadDeviceInfoReqEx(AdsPort, &RemoteAmsAddress_, RemoteAsciiDevName, &RemoteVersion);
	
	if (Err)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red,
				 FString::Printf( TEXT("Failed to get device info from '%s'\nAborting ADS communication"), *RemoteAmsNetId));
		}
		return;
	}

	FString RemoteDevName(RemoteAsciiDevName);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green,
			 FString::Printf( TEXT("Connected to '%s' using version %u.%u.%u \nStarting ADS communication"),
			 	*RemoteDevName,
			 	RemoteVersion.version,
			 	RemoteVersion.revision,
			 	RemoteVersion.build
			 	));
	}

	// Start timers
	GetWorldTimerManager().SetTimer(ReadValuesTimerHandle_,this, &ATcAdsMaster::ReadValues, ReadValuesInterval, true);
	GetWorldTimerManager().SetTimer(WriteValuesTimerHandle_,this, &ATcAdsMaster::WriteValues, WriteValuesInterval, true);
}

void ATcAdsMaster::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Stop timers
	GetWorldTimerManager().ClearTimer(ReadValuesTimerHandle_);
	GetWorldTimerManager().ClearTimer(WriteValuesTimerHandle_);

	/// Seems like we don't need to release handles, since we are actually using the index group and offset to access
	/// variables, and therefore handles are not created for us. Might be wrong though, the documentation isn't exactly
	/// clear on this...

	// Clear lists. Need to do this since these are uproperties exposed to the editor and therefore have a weird ass lifespan
	ReadVariableList.Empty();
	WriteVariableList.Empty();
	
	if (AdsPort)
	{
		AdsPortCloseEx(AdsPort);
		AdsPort = 0;
	}
	
}

void ATcAdsMaster::RemoveVariable(const UTcAdsVariable* Variable, TArray<UTcAdsVariable*>& VarList,
	TArray<FDataPar>& ReqList, size_t& BufferSize) //, EAdsAccessType Access)
{
	if (!Variable)
		return;
	if (!Variable->ValidAds)
		return;

	auto Idx = VarList.IndexOfByKey(Variable);
	if (Idx == INDEX_NONE)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow,
			   FString::Printf(TEXT("No variable '%s' found in list. No action taken"), *Variable->AdsName));
		}
	}
	else
	{
		VarList.RemoveAt(Idx);
		ReqList.RemoveAt(Idx);
		BufferSize -= Variable->TransferSize();
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green,
			   FString::Printf(TEXT("Unsubscribed variable '%s'"), *Variable->AdsName));
		}
	}
}

void ATcAdsMaster::CheckForNewVars(TArray<UTcAdsVariable*>& Vars, TArray<FDataPar>& ReqBuffer, size_t& BufferSize) //, EAdsAccessType Access)
{
	// Check for new variables in the list. Since new variables are always added at the end, we check in reverse
	// (so that normally we will check the first entry and immediately step out).
	bool NewVar = false;
	int32 NewIndex = 0;
	for (auto i = Vars.Num(); i--;)
	{
		// Skip variables that are pending kill (this shouldn't happen, but you never know with unreal...)
		if (!IsValid(Vars[i]))
			continue;
		
		if (Vars[i]->NewVar())
		{
			NewVar = true;
			NewIndex = i;
		}
		else
			break;
	}

	// Nothing to do
	if (!NewVar)
		return;

	// If there are any new variables then we must ask ADS for them in forward order, so that the
	// arrays xxxVariableList and xxxReqBuffer_ are synchronized
	for (auto i = NewIndex; i < Vars.Num(); ++i)
	{
		// Skip variables that are pending kill again
		if (!IsValid(Vars[i]))
			continue;
		
		auto Err = Vars[i]->GetSymbolEntryFromAds(AdsPort, RemoteAmsAddress_, ReqBuffer);
		if (!Err)
		{
			BufferSize += Vars[i]->TransferSize();
		}
		
		if (GEngine)
		{
			if (Err)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Yellow,
					FString::Printf(TEXT("Failed to subscribe to '%s' variable '%s'. Error code 0x%x"),
						AdsAccessTypeName(Vars[i]->Access),
						*Vars[i]->AdsName,
						Err
						));
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green,
					FString::Printf(TEXT("Successfully subscribed to '%s' variable '%s'"),
						AdsAccessTypeName(Vars[i]->Access),
						*Vars[i]->AdsName
						));
			}
		}
	}
}

bool ATcAdsMaster::ParseAmsAddress(const FString& NetId, const int32 Port, AmsAddr& Out)
{
	if ((Port > 65535) || (Port < 0))
		return false;
	
	AmsAddr TempAddr = {{0, 0,0 ,0, 0,0}, static_cast<uint16>(Port)};

	TCHAR Delim = TEXT('.');
	
	TArray<FString> Tmp;
	NetId.ParseIntoArray(Tmp, &Delim);

	if (Tmp.Num() != 6)
		return false;

	int32 Idx = 0;
	for (auto& Str : Tmp)
	{
		int32 Val;
		bool Success = FDefaultValueHelper::ParseInt(Str, Val);
		if ((Val > 255) || (Val < 0))
			Success = false;

		if (!Success)
			return false;

		TempAddr.netId.b[Idx] = static_cast<uint8>(Val);
		++Idx;
	}

	Out = TempAddr;

	return true;
}

// Called every frame
void ATcAdsMaster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATcAdsMaster::AddReadVariable(UTcAdsVariable* Variable)
{
	if (Variable)
		ReadVariableList.Add(Variable);
}

void ATcAdsMaster::AddWriteVariable(UTcAdsVariable* Variable)
{
	if (Variable)
		WriteVariableList.Add(Variable);
}

void ATcAdsMaster::ReadValues()
{
	CheckForNewVars(ReadVariableList, ReadReqBuffer_, ReadBufferSize_); //, EAdsAccessType::Read);

	// Nothing to do
	if (ReadReqBuffer_.Num() == 0)
		return;
	
	TSimpleBuffer<char> ReadBuffer(ReadBufferSize_);
	
	// Get data from ADS
	unsigned long BytesRead;
	long Err = AdsSyncReadWriteReqEx2(
		AdsPort,
		&RemoteAmsAddress_,
		ADSIGRP_SUMUP_READ,
		ReadReqBuffer_.Num(),
		ReadBuffer.ByteSize(),
		ReadBuffer.GetData(),
		ReadReqBuffer_.Num()*sizeof(FDataPar),
		ReadReqBuffer_.GetData(),
		&BytesRead
	);

	if (Err && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 5, FColor::Red,
		   FString::Printf(TEXT("Failed to get data from ADS, error code 0x%x"), Err));
	}

	// Unpack data
	char* pErrorPos = ReadBuffer.GetData();
	char* pValuePos = pErrorPos + ReadReqBuffer_.Num()*sizeof(uint32);
	for (auto AdsVar : ReadVariableList)
	{
		if (IsValid(AdsVar) && AdsVar->ValidAds)
		{
			pValuePos += AdsVar->UnpackValues(pErrorPos, pValuePos, Err);
			pErrorPos += sizeof(uint32);
		}
	}
}

void ATcAdsMaster::WriteValues()
{
	CheckForNewVars(WriteVariableList, WriteReqBuffer_, WriteBufferSize_); //, EAdsAccessType::Write);

	// Nothing to do
	if (WriteReqBuffer_.Num() == 0)
		return;

	size_t ReqSize = WriteReqBuffer_.Num()*sizeof(FDataPar);
	
	TSimpleBuffer<char> WriteBuffer(WriteBufferSize_);
	// Copy data from Write request buffer
	memcpy_s(WriteBuffer.GetData(), WriteBuffer.ByteSize(), WriteReqBuffer_.GetData(), ReqSize);

	char* BufferPos = WriteBuffer.GetData() + ReqSize;
	
	// Get data from variables
	for (auto AdsVar : WriteVariableList)
	{
		if (IsValid(AdsVar) && AdsVar->ValidAds)
		{
			BufferPos += AdsVar->PackValues(BufferPos);
		}
	}

	// Make a buffer for the ADS return codes
	TSimpleBuffer<uint32> ErrorBuffer(WriteReqBuffer_.Num());
	
	// Get data from ADS
	unsigned long BytesRead;
	long Err = AdsSyncReadWriteReqEx2(
		AdsPort,
		&RemoteAmsAddress_,
		ADSIGRP_SUMUP_WRITE,
		WriteReqBuffer_.Num(),
		ErrorBuffer.ByteSize(),
		ErrorBuffer.GetData(),
		WriteBuffer.ByteSize(),
		WriteBuffer.GetData(),
		&BytesRead
	);

	if (Err && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2, 5, FColor::Red,
		   FString::Printf(TEXT("Failed to write data to ADS, error code 0x%x"), Err));
	}

	// Unpack errors
	uint32* pErrorPos = ErrorBuffer.GetData();
	for (auto AdsVar : WriteVariableList)
	{
		if (IsValid(AdsVar) && AdsVar->ValidAds)
		{
			AdsVar->Error = *pErrorPos;
			++pErrorPos;
		}
	}
}
