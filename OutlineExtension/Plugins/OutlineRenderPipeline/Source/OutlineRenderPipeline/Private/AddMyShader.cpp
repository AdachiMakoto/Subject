#include "AddMyShader.h"

#include "PixelShaderUtils.h"
#include "SceneRendering.h"
#include "ScreenPass.h"
#include "ShaderParameterStruct.h"
#include "DataDrivenShaderPlatformInfo.h"


DECLARE_GPU_STAT(AddMyPS);

namespace
{
	class FAddMyShaderPS : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FAddMyShaderPS);
		SHADER_USE_PARAMETER_STRUCT(FAddMyShaderPS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
			SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureShaderParameters, SceneTextures)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()
	};
	
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) || IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}
	IMPLEMENT_GLOBAL_SHADER(FAddMyShaderPS, "/Plugin/OutlineRenderPipeline/Private/AddMy.usf", "AddMyPS", SF_Pixel);
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