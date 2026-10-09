using UnrealBuildTool;

public class NowaprobaTarget : TargetRules
{
	public NowaprobaTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Nowaproba");
	}
}
