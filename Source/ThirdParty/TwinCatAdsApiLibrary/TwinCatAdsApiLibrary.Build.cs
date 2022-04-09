// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class TwinCatAdsApiLibrary : ModuleRules
{
	public TwinCatAdsApiLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Add the import library
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "Lib", "Win64", "TcAdsDll.lib"));

			// Delay-load the DLL, so we can load it from the right place first
			PublicDelayLoadDLLs.Add("TcAdsDll.dll");

			// Doesn't seem like I need this one... No idea why
			// Ensure that the DLL is staged along with the executable
			// RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/TwinCatAdsApiLibrary/Win64/TcAdsDll.dll");
        }
	}
}
