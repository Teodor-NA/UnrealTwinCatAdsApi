#include "TcAdsAsyncVariable_Private.h"

#include "TcAdsAPI.h"
#include "TcAdsAsyncVariable.h"


FTcAdsAsyncVariable::FTcAdsAsyncVariable(UTcAdsAsyncVariable* reference)
	: TTcAdsValueStruct({reference->initialValue, 0})
	, _updateMode(GetAdsUpdateMode(reference->adsMode))
	, _updateCounter(reference->updateInterval)
	, _reference(reference)
	, _updateInterval(reference->updateInterval)
	, _symbolEntry({0, 0,0, 0, 0, 0, 0, 0, 0})
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
		_reference->_variable = nullptr;
		LOG_TCADS_DISPLAY("Ads variable '%s' was invalidated from worker",
			*_reference->adsName
		);
	}
}

TTcAdsValueStruct<FTcAdsAsyncVariable::ValueType, FTcAdsAsyncVariable::ErrorType> FTcAdsAsyncVariable::getValueAndError()
{
	_mutexLock.lock();
	TTcAdsValueStruct tmp = *this;
	_mutexLock.unlock();
	return tmp;
}

FTcAdsAsyncVariable::ValueType FTcAdsAsyncVariable::getValue()
{
	_mutexLock.lock();
	ValueType tmp = adsValue;
	_mutexLock.unlock();
	return tmp;
}

FTcAdsAsyncVariable::ErrorType FTcAdsAsyncVariable::getError()
{
	_mutexLock.lock();
	ErrorType tmp = adsError;
	_mutexLock.unlock();
	return tmp;
}

// TTcAdsValueStruct FTcAdsAsyncVariable::getValue()
// {
// }

FTcAdsAsyncVariable::ValueType FTcAdsAsyncVariable::setValue(ValueType val, ErrorType error)
{
	_mutexLock.lock();
	adsValue = val;
	adsError = error;
	_mutexLock.unlock();

	return val;
}

FTcAdsAsyncVariable::ValueType FTcAdsAsyncVariable::setValue(ValueType val)
{
	_mutexLock.lock();
	adsValue = val;
	_mutexLock.unlock();
	return val;
}

FTcAdsAsyncVariable::ErrorType FTcAdsAsyncVariable::setError(ErrorType error)
{
	_mutexLock.lock();
	adsError = error;
	_mutexLock.unlock();
	return error;
}

EAdsDataTypeId FTcAdsAsyncVariable::getDataType()
{
	_mutexLock.lock();
	EAdsDataTypeId tmp = _getDataType();
	_mutexLock.unlock();
	return tmp;
}

size_t FTcAdsAsyncVariable::pack(void* pValueDst, void* pSymbolDst)
{
	_mutexLock.lock();
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
	
	size_t tmp = _symbolEntry.size;

	_mutexLock.unlock();
	
	return tmp;
}

size_t FTcAdsAsyncVariable::unpack(const void* pValueSrc, ErrorType errorIn, const void* pErrorSrc)
{
	_mutexLock.lock();

	if (errorIn)
	{
		adsError = errorIn;
		size_t tmp = _symbolEntry.size;
		_mutexLock.unlock();
		return tmp;
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

	if (pErrorSrc)
		adsError = *static_cast<const ErrorType*>(pErrorSrc);
	
	size_t tmp = _symbolEntry.size;

	_mutexLock.unlock();
	
	return tmp;
}

FTcAdsAsyncVariable::ErrorType FTcAdsAsyncVariable::fetchSymbolEntry(const FString& adsName, LONG adsPort, AmsAddr* amsAddr)
{
	if (adsName == TEXT(""))
		return setError(ADSERR_DEVICE_INVALIDPARM);
	
	FSimpleAsciiString varName(adsName);
	unsigned long bytesRead;

	_mutexLock.lock();
	auto err = AdsSyncReadWriteReqEx2(
		adsPort,
		amsAddr,
		ADSIGRP_SYM_INFOBYNAMEEX,
		0,
		sizeof(_symbolEntry),
		&_symbolEntry,
		varName.byteSize(),
		varName.getData(),
		&bytesRead
	);

	adsError = err;
	_mutexLock.unlock();
	
	return err;
}

EAdsUpdateMode FTcAdsAsyncVariable::readyForUpdate()
{
	// // Variable is not updated or remotely updated
	// if (_updateMode == EAdsUpdateMode::None)
	// 	return _updateMode;

	if (++_updateCounter >= _updateInterval)
	{
		_updateCounter = 0;
		return _updateMode;
	}

	return EAdsUpdateMode::None;
}
