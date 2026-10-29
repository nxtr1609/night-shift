using UnrealBuildTool;

public class HanhLangLapEditorTarget : TargetRules
{
	public HanhLangLapEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("HanhLangLap");
	}
}
