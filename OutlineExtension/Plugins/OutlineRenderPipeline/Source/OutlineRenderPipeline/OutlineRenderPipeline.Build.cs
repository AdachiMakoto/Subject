// Copyright 2024 kafues511. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class OutlineRenderPipeline : ModuleRules
{
	public OutlineRenderPipeline(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// TODO:Adachi 『PostProcess/PostProcessing.h』『ScenePrivate.h』『SceneTextureParameters.h』のincludeを通す為に入れる
		PublicDependencyModuleNames.AddRange(new string[] { "Renderer" });
		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(GetModuleDirectory("Renderer"), "Private"),
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Projects",
				"RenderCore",
				"Renderer",
				"RHI",
			}
		);
	}
}
