#include "AddMyShader.h"

#include "PixelShaderUtils.h"
#include "SceneRendering.h"
#include "ScreenPass.h"
#include "ShaderParameterStruct.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "RenderTargetPool.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "PostProcess/PostProcessing.h"

//  GPU パフォーマンスの統計を記録・表示するためのマクロ
DECLARE_GPU_STAT(AddMyCS);
DECLARE_GPU_STAT(AddMyPS);

namespace
{
	enum EValueType
	{
		Color,
		Normal,
		Material,
		MAX
	};
	class FValueType : SHADER_PERMUTATION_ENUM_CLASS("VALUE_TYPE", EValueType);
	using FCommonDomain = TShaderPermutationDomain<FValueType>;
	
	class FAddMyShaderCS : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FAddMyShaderCS);
		SHADER_USE_PARAMETER_STRUCT(FAddMyShaderCS, FGlobalShader);
		
		using FPermutationDomain = FCommonDomain;

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
			SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutputTexture)
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params)
		{
			return IsFeatureLevelSupported(Params.Platform, ERHIFeatureLevel::SM5) || IsFeatureLevelSupported(Params.Platform, ERHIFeatureLevel::SM6);
		}
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


FRDGTextureRef AddComputePass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FAddMyShaderCSInput& Inputs)
{
	FIntPoint Resolution = View.ViewRect.Size();

	FScreenPassTextureViewport viewport = FScreenPassTextureViewport(View.ViewRect);
	FRDGTextureDesc desc = FRDGTextureDesc::Create2D(Inputs.Target->Desc.Extent, PF_A32B32G32R32F, FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV);
	FRDGTextureRef outputTexture = GraphBuilder.CreateTexture(desc, TEXT("myTexture"));

	FGlobalShaderMap* shaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	// TShaderMapRef<FAddMyShaderCS> computeShader(shaderMap);

	FAddMyShaderCS::FParameters* parameters = GraphBuilder.AllocParameters<FAddMyShaderCS::FParameters>();
	parameters->View = View.ViewUniformBuffer;
	parameters->Input = GetScreenPassTextureViewportParameters(viewport);
	parameters->InputTexture = Inputs.Target;
	// 出力は、(*Inputs.SceneTextures)->SceneColorTexture に対して行う
	parameters->OutputTexture = GraphBuilder.CreateUAV(Inputs.Target);
	// outputTexture(出力バッファ)に書き込むとおかしくなる
	// parameters->OutputTexture = GraphBuilder.CreateUAV(outputTexture);

	FCommonDomain PermutationVector{};
	PermutationVector.Set<FValueType>(EValueType::Color);
	
	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("AddMyCS"),
		TShaderMapRef<FAddMyShaderCS>(shaderMap, PermutationVector),
		parameters,
		FComputeShaderUtils::GetGroupCount(viewport.Rect.Size(),
			FIntPoint(16, 16))
			);

	/*
	 * 結果が反映されない
	 * 
	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("AddMyCS"),
		computeShader,
		parameters,
		FComputeShaderUtils::GetGroupCount(viewport.Rect.Size(),
		FIntPoint(16, 16)));
		*/
	
	return outputTexture;
} 


void AddPixelPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FAddMyShaderPSInput& Inputs)
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
		Parameters->SceneColorTexture = Inputs.OutputTexture;
		// 加工前のバッファを表示
		// Parameters->SceneColorTexture = (*Inputs.SceneTextures)->SceneColorTexture;
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