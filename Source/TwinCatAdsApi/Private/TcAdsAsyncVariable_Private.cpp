#include "TcAdsAsyncVariable_Private.h"

#include "TcAdsAPI.h"
#include "TcAdsAsyncMaster.h"
#include "TcAdsAsyncVariable.h"


FTcAdsAsyncVariable::FTcAdsAsyncVariable(UTcAdsAsyncVariable* reference, int32 index)
	: TTcAdsValueStruct({reference->adsSetupInfo.initialValue, 0})
	, _adsMode(reference->adsSetupInfo.adsMode)
	// Set the counter to interval to update on first tick
	, _updateCounter(reference->adsSetupInfo.cycleMultiplier)
	, _updateInterval(reference->adsSetupInfo.cycleMultiplier)
	// Make the prev value different than the initial value to force update on first tick
	, _prevVal(reference->adsSetupInfo.initialValue + 1.0f)
	, _index(index)
	, _notification(0)
	, _reference(reference)
	, _symbolEntry({0, 0, 0, 0, 0, 0, 0, 0, 0})
{
}

// FTcAdsAsyncVariable::FTcAdsAsyncVariable(ULONG port, AmsAddr* addr, float initialValue)
// 	: FTcAdsAsyncVariable(initialValue)
// {
// 	
// }

FTcAdsAsyncVariable::~FTcAdsAsyncVariable()
{
	// Invalidate reference if it exists
	if (IsValid(_reference))
	{
		// If it is a callback variable, then it must be released
		if (_notification != 0)
		{
			AmsAddr addr = _reference->adsSetupInfo.adsMaster->getRemoteAmsAddr();
			auto err = releaseCallback(_reference->adsSetupInfo.adsMaster->getLocalAdsPort(), &addr);

			if (err)
			{
				LOG_TCADS_ERROR("Failed to release callback for '%s(%d)' variable '%s' from the worker thread. Error code 0x%x"
					, *GetAdsAccessTypeName(_adsMode)
					, _adsMode
					, *_reference->adsSetupInfo.adsName
					, err
				);
			}
			else
			{
				LOG_TCADS_DISPLAY("Released callback for '%s(%d)' variable '%s' from the worker thread"
					, *GetAdsAccessTypeName(_adsMode)
					, _adsMode
					, *_reference->adsSetupInfo.adsName
				);
			}
		}
		
		_reference->_variable = nullptr;
		LOG_TCADS_DISPLAY("Ads variable '%s' was invalidated from the worker thread",
			*_reference->adsSetupInfo.adsName
		);
	}
}

TTcAdsValueStruct<FTcAdsAsyncVariable::ValueType, FTcAdsAsyncVariable::ErrorType> FTcAdsAsyncVariable::getValueAndError()
{
	std::lock_guard<std::mutex> lock(_mutexLock);
	return static_cast<TTcAdsValueStruct>(*this);
}

FTcAdsAsyncVariable::ValueType FTcAdsAsyncVariable::getValue()
{
	std::lock_guard<std::mutex> lock(_mutexLock);
	return adsValue;
}

FTcAdsAsyncVariable::ErrorType FTcAdsAsyncVariable::getError()
{
	std::lock_guard<std::mutex> lock(_mutexLock);
	return adsError;
}

FTcAdsAsyncVariable::ErrorType FTcAdsAsyncVariable::setValue(ValueType val)
{
	std::lock_guard<std::mutex> lock(_mutexLock);
	adsValue = val;
	return adsError;
}

FTcAdsAsyncVariable::ErrorType FTcAdsAsyncVariable::setError(ErrorType error)
{
	std::lock_guard<std::mutex> lock(_mutexLock);
	return (adsError = error);
}

EAdsDataTypeId FTcAdsAsyncVariable::getDataType()
{
	std::lock_guard<std::mutex> lock(_mutexLock);
	return _getDataType();
}

int32 FTcAdsAsyncVariable::getIndex()
{
	std::lock_guard<std::mutex> lock(_mutexLock);
	return _index;
}

EAdsAccessMode FTcAdsAsyncVariable::getAdsMode()
{
	std::lock_guard<std::mutex> lock(_mutexLock);
	return _adsMode;
}

size_t FTcAdsAsyncVariable::pack(void* pValueDst, void* pSymbolDst)
{
	std::lock_guard<std::mutex> lock(_mutexLock);

	switch (_getDataType())
	{
	// case EAdsDataTypeId::ADST_VOID: // Invalid
	case EAdsDataTypeId::ADST_INT8:
		_CopyCast<int8, ValueType>(pValueDst, &adsValue);
		break;
	case EAdsDataTypeId::ADST_UINT8:
		_CopyCast<uint8, ValueType>(pValueDst, &adsValue);
		break;
	case EAdsDataTypeId::ADST_INT16:
		_CopyCast<int16, ValueType>(pValueDst, &adsValue);
		break;
	case EAdsDataTypeId::ADST_UINT16:
		_CopyCast<uint16, ValueType>(pValueDst, &adsValue);
		break;
	case EAdsDataTypeId::ADST_INT32:
		_CopyCast<int32, ValueType>(pValueDst, &adsValue);
		break;
	case EAdsDataTypeId::ADST_UINT32:
		_CopyCast<uint32, ValueType>(pValueDst, &adsValue);
		break;
	case EAdsDataTypeId::ADST_INT64:
		_CopyCast<int64, ValueType>(pValueDst, &adsValue);
		break;
	case EAdsDataTypeId::ADST_UINT64:
		_CopyCast<uint64, ValueType>(pValueDst, &adsValue);
		break;
	case EAdsDataTypeId::ADST_REAL32: 
		_CopyCast<float, ValueType>(pValueDst, &adsValue);
		break;
	case EAdsDataTypeId::ADST_REAL64:
		_CopyCast<double, ValueType>(pValueDst, &adsValue);
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

	*static_cast<FDataPar*>(pSymbolDst) = _getDataPar();
	
	return _symbolEntry.size;
}

size_t FTcAdsAsyncVariable::unpack(const void* pValueSrc, bool overwritePrevVal, ErrorType errorIn, const void* pErrorSrc)
{
	std::lock_guard<std::mutex> lock(_mutexLock);

	if (errorIn)
	{
		adsError = errorIn;
		return _symbolEntry.size;
	}
	
	switch (_getDataType())
	{
	// case EAdsDataTypeId::ADST_VOID: // Invalid
	case EAdsDataTypeId::ADST_INT8:
		_CopyCast<ValueType, int8>(&adsValue, pValueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT8:
		_CopyCast<ValueType, uint8>(&adsValue, pValueSrc);
		break;
	case EAdsDataTypeId::ADST_INT16:
		_CopyCast<ValueType, int16>(&adsValue, pValueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT16:
		_CopyCast<ValueType, uint16>(&adsValue, pValueSrc);
		break;
	case EAdsDataTypeId::ADST_INT32:
		_CopyCast<ValueType, int32>(&adsValue, pValueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT32:
		_CopyCast<ValueType, uint32>(&adsValue, pValueSrc);
		break;
	case EAdsDataTypeId::ADST_INT64:
		_CopyCast<ValueType, int64>(&adsValue, pValueSrc);
		break;
	case EAdsDataTypeId::ADST_UINT64:
		_CopyCast<ValueType, uint64>(&adsValue, pValueSrc);
		break;
	case EAdsDataTypeId::ADST_REAL32: 
		_CopyCast<ValueType, float>(&adsValue, pValueSrc);
		break;
	case EAdsDataTypeId::ADST_REAL64:
		_CopyCast<ValueType, double>(&adsValue, pValueSrc);
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

	if (overwritePrevVal)
		_prevVal = adsValue;
	
	if (pErrorSrc)
		adsError = *static_cast<const ErrorType*>(pErrorSrc);
	
	return _symbolEntry.size;
}

FTcAdsAsyncVariable::ErrorType FTcAdsAsyncVariable::fetchSymbolEntry(const FString& adsName, LONG adsPort, AmsAddr* amsAddr)
{
	if (adsName == TEXT(""))
		return setError(ADSERR_DEVICE_INVALIDPARM);
	
	FSimpleAsciiString varName(adsName);
	unsigned long bytesRead;

	AdsSymbolEntry symbolEntry;
	
	auto err = AdsSyncReadWriteReqEx2(
		adsPort,
		amsAddr,
		ADSIGRP_SYM_INFOBYNAMEEX,
		0,
		sizeof(symbolEntry),
		&symbolEntry,
		varName.byteSize(),
		varName.getData(),
		&bytesRead
	);

	{
		std::lock_guard<std::mutex> lock(_mutexLock);
	
		adsError = err;
		if (!err)
			_symbolEntry = symbolEntry;
	
	}

	return err;
}

FTcAdsAsyncVariable::ErrorType FTcAdsAsyncVariable::setCallback(LONG port, AmsAddr* amsAddr, ULONG cycleTime)
{
	ULONG iGroup, iOffs;
	int32 hUser;

	AdsNotificationAttrib notificationAttrib;
	
	{ // Limit the lock scope so that the PLC call happens unlocked
		std::lock_guard<std::mutex> lock(_mutexLock);
		
		ADSTRANSMODE transMode; 
		switch (_adsMode)
		{
		case EAdsAccessMode::ReadCyclic:
			transMode = ADSTRANS_SERVERCYCLE;
			break;
		case EAdsAccessMode::ReadOnChange:
			// Fallthrough
		case EAdsAccessMode::ReadWriteOnChange:
			transMode = ADSTRANS_SERVERONCHA;
			break;
		default:
			return ADSERR_DEVICE_INVALIDDATA;
		}
	
		notificationAttrib = {
			_symbolEntry.size,
			transMode,
			0,
			cycleTime
		};

		iGroup = _symbolEntry.iGroup;
		iOffs = _symbolEntry.iOffs;
		hUser = _index;
	} // end lock
	
	ULONG hNotification;
	// Setup callback
	auto err = AdsSyncAddDeviceNotificationReqEx(
		port,
		amsAddr,
		iGroup,
		iOffs,
		&notificationAttrib,
		&FTcAdsAsyncWorker::ReadCallback,
		hUser,
		&hNotification
	);

	if (!err)
	{
		std::lock_guard<std::mutex> lock(_mutexLock);

		adsError = err;
		_notification = hNotification;
	}

	return err;
}

FTcAdsAsyncVariable::ErrorType FTcAdsAsyncVariable::releaseCallback(LONG port, AmsAddr* amsAddr)
{
	uint32 hNotification;
	{
		std::lock_guard<std::mutex> lock(_mutexLock);
		hNotification = _notification;
	}

	if (hNotification == 0)
		return 0;
	
	auto err = AdsSyncDelDeviceNotificationReqEx(
		port,
		amsAddr,
		hNotification
	);
	
	{
		std::lock_guard<std::mutex> lock(_mutexLock);
		adsError = err;
		_notification = 0;
	}
	
	return err;
}

EAdsAccessMode FTcAdsAsyncVariable::readyForUpdate()
{
	// If the value is not updated by the local timer there is no point in checking
	if (!CheckAdsUpdateFlag(_adsMode, EAdsRemoteReadFlag | EAdsRemoteWriteFlag))
		return EAdsAccessMode::None;
	
	if (++_updateCounter >= _updateInterval)
	{
		// If the value is supposed to only be updated on change, we first check if the value has changed
		if (CheckAdsUpdateFlag(_adsMode, EAdsUpdateOnChange))
		{
			// Check if the value has changed (use getValue() for thread safety)
			float tmp = getValue();
			if (tmp == _prevVal)
			{
				// Decrement counter to avoid overflow
				--_updateCounter;
				return EAdsAccessMode::None;
			}

			_prevVal = tmp;
		}
		
		_updateCounter = 0;
		return _adsMode;
	}

	return EAdsAccessMode::None;
}
