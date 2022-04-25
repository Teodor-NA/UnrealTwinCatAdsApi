// Fill out your copyright notice in the Description page of Project Settings.


#include "TcAdsVariable.h"
#include "TcAdsMaster.h"
#include "TcAdsAPI.h"

UTcAdsVariable::UTcAdsVariable() :
	AdsName(TEXT("")),
	Access(EAdsAccessType::None),
	Value(0.0f),
	Error(0),
	ValidAds(false),
	AdsMaster(nullptr),
	DataType_(EAdsDataTypeId::ADST_VOID),
	Size_(0),
	_newVar(true)
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
		AdsMaster->addVariable(this);
		// switch (Access)
		// {
		// // case EAdsAccessType::None:
		// // 	break;
		// case EAdsAccessType::Read:
		// 	AdsMaster->AddReadVariable(this);
		// 	break;
		// case EAdsAccessType::Write:
		// 	AdsMaster->AddWriteVariable(this);
		// 	break;
		// default:
		// 	break;
		// }
	}
}

void UTcAdsVariable::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);

	// Remove ourselves from the list
	if (AdsMaster)
	{
		AdsMaster->removeVariable(this);
	// 	switch (Access)
	// 	{
	// 	// case EAdsAccessType::None:
	// 	// 	break;
	// 	case EAdsAccessType::Read:
	// 		AdsMaster->RemoveReadVariable(this);
	// 		break;
	// 	case EAdsAccessType::Write:
	// 		AdsMaster->RemoveWriteVariable(this);
	// 		break;
	// 	default:
	// 		break;
	// 	}
	}
}

void UTcAdsVariable::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
	Super::TickComponent(deltaTime, tickType, thisTickFunction);
}

size_t UTcAdsVariable::transferSize() const
{
	switch (Access)
	{
	// case EAdsAccessType::None:
	// 	break;
	case EAdsAccessType::Read:
		return readSize();
	case EAdsAccessType::Write:
		return writeSize();
	default:
		return 0;
	}
}

uint32 UTcAdsVariable::getSymbolEntryFromAds(const int32 adsPort, AmsAddr& amsAddress, TArray<FDataPar>& out)
{
	if (AdsName == TEXT(""))
		return ADSERR_DEVICE_INVALIDPARM;
	
	FSimpleAsciiString varName(AdsName);
	unsigned long bytesRead;
	AdsSymbolEntry symbolEntry;
	
	Error = AdsSyncReadWriteReqEx2(
		adsPort,
		&amsAddress,
		ADSIGRP_SYM_INFOBYNAMEEX,
		0,
		sizeof(symbolEntry),
		&symbolEntry,
		varName.byteSize(),
		varName.getData(),
		&bytesRead
	);
	
	if (!Error)
	{
		ValidAds = true;
		Size_ = symbolEntry.size;
		DataType_ = static_cast<EAdsDataTypeId>(symbolEntry.dataType);
		out.Emplace(*reinterpret_cast<FDataPar*>(&symbolEntry.iGroup));
	}

	_newVar = false;
	return Error;
}

size_t UTcAdsVariable::unpackValues(const char* errorSrc, const char* valueSrc, uint32 errorIn)
{
	if (errorIn)
	{
		Error = errorIn;
		return Size_;
	}
	else
	{
		Error = *reinterpret_cast<const uint32*>(errorSrc);
	}

	switch (DataType_)
	{
		// case EAdsDataTypeId::ADST_VOID: // Invalid
	case EAdsDataTypeId::ADST_INT8:
		copyCast<float, int8>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT8:
		copyCast<float, uint8>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_INT16:
		copyCast<float, int16>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT16:
		copyCast<float, uint16>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_INT32:
		copyCast<float, int32>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT32:
		copyCast<float, uint32>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_INT64:
		copyCast<float, int64>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT64:
		copyCast<float, uint64>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_REAL32: 
		copyCast<float, float>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_REAL64:
		copyCast<float, double>(&Value, valueSrc);
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

size_t UTcAdsVariable::packValues(char* valueDst) const
{
	switch (DataType_)
	{
		// case EAdsDataTypeId::ADST_VOID: // Invalid
	case EAdsDataTypeId::ADST_INT8:
		*reinterpret_cast<int8*>(valueDst) = static_cast<int8>(Value);
		break;
	case EAdsDataTypeId::ADST_UINT8:
		*reinterpret_cast<uint8*>(valueDst) = static_cast<uint8>(Value);
		break;
	case EAdsDataTypeId::ADST_INT16:
		*reinterpret_cast<int16*>(valueDst) = static_cast<int16>(Value);
		break;
	case EAdsDataTypeId::ADST_UINT16:
		*reinterpret_cast<uint16*>(valueDst) = static_cast<uint16>(Value);
		break;
	case EAdsDataTypeId::ADST_INT32:
		*reinterpret_cast<int32*>(valueDst) = static_cast<int32>(Value);
		break;
	case EAdsDataTypeId::ADST_UINT32:
		*reinterpret_cast<uint32*>(valueDst) = static_cast<uint32>(Value);
		break;
	case EAdsDataTypeId::ADST_INT64:
		*reinterpret_cast<int64*>(valueDst) = static_cast<int64>(Value);
		break;
	case EAdsDataTypeId::ADST_UINT64:
		*reinterpret_cast<uint64*>(valueDst) = static_cast<uint64>(Value);
		break;
	case EAdsDataTypeId::ADST_REAL32: 
		*reinterpret_cast<float*>(valueDst) = static_cast<float>(Value);
		break;
	case EAdsDataTypeId::ADST_REAL64:
		*reinterpret_cast<double*>(valueDst) = static_cast<double>(Value);
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
