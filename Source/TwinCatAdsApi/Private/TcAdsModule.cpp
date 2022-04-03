// Fill out your copyright notice in the Description page of Project Settings.


#include "TcAdsModule.h"

#include "TcAdsMaster.h"
// #include "ThirdParty/TwinCatAdsApiLibrary/Include/TcAdsAPI.h"



// Sets default values for this component's properties
UTcAdsModule::UTcAdsModule() : AdsMaster(nullptr) // : ValidVariableCount_(0), AdsPort(0), StepTime(0.1), TargetAmsPort(851)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// TargetAmsNetId.SetNum(6, false);
	// TargetAmsNetId[0] = 127;
	// TargetAmsNetId[1] = 0;
	// TargetAmsNetId[2] = 0;
	// TargetAmsNetId[3] = 1;
	// TargetAmsNetId[4] = 1;
	// TargetAmsNetId[5] = 1;
	
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green,TEXT("Ads module loaded"));

	
	// ...
}

// void UTcAdsModule::OnComponentCreated()
// {
// 	Super::OnComponentCreated();
//
// 	
// }

// Called when the game starts
void UTcAdsModule::BeginPlay()
{
	Super::BeginPlay();

	if (AdsMaster)
		AdsMaster->AddModule(this);
	
	// GetWorld()->GetTimerManager().SetTimer(TimerHandle,this, &UTcAdsModule::UpdateValues, StepTime, true);
	//
	// TargetAmsAddress_.port = TargetAmsPort;
	//
	// for (int i = 0; i < 6; ++i)
	// {
	// 	TargetAmsAddress_.netId.b[i] = TargetAmsNetId[i];
	// }
	//
	// // Check DLL Version
	// *reinterpret_cast<uint32*>(&DllVersion) =  AdsGetDllVersion();
	// if (GEngine)
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow,
	// 		FString::Printf(TEXT("Using TC ADS DLL version: %hhu.%hhu.%hu"), DllVersion.version, DllVersion.revision, DllVersion.build));
	// }
	//
	// // Open port
	// AdsPort = AdsPortOpenEx();
	//
	// if (AdsPort)
	// {
	// 	if (GEngine)
	// 		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, FString::Printf(TEXT("Port: %ld opened"), AdsPort));
	// }
	// else
	// {
	// 	if (GEngine)
	// 		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, TEXT("Port opening failed"));
	//
	// 	return;
	// }	
	//
	// // Get local address
	// AdsGetLocalAddressEx(AdsPort, &LocalAmsAddr);
	//
	// if (GEngine)
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow,
	// 		FString::Printf(TEXT("Local ams address: %hhu.%hhu.%hhu.%hhu.%hhu.%hhu"),
	// 			LocalAmsAddr.netId.b[0], LocalAmsAddr.netId.b[1], LocalAmsAddr.netId.b[2], LocalAmsAddr.netId.b[3], LocalAmsAddr.netId.b[4], LocalAmsAddr.netId.b[5]
	// 			)
	// 		);
	// 	GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow,
	// 		FString::Printf(TEXT("Local ams port: %hu"), LocalAmsAddr.port));
	// }
	//
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

// void UTcAdsModule::EndPlay(const EEndPlayReason::Type EndPlayReason)
// {
// 	Super::EndPlay(EndPlayReason);
//
// 	// GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
// 	//
// 	// // Release all handles
// 	// for (auto& Variable : Variables)
// 	// {
// 	// 	long Err = AdsSyncWriteReqEx(
// 	// 		AdsPort,
// 	// 		&TargetAmsAddress_,
// 	// 		ADSIGRP_SYM_RELEASEHND,
// 	// 		0,
// 	// 		sizeof(Variable.IndexOffset),
// 	// 		&Variable.IndexOffset
// 	// 	);
// 	//
// 	// 	if (Err)
// 	// 		if (GEngine)
// 	// 			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Failed to release variable '%s'"), *Variable.Name));
// 	// 	
// 	// }
// 	//
// 	// if (AdsPort)
// 	// {
// 	// 	AdsPortCloseEx(AdsPort);
// 	// 	AdsPort = 0;
// 	// }
// 	//
// 	// // delete[] ReceiveBuffer_;
// 	// // ReceiveBuffer_ = nullptr;
// 	// // ReceiveBufferSize_ = 0;
// }


// size_t UTcAdsModule::UnpackAdsValues(const char* BufferPos, FSubscriberInputData& Out)
// {
// 	// Out.Error = *reinterpret_cast<const uint32*>(BufferPos);
// 	// BufferPos += 4;
//
// 	switch (Out.DataType)
// 	{
// 	// case EAdsDataTypeId::ADST_VOID:
// 	case EAdsDataTypeId::ADST_INT8:
// 		Out.Value = *reinterpret_cast<const int8*>(BufferPos);
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
// 	// case EAdsDataTypeId::ADST_STRING:	// Not supported
// 	// case EAdsDataTypeId::ADST_WSTRING:	// Not supported
// 	// case EAdsDataTypeId::ADST_REAL80:	// Not supported
// 	// case EAdsDataTypeId::ADST_BIT:		// Not supported
// 	// case EAdsDataTypeId::ADST_BIGTYPE:	// Not supported
// 	// case EAdsDataTypeId::ADST_MAXTYPES:	// Not supported
// 	default:
// 		break;
// 	}
// 	
// 	return Out.Size;
// }

// void UTcAdsModule::UpdateValues()
// {
// 	if (ValidVariableCount_ == 0)
// 		return;
//
// 	TSimpleBuffer<FDataPar> ReqBuffer(ValidVariableCount_);
// 	FDataPar* ReqBufferData = ReqBuffer.Data();
// 	
// 	// FDataPar* ReqBuffer = new FDataPar[ValidVariableCount_];
// 	
// 	for (size_t i = 0; i < Variables.Num(); ++i)
// 	{
// 		auto& Variable = Variables[i];
// 		
// 		if (!Variable.Valid)
// 			continue;
//
// 		ReqBufferData[i].IndexGroup = Variable.IndexGroup;
// 		ReqBufferData[i].IndexOffset = Variable.IndexOffset;
// 		ReqBufferData[i].Length = Variable.Size;
// 	}
//
// 	unsigned long BytesRead;
// 	long Err = AdsSyncReadWriteReqEx2(
// 		AdsPort,
// 		&TargetAmsAddress_,
// 		ADSIGRP_SUMUP_READ,
// 		ValidVariableCount_,
// 		ReceiveBuffer_.ByteSize(),
// 		ReceiveBuffer_.Data(),
// 		ValidVariableCount_*sizeof(FDataPar),
// 		ReqBufferData,
// 		&BytesRead
// 	);
//
// 	// delete[] ReqBufferData;
// 	// ReqBufferData = nullptr;
//
// 	if (Err)
// 	{
// 		if (GEngine)
// 			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Read command failed with error code %u"), Err));
//
// 	}
// 	else
// 	{
// 		char* ReadBuffer = ReceiveBuffer_.Data();
// 		// Read errors
// 		for (auto& Variable : Variables)
// 		{
// 			if (!Variable.Valid)
// 				continue;
//
// 			Variable.Error = *reinterpret_cast<const uint32*>(ReadBuffer);
// 			ReadBuffer += sizeof(uint32);
// 		}
//
// 		// Read values
// 		for (auto& Variable : Variables)
// 		{
// 			ReadBuffer += UnpackAdsValues(ReadBuffer, Variable);
// 		}
//
// 	}
// }


// Called every frame
void UTcAdsModule::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...

	// if (ValidVariableCount_ == 0)
	// 	return;
	//
	// FDataPar* ReqBuffer = new FDataPar[ValidVariableCount_];
	//
	// for (size_t i = 0; i < Variables.Num(); ++i)
	// {
	// 	auto& Variable = Variables[i];
	// 	
	// 	if (!Variable.Valid)
	// 		continue;
	//
	// 	ReqBuffer[i].IndexGroup = Variable.IndexGroup;
	// 	ReqBuffer[i].IndexOffset = Variable.IndexOffset;
	// 	ReqBuffer[i].Length = Variable.Size;
	// }
	//
	// unsigned long BytesRead;
	// long Err = AdsSyncReadWriteReqEx2(
	// 	AdsPort,
	// 	&TargetAmsAddress_,
	// 	ADSIGRP_SUMUP_READ,
	// 	ValidVariableCount_,
	// 	ReceiveBufferSize_,
	// 	ReceiveBuffer_,
	// 	ValidVariableCount_*sizeof(FDataPar),
	// 	ReqBuffer,
	// 	&BytesRead
	// );
	//
	// delete[] ReqBuffer;
	// ReqBuffer = nullptr;
	//
	// if (Err)
	// {
	// 	if (GEngine)
	// 		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Read command failed with error code %u"), Err));
	//
	// }
	// else
	// {
	// 	char* ReadBuffer = ReceiveBuffer_;
	// 	for (auto& Variable : Variables)
	// 	{
	// 		if (!Variable.Valid)
	// 			continue;
	//
	// 		ReadBuffer += UnpackAdsValues(ReadBuffer, Variable);
	// 	}
	// }
}

