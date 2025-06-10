// Copyright Adachi Makoto
// 異方性桑原フィルタ実行処理

#include "AnisotropicKuwahara.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "FXRenderingUtils.h"
#include "GlobalShader.h"
#include "ScenePrivate.h"

// "ScenePrivate.h"は、　FViewInfoのstatic_castのエラー解決に必要
//
// auto& inView = static_cast<const FViewInfo&>(View);
// 

//  GPU パフォーマンスの統計を記録・表示するためのマクロ
DECLARE_GPU_STAT(AnisotropicKuwaharaCS);



namespace {

class FAnisotropicKuwaharaEigenvectorCS : public FGlobalShader
{
public:
	static constexpr uint32 THREADGROUPSIZE_X = 16;
	static constexpr uint32 THREADGROUPSIZE_Y = 16;
	static constexpr uint32 THREADGROUPSIZE_Z = 1;

public:
	DECLARE_GLOBAL_SHADER(FAnisotropicKuwaharaEigenvectorCS);
	SHADER_USE_PARAMETER_STRUCT(FAnisotropicKuwaharaEigenvectorCS, FGlobalShader);
		
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, eigenvectorpass_SourceTexture)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, eigenvectorpass_OutputTexture)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		// return IsMobilePlatform(Parameters.Platform);
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) || IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), THREADGROUPSIZE_X);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), THREADGROUPSIZE_Y);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), THREADGROUPSIZE_Z);
	}
};

IMPLEMENT_GLOBAL_SHADER(FAnisotropicKuwaharaEigenvectorCS, "/Plugin/OutlineRenderPipeline/Private/AnisotropicKuwahara.usf", "AnisoKuwaharaEigenvectorCS", SF_Compute);


class FAnisoKuwaharaBlurCS : public FGlobalShader
{
public:
	static constexpr uint32 THREADGROUPSIZE_X = 16;
	static constexpr uint32 THREADGROUPSIZE_Y = 16;
	static constexpr uint32 THREADGROUPSIZE_Z = 1;

public:
	DECLARE_GLOBAL_SHADER(FAnisoKuwaharaBlurCS);
	SHADER_USE_PARAMETER_STRUCT(FAnisoKuwaharaBlurCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, blurpass_SourceTexture)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, blurpass_OutputTexture)
		SHADER_PARAMETER(FUintVector2, blurpass_OutputDimensions)
		SHADER_PARAMETER(int32, GaussRadius)
		SHADER_PARAMETER(float, GaussSigma)
	END_SHADER_PARAMETER_STRUCT()

	//Called by the engine to determine which permutations to compile for this shader
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		//return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) || IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}
	//Modifies the compilations environment of the shader
	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), THREADGROUPSIZE_X);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), THREADGROUPSIZE_Y);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), THREADGROUPSIZE_Z);
	}
};
	
IMPLEMENT_GLOBAL_SHADER(FAnisoKuwaharaBlurCS, "/Plugin/OutlineRenderPipeline/Private/AnisotropicKuwahara.usf", "AnisoKuwaharaBlurCS", SF_Compute );


class FAnisoKuwaharaCalcAnisoCS : public FGlobalShader
{
public:
	static constexpr uint32 THREADGROUPSIZE_X = 16;
	static constexpr uint32 THREADGROUPSIZE_Y = 16;
	static constexpr uint32 THREADGROUPSIZE_Z = 1;

public:
	DECLARE_GLOBAL_SHADER(FAnisoKuwaharaCalcAnisoCS);
	SHADER_USE_PARAMETER_STRUCT(FAnisoKuwaharaCalcAnisoCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, calcanisopass_SourceTexture)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, calcanisopass_OutputTexture)
	END_SHADER_PARAMETER_STRUCT()

	//Called by the engine to determine which permutations to compile for this shader
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		//return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) || IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}
	//Modifies the compilations environment of the shader
	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), THREADGROUPSIZE_X);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), THREADGROUPSIZE_Y);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), THREADGROUPSIZE_Z);
	}
};

IMPLEMENT_GLOBAL_SHADER(FAnisoKuwaharaCalcAnisoCS, "/Plugin/OutlineRenderPipeline/Private/AnisotropicKuwahara.usf", "AnisoKuwaharaCalcAnisoCS", SF_Compute );


class FLineIntegralConvolutionCS : public FGlobalShader
{
public:
	static constexpr uint32 THREADGROUPSIZE_X = 16;
	static constexpr uint32 THREADGROUPSIZE_Y = 16;
	static constexpr uint32 THREADGROUPSIZE_Z = 1;

public:
	DECLARE_GLOBAL_SHADER(FLineIntegralConvolutionCS);
	SHADER_USE_PARAMETER_STRUCT(FLineIntegralConvolutionCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, lineintegranpass_TFMTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, lineintegranpass_ColorTexture)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, lineintegranpass_OutputTexture)
		SHADER_PARAMETER(int32, lineintegranpass_GaussRadius)
		SHADER_PARAMETER(float, lineintegranpass_GaussSigma)
	END_SHADER_PARAMETER_STRUCT()

	//Called by the engine to determine which permutations to compile for this shader
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		//return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) || IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}
	//Modifies the compilations environment of the shader
	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), THREADGROUPSIZE_X);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), THREADGROUPSIZE_Y);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), THREADGROUPSIZE_Z);
	}
};

IMPLEMENT_GLOBAL_SHADER(FLineIntegralConvolutionCS, "/Plugin/OutlineRenderPipeline/Private/AnisotropicKuwahara.usf", "LineIntegralConvolutionCS", SF_Compute );

	
class FAnisoKuwaharaFinalCS : public FGlobalShader
{
public:
	static constexpr uint32 THREADGROUPSIZE_X = 16;
	static constexpr uint32 THREADGROUPSIZE_Y = 16;
	static constexpr uint32 THREADGROUPSIZE_Z = 1;

public:
	DECLARE_GLOBAL_SHADER(FAnisoKuwaharaFinalCS);
	SHADER_USE_PARAMETER_STRUCT(FAnisoKuwaharaFinalCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, finalpass_TFMTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, finalpass_SourceSampler)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, finalpass_SceneTexture)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, finalpass_OutputTexture)

		SHADER_PARAMETER(float, finalpass_aniso_control)	// 1
		SHADER_PARAMETER(float, finalpass_hardness)			// 8
		SHADER_PARAMETER(float, finalpass_sharpness)		// 8

		SHADER_PARAMETER(int32, KuwaharaRadius)
		SHADER_PARAMETER(int32, KuwaharaQ)
		SHADER_PARAMETER(float, KuwaharaAlpha)
	END_SHADER_PARAMETER_STRUCT()

	//Called by the engine to determine which permutations to compile for this shader
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		//return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) || IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}
	//Modifies the compilations environment of the shader
	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), THREADGROUPSIZE_X);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), THREADGROUPSIZE_Y);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), THREADGROUPSIZE_Z);
	}
};

IMPLEMENT_GLOBAL_SHADER(FAnisoKuwaharaFinalCS, "/Plugin/OutlineRenderPipeline/Private/AnisotropicKuwahara.usf", "AnisoKuwaharaFinalCS", SF_Compute );
	
}


//
// 呼び出し関数
//
void AnisotropicKuwaharaPass(FRDGBuilder& GraphBuilder, const FSceneView& View, const FAnisotropicKuwaharaCSInput& Inputs)
{
	const FIntRect PrimaryViewRect = UE::FXRenderingUtils::GetRawViewRectUnsafe(View);
	FUintVector2 WorkRect(PrimaryViewRect.Width(), PrimaryViewRect.Height());
	int width = PrimaryViewRect.Width();
	int height = PrimaryViewRect.Height();
	// TODO : 解像度を変更したい？ ⇒ 失敗。ビューポートがおかしくなるだけ
	// int width = FMath::CeilToInt(Inputs.AnisoKuwaharaResolutionScale * WorkRect.X);
	// int height = FMath::CeilToInt(Inputs.AnisoKuwaharaResolutionScale * WorkRect.Y);

	FRHISamplerState* PointClampSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();


	RDG_EVENT_SCOPE(GraphBuilder, "Naga_AnisoKuwaharaPass %dx%d", width, height);

	
	FRDGTextureRef tex_eigenvector = GraphBuilder.CreateTexture(
		FRDGTextureDesc::Create2D(FIntPoint(width, height),
			EPixelFormat::PF_FloatRGBA,
			{},
			ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::UAV
		), TEXT("AnisoKuwaharaEigen0"));
	
	FRDGTextureRef tex_eigenvector_blurwork = GraphBuilder.CreateTexture(
		FRDGTextureDesc::Create2D(
			tex_eigenvector->Desc.Extent,
			tex_eigenvector->Desc.Format,
			{},
			tex_eigenvector->Desc.Flags
		), TEXT("AnisoKuwaharaEigen1"));

	FRDGTextureRef tex_lineintegralconvolution_blurwork = GraphBuilder.CreateTexture(
		FRDGTextureDesc::Create2D(
			tex_eigenvector->Desc.Extent,
			tex_eigenvector->Desc.Format,
			{},
			tex_eigenvector->Desc.Flags
		), TEXT("AnisoKuwaharaEigen2"));
	
	FRDGTextureRef tex_aniso = GraphBuilder.CreateTexture(
		FRDGTextureDesc::Create2D(
			tex_eigenvector->Desc.Extent,
			tex_eigenvector->Desc.Format,
			{},
			tex_eigenvector->Desc.Flags
		), TEXT("AnisoKuwaharaAniso"));

	// Eigenvector.
	{
		FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(tex_eigenvector);
		FAnisotropicKuwaharaEigenvectorCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAnisotropicKuwaharaEigenvectorCS::FParameters>();
		{
			Parameters->eigenvectorpass_SourceTexture = Inputs.SceneTextures->GetParameters()->SceneColorTexture;
			Parameters->eigenvectorpass_OutputTexture = WorkUav;
		}
	
		TShaderMapRef<FAnisotropicKuwaharaEigenvectorCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(WorkRect.X, cs->THREADGROUPSIZE_X), FMath::DivideAndRoundUp(WorkRect.Y, cs->THREADGROUPSIZE_Y), 1);

		// TODO : 解像度の変更に対応する ⇒ 失敗。ビューポートがおかしくなるだけ
		// float fx = static_cast<float>(cs->THREADGROUPSIZE_X);
		// float fy = static_cast<float>(cs->THREADGROUPSIZE_Y);
		// int widthThread =  FMath::CeilToInt(width/fx);
		// int heightThread =  FMath::CeilToInt(height/fy);
		// int workx = WorkRect.X;
		// int worky = WorkRect.Y;
		// FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(workx, widthThread),FMath::DivideAndRoundUp(worky, heightThread), 1);

		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("AnisoKuwaharaEigenvector"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
	}

	// Blur.
	{
		FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(tex_eigenvector_blurwork);
		FAnisoKuwaharaBlurCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAnisoKuwaharaBlurCS::FParameters>();
		{
			// tex_eigenvectorは、固有ベクトルの結果を持つ
			Parameters->blurpass_SourceTexture = tex_eigenvector;
			Parameters->blurpass_OutputTexture = WorkUav;
			Parameters->blurpass_OutputDimensions = WorkRect;
			Parameters->GaussRadius = Inputs.AnisoKuwaharaGaussRadius;
			Parameters->GaussSigma = Inputs.AnisoKuwaharaGaussSigma;
			// UE_LOG(LogTemp, Log, TEXT("#### Inputs.AnisoKuwaharaGaussSigma=%f"), Inputs.AnisoKuwaharaGaussSigma);
			// UE_LOG(LogTemp, Log, TEXT("#### Inputs.AnisoKuwaharaGaussSigma=%f"), Inputs.AnisoKuwaharaAlpha);
		}
	
		TShaderMapRef<FAnisoKuwaharaBlurCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(WorkRect.X, cs->THREADGROUPSIZE_X), FMath::DivideAndRoundUp(WorkRect.Y, cs->THREADGROUPSIZE_Y), 1);

		// TODO : 解像度の変更に対応する ⇒ 失敗。ビューポートがおかしくなるだけ
		// float fx = static_cast<float>(cs->THREADGROUPSIZE_X);
		// float fy = static_cast<float>(cs->THREADGROUPSIZE_Y);
		// int widthThread =  FMath::CeilToInt(width/fx);
		// int heightThread =  FMath::CeilToInt(height/fy);
		// int workx = WorkRect.X;
		// int worky = WorkRect.Y;
		// FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(workx, widthThread),FMath::DivideAndRoundUp(worky, heightThread), 1);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("AnisoKuwaharaBlur"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
	}

	// CalcAniso.
	{
		FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(tex_aniso);
		FAnisoKuwaharaCalcAnisoCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAnisoKuwaharaCalcAnisoCS::FParameters>();
		{
			Parameters->calcanisopass_SourceTexture = tex_eigenvector_blurwork;
			Parameters->calcanisopass_OutputTexture = WorkUav;
		}
	
		TShaderMapRef<FAnisoKuwaharaCalcAnisoCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(WorkRect.X, cs->THREADGROUPSIZE_X), FMath::DivideAndRoundUp(WorkRect.Y, cs->THREADGROUPSIZE_Y), 1);

		// TODO : 解像度の変更に対応する ⇒ 失敗。ビューポートがおかしくなるだけ
		// float fx = static_cast<float>(cs->THREADGROUPSIZE_X);
		// float fy = static_cast<float>(cs->THREADGROUPSIZE_Y);
		// int widthThread =  FMath::CeilToInt(width/fx);
		// int heightThread =  FMath::CeilToInt(height/fy);
		// int workx = WorkRect.X;
		// int worky = WorkRect.Y;
		// FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(workx, widthThread),FMath::DivideAndRoundUp(worky, heightThread), 1);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("AnisoKuwaharaCalcAniso"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
	}
	
	FRDGTextureDesc TexSceneColorCopyDesc = {};
	{
		const auto scene_color_desc = Inputs.SceneTextures->GetParameters()->SceneColorTexture->Desc;
		
		TexSceneColorCopyDesc = FRDGTextureDesc::Create2D(
			scene_color_desc.Extent,
			scene_color_desc.Format, scene_color_desc.ClearValue,
			ETextureCreateFlags::ShaderResource|ETextureCreateFlags::RenderTargetable|ETextureCreateFlags::UAV
			);
	}

	FRDGTextureRef tex_scene_color_copy = GraphBuilder.CreateTexture(TexSceneColorCopyDesc, TEXT("NagaViewExtensionSceneColorCopy"));

	// Copy.
	{
		FRHICopyTextureInfo copy_info{};
		AddCopyTexturePass(GraphBuilder, Inputs.SceneTextures->GetParameters()->SceneColorTexture, tex_scene_color_copy, copy_info);
	}

	// LineIntegralConvolution
	{
		FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(Inputs.SceneTextures->GetParameters()->SceneColorTexture);
		// TODO : 元コード
		// FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(tex_lineintegralconvolution_blurwork);
		FLineIntegralConvolutionCS::FParameters* Parameters = GraphBuilder.AllocParameters<FLineIntegralConvolutionCS::FParameters>();
		{
			Parameters->lineintegranpass_TFMTexture = tex_aniso;
			Parameters->lineintegranpass_ColorTexture = tex_scene_color_copy;
			Parameters->lineintegranpass_OutputTexture = WorkUav;
			Parameters->lineintegranpass_GaussRadius = Inputs.AnisoKuwaharaGaussRadius;
			Parameters->lineintegranpass_GaussSigma = Inputs.AnisoKuwaharaGaussSigma;
		}
	
		TShaderMapRef<FLineIntegralConvolutionCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(WorkRect.X, cs->THREADGROUPSIZE_X), FMath::DivideAndRoundUp(WorkRect.Y, cs->THREADGROUPSIZE_Y), 1);

		// TODO : 解像度の変更に対応する ⇒ 失敗。ビューポートがおかしくなるだけ
		// float fx = static_cast<float>(cs->THREADGROUPSIZE_X);
		// float fy = static_cast<float>(cs->THREADGROUPSIZE_Y);
		// int widthThread =  FMath::CeilToInt(width/fx);
		// int heightThread =  FMath::CeilToInt(height/fy);
		// int workx = WorkRect.X;
		// int worky = WorkRect.Y;
		// FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(workx, widthThread),FMath::DivideAndRoundUp(worky, heightThread), 1);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("LineIntegralConvolution"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);		
	}
	/*
	// Final.
	{
		FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(Inputs.SceneTextures->GetParameters()->SceneColorTexture);
		FAnisoKuwaharaFinalCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAnisoKuwaharaFinalCS::FParameters>();
		{
			Parameters->finalpass_TFMTexture = tex_aniso;          // テンソル構造ベクトル
			Parameters->finalpass_SceneTexture = tex_lineintegralconvolution_blurwork;
			Parameters->finalpass_SourceSampler = PointClampSampler;  // 元のコピー画像そのまま
			// 元コード
			// Parameters->finalpass_SceneTexture = tex_scene_color_copy;
			Parameters->finalpass_OutputTexture = WorkUav;

			Parameters->finalpass_aniso_control = Inputs.aniso_kuwahara_aniso_control;
			Parameters->finalpass_hardness = Inputs.aniso_kuwahara_hardness;
			Parameters->finalpass_sharpness = Inputs.aniso_kuwahara_sharpness;

			Parameters->KuwaharaRadius = Inputs.AnisoKuwaharaRadius;
			Parameters->KuwaharaQ = Inputs.AnisoKuwaharaQ;
			Parameters->KuwaharaAlpha = Inputs.AnisoKuwaharaAlpha;
		}
	
		TShaderMapRef<FAnisoKuwaharaFinalCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(WorkRect.X, cs->THREADGROUPSIZE_X), FMath::DivideAndRoundUp(WorkRect.Y, cs->THREADGROUPSIZE_Y), 1);

		// TODO : 解像度の変更に対応する
		// float fx = static_cast<float>(cs->THREADGROUPSIZE_X);
		// float fy = static_cast<float>(cs->THREADGROUPSIZE_Y);
		// int widthThread =  FMath::CeilToInt(width/fx);
		// int heightThread =  FMath::CeilToInt(height/fy);
		// int workx = WorkRect.X;
		// int worky = WorkRect.Y;
		// FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(workx, widthThread),FMath::DivideAndRoundUp(worky, heightThread), 1);		
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("AnisoKuwaharaEigen"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
	}
	*/

}