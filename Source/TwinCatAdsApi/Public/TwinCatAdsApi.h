// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FTwinCatAdsApiModule : public IModuleInterface
{
public:
	FTwinCatAdsApiModule() : TcAdsDllLibraryHandle(nullptr) {}
	
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Handle to the TwinCAT ADS dll */
	void* TcAdsDllLibraryHandle;
};
