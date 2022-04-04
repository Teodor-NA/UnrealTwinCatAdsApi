// Fill out your copyright notice in the Description page of Project Settings.


#include "TcAdsMaster.h"
#include "ThirdParty/TwinCatAdsApiLibrary/Include/TcAdsAPI.h"

// Sets default values
ATcAdsMaster::ATcAdsMaster() : ReadValuesInterval(0.1), WriteValuesInterval(0.1), TargetAmsNetId({127, 0, 0, 1, 1, 1}), TargetAmsPort(851)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green,TEXT("Ads master loaded"));

	UE_LOG(LogTemp, Display, TEXT("Ads master loaded") );

}

// Called when the game starts or when spawned
void ATcAdsMaster::BeginPlay()
{
	Super::BeginPlay();
	
	GetWorldTimerManager().SetTimer(ReadValuesTimerHandle_,this, &ATcAdsMaster::ReadValues, ReadValuesInterval, true);
	// Delay getting symbol entries from ads by 100ms so that we are sure that all the variables have had time to load
	GetWorldTimerManager().SetTimer(SymbolEntriesTimerHandle_,this, &ATcAdsMaster::GetSymbolEntriesFromAds, 0.1, false);
	
	TargetAmsAddress_.port = TargetAmsPort;
	memcpy_s(TargetAmsAddress_.netId.b, 6, TargetAmsNetId.GetData(), 6);
	
	// Open port
	AdsPort = AdsPortOpenEx();
	
	if (AdsPort)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, FString::Printf(TEXT("Port: %ld opened"), AdsPort));
	}
	else
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, TEXT("Port opening failed"));
	
		return;
	}	
	
	// Get local address
	AdsGetLocalAddressEx(AdsPort, &LocalAmsAddr);
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow,
			FString::Printf(TEXT("Local ams address: %hhu.%hhu.%hhu.%hhu.%hhu.%hhu"),
				LocalAmsAddr.netId.b[0], LocalAmsAddr.netId.b[1], LocalAmsAddr.netId.b[2], LocalAmsAddr.netId.b[3], LocalAmsAddr.netId.b[4], LocalAmsAddr.netId.b[5]
				)
			);
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow,
			FString::Printf(TEXT("Local ams port: %hu"), LocalAmsAddr.port));
	}

	// GetSymbolEntriesFromAds();
}

void ATcAdsMaster::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Stop timers
	GetWorldTimerManager().ClearTimer(ReadValuesTimerHandle_);
	GetWorldTimerManager().ClearTimer(SymbolEntriesTimerHandle_);

	/// Seems like we don't need to release handles, since we are actually using the index group and offset to access
	/// variables, and therefore handles are not created for us. Might be wrong though, the documentation isn't exactly
	/// clear on this...
	/// 
	// Release all handles
	// for (auto& Var : ReadVariableList)
	// {
	// 	if (!Var.Valid)
	// 		continue;
	// 	
	// 	long Err = AdsSyncWriteReqEx(
	// 		AdsPort,
	// 		&TargetAmsAddress_,
	// 		ADSIGRP_SYM_RELEASEHND,
	// 		0,
	// 		sizeof(Var.IndexOffset),
	// 		&Var.IndexOffset
	// 	);
	//
	// 	if (Err)
	// 	{
	// 		UE_LOG(LogTemp, Warning, TEXT("Failed to release handle for variable '%s'. Error code 0x%x"), *Var.Name, Err);
	// 		// if (GEngine)
	// 		// 	GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Failed to release variable '%s'"), *Var.Name));
	// 	}
	// }

	ReadVariableList.Empty();
	WriteVariableList.Empty();
	
	if (AdsPort)
	{
		AdsPortCloseEx(AdsPort);
		AdsPort = 0;
	}
	
}

// void ATcAdsMaster::OnConstruction(const FTransform& Transform)
// {
// 	Super::OnConstruction(Transform);
//
// 	ReadVariableList.Empty();
// 	WriteVariableList.Empty();
// }

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

// float ATcAdsMaster::GetReadVariable(int Index) const
// {
// 	if ((Index < 0) || (Index >= ReadVariableList.Num()))
// 		return 0.0f;
//
// 	return ReadVariableList[Index]->Value;
// }
//
// void ATcAdsMaster::SetWriteVariable(int Index, float Val)
// {
// 	if ((Index < 0) || (Index >= WriteVariableList.Num()))
// 		return;
// 	
// 	WriteVariableList[Index]->Value = Val;
// }

// float* ATcAdsMaster::GetReadVariableReferenceByName(const FString& Name)
// {
// 	return nullptr;
// }

void ATcAdsMaster::ReadValues()
{
	// Find out how many variables we should order and the total size buffer we need
	uint32 VariableCount = 0;
	size_t BufferSize = 0;
	for (auto AdsVar : ReadVariableList)
	{
		if (AdsVar->Valid)
		{
			++VariableCount;
			BufferSize += AdsVar->Size() + sizeof(uint32);
		}
	}

	// No point in doing this if the list of valid vars is empty
	if (VariableCount == 0)
		return;
	
	// Make request buffer
	TSimpleBuffer<FDataPar> ReqBuffer(VariableCount);
	// Make receive buffer
	TSimpleBuffer<char> ReceiveBuffer(BufferSize);

	FDataPar* pReqBuffer = ReqBuffer.GetData();
	// Copy request info from each variable
	for (auto AdsVar : ReadVariableList)
	{
		if (AdsVar->Valid)
		{
			AdsVar->SetExternalRequestData(*pReqBuffer);
			++pReqBuffer;
		}
	}

	// Get data from ADS
	unsigned long BytesRead;
	long Err = AdsSyncReadWriteReqEx2(
		AdsPort,
		&TargetAmsAddress_,
		ADSIGRP_SUMUP_READ,
		ReqBuffer.Count(),
		ReceiveBuffer.ByteSize(),
		ReceiveBuffer.GetData(),
		ReqBuffer.ByteSize(),
		ReqBuffer.GetData(),
		&BytesRead
	);

	// Unpack data
	char* pErrorPos = ReceiveBuffer.GetData();
	char* pValuePos = pErrorPos + ReqBuffer.Count()*sizeof(uint32);
	for (auto AdsVar : ReadVariableList)
	{
		if (AdsVar->Valid)
		{
			pValuePos += AdsVar->UnpackValues(pErrorPos, pValuePos);
			pErrorPos += sizeof(uint32);
		}
	}
	
	// if (ReqBuffer_.Count() == 0)
	// 	return;
	//
	// unsigned long BytesRead;
	// long Err = AdsSyncReadWriteReqEx2(
	// 	AdsPort,
	// 	&TargetAmsAddress_,
	// 	ADSIGRP_SUMUP_READ,
	// 	ReqBuffer_.Count(),
	// 	ReceiveBuffer_.ByteSize(),
	// 	ReceiveBuffer_.GetData(),
	// 	ReqBuffer_.ByteSize(),
	// 	ReqBuffer_.GetData(),
	// 	&BytesRead
	// );
	//
	// if (Err)
	// {
	// 	if (GEngine)
	// 		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Read command failed with error code %u"), Err));
	//
	// }
	// else
	// {
	// 	char* pErr = ReceiveBuffer_.GetData();
	// 	char* pValue = ReceiveBuffer_.GetData() + ReqBuffer_.Count()*sizeof(uint32);
	//
	// 	for (auto& Variable : ReadVariableList)
	// 	{
	// 		if (!Variable.Valid)
	// 			continue;
	// 		
	// 		// Read Error
	// 		Variable.Error = *reinterpret_cast<const uint32*>(pErr);
	// 		pErr += sizeof(uint32);
	// 		// Read value
	// 		pValue += UnpackAdsValues(pValue, Variable);
	// 	}
	// }
}

void ATcAdsMaster::WriteValues()
{
}

void ATcAdsMaster::GetSymbolEntriesFromAds()
{
	for (auto Var : ReadVariableList)
	{
		if (!Var)
			continue;
		
		auto Err = Var->GetSymbolEntryFromAds(AdsPort, TargetAmsAddress_);

		if (Err)
		{
			if (GEngine)
				GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow,
					FString::Printf(TEXT("Failed to subscribe to variable '%s'. Error code 0x%x"), *Var->Name, Err));
			
			// UE_LOG(LogTemp, Warning, TEXT("Failed to subscribe to variable '%s'. Error code %x"), *Var->Name, Err);
		}
		else
		{
			if (GEngine)
				GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green,
					FString::Printf(TEXT("Successfully subscribed to variable '%s'"), *Var->Name));

			// UE_LOG(LogTemp, Display, TEXT("Successfully subscribed to variable '%s'"), *Var->Name);
		}
	}
	
	// size_t BufferSize = 0;
	// size_t ValidVariableCount = 0;
	//
	// for (auto& Var : ReadVariableList)
	// {
	// 	// Get variable info from PLC
	//
	// 	// Get name
	// 	FSimpleAsciiString VarName(Var.Name);
	// 	
	// 	unsigned long BytesRead;
	// 	AdsSymbolEntry SymbolEntry;
	// 	long Err = AdsSyncReadWriteReqEx2(
	// 		AdsPort,
	// 		&TargetAmsAddress_,
	// 		ADSIGRP_SYM_INFOBYNAMEEX,
	// 		0,
	// 		sizeof(SymbolEntry),
	// 		&SymbolEntry,
	// 		VarName.ByteSize(),
	// 		VarName.GetData(),
	// 		&BytesRead
	// 	);
	//
	// 	if (Err)
	// 	{
	// 		if (GEngine)
	// 			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Could not add variable: '%s', error code %d"), *Var.Name, Err));
	// 	}
	// 	else
	// 	{
	// 		Var.Size = SymbolEntry.size;
	// 		Var.DataType = static_cast<EAdsDataTypeId>(SymbolEntry.dataType);
	// 		Var.Valid = true;
	// 		Var.IndexGroup = SymbolEntry.iGroup;
	// 		Var.IndexOffset = SymbolEntry.iOffs;
	// 		BufferSize += Var.Size + 4; // Additional error code (long) for each variable
	// 		++ValidVariableCount;
	// 	}
	// 		
	// 	// }
	// }
	//
	// if (BufferSize != 0)
	// {
	// 	ReceiveBuffer_.Reserve(BufferSize);
	// }
	//
	// if (ValidVariableCount == 0)
	// 	return;
	//
	// ReqBuffer_.Reserve(ValidVariableCount);
	// FDataPar* ReqBufferData = ReqBuffer_.GetData();
	//
	// size_t i = 0;
	// for (const auto& Var : ReadVariableList)
	// {
	// 	if (!Var.Valid)
	// 		continue;
	//
	// 	ReqBufferData[i].IndexGroup = Var.IndexGroup;
	// 	ReqBufferData[i].IndexOffset = Var.IndexOffset;
	// 	ReqBufferData[i].Length = Var.Size;
	//
	// 	++i;
	// }
}

// void ATcAdsMaster::AddModule(UTcAdsModule* AdsModule)
// {
// 	if (AdsModule)
// 		ReadVariableList.Emplace(AdsModule);
// }

// size_t ATcAdsMaster::UnpackAdsValues(const char* BufferPos, FSubscriberInputData& Out)
// {
// 	// Out.Error = *reinterpret_cast<const uint32*>(BufferPos);
// 	// BufferPos += 4;
//
// 	switch (Out.DataType)
// 	{
// 		// case EAdsDataTypeId::ADST_VOID:
// 		case EAdsDataTypeId::ADST_INT8:
// 			Out.Value = *reinterpret_cast<const int8*>(BufferPos);
// 		break;
// 	case EAdsDataTypeId::ADST_UINT8:
// 		Out.Value = *reinterpret_cast<const uint8*>(BufferPos);
// 		break;
// 	case EAdsDataTypeId::ADST_INT16:
// 		Out.Value = *reinterpret_cast<const int16*>(BufferPos);
// 		break;
// 	case EAdsDataTypeId::ADST_UINT16:
// 		Out.Value = *reinterpret_cast<const uint16*>(BufferPos);
// 		break;
// 	case EAdsDataTypeId::ADST_INT32:
// 		Out.Value = *reinterpret_cast<const int32*>(BufferPos);
// 		break;
// 	case EAdsDataTypeId::ADST_UINT32:
// 		Out.Value = *reinterpret_cast<const uint32*>(BufferPos);
// 		break;
// 	case EAdsDataTypeId::ADST_INT64:
// 		Out.Value = *reinterpret_cast<const int64*>(BufferPos);
// 		break;
// 	case EAdsDataTypeId::ADST_UINT64:
// 		Out.Value = *reinterpret_cast<const uint64*>(BufferPos);
// 		break;
// 	case EAdsDataTypeId::ADST_REAL32: 
// 		Out.Value = *reinterpret_cast<const float*>(BufferPos);
// 		break;
// 	case EAdsDataTypeId::ADST_REAL64:
// 		Out.Value = *reinterpret_cast<const double*>(BufferPos);
// 		break;
// 		// case EAdsDataTypeId::ADST_STRING:	// Not supported
// 		// case EAdsDataTypeId::ADST_WSTRING:	// Not supported
// 		// case EAdsDataTypeId::ADST_REAL80:	// Not supported
// 		// case EAdsDataTypeId::ADST_BIT:		// Not supported
// 		// case EAdsDataTypeId::ADST_BIGTYPE:	// Not supported
// 		// case EAdsDataTypeId::ADST_MAXTYPES:	// Not supported
// 		default:
// 			break;
// 	}
// 	
// 	return Out.Size;
// }
