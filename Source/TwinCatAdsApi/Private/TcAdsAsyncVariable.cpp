#include "TcAdsAsyncVariable.h"
#include "TcAdsAsyncMaster.h"
#include "TcAdsAsyncVariable_Private.h"

UTcAdsAsyncVariable::UTcAdsAsyncVariable()
	: _variable(nullptr)
	, adsMode(EAdsAccessMode::None)
	, initialValue(0.0f)
	, updateInterval(0)
{
	PrimaryComponentTick.bCanEverTick = false;

	
}

void UTcAdsAsyncVariable::TickComponent(float deltaTime, ELevelTick tickType,
	FActorComponentTickFunction* thisTickFunction)
{
	Super::TickComponent(deltaTime, tickType, thisTickFunction);
	
}

float UTcAdsAsyncVariable::getValue() const
{
	if(!_variable)
		return 0.0f;

	return _variable->getValue();
}

float UTcAdsAsyncVariable::setValue(float val) const
{
	if(!_variable)
		return val;

	return _variable->setValue(val);
}

void UTcAdsAsyncVariable::BeginPlay()
{
	Super::BeginPlay();

	if (adsMaster)
		_variable = adsMaster->createVariable(this);
	else
	{
		LOG_TCADS_WARNING("Cannot create ADS '%s(%d)' variable '%s'. No master is connected"
			, *GetAdsAccessTypeName(adsMode)
			, adsMode
			, *adsName
		);
	}
}

void UTcAdsAsyncVariable::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);

	// Invalidate reference if it exists
	if (_variable)
	{
		_variable->_reference = nullptr;
		LOG_TCADS_DISPLAY("Ads variable '%s' was invalidated from main", *adsName);
	}
}
