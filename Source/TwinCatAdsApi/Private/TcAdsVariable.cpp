// Fill out your copyright notice in the Description page of Project Settings.


#include "TcAdsVariable.h"
#include "TcAdsMaster.h"
#include "TcAdsAPI.h"

UTcAdsVariable::UTcAdsVariable() :
	AdsName(TEXT("")),
	Access(EAdsAccessType::None),
	Error(0),
	ValidAds(false),
	Value(0.0f),
	AdsMaster(nullptr),
	_symbolEntry({0, 0, 0, 0, 0, 0, 0, 0, 0}),
	// _dataType(EAdsDataTypeId::ADST_VOID),
	// _size(0),
	_prevVal(0.0f)
	// SymbolEntry_({0, 0, 0, 0, 0, 0, 0, 0, 0})
	// DataPar_({0,0,0})
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTcAdsVariable::BeginPlay()
{
	Super::BeginPlay();
	
	// Insert ourselves in the list of variables
	if (IsValid(AdsMaster))
		AdsMaster->addVariable(this);

}

void UTcAdsVariable::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);

	release();	
}

int32 UTcAdsVariable::getHandle(int32 adsPort, AmsAddr* pAmsAddr, ULONG& handle)
{
	FSimpleAsciiString varName(AdsName);
	unsigned long bytesRead;
	Error = AdsSyncReadWriteReqEx2(
		adsPort,
		pAmsAddr,
		ADSIGRP_SYM_HNDBYNAME,
		0x0,
		sizeof(handle),
		&handle,
		varName.byteSize(),
		varName.getData(),
		&bytesRead
	);

	return Error;
}

int32 UTcAdsVariable::releaseHandle(int32 adsPort, AmsAddr* pAmsAddr, ULONG& handle)
{
	Error = AdsSyncWriteReqEx(
		adsPort,
		pAmsAddr,
		ADSIGRP_SYM_RELEASEHND,
		0,
		sizeof(handle),
		&handle
	);

	handle = 0;

	return Error;
}

int32 UTcAdsVariable::setCallback(int32 adsPort, AmsAddr* amsAddr, ADSTRANSMODE adsTransMode,
	TcAdsCallbackStruct& callbackStruct)
{
	AdsNotificationAttrib notificationAttrib = {
		_symbolEntry.size,
		adsTransMode,
		0,
		 static_cast<unsigned long>(AdsMaster->ReadValuesInterval*10000000.0f)
	};

	// Setup callback
	Error = AdsSyncAddDeviceNotificationReqEx(
		adsPort,
		amsAddr,
		ADSIGRP_SYM_VALBYHND,
		callbackStruct.hUser,
		&notificationAttrib,
		&ATcAdsMaster::ReadCallback,
		callbackStruct.hUser,
		&callbackStruct.hNotification
	);

	return Error;
}

int32 UTcAdsVariable::releaseCallback(int32 adsPort, AmsAddr* amsAddr, TcAdsCallbackStruct& callbackStruct)
{
	Error = AdsSyncDelDeviceNotificationReqEx(
		adsPort,
		amsAddr,
		callbackStruct.hNotification
	);

	callbackStruct.hNotification = 0;

	return Error;
}

void UTcAdsVariable::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
	Super::TickComponent(deltaTime, tickType, thisTickFunction);
}

size_t UTcAdsVariable::transferSize() const
{
	switch (Access)
	{
	case EAdsAccessType::None: break;
	case EAdsAccessType::Read:
		return readSize();
	case EAdsAccessType::ReadCyclic:
	case EAdsAccessType::ReadOnChange:
		return _symbolEntry.size;
	case EAdsAccessType::Write:
	case EAdsAccessType::WriteOnChange:
		return writeSize();
	default: ;
	}

	return 0;
}

void UTcAdsVariable::release()
{
	if (IsValid(AdsMaster))
	{
		AdsMaster->removeVariable(this);
	}

	ValidAds = false;
}

uint32 UTcAdsVariable::getSymbolEntryFromAds(const int32 adsPort, AmsAddr* amsAddress)
{
	if (AdsName == TEXT(""))
		return ADSERR_DEVICE_INVALIDPARM;
	
	FSimpleAsciiString varName(AdsName);
	unsigned long bytesRead;
	
	Error = AdsSyncReadWriteReqEx2(
		adsPort,
		amsAddress,
		ADSIGRP_SYM_INFOBYNAMEEX,
		0,
		sizeof(_symbolEntry),
		&_symbolEntry,
		varName.byteSize(),
		varName.getData(),
		&bytesRead
	);

	ValidAds = !Error;
	
	return Error;
}

int32 UTcAdsVariable::setupCallbackVariable(int32 adsPort, AmsAddr* amsAddr, ADSTRANSMODE adsTransMode, TcAdsCallbackStruct& callbackStruct)
{
	ValidAds = false;
	
	// Get handle
	getHandle(adsPort, amsAddr, callbackStruct.hUser);

	if (Error)
	{
		UE_LOG(LogTcAds, Error,
			TEXT("Failed to get handle for '%s' variable '%s'. Error code: 0x%x"),
			AdsAccessTypeName(Access),
			*AdsName,
			Error
		);
		
		return Error;
	}

	setCallback(adsPort, amsAddr, adsTransMode, callbackStruct);

	if (Error)
	{
		UE_LOG(LogTcAds, Error,
			TEXT("Failed to set up callback for '%s' variable '%s'. Error code 0x%x"),
			AdsAccessTypeName(Access),
			*AdsName,
			Error
		);
		
		// Attempt to release handle
		Error = releaseHandle(adsPort, amsAddr, callbackStruct.hUser);
		
		return Error;
	}

	UE_LOG(LogTcAds, Display,
		TEXT("Successfully set up callback for '%s' variable '%s'"),
		AdsAccessTypeName(Access),
		*AdsName
	);

	ValidAds = true;
	
	return 0;
}

int32 UTcAdsVariable::terminateCallbackVariable(int32 adsPort, AmsAddr* amsAddr, TcAdsCallbackStruct& callbackStruct)
{
	// Release callback
	releaseCallback(adsPort, amsAddr, callbackStruct);

	if (Error)
	{
		UE_LOG(LogTcAds, Error,
		   TEXT("Failed to delete callback notification for '%s' variable '%s'. Error code: 0x%x"),
		   AdsAccessTypeName(Access),
		   *AdsName,
		   Error
	   );
	}
	else
	{
		UE_LOG(LogTcAds, Display,
			TEXT("Deleted callback notification for '%s' variable '%s'"),
			AdsAccessTypeName(Access),
			*AdsName
		);
	 }
	
	// Release handle
	releaseHandle(adsPort, amsAddr, callbackStruct.hUser);

	if (Error)
	{
		UE_LOG(LogTcAds, Error,
		   TEXT("Failed to release handle for '%s' variable '%s'. Error code: 0x%x"),
		   AdsAccessTypeName(Access),
		   *AdsName,
		   Error
	   );
	}
	else
	{
		UE_LOG(LogTcAds, Display,
			TEXT("Released handle for '%s' variable '%s'"),
			AdsAccessTypeName(Access),
			*AdsName
		);
	}

	return Error;
}

size_t UTcAdsVariable::unpackValue(const char* errorSrc, const char* valueSrc, uint32 errorIn)
{
	if (errorIn)
	{
		Error = errorIn;
		return _symbolEntry.size;
	}

	Error = *reinterpret_cast<const uint32*>(errorSrc);

	return unpackValue(valueSrc);
}

size_t UTcAdsVariable::unpackValue(const char* valueSrc)
{
	switch (GetDataType())
	{
	// case EAdsDataTypeId::ADST_VOID: // Invalid
	case EAdsDataTypeId::ADST_INT8:
			CopyCast<float, int8>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT8:
		CopyCast<float, uint8>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_INT16:
		CopyCast<float, int16>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT16:
		CopyCast<float, uint16>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_INT32:
		CopyCast<float, int32>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT32:
		CopyCast<float, uint32>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_INT64:
		CopyCast<float, int64>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT64:
		CopyCast<float, uint64>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_REAL32: 
		CopyCast<float, float>(&Value, valueSrc);
		break;
	case EAdsDataTypeId::ADST_REAL64:
		CopyCast<float, double>(&Value, valueSrc);
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
	
	return _symbolEntry.size;
}

size_t UTcAdsVariable::packValue(char* valueDst) const
{
	switch (GetDataType())
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
	
	return _symbolEntry.size;
}

bool UTcAdsVariable::valueChanged()
{
	bool tmp = (Value != _prevVal);
	_prevVal = Value;
	return tmp;
}

bool UTcAdsVariable::ValidAdsVariable(const UTcAdsVariable* variable)
{
	return IsValid(variable) && variable->ValidAds;
}
