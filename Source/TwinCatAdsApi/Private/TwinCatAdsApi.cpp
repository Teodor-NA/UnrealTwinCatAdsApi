// Copyright Epic Games, Inc. All Rights Reserved.

#include "TwinCatAdsApi.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "ThirdParty/TwinCatAdsApiLibrary/Include/TcAdsDef.h"
#include "ThirdParty/TwinCatAdsApiLibrary/Include/TcAdsAPI.h"

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

	// Add on the relative location of the third party dll and load it
	// FString LibraryPath;

	//	LibraryPath = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/Win64/TcAdsDll.dll"));
	FString LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/TwinCatAdsApiLibrary/Bin/TcAdsDll.dll"));

	// ExampleLibraryHandle = !LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*LibraryPath) : nullptr;
	if (LibraryPath.IsEmpty())
	{
		TcAdsDllLibraryHandle = nullptr;
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("AdsLibrary", "TwinCAT ADS library path is not valid"));
	}
	else
	{
		TcAdsDllLibraryHandle = FPlatformProcess::GetDllHandle(*LibraryPath);
	}

	if (TcAdsDllLibraryHandle)
	{
		// Call the test function in the third party library that opens a message box
		AdsVersion Ver;
		*reinterpret_cast<uint32*>(&Ver) = AdsGetDllVersion();

		// FText Disp = FText::Format(LOCTEXT("AdsLibrary", "Loaded third party TwinCAT ADS library version: %ld"), Ver);

		FText Disp = FText::FromString(FString::Printf(TEXT("Loaded third party TwinCAT ADS library version: %hhu.%hhu.%hu"), Ver.version, Ver.revision, Ver.build));
		
		FMessageDialog::Open(EAppMsgType::Ok, Disp);
	}
	else
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
