using UnrealBuildTool;

public class HanhLangLap : ModuleRules
{
	public HanhLangLap(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Slate", "SlateCore", "ProceduralMeshComponent", "AssetRegistry", "AudioMixer" });
	}
}
