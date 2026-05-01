// Copyright (c) Jared Taylor

using UnrealBuildTool;

public class GraspSmartObject : ModuleRules
{
	public GraspSmartObject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayAbilities",
				"GameplayTags",
				"GameplayTasks",
				"Grasp",
				"GameplayBehaviorsModule",
				"SmartObjectsModule",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AIModule",
				"GameplayBehaviorSmartObjectsModule",
			}
		);
	}
}
