// Copyright 2024 kafues511. All Rights Reserved.

#include "OutlineViewExtension.h"
#include "PostProcess/PostProcessing.h"
// #include "PostProcessMaterial.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "ScenePrivate.h"
#include "SystemTextures.h"
#include "SceneTextureParameters.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "OutlineSubsystem.h"
#include "AddMyShader.h"


DECLARE_GPU_STAT(Outline);

class FOutlinePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FOutlinePS);
	SHADER_USE_PARAMETER_STRUCT(FOutlinePS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureShaderParameters, SceneTextures)
		SHADER_PARAMETER(float, Radius)
		SHADER_PARAMETER(float, Bias)
		SHADER_PARAMETER(float, Intensity)
		SHADER_PARAMETER(FVector3f, Color)
		SHADER_PARAMETER(FVector4f, TestColor)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) || IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}
};

IMPLEMENT_GLOBAL_SHADER(FOutlinePS, "/Plugin/OutlineRenderPipeline/Private/Outline.usf", "MainPS", SF_Pixel);

FOutlineViewExtension::FOutlineViewExtension(const FAutoRegister& AutoRegister, UOutlineSubsystem* InOutlineSubsystem)
	: FSceneViewExtensionBase(AutoRegister)
	, OutlineSubsystem(InOutlineSubsystem)
{
}

void FOutlineViewExtension::Invalidate()
{
	OutlineSubsystem = nullptr;
}

void FOutlineViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
	check(IsInGameThread());

	if (!IsValid(OutlineSubsystem))
	{
		return;
	}

	FinalOutlineSettings = OutlineSubsystem->GetOutlineSettings();
}

void FOutlineViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs)
{
	if (!FinalOutlineSettings.Enabled)
	{
		return;
	}

	Inputs.Validate();

	const FIntRect PrimaryViewRect = static_cast<const FViewInfo&>(View).ViewRect;

	// Scene color is updated incrementally through the post process pipeline.
	FScreenPassTexture SceneColor((*Inputs.SceneTextures)->SceneColorTexture, PrimaryViewRect);

	RDG_EVENT_SCOPE(GraphBuilder, "Outline");
	RDG_GPU_STAT_SCOPE(GraphBuilder, Outline);

	const FScreenPassTextureViewport InputViewport(SceneColor);
	const FScreenPassTextureViewport OutputViewport(InputViewport);

	FRDGTextureRef OutputTexture;
	{
		FRDGTextureDesc OutputTextureDesc = SceneColor.Texture->Desc;
		OutputTextureDesc.Reset();
		OutputTextureDesc.Flags |= TexCreate_RenderTargetable | TexCreate_ShaderResource;
		OutputTexture = GraphBuilder.CreateTexture(OutputTextureDesc, TEXT("Outline.Output"));
	}

	// Outline Pass
	{
		TShaderMapRef<FScreenVS> VertexShader(static_cast<const FViewInfo&>(View).ShaderMap);
		TShaderMapRef<FOutlinePS> PixelShader(static_cast<const FViewInfo&>(View).ShaderMap);

		FOutlinePS::FParameters* PassParameters = GraphBuilder.AllocParameters<FOutlinePS::FParameters>();
		PassParameters->View = View.ViewUniformBuffer;
		PassParameters->SceneTextures = GetSceneTextureShaderParameters(Inputs.SceneTextures);
		PassParameters->Radius = FinalOutlineSettings.Radius;
		PassParameters->Bias = FinalOutlineSettings.Bias;
		PassParameters->Intensity = FinalOutlineSettings.Intensity;
		PassParameters->Color = FVector3f(FinalOutlineSettings.Color);
		PassParameters->RenderTargets[0] = FRenderTargetBinding(OutputTexture, ERenderTargetLoadAction::EClear);

		PassParameters->TestColor = FVector4f(0.0, 1.0, 0.0, 1.0);

		const FScreenPassTexture BlackDummy(GSystemTextures.GetBlackDummy(GraphBuilder));

		// This gets passed in whether or not it's used.
		GraphBuilder.RemoveUnusedTextureWarning(BlackDummy.Texture);

		AddDrawScreenPass(
			GraphBuilder,
			RDG_EVENT_NAME("Outline"),
			View,
			OutputViewport,
			InputViewport,
			VertexShader,
			PixelShader,
			TStaticBlendState<>::GetRHI(),
			TStaticDepthStencilState<false, CF_Always>::GetRHI(),
			PassParameters);
	}


	{
		FAddMyShaderInput addMyInputs;
		addMyInputs.Target = (*Inputs.SceneTextures)->SceneColorTexture;
		addMyInputs.LineWidth = 1;
		addMyInputs.LineColor = FLinearColor(0.f, 0.f, 0.f, 1.f);
		addMyInputs.LineTexture = nullptr;
		addMyInputs.SceneDepth = (*Inputs.SceneTextures)->SceneDepthTexture;
		
		// FRenderTargetBinding(OutputTexture, ERenderTargetLoadAction::EClear);
		// addMyInputs.SceneColor = Inputs.GetInput(EPostProcessMaterialInput::SceneColor);
		// FVector3f(FinalOutlineSettings.Color);
		
		auto& inView = static_cast<const FViewInfo&>(View);
		AddPixelPass(GraphBuilder, inView, addMyInputs);
		// TODO : Confirmed.
		// UE_LOG(LogTemp, Warning, TEXT("##### This route is now currency. #####"));
	}

	// Copy Pass
	{
		AddCopyTexturePass(GraphBuilder, OutputTexture, SceneColor.Texture);
	}
}
