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
#include "AnisotropicKuwahara.h"


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
	/*
	 * FPostProcessingInputs& Inputs の中身
	 * TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures・・・ポストプロセスをかけるのバッファ
	 * FRDGTextureRef ViewFamilyTexture・・・ポストプロセスチェーンの入力として使われるテクスチャ情報        // 白っぽい画面になる？
	 * FRDGTextureRef CustomDepthTexture・・・カスタムデプス（深度）バッファ                            // 正しく表示されない？
	 * FRDGTextureRef ExposureIlluminance・・・シーンの自動露出（Auto Exposure）や明るさの解析に関する情報 // クラッシュする
	 */
	
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
	
	
	// FRDGTextureRef OutputTexture;
	// {
	// 	FRDGTextureDesc OutputTextureDesc = SceneColor.Texture->Desc;
	// 	OutputTextureDesc.Reset();
	// 	OutputTextureDesc.Flags |= TexCreate_RenderTargetable | TexCreate_ShaderResource;
	// 	OutputTexture = GraphBuilder.CreateTexture(OutputTextureDesc, TEXT("Outline.Output"));
	// }

	//
	// 初期のバッファを保存
	//
	FRDGTextureRef SetupTexture;
	{
		FRDGTextureDesc OutputTextureDesc = SceneColor.Texture->Desc;
		OutputTextureDesc.Reset();
		OutputTextureDesc.Flags |= TexCreate_RenderTargetable | TexCreate_ShaderResource;
		SetupTexture = GraphBuilder.CreateTexture(OutputTextureDesc, TEXT("SetupTexture"));
		AddCopyTexturePass(GraphBuilder, SceneColor.Texture, SetupTexture);
	}

/*	
	//
	// Outline Pass
	//
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
		// TODO : テスト用
		PassParameters->TestColor = FVector4f(0.0, 1.0, 0.0, 1.0);
		// TODO : 動くコード
		PassParameters->RenderTargets[0] = FRenderTargetBinding(SceneColor.Texture, ERenderTargetLoadAction::ELoad);

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
*/	
/*
	//
	// Add Compute Shader
	//
	FRDGTextureRef OutputMyCSTexture;
	{
		FAddMyShaderCSInput addMyInputCS;
		// TODO : 動くコード
		addMyInputCS.Target = (*Inputs.SceneTextures)->SceneColorTexture;
		addMyInputCS.InputTexture = SceneColor.Texture;

		auto& inView = static_cast<const FViewInfo&>(View);
		OutputMyCSTexture =  AddComputePass(GraphBuilder, inView, addMyInputCS);
	} 
*/
	
	/*
	// Add Pixel Shader
	{
		FAddMyShaderPSInput addMyInputsPS;
		addMyInputsPS.Target = (*Inputs.SceneTextures)->SceneColorTexture;
		addMyInputsPS.SceneTextures = Inputs.SceneTextures;
		addMyInputsPS.OutputTexture = OutputTexture;
		
		auto& inView = static_cast<const FViewInfo&>(View);
		AddPixelPass(GraphBuilder, inView, addMyInputsPS);
		// TODO : Confirmed.
		// UE_LOG(LogTemp, Warning, TEXT("##### This route is now currency. #####"));
	}
	*/
	
/*
	//
	// バッファをリセットする
	//
	FRDGTextureRef copyTexture = nullptr;
	{
		FCopyShaderCSInput copyInputCS;
		copyInputCS.Target = (*Inputs.SceneTextures)->SceneColorTexture;
		copyInputCS.InputTexture = SetupTexture;

		auto& inView = static_cast<const FViewInfo&>(View);
		CopyComputePass(GraphBuilder, inView, copyInputCS);
	}
*/
	//
	// 異方性桑原フィルタ
	//
	/*
	if (OutlineSubsystem->EnableAnisoKuwahara)
	{
		FAnisotropicKuwaharaCSInput akInputCS;
		akInputCS.SceneTextures = Inputs.SceneTextures;
		akInputCS.AKTarget = (*Inputs.SceneTextures)->SceneColorTexture;
		akInputCS.aniso_kuwahara_aniso_control = OutlineSubsystem->AnisoKuwahara_AnisoControl;
		akInputCS.aniso_kuwahara_hardness = OutlineSubsystem->AnisoKuwahara_Hardness;
		akInputCS.aniso_kuwahara_sharpness = OutlineSubsystem->AnisoKuwahara_Sharpness;
		akInputCS.AKInputTexture = SetupTexture;

		akInputCS.AnisoKuwaharaGaussRadius = OutlineSubsystem->UnityAnisoKuwaharaGaussRadius;
		akInputCS.AnisoKuwaharaGaussSigma = OutlineSubsystem->UnityAnisoKuwaharaGaussSigma;
		akInputCS.AnisoKuwaharaRadius = OutlineSubsystem->UnityAnisoKuwaharaRadius;
		akInputCS.AnisoKuwaharaQ = OutlineSubsystem->UnityAnisoKuwaharaQ;
		akInputCS.AnisoKuwaharaAlpha = OutlineSubsystem->UnityAnisoKuwaharaAlpha;
		akInputCS.AnisoKuwaharaResolutionScale = OutlineSubsystem->UnityAnisoKuwaharaResolutionScale;
		// UE_LOG(LogTemp, Log, TEXT("@@@@ OutlineSubsystem->UnityAnisoKuwaharaGaussSigma=%f"), OutlineSubsystem->UnityAnisoKuwaharaGaussSigma);
		// UE_LOG(LogTemp, Log, TEXT("==== akInputCS.AnisoKuwaharaGaussSigma=%f"), akInputCS.AnisoKuwaharaGaussSigma);
		AnisotropicKuwaharaPass(GraphBuilder, View, akInputCS);
	}
	*/
	{
		FAnisotropicKuwaharaCSInput akInputCS;
		akInputCS.SceneTextures = Inputs.SceneTextures;
		akInputCS.AnisoKuwaharaAlpha = OutlineSubsystem->UnityAnisoKuwaharaAlpha;
		akInputCS.AnisoKuwaharaRadius = OutlineSubsystem->UnityAnisoKuwaharaRadius;
		AIKuwaharaPass(GraphBuilder, View, akInputCS);
	}

	
	// Copy Pass
	// {
		// NOTE : ここを実行すると画面がおかしくなる。『utputTexture』もしくは『SceneColor.Texture』が正しくデータが入っていない
		// NOTE : コンピュートシェーダで使用すると、画面が壊れる
		// AddCopyTexturePass(GraphBuilder, OutputTexture, SceneColor.Texture);
		// AddCopyTexturePass(GraphBuilder, OutputMyCSTexture, SceneColor.Texture);
	// }

}

//
// PostRenderBasePassDeferred_RenderThread
//
void FOutlineViewExtension::PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& View, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
{
/*
	auto& inView = static_cast<const FViewInfo&>(View);
	
	// NOTE : ひとます動く
	FPostAddMyShaderPSInput postAddMyInputPS;
	postAddMyInputPS.Target = RenderTargets[0].GetTexture();
	postAddMyInputPS.InputTexture = SceneTextures->GetParameters()->SceneColorTexture;
	postAddMyInputPS.SceneTextures = SceneTextures;
	PostAddMyPS(GraphBuilder, inView, postAddMyInputPS);
*/
}


//
// SubscribeToPostProcessingPass
//
void FOutlineViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	if (Pass == EPostProcessingPass::Tonemap)
	{
		// example : CallTestPassを呼び出す場合
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FOutlineViewExtension::CallTestPass));
	}
}

const int32 GDownsampleFactor = 4;

FScreenPassTexture FOutlineViewExtension::CallTestPass(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs)
{
	// NOTE : ログが出力されている事を確認した
	// UE_LOG(LogTemp, Warning, TEXT("**** Call FOutlineViewExtension::CallTestPass. ****"));
	// TODO : 何もしない呼び出し？
	return Inputs.GetInput(EPostProcessMaterialInput::SceneColor);
}