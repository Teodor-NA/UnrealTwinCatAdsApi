// Copyright Epic Games, Inc. All Rights Reserved.

#include "TwinCatAdsApi.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FTwinCatAdsApiModule"

void FTwinCatAdsApiModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
#if !(PLATFORM_WINDOWS && PLATFORM_64BITS)
	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("AdsLibrary",
		"Sorry. This plugin is currently only supported by Windows x64. (Maybe I'll add support for other platforms later...)"));
	return;
#endif
	
	// Get the base directory of this plugin
	FString BaseDir = IPluginManager::Get().FindPlugin("TwinCatAdsApi")->GetBaseDir();
	FString ModuleDir = FPaths::Combine(*BaseDir, TEXT("Source"), TEXT("ThirdParty"), TEXT("TwinCatAdsApiLibrary"));
	
	// Add on the relative location of the third party dll and load it
#if PLATFORM_WINDOWS
#if PLATFORM_64BITS
	FString LibraryPath = FPaths::Combine(*ModuleDir, TEXT("Bin"), TEXT("Win64"), TEXT("TcAdsDll.dll"));
#endif
#elif PLATFORM_LINUX
	// FString LibraryPath = ; Load linux .so
#endif
	
	if (LibraryPath.IsEmpty())
	{
		TcAdsDllLibraryHandle = nullptr;
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("AdsLibrary", "TwinCAT ADS library path is not valid"));
	}
	else
	{
		TcAdsDllLibraryHandle = FPlatformProcess::GetDllHandle(*LibraryPath);
	}

	if (!TcAdsDllLibraryHandle)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("AdsLibrary", "Failed to load third party TwinCAT ADS library"));
	}
}

void FTwinCatAdsApiModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	// Free the dll handle
	FPlatformProcess::FreeDllHandle(TcAdsDllLibraryHandle);
	TcAdsDllLibraryHandle = nullptr;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTwinCatAdsApiModule, TwinCatAdsApi)
