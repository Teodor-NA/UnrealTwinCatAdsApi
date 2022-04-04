// Fill out your copyright notice in the Description page of Project Settings.


#include "TcAdsVariable.h"

#include "ThirdParty/TwinCatAdsApiLibrary/Include/TcAdsAPI.h"

UTcAdsVariable::UTcAdsVariable() :
	Name(TEXT("")),
	Value(0.0f),
	Error(0),
	Valid(false),
	SymbolEntry_({0, 0, 0, 0, 0, 0, 0, 0, 0})
	// DataPar_({0,0,0})
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTcAdsVariable::BeginPlay()
{
	Super::BeginPlay();
}

void UTcAdsVariable::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

uint32 UTcAdsVariable::GetSymbolEntryFromAds(int32 AdsPort, AmsAddr& AmsAddress)
{
	FSimpleAsciiString VarName(Name);
	unsigned long BytesRead;
	
	Error = AdsSyncReadWriteReqEx2(
		AdsPort,
		&AmsAddress,
		ADSIGRP_SYM_INFOBYNAMEEX,
		0,
		sizeof(SymbolEntry_),
		&SymbolEntry_,
		VarName.ByteSize(),
		VarName.GetData(),
		&BytesRead
	);

	if (!Error)
		Valid = true;
	
	return Error;
}

size_t UTcAdsVariable::UnpackValues(const char* ErrorSrc, const char* ValueSrc)
{
	Error = *reinterpret_cast<const uint32*>(ErrorSrc);
	
	switch (GetDataType())
	{
		// case EAdsDataTypeId::ADST_VOID:
		case EAdsDataTypeId::ADST_INT8:
			Value = *reinterpret_cast<const int8*>(ValueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT8:
		Value = *reinterpret_cast<const uint8*>(ValueSrc);
		break;
	case EAdsDataTypeId::ADST_INT16:
		Value = *reinterpret_cast<const int16*>(ValueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT16:
		Value = *reinterpret_cast<const uint16*>(ValueSrc);
		break;
	case EAdsDataTypeId::ADST_INT32:
		Value = *reinterpret_cast<const int32*>(ValueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT32:
		Value = *reinterpret_cast<const uint32*>(ValueSrc);
		break;
	case EAdsDataTypeId::ADST_INT64:
		Value = *reinterpret_cast<const int64*>(ValueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT64:
		Value = *reinterpret_cast<const uint64*>(ValueSrc);
		break;
	case EAdsDataTypeId::ADST_REAL32: 
		Value = *reinterpret_cast<const float*>(ValueSrc);
		break;
	case EAdsDataTypeId::ADST_REAL64:
		Value = *reinterpret_cast<const double*>(ValueSrc);
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
	
	return Size();
}
