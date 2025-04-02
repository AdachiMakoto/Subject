#include "AddMyShader.h"

#include "PixelShaderUtils.h"
#include "SceneRendering.h"
#include "ScreenPass.h"
#include "ShaderParameterStruct.h"


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
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, LineTexture)
			SHADER_PARAMETER(FVector4f, LineColor)
			SHADER_PARAMETER(int, LineWidth)
			SHADER_PARAMETER(int, SearchRangeMin)
			SHADER_PARAMETER(int, SearchRangeMax)
			RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()
	};

	IMPLEMENT_GLOBAL_SHADER(FAddMyShaderPS, "/Plugin/OutlineRenderPipeline/Private/AddMy.usf", "AddMyPS", SF_Pixel);
}

void AddPixelPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FAddMyShaderInput& Inputs)
{
	RDG_EVENT_SCOPE(GraphBuilder, "PostProcessAddMyPS");
	RDG_GPU_STAT_SCOPE(GraphBuilder, AddMyPS);
	
	FGlobalShaderMap* shaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	FScreenPassTextureViewport Viewport = FScreenPassTextureViewport(View.ViewRect);

	{
		FAddMyShaderPS::FParameters* Parameters = GraphBuilder.AllocParameters<FAddMyShaderPS::FParameters>();
		// Parameters->Input = GetScreenPassTextureViewportParameters(Viewport);
		Parameters->View = View.ViewUniformBuffer;
		Parameters->Input = GetScreenPassTextureViewportParameters(Viewport);
		Parameters->LineTexture = Inputs.LineTexture;
		Parameters->LineColor = Inputs.LineColor;
		Parameters->LineWidth = Inputs.LineWidth * Inputs.LineWidth;
		Parameters->SearchRangeMin = -(Inputs.LineWidth / 2);
		Parameters->SearchRangeMax = (Inputs.LineWidth / 2);
		Parameters->RenderTargets[0] = FRenderTargetBinding(Inputs.Target, ERenderTargetLoadAction::ELoad);
		Parameters->RenderTargets.DepthStencil = FDepthStencilBinding(Inputs.SceneDepth, ERenderTargetLoadAction::ELoad, FExclusiveDepthStencil::DepthWrite_StencilNop);

		FRHIBlendState* blendState = TStaticBlendState<CW_RGB>::GetRHI();
		FRHIDepthStencilState* depthStencilState = TStaticDepthStencilState<true, CF_Always>::GetRHI();

		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder,
			shaderMap,
			RDG_EVENT_NAME("AddMyPS"),
			TShaderMapRef<FAddMyShaderPS>(shaderMap),
			Parameters,
			Viewport.Rect,
			blendState,
			nullptr,
			depthStencilState);
	}

	// FRDGTextureRef TestTexture;
	// {
	// 	FIntPoint TestTextureEvent = FIntPoint::DivideAndRoundUp(Inputs.SceneColor.ViewRect.Size(), PF_B8G8R8A8, FClear);
	// 	FRDGTextureDesc Desc = FRDGTextureDesc::Create2D();
	// }
}