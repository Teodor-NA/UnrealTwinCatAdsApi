#include "TcAdsAsyncVariable.h"
#include "TcAdsAsyncMaster.h"
#include "TcAdsAsyncVariable_Private.h"

UTcAdsAsyncVariable::UTcAdsAsyncVariable()
	: _variable(nullptr)
	, _prevVal(0.0f)
	, adsSetupInfo({TEXT(""), EAdsAccessMode::None, 0, 0.0f, nullptr})
	, adsValue(0.0f)
	, adsError(0)
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UTcAdsAsyncVariable::TickComponent(float deltaTime, ELevelTick tickType,
	FActorComponentTickFunction* thisTickFunction)
{
	Super::TickComponent(deltaTime, tickType, thisTickFunction);

	if (_variable)
	{
		bool change = (adsValue != _prevVal);
		_prevVal = adsValue;
		
		if (CheckAdsUpdateFlag(adsSetupInfo.adsMode, EAdsLocalWriteFlag))
		{
			// Only write if there is a change
			if (change)
				adsError = _variable->setValue(adsValue);
		}
		if (CheckAdsUpdateFlag(adsSetupInfo.adsMode, EAdsLocalReadFlag))
		{
			// Delay reading if there is a local change
			if (!change)
			{
				auto tmp = _variable->getValueAndError();
				adsError = tmp.adsError;
				adsValue = _prevVal = tmp.adsValue; // disable change detection if there was a remote change
			}
		}
	}
}

void UTcAdsAsyncVariable::BeginPlay()
{
	Super::BeginPlay();

	_prevVal = adsValue;
	
	if (adsSetupInfo.adsMaster)
	{
		if (adsSetupInfo.adsName == TEXT(""))
		{
			LOG_TCADS_WARNING("Cannot create ADS '%s(%d)' in object '%s'. No variable name is specified"
				, *GetAdsAccessTypeName(adsSetupInfo.adsMode)
				, adsSetupInfo.adsMode
				, *GetName()
			);
			return;
		}
		
		_variable = adsSetupInfo.adsMaster->createVariable(this);
	}
	else
	{
		LOG_TCADS_WARNING("Cannot create ADS '%s(%d)' variable '%s'. No master is connected"
			, *GetAdsAccessTypeName(adsSetupInfo.adsMode)
			, adsSetupInfo.adsMode
			, *adsSetupInfo.adsName
		);
	}
}

void UTcAdsAsyncVariable::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);

	// Invalidate reference if it exists
	if (_variable)
	{
		// If it is a callback variable, then it must be released
		if (CheckAdsUpdateFlag(_variable->getAdsMode(), EAdsCallbackFlag))
		{
			AmsAddr addr = adsSetupInfo.adsMaster->getRemoteAmsAddr();
			adsError = _variable->releaseCallback(adsSetupInfo.adsMaster->getLocalAdsPort(), &addr);

			if (adsError)
			{
				LOG_TCADS_ERROR("Failed to release callback for '%s(%d)' variable '%s' from the main thread. Error code: 0x%x"
					, *GetAdsAccessTypeName(adsSetupInfo.adsMode)
					, adsSetupInfo.adsMode
					, *adsSetupInfo.adsName
					, adsError
				);
			}
			else
			{
				LOG_TCADS_DISPLAY("Released callback for '%s(%d)' variable '%s' from the main thread"
					, *GetAdsAccessTypeName(adsSetupInfo.adsMode)
					, adsSetupInfo.adsMode
					, *adsSetupInfo.adsName
				);
			}
			
		}
		
		_variable->_reference = nullptr;
		LOG_TCADS_DISPLAY("Ads variable '%s' was invalidated from main thread", *adsSetupInfo.adsName);
	}
}
