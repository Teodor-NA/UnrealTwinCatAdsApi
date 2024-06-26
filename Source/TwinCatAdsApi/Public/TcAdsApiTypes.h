﻿
#pragma once

#include "TcAdsApiTypes.generated.h"

// Forward declarations
class UTcAdsVariable;
class ATcAdsMaster;

class UTcAdsAsyncVariable;
class ATcAdsAsyncMaster;
class FTcAdsAsyncWorker;
class FTcAdsAsyncVariable;

// Create custom UE log category
DECLARE_LOG_CATEGORY_EXTERN(LogTcAds, Display, Log);

// Custom log macros
#define LOG_TCADS_DISPLAY(message, ...) UE_LOG(LogTcAds, Display, TEXT(message), __VA_ARGS__)
#define LOG_TCADS_WARNING(message, ...) UE_LOG(LogTcAds, Warning, TEXT(message), __VA_ARGS__)
#define LOG_TCADS_ERROR(message, ...) UE_LOG(LogTcAds, Error, TEXT(message), __VA_ARGS__)

// Making the type enum explicitly since including wtypes.h seems to break Unreal

/* From wtypes.h
 * VT_EMPTY	= 0,
 * VT_NULL	= 1,
 * VT_I2	= 2,
 * VT_I4	= 3,
 * VT_R4	= 4,
 * VT_R8	= 5,
 * VT_CY	= 6,
 * VT_DATE	= 7,
 * VT_BSTR	= 8,
 * VT_DISPATCH	= 9,
 * VT_ERROR	= 10,
 * VT_BOOL	= 11,
 * VT_VARIANT	= 12,
 * VT_UNKNOWN	= 13,
 * VT_DECIMAL	= 14,
 * VT_I1	= 16,
 * VT_UI1	= 17,
 * VT_UI2	= 18,
 * VT_UI4	= 19,
 * VT_I8	= 20,
 * VT_UI8	= 21,
 * VT_INT	= 22,
 * VT_UINT	= 23,
 * VT_VOID	= 24,
 * VT_HRESULT	= 25,
 * VT_PTR	= 26,
 * VT_SAFEARRAY	= 27,
 * VT_CARRAY	= 28,
 * VT_USERDEFINED	= 29,
 * VT_LPSTR	= 30,
 * VT_LPWSTR	= 31,
 * VT_RECORD	= 36,
 * VT_INT_PTR	= 37,
 * VT_UINT_PTR	= 38,
 * VT_FILETIME	= 64,
 * VT_BLOB	= 65,
 * VT_STREAM	= 66,
 * VT_STORAGE	= 67,
 * VT_STREAMED_OBJECT	= 68,
 * VT_STORED_OBJECT	= 69,
 * VT_BLOB_OBJECT	= 70,
 * VT_CF	= 71,
 * VT_CLSID	= 72,
 * VT_VERSIONED_STREAM	= 73,
 * VT_BSTR_BLOB	= 0xfff,
 * VT_VECTOR	= 0x1000,
 * VT_ARRAY	= 0x2000,
 * VT_BYREF	= 0x4000,
 * VT_RESERVED	= 0x8000,
 * VT_ILLEGAL	= 0xffff,
 * VT_ILLEGALMASKED	= 0xfff,
 * VT_TYPEMASK	= 0xfff
*/

/* From https://infosys.beckhoff.com/content/1033/tc3_adsdll2/124832011.html?id=891376196530976748
 * ADST_VOID = VT_EMPTY,
 * ADST_INT8 = VT_I1,
 * ADST_UINT8 = VT_UI1,
 * ADST_INT16 = VT_I2,
 * ADST_UINT16 = VT_UI2,
 * ADST_INT32 = VT_I4,
 * ADST_UINT32 = VT_UI4,
 * ADST_INT64 = VT_I8,
 * ADST_UINT64 = VT_UI8,
 * ADST_REAL32 = VT_R4, 
 * ADST_REAL64 = VT_R8,
 * ADST_STRING = VT_LPSTR,
 * ADST_WSTRING = VT_LPWSTR,
 * ADST_REAL80 = VT_LPWSTR+1,
 * ADST_BIT = VT_LPWSTR+2,
 * ADST_BIGTYPE = VT_BLOB,
 * ADST_MAXTYPES = VT_STORAGE
*/

UENUM()
enum class EAdsDataTypeId : uint32
{
	ADST_VOID = 0,
	ADST_INT8 = 16,
	ADST_UINT8 = 17,
	ADST_INT16 = 2,
	ADST_UINT16 = 18,
	ADST_INT32 = 3,
	ADST_UINT32 = 19,
	ADST_INT64 = 20,
	ADST_UINT64 = 21,
	ADST_REAL32 = 4, 
	ADST_REAL64 = 5,
	ADST_STRING = 30,
	ADST_WSTRING = 31,
	ADST_REAL80 = 32,
	ADST_BIT = 33,
	ADST_BIGTYPE = 65,
	ADST_MAXTYPES = 67
};

constexpr uint8 EAdsRemoteReadFlag = 0x80;
constexpr uint8 EAdsRemoteWriteFlag = 0x40;
constexpr uint8 EAdsLocalReadFlag = 0x20;
constexpr uint8 EAdsLocalWriteFlag = 0x10;
constexpr uint8 EAdsUpdateOnChange = 0x8;
constexpr uint8 EAdsCallbackFlag = 0x4;

UENUM(BlueprintType)
enum class EAdsAccessMode : uint8
{
	// Disabled
	None = 0x0  UMETA(DisplayName = "None"),
	// Read on remote cycle
	ReadCyclic
		= EAdsLocalReadFlag
		| EAdsCallbackFlag
		UMETA(DisplayName = "Read Cyclic"),
	// Read on change
	ReadOnChange
		= EAdsLocalReadFlag
		| EAdsUpdateOnChange
		| EAdsCallbackFlag
		UMETA(DisplayName = "Read On Change"),
	// Read on local cycle
	Read
		= EAdsRemoteReadFlag
		| EAdsLocalReadFlag
		UMETA(DisplayName = "Read"),
	// Write on local cycle
	Write
		= EAdsRemoteWriteFlag
		| EAdsLocalWriteFlag
		UMETA(DisplayName = "Write"),
	// Write on change
	WriteOnChange
		= EAdsRemoteWriteFlag
		| EAdsLocalWriteFlag
		| EAdsUpdateOnChange
		UMETA(DisplayName = "Write On Change"),
	// Read and write on change
	ReadWriteOnChange
		= EAdsLocalReadFlag
		| EAdsLocalWriteFlag
		| EAdsRemoteWriteFlag
		| EAdsUpdateOnChange
		| EAdsCallbackFlag
		UMETA(DisplayName = "Read/Write On Change")
};

template<typename EnumType>
FString GetEnumTypeName(EnumType val, const TCHAR* name)
{
	const UEnum* enumPtr = FindObject<UEnum>(ANY_PACKAGE, name, true);
	if (enumPtr)
		return enumPtr->GetDisplayNameTextByValue(static_cast<int64>(val)).ToString();

	return FString(TEXT("Invalid"));
}

inline FString GetAdsAccessTypeName(EAdsAccessMode val)
{
	return GetEnumTypeName<EAdsAccessMode>(val, TEXT("EAdsAccessMode"));
}

template <class T>
constexpr bool CheckAdsUpdateFlag(T accessMode, uint8 flag) { return (static_cast<uint8>(accessMode) & flag); }

USTRUCT(BlueprintType)
struct FTcAdsVariableInfo
{
	GENERATED_BODY()

	/**
	 * @brief Full name of the remote variable (including scope)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "ADS Name"))
	FString adsName;
	/**
	 * @brief Ads access mode
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "ADS Access Mode"))
	EAdsAccessMode adsMode;
	/**
	 * @brief Multiple of the connected master's base time interval to update the variable.
	 * example: if update interval is 3 and the master's base time is 0.01 s the variable will be updated every 0.03 s.
	 * if value is <= 1 the value will update with every tick
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "Cyckle Multiplier"))
	int32 cycleMultiplier;
	/**
	 * @brief Current value
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "Initial Value"))
	float initialValue;
	/**
	 * @brief Instance of master module \link ATcAdsAsyncMaster \endlink to handle the communication
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS Info", meta = (DisplayName = "ADS Master"))
	ATcAdsAsyncMaster* adsMaster;
};

// Struct for getting data from ADS using ADSIGRP_SUMUP_READ or ADSIGRP_SUMUP_WRITE
struct FDataPar
{
	ULONG indexGroup;	// index group in ADS server interface
	ULONG indexOffset;	// index offset in ADS server interface
	ULONG length;		// count of bytes to read	
};

/*!
 * A simple buffer class for handling dynamic memory allocation. Does \b not initialize new memory!
 * @tparam T Buffer type
 */
template <class T>
class TSimpleBuffer
{
public:
	explicit TSimpleBuffer(size_t Count = 0) : _data(nullptr), _count(0) { reserve(Count); }
	~TSimpleBuffer() { destroy(); }

	/*!
	 * Size of data type \a T
	 */
	static constexpr size_t TypeSize = sizeof(T);
	
	/*!
	 * Reserve new memory if \a Count is greater than currently allocated memory, otherwise does nothing. Deletes
	 * existing memory if already allocated. Does not copy content (any existing content is lost). Does not initialize
	 * new memory. Does not shrink.
	 * @param num Number of elements to reserve
	 */
	void reserve(const size_t num)
	{
		if (num <= _count)
			return;

		destroy();
		_data = static_cast<T*>(malloc(num * TypeSize));
		_count = num;
	}
	/*!
	 * Release any reserved memory. If no memory is reserved no action is taken
	 */
	void destroy()
	{
		if (_data == nullptr)
			return;

		free(_data);
		_data = nullptr;
		_count = 0;

	}

	/*!
	 * Get const data pointer
	 */
	const T* getData() const { return _data; }
	/*!
	 * Get data pointer
	 */
	T* getData() { return _data; }
	/*!
	 * Get the number of \a T elements currently reserved
	 */
	size_t count() const { return _count; }
	/*!
	 * Get the byte size of reserved memory
	 */
	size_t byteSize() const { return (_count*TypeSize); }
	
private:
	T* _data;
	size_t _count;
};

class FSimpleAsciiString : public TSimpleBuffer<char>
{
public:
	
	explicit FSimpleAsciiString(const char* cString) : TSimpleBuffer(strlen(cString) + 1)
	{
		strcpy_s(getData(), byteSize(), cString);
	}

	explicit FSimpleAsciiString(const FString& src) : FSimpleAsciiString(StringCast<ANSICHAR>(*src).Get()) {}
};

struct TcAdsCallbackStruct
{
	explicit TcAdsCallbackStruct(UTcAdsVariable* pVar = nullptr, int32 idx = 0) : variable(pVar), hUser(0), hNotification(0), index(idx) {}

	UTcAdsVariable* variable;
	ULONG hUser;
	ULONG hNotification;
	int32 index;

	bool operator ==(const UTcAdsVariable* other) const { return (other == variable);}
	bool operator !=(const UTcAdsVariable* other) const { return (other != variable);}
};

