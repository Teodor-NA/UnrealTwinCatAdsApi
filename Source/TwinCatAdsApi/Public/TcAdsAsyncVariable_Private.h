#pragma once

#include <mutex>

#include "TcAdsDef.h"
#include "TcAdsApiTypes.h"

template <class _TVal, class _TErr>
struct TTcAdsValueStruct
{
	using ValueType = _TVal;
	using ErrorType = _TErr;
	
	ValueType adsValue;
	ErrorType adsError;
};

class FTcAdsAsyncVariable : TTcAdsValueStruct<float, LONG>
{
	friend UTcAdsAsyncVariable;
	
public:
	// using ValueType = ValueType;
	// using ErrorType = ErrorType;

	explicit FTcAdsAsyncVariable(UTcAdsAsyncVariable* reference);
	// FTcAdsAsyncVariable(ULONG port, AmsAddr* addr, float initialValue = 0);
	~FTcAdsAsyncVariable();

	TTcAdsValueStruct getValueAndError();
	ValueType getValue();
	ErrorType getError();

	ValueType setValue(ValueType val, ErrorType error);
	ValueType setValue(ValueType val);
	ErrorType setError(ErrorType error);
	
	EAdsDataTypeId getDataType();

	size_t pack(void* pValueDst, void* pSymbolDst);
	size_t unpack(const void* pValueSrc, const void* pErrorSrc = nullptr);
	
	ErrorType getSymbolEntry(const FString& adsName, LONG adsPort, AmsAddr* amsAddr);
	
private:
	AdsSymbolEntry _symbolEntry;
	std::mutex _mutexLock;

	UTcAdsAsyncVariable* _reference;
	
	EAdsDataTypeId _getDataType() const { return static_cast<EAdsDataTypeId>(_symbolEntry.dataType); }
	const FDataPar& _getDataPar() const { return *reinterpret_cast<const FDataPar*>(&_symbolEntry.iGroup); }
	
	template <class TDst, class TSrc>
	static constexpr void _CopyCast(void* pDst, const void* pSrc);
};

template <class TDst, class TSrc>
constexpr void FTcAdsAsyncVariable::_CopyCast(void* pDst, const void* pSrc)
{
	// TDst* dst = pDst;
	// TSrc* src = pSrc;
	// *dst = static_cast<TDst>(*src);

	*static_cast<TDst*>(pDst) = static_cast<TDst>(*static_cast<const TSrc*>(pSrc));
}
