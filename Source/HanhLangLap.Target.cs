using UnrealBuildTool;

public class HanhLangLapTarget : TargetRules
{
	public HanhLangLapTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("HanhLangLap");
	}
}
