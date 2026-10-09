using UnrealBuildTool;

public class NowaprobaEditorTarget : TargetRules
{
	public NowaprobaEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Nowaproba");
	}
}
