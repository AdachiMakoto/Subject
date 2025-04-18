#include "AddMyShader.h"

#include "PixelShaderUtils.h"
#include "SceneRendering.h"
#include "ScreenPass.h"
#include "ShaderParameterStruct.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "RenderTargetPool.h"

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
			// SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
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

/*
void AddComputePass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FAddMyShaderCSInput& Inputs)
{
	// ComputeShaderだけで反映させる
	ENQUEUE_RENDER_COMMAND(ComputeShaderPass)(
		[View, Inputs](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			const FIntPoint Size = FIntPoint(View.ViewRect.Width(), View.ViewRect.Height());

			// RDG用出力テクスチャを作成
			FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(
				FRDGTextureDesc::Create2D(
					Size,
					PF_FloatRGBA,
					FClearValueBinding::None,
					TexCreate_ShaderResource | TexCreate_UAV | TexCreate_RenderTargetable
				),
				TEXT("AddMyCSOutput")
			);

			// UAVパラメータ
			FScreenPassTextureViewport viewport = FScreenPassTextureViewport(View.ViewRect);
			FAddMyShaderCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAddMyShaderCS::FParameters>();
			Parameters->OutputTexture = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutputTexture));
			Parameters->Input = GetScreenPassTextureViewportParameters(viewport);
			Parameters->InputTexture = Inputs.Target;
			Parameters->OutputTexture = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutputTexture));

			// シェーダー取得
			TShaderMapRef<FAddMyShaderCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

			// パス登録
			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("AddMyCS"),
				ComputeShader,
				Parameters,
				FIntVector(Size.X / 8, Size.Y / 8, 1)
			);

			// 結果を外部テクスチャとして取り出す
			TRefCountPtr<IPooledRenderTarget> PooledOutput;
			GraphBuilder.QueueTextureExtraction(OutputTexture, &PooledOutput);

			GraphBuilder.Execute();

			// RenderTarget へコピー
			FTexture2DRHIRef OutputRHI = PooledOutput->GetRenderTargetItem().ShaderResourceTexture;
			// FRHITexture* DestTexture = RenderTarget->GetRenderTargetResource()->GetRenderTargetTexture();
	
			RHICmdList.CopyTexture(OutputRHI, Inputs.OutputTexture, FRHICopyTextureInfo());
		});
	
	/ *
	RDG_EVENT_SCOPE(GraphBuilder, "AddComputePass");
	FGlobalShaderMap* shaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	FScreenPassTextureViewport viewport = FScreenPassTextureViewport(View.ViewRect);
	
	 FRDGTextureRef myTexture;
	{
		FRDGTextureDesc desc = FRDGTextureDesc::Create2D(viewport.Extent, PF_R32_UINT, FClearValueBinding::Black, TexCreate_ShaderResource | TexCreate_UAV);
		myTexture = GraphBuilder.CreateTexture(desc, TEXT("myTexture"));
		// FRDGTextureUAVRef myTextureUAV = GraphBuilder.CreateUAV(myTexture);
		// AddClearUAVPass(GraphBuilder, myTextureUAV, (UE::HLSL::float4)0);

		FAddMyShaderCS::FParameters* parameters = GraphBuilder.AllocParameters<FAddMyShaderCS::FParameters>();
		// parameters->View = View.ViewUniformBuffer;
		parameters->Input = GetScreenPassTextureViewportParameters(viewport);
		// parameters->InputTexture = Inputs.OutputTexture;
		parameters->InputTexture = Inputs.Target;
		// Error : Attempted to create UAV from texture Outline.Output which was not created with TexCreate_UAV
		// parameters->OutputTexture = GraphBuilder.CreateUAV(FRDGTextureUAVDesc((Inputs.OutputTexture)));
		// エラーは出ないが正しく動かない
		parameters->OutputTexture = GraphBuilder.CreateUAV(Inputs.Target);
		// Test
		parameters->OutputTexture = GraphBuilder.CreateUAV(myTexture);
		// parameters->OutputTexture = myTextureUAV;

		TShaderMapRef<FAddMyShaderCS> computeShader(shaderMap);
		// numthreads(8,8,1) * FIntPoint(8,8) = 1024スレッド
		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("AddMyCS"),
			computeShader,
			parameters,
			FComputeShaderUtils::GetGroupCount(viewport.Rect.Size(),
			FIntPoint(16, 16)));
		
		// UE_LOG(LogTemp, Warning, TEXT("##### This route is now currency. bbbbbbbbbbb #####"));
	}
	* /
	
	//
	// バッファのコピー
	//
	// RDGの外へ出す
	// TRefCountPtr<IPooledRenderTarget> PooledOutput;
	// GraphBuilder.QueueTextureExtraction(myTexture, &PooledOutput);
	// RDGの実行
	// GraphBuilder.Execute();
	// 外部で RHITexture を取り出す
	// FTexture2DRHIRef OutputRHI = PooledOutput->GetRenderTargetItem().ShaderResourceTexture;
	// Copy to UTextureRenderTarget2D
	// CopyRDGOutputToRenderTarget(MyRenderTarget, OutputRHI);
}
*/


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