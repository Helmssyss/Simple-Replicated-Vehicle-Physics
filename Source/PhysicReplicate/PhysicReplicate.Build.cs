// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PhysicReplicate : ModuleRules
{
	public PhysicReplicate(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" ,"PhysXVehicles","PhysXVehicleLib"});
		PrivateDependencyModuleNames.AddRange(new string[] {  });
	}
}
