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

	explicit FTcAdsAsyncVariable(UTcAdsAsyncVariable* reference, int32 index);
	// FTcAdsAsyncVariable(ULONG port, AmsAddr* addr, float initialValue = 0);
	~FTcAdsAsyncVariable();

	TTcAdsValueStruct getValueAndError();
	ValueType getValue();
	ErrorType getError();

//	ValueType setValue(ValueType val, ErrorType error);
	ErrorType setValue(ValueType val);
	ErrorType setError(ErrorType error);
	
	EAdsDataTypeId getDataType();
	int32 getIndex();
	EAdsAccessMode getAdsMode();

	size_t size() const { return _symbolEntry.size; }
	size_t readSize() const { return (size() + sizeof(ErrorType)); }
	size_t writeSize() const { return (size() + sizeof(FDataPar)); }
	
	size_t pack(void* pValueDst, void* pSymbolDst);
	size_t unpack(const void* pValueSrc, bool overwritePrevVal, ErrorType errorIn = 0, const void* pErrorSrc = nullptr);
	
	ErrorType fetchSymbolEntry(const FString& adsName, LONG adsPort, AmsAddr* amsAddr);
	ErrorType setCallback(LONG port, AmsAddr* amsAddr, ULONG cycleTime);
	ErrorType releaseCallback(LONG port, AmsAddr* amsAddr);

	const AdsSymbolEntry& getSymbolEntry() const { return _symbolEntry; }
	const FDataPar& getDataPar() const { return *reinterpret_cast<const FDataPar*>(&_symbolEntry.iGroup); }
	
	EAdsAccessMode readyForUpdate();
	
private:
	const EAdsAccessMode _adsMode;
	int32 _updateCounter;
	const int32 _updateInterval;
	float _prevVal;
	int32 _index;
	uint32 _notification;
	UTcAdsAsyncVariable* _reference;
	AdsSymbolEntry _symbolEntry;
	
	std::mutex _mutexLock;
	
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
