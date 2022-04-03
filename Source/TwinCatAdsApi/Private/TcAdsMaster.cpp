// Fill out your copyright notice in the Description page of Project Settings.


#include "TcAdsMaster.h"

#include "TcAdsModule.h"
#include "ThirdParty/TwinCatAdsApiLibrary/Include/TcAdsAPI.h"

// Sets default values
ATcAdsMaster::ATcAdsMaster() : GetDataInterval(0.1), CheckForVariablesDelay(1), TargetAmsNetId({127, 0, 0, 1, 1, 1}), TargetAmsPort(851)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green,TEXT("Ads master loaded"));

}

// Called when the game starts or when spawned
void ATcAdsMaster::BeginPlay()
{
	Super::BeginPlay();

	if (AttachedModules.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow,TEXT("No modules connected (yet)"));
	}
	else
	{
		for (const auto& Module : AttachedModules)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow,
				 FString::Printf(TEXT("Attached to module: '%s'"), *Module->GetOwner()->GetName()));
		}
	}
	
	GetWorldTimerManager().SetTimer(GetDataTimerHandle_,this, &ATcAdsMaster::UpdateValues, GetDataInterval, true);
	GetWorldTimerManager().SetTimer(CheckVarsTimerHandle_,this, &ATcAdsMaster::UpdateVars, CheckForVariablesDelay, false);
	
	TargetAmsAddress_.port = TargetAmsPort;
	
	memcpy_s(TargetAmsAddress_.netId.b, 6, TargetAmsNetId.GetData(), 6);
	
	// // Check DLL Version
	// *reinterpret_cast<uint32*>(&DllVersion) =  AdsGetDllVersion();
	// if (GEngine)
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow,
	// 		FString::Printf(TEXT("Using TC ADS DLL version: %hhu.%hhu.%hu"), DllVersion.version, DllVersion.revision, DllVersion.build));
	// }
	
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
	
	// // Check target device
	// char DevName[50];
	// AdsVersion DevVersion;
	//
	// long Err = AdsSyncReadDeviceInfoReqEx(AdsPort, &TargetAmsAddress_, DevName, &DevVersion);
	//
	// if (GEngine)
	// {
	// 	if (Err)
	// 	{
	// 		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Reading device info failed with code: %d"), Err));
	// 	}
	// 	else
	// 	{
	// 		FString Name(DevName);
	// 		FString Version = FString::Printf(TEXT("%u.%u.%u"), DevVersion.version, DevVersion.revision, DevVersion.build);
	// 		
	// 		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green,
	// 			FString::Printf(TEXT("Connected to device %s running version %s"), GetData(Name), GetData(Version)));
	//
	// 	}
	// }
	//
	// size_t NeededBufferSize = 0;
	//
	// // Get variable handles
	// for (auto& Variable : Variables)
	// {
	// 	const char* Tmp = StringCast<ANSICHAR>(*Variable.Name).Get();
	// 	// Have to copy this to a char since stupid Beckhoff dont accept a const char....
	// 	size_t VarSize = strlen(Tmp) + 1;
	// 	char* VarName = new char[VarSize];
	// 	strcpy_s(VarName, VarSize, Tmp);
	// 	
	// 	unsigned long BytesRead;
	// 	AdsSymbolEntry SymbolEntry;
	// 	Err = AdsSyncReadWriteReqEx2(
	// 		AdsPort,
	// 		&TargetAmsAddress_,
	// 		ADSIGRP_SYM_INFOBYNAMEEX,
	// 		0,
	// 		sizeof(SymbolEntry),
	// 		&SymbolEntry,
	// 		VarSize,
	// 		VarName,
	// 		&BytesRead
	// 	);
	// 	
	// 	delete[] VarName;
	// 	VarName = nullptr;
	//
	//
	// 	if (Err)
	// 	{
	// 		if (GEngine)
	// 			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Could not add variable: '%s', error code %d"), *Variable.Name, Err));
	// 	}
	// 	else
	// 	{
	// 		Variable.Size = SymbolEntry.size;
	// 		Variable.DataType = static_cast<EAdsDataTypeId>(SymbolEntry.dataType);
	// 		Variable.Valid = true;
	// 		Variable.IndexGroup = SymbolEntry.iGroup;
	// 		Variable.IndexOffset = SymbolEntry.iOffs;
	// 		NeededBufferSize += Variable.Size + 4; // Additional error code (long) for each variable
	// 		++ValidVariableCount_;
	// 	}
	// }
	//
	// ReceiveBuffer_.Reserve(NeededBufferSize);
}

void ATcAdsMaster::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Stop timers
	GetWorld()->GetTimerManager().ClearTimer(GetDataTimerHandle_);
	GetWorld()->GetTimerManager().ClearTimer(CheckVarsTimerHandle_);
	
	// Release all handles
	for (const auto Module : AttachedModules)
	{
		for (auto& Var : Module->Variables)
		{
			if (!Var.Valid)
				continue;
			
			long Err = AdsSyncWriteReqEx(
				AdsPort,
				&TargetAmsAddress_,
				ADSIGRP_SYM_RELEASEHND,
				0,
				sizeof(Var.IndexOffset),
				&Var.IndexOffset
			);
	
			if (Err)
				if (GEngine)
					GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Failed to release variable '%s'"), *Var.Name));
		}
		
	}
	
	if (AdsPort)
	{
		AdsPortCloseEx(AdsPort);
		AdsPort = 0;
	}
	
}

// Called every frame
void ATcAdsMaster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATcAdsMaster::UpdateValues()
{
	if (ValidVariableCount_ == 0)
		return;
	
	unsigned long BytesRead;
	long Err = AdsSyncReadWriteReqEx2(
		AdsPort,
		&TargetAmsAddress_,
		ADSIGRP_SUMUP_READ,
		ValidVariableCount_,
		ReceiveBuffer_.ByteSize(),
		ReceiveBuffer_.GetData(),
		ReqBuffer_.ByteSize(),
		ReqBuffer_.GetData(),
		&BytesRead
	);
	
	if (Err)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Read command failed with error code %u"), Err));
	
	}
	else
	{
		char* pErr = ReceiveBuffer_.GetData();
		char* pValue = ReceiveBuffer_.GetData() + ValidVariableCount_*sizeof(uint32);

		for (const auto Module : AttachedModules)
		{
			for (auto& Variable : Module->Variables)
			{
				if (!Variable.Valid)
					continue;
				
				// Read Error
				Variable.Error = *reinterpret_cast<const uint32*>(pErr);
				pErr += sizeof(uint32);
				// Read value
				pValue += UnpackAdsValues(pValue, Variable);
			}
		}
	
	}
}

void ATcAdsMaster::UpdateVars()
{
	size_t ExtendBuffer = 0;
	
	for (const auto Module : AttachedModules)
	{
		for (auto& Var : Module->Variables)
		{
			
			// Get variable handle

			// Get name
			const char* Tmp = StringCast<ANSICHAR>(*Var.Name).Get();
			// Have to copy this to a char since stupid Beckhoff dont accept a const char....
			TSimpleBuffer<char> VarName(strlen(Tmp) + 1);
			strcpy_s(VarName.GetData(), VarName.Count(), Tmp);

			unsigned long BytesRead;
			AdsSymbolEntry SymbolEntry;
			long Err = AdsSyncReadWriteReqEx2(
				AdsPort,
				&TargetAmsAddress_,
				ADSIGRP_SYM_INFOBYNAMEEX,
				0,
				sizeof(SymbolEntry),
				&SymbolEntry,
				VarName.ByteSize(),
				VarName.GetData(),
				&BytesRead
			);

			if (Err)
			{
				if (GEngine)
					GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Could not add variable: '%s', error code %d"), *Var.Name, Err));
			}
			else
			{
				Var.Size = SymbolEntry.size;
				Var.DataType = static_cast<EAdsDataTypeId>(SymbolEntry.dataType);
				Var.Valid = true;
				Var.IndexGroup = SymbolEntry.iGroup;
				Var.IndexOffset = SymbolEntry.iOffs;
				ExtendBuffer += Var.Size + 4; // Additional error code (long) for each variable
				++ValidVariableCount_;
			}
			
		}
	}

	if (ExtendBuffer != 0)
	{
		ExtendBuffer += ReceiveBuffer_.Count();
		ReceiveBuffer_.Reserve(ExtendBuffer);
	}

	if (ValidVariableCount_ == 0)
		return;

	ReqBuffer_.Reserve(ValidVariableCount_);
	FDataPar* ReqBufferData = ReqBuffer_.GetData();

	size_t i = 0;
	for (const auto Module : AttachedModules)
	{
		for (auto& Var : Module->Variables)
		{
			if (!Var.Valid)
				continue;
	
			ReqBufferData[i].IndexGroup = Var.IndexGroup;
			ReqBufferData[i].IndexOffset = Var.IndexOffset;
			ReqBufferData[i].Length = Var.Size;

			++i;
		}
	}
}

void ATcAdsMaster::AddModule(UTcAdsModule* AdsModule)
{
	if (AdsModule)
		AttachedModules.Emplace(AdsModule);
}

size_t ATcAdsMaster::UnpackAdsValues(const char* BufferPos, FSubscriberInputData& Out)
{
	// Out.Error = *reinterpret_cast<const uint32*>(BufferPos);
	// BufferPos += 4;

	switch (Out.DataType)
	{
		// case EAdsDataTypeId::ADST_VOID:
		case EAdsDataTypeId::ADST_INT8:
			Out.Value = *reinterpret_cast<const int8*>(BufferPos);
		break;
	case EAdsDataTypeId::ADST_UINT8:
		Out.Value = *reinterpret_cast<const uint8*>(BufferPos);
		break;
	case EAdsDataTypeId::ADST_INT16:
		Out.Value = *reinterpret_cast<const int16*>(BufferPos);
		break;
	case EAdsDataTypeId::ADST_UINT16:
		Out.Value = *reinterpret_cast<const uint16*>(BufferPos);
		break;
	case EAdsDataTypeId::ADST_INT32:
		Out.Value = *reinterpret_cast<const int32*>(BufferPos);
		break;
	case EAdsDataTypeId::ADST_UINT32:
		Out.Value = *reinterpret_cast<const uint32*>(BufferPos);
		break;
	case EAdsDataTypeId::ADST_INT64:
		Out.Value = *reinterpret_cast<const int64*>(BufferPos);
		break;
	case EAdsDataTypeId::ADST_UINT64:
		Out.Value = *reinterpret_cast<const uint64*>(BufferPos);
		break;
	case EAdsDataTypeId::ADST_REAL32: 
		Out.Value = *reinterpret_cast<const float*>(BufferPos);
		break;
	case EAdsDataTypeId::ADST_REAL64:
		Out.Value = *reinterpret_cast<const double*>(BufferPos);
		break;
		// case EAdsDataTypeId::ADST_STRING:	// Not supported
		// case EAdsDataTypeId::ADST_WSTRING:	// Not supported
		// case EAdsDataTypeId::ADST_REAL80:	// Not supported
		// case EAdsDataTypeId::ADST_BIT:		// Not supported
		// case EAdsDataTypeId::ADST_BIGTYPE:	// Not supported
		// case EAdsDataTypeId::ADST_MAXTYPES:	// Not supported
		default:
			break;
	}
	
	return Out.Size;
}
