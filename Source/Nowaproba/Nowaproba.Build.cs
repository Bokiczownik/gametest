using UnrealBuildTool;

public class Nowaproba : ModuleRules
{
	public Nowaproba(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "UMG", "GameplayAbilities", "GameplayTags", "GameplayTasks" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "EnhancedInput", "InputCore", "AIModule", "NavigationSystem" });
	}
}
