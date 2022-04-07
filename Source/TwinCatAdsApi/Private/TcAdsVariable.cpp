// Fill out your copyright notice in the Description page of Project Settings.


#include "TcAdsVariable.h"

#include "TcAdsMaster.h"
#include "ThirdParty/TwinCatAdsApiLibrary/Include/TcAdsAPI.h"

UTcAdsVariable::UTcAdsVariable() :
	AdsName(TEXT("")),
	Access(EAdsAccessType::None),
	Value(0.0f),
	Error(0),
	Valid(false),
	AdsMaster(nullptr),
	DataType_(EAdsDataTypeId::ADST_VOID),
	Size_(0),
	NewVar_(true)
	// SymbolEntry_({0, 0, 0, 0, 0, 0, 0, 0, 0})
	// DataPar_({0,0,0})
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTcAdsVariable::BeginPlay()
{
	Super::BeginPlay();

	// Insert ourselves in the list of variables
	if (AdsMaster)
	{
		switch (Access)
		{
		// case EAdsAccessType::None:
		// 	break;
		case EAdsAccessType::Read:
			AdsMaster->AddReadVariable(this);
			break;
		case EAdsAccessType::Write:
			AdsMaster->AddWriteVariable(this);
			break;
		default:
			break;
		}
	}
}

void UTcAdsVariable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Remove ourselves from the list
	if (AdsMaster)
	{
		switch (Access)
		{
		// case EAdsAccessType::None:
		// 	break;
		case EAdsAccessType::Read:
			AdsMaster->RemoveReadVariable(this);
			break;
		case EAdsAccessType::Write:
			AdsMaster->RemoveWriteVariable(this);
			break;
		default:
			break;
		}
	}
}

void UTcAdsVariable::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

uint32 UTcAdsVariable::GetSymbolEntryFromAds(int32 AdsPort, AmsAddr& AmsAddress, TArray<FDataPar>& Out)
{
	if (AdsName == TEXT(""))
		return ADSERR_DEVICE_INVALIDPARM;
	
	FSimpleAsciiString VarName(AdsName);
	unsigned long BytesRead;
	AdsSymbolEntry SymbolEntry;
	
	Error = AdsSyncReadWriteReqEx2(
		AdsPort,
		&AmsAddress,
		ADSIGRP_SYM_INFOBYNAMEEX,
		0,
		sizeof(SymbolEntry),
		&SymbolEntry,
		VarName.ByteSize(),
		VarName.GetData(),
		&BytesRead
	);
	
	if (!Error)
	{
		Valid = true;
		Size_ = SymbolEntry.size;
		DataType_ = static_cast<EAdsDataTypeId>(SymbolEntry.dataType);
		Out.Emplace(*reinterpret_cast<FDataPar*>(&SymbolEntry.iGroup));
	}

	NewVar_ = false;
	return Error;
}

size_t UTcAdsVariable::UnpackValues(const char* ErrorSrc, const char* ValueSrc, uint32 ErrorIn)
{
	if (ErrorIn)
	{
		Error = ErrorIn;
		return Size_;
	}
	else
	{
		Error = *reinterpret_cast<const uint32*>(ErrorSrc);
	}

	switch (DataType_)
	{
		// case EAdsDataTypeId::ADST_VOID: // Invalid
	case EAdsDataTypeId::ADST_INT8:
		CopyCast<float, int8>(&Value, ValueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT8:
		CopyCast<float, uint8>(&Value, ValueSrc);
		break;
	case EAdsDataTypeId::ADST_INT16:
		CopyCast<float, int16>(&Value, ValueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT16:
		CopyCast<float, uint16>(&Value, ValueSrc);
		break;
	case EAdsDataTypeId::ADST_INT32:
		CopyCast<float, int32>(&Value, ValueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT32:
		CopyCast<float, uint32>(&Value, ValueSrc);
		break;
	case EAdsDataTypeId::ADST_INT64:
		CopyCast<float, int64>(&Value, ValueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT64:
		CopyCast<float, uint64>(&Value, ValueSrc);
		break;
	case EAdsDataTypeId::ADST_REAL32: 
		CopyCast<float, float>(&Value, ValueSrc);
		break;
	case EAdsDataTypeId::ADST_REAL64:
		CopyCast<float, double>(&Value, ValueSrc);
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
	
	// switch (DataType_)
	// {
	// 	// case EAdsDataTypeId::ADST_VOID: // Invalid
	// case EAdsDataTypeId::ADST_INT8:
	// 	Value = *reinterpret_cast<const int8*>(ValueSrc);
	// 	break;
	// case EAdsDataTypeId::ADST_UINT8:
	// 	Value = *reinterpret_cast<const uint8*>(ValueSrc);
	// 	break;
	// case EAdsDataTypeId::ADST_INT16:
	// 	Value = *reinterpret_cast<const int16*>(ValueSrc);
	// 	break;
	// case EAdsDataTypeId::ADST_UINT16:
	// 	Value = *reinterpret_cast<const uint16*>(ValueSrc);
	// 	break;
	// case EAdsDataTypeId::ADST_INT32:
	// 	Value = *reinterpret_cast<const int32*>(ValueSrc);
	// 	break;
	// case EAdsDataTypeId::ADST_UINT32:
	// 	Value = *reinterpret_cast<const uint32*>(ValueSrc);
	// 	break;
	// case EAdsDataTypeId::ADST_INT64:
	// 	Value = *reinterpret_cast<const int64*>(ValueSrc);
	// 	break;
	// case EAdsDataTypeId::ADST_UINT64:
	// 	Value = *reinterpret_cast<const uint64*>(ValueSrc);
	// 	break;
	// case EAdsDataTypeId::ADST_REAL32: 
	// 	Value = *reinterpret_cast<const float*>(ValueSrc);
	// 	break;
	// case EAdsDataTypeId::ADST_REAL64:
	// 	Value = *reinterpret_cast<const double*>(ValueSrc);
	// 	break;
	// 	// case EAdsDataTypeId::ADST_STRING:	// Not supported
	// 	// case EAdsDataTypeId::ADST_WSTRING:	// Not supported
	// 	// case EAdsDataTypeId::ADST_REAL80:	// Not supported
	// 	// case EAdsDataTypeId::ADST_BIT:		// Not supported
	// 	// case EAdsDataTypeId::ADST_BIGTYPE:	// Not supported
	// 	// case EAdsDataTypeId::ADST_MAXTYPES:	// Not supported
	// 	default:
	// 		break;
	// }
	
	return Size_;
}

size_t UTcAdsVariable::PackValues(char* ValueDst) const
{
	switch (DataType_)
	{
		// case EAdsDataTypeId::ADST_VOID: // Invalid
	case EAdsDataTypeId::ADST_INT8:
		*reinterpret_cast<int8*>(ValueDst) = static_cast<int8>(Value);
		break;
	case EAdsDataTypeId::ADST_UINT8:
		*reinterpret_cast<uint8*>(ValueDst) = static_cast<uint8>(Value);
		break;
	case EAdsDataTypeId::ADST_INT16:
		*reinterpret_cast<int16*>(ValueDst) = static_cast<int16>(Value);
		break;
	case EAdsDataTypeId::ADST_UINT16:
		*reinterpret_cast<uint16*>(ValueDst) = static_cast<uint16>(Value);
		break;
	case EAdsDataTypeId::ADST_INT32:
		*reinterpret_cast<int32*>(ValueDst) = static_cast<int32>(Value);
		break;
	case EAdsDataTypeId::ADST_UINT32:
		*reinterpret_cast<uint32*>(ValueDst) = static_cast<uint32>(Value);
		break;
	case EAdsDataTypeId::ADST_INT64:
		*reinterpret_cast<int64*>(ValueDst) = static_cast<int64>(Value);
		break;
	case EAdsDataTypeId::ADST_UINT64:
		*reinterpret_cast<uint64*>(ValueDst) = static_cast<uint64>(Value);
		break;
	case EAdsDataTypeId::ADST_REAL32: 
		*reinterpret_cast<float*>(ValueDst) = static_cast<float>(Value);
		break;
	case EAdsDataTypeId::ADST_REAL64:
		*reinterpret_cast<double*>(ValueDst) = static_cast<double>(Value);
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
	
	return Size_;
}
