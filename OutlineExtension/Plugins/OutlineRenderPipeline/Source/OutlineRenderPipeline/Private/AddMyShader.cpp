#include "AddMyShader.h"

#include "PixelShaderUtils.h"
#include "SceneRendering.h"
#include "ScreenPass.h"
#include "ShaderParameterStruct.h"
#include "DataDrivenShaderPlatformInfo.h"

//  GPU パフォーマンスの統計を記録・表示するためのマクロ
DECLARE_GPU_STAT(AddMyCS);
DECLARE_GPU_STAT(AddMyPS);

namespace
{
	class FAddMyShaderCS : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FAddMyShaderCS);
		SHADER_USE_PARAMETER_STRUCT(FAddMyShaderCS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
			SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutputTexture)
		END_SHADER_PARAMETER_STRUCT()
	};

	IMPLEMENT_GLOBAL_SHADER(FAddMyShaderCS, "/Plugin/OutlineRenderPipeline/Private/AddMy.usf", "AddMyCS", SF_Compute);

	

	class FAddMyShaderPS : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FAddMyShaderPS);
		SHADER_USE_PARAMETER_STRUCT(FAddMyShaderPS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
			SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureShaderParameters, SceneTextures)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
			SHADER_PARAMETER_SAMPLER(SamplerState, SceneColorSampler)
			RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()
	};
	
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) || IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}
	IMPLEMENT_GLOBAL_SHADER(FAddMyShaderPS, "/Plugin/OutlineRenderPipeline/Private/AddMy.usf", "AddMyPS", SF_Pixel);
}

void AddComputePass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FAddMyShaderInput& Inputs)
{
	RDG_EVENT_SCOPE(GraphBuilder, "AddComputePass");
	FGlobalShaderMap* shaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	FScreenPassTextureViewport viewport = FScreenPassTextureViewport(View.ViewRect);
	
	FRDGTextureRef myTexture{};
	{
		FRDGTextureDesc desc = FRDGTextureDesc::Create2D(viewport.Extent, PF_R32_UINT, FClearValueBinding::Black, TexCreate_ShaderResource | TexCreate_UAV);
		myTexture = GraphBuilder.CreateTexture(desc, TEXT("myTexture"));
		FRDGTextureUAVRef myTextureUAV = GraphBuilder.CreateUAV(myTexture);
		AddClearUAVPass(GraphBuilder, myTextureUAV, (UE::HLSL::float4)0);

		FAddMyShaderCS::FParameters* parameters = GraphBuilder.AllocParameters<FAddMyShaderCS::FParameters>();
		parameters->View = View.ViewUniformBuffer;
		parameters->Input = GetScreenPassTextureViewportParameters(viewport);
		parameters->InputTexture = Inputs.OutputTexture;
		parameters->OutputTexture = myTextureUAV;

		TShaderMapRef<FAddMyShaderCS> computeShader(shaderMap);
		// numthreads(8,8,1) * FIntPoint(8,8) = 1024スレッド
		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("AddMyCS"),
			computeShader,
			parameters,
			FComputeShaderUtils::GetGroupCount(viewport.Rect.Size(), FIntPoint(8, 8)));
	}	
}

void AddPixelPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FAddMyShaderInput& Inputs)
{
	RDG_EVENT_SCOPE(GraphBuilder, "PostProcessAddMyPS");
	RDG_GPU_STAT_SCOPE(GraphBuilder, AddMyPS);
	
	FGlobalShaderMap* shaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	FScreenPassTextureViewport Viewport = FScreenPassTextureViewport(View.ViewRect);

	{
		const FIntRect PrimaryViewRect = static_cast<const FViewInfo&>(View).ViewRect;

		// Scene color is updated incrementally through the post process pipeline.
		FScreenPassTexture SceneColor((*Inputs.SceneTextures)->SceneColorTexture, PrimaryViewRect);
		const FScreenPassTextureViewport InputViewport(SceneColor);
		const FScreenPassTextureViewport OutputViewport(InputViewport);
		TShaderMapRef<FAddMyShaderPS> PixelShader(static_cast<const FViewInfo&>(View).ShaderMap);

		FAddMyShaderPS::FParameters* Parameters = GraphBuilder.AllocParameters<FAddMyShaderPS::FParameters>();
		Parameters->View = View.ViewUniformBuffer;
		Parameters->Input = GetScreenPassTextureViewportParameters(Viewport);
		// NOTE : 動く
		Parameters->SceneTextures = GetSceneTextureShaderParameters(Inputs.SceneTextures);
		Parameters->SceneColorTexture =  Inputs.OutputTexture;
		Parameters->SceneColorSampler = TStaticSamplerState<SF_Point>::GetRHI();
		//
		// 読み出しに前の工程で使ったテクスチャが使えれば、追加加工できるはず・・・？
		//
		//Parameters->SceneTextures = GetSceneTextureShaderParameters(Inputs.OutputTexture);
		// SV_Target0(出力)
		Parameters->RenderTargets[0] = FRenderTargetBinding(Inputs.OutputTexture, ERenderTargetLoadAction::ELoad);
		AddDrawScreenPass(
			GraphBuilder,
			RDG_EVENT_NAME("AddMyPS"),
			View,
			OutputViewport,
			InputViewport,
			PixelShader,
			Parameters);
	}
}