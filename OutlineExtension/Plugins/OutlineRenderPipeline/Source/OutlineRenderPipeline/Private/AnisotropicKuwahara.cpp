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
		SHADER_PARAMETER(FUintVector2, eigenvectorpass_SourceDimensions)
		SHADER_PARAMETER_SAMPLER(SamplerState, eigenvectorpass_SourceSampler)
	
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, eigenvectorpass_OutputTexture)
		SHADER_PARAMETER(FUintVector2, eigenvectorpass_OutputDimensions)
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
		SHADER_PARAMETER(FUintVector2, blurpass_SourceDimensions)
		SHADER_PARAMETER_SAMPLER(SamplerState, blurpass_SourceSampler)

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, blurpass_OutputTexture)
		SHADER_PARAMETER(FUintVector2, blurpass_OutputDimensions)
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
		SHADER_PARAMETER(FUintVector2, calcanisopass_SourceDimensions)
		SHADER_PARAMETER_SAMPLER(SamplerState, calcanisopass_SourceSampler)

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, calcanisopass_OutputTexture)
		SHADER_PARAMETER(FUintVector2, calcanisopass_OutputDimensions)
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
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, finalpass_SourceTexture)
		SHADER_PARAMETER(FUintVector2, finalpass_SourceDimensions)
		SHADER_PARAMETER_SAMPLER(SamplerState, finalpass_SourceSampler)

		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, finalpass_SceneTexture)

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, finalpass_OutputTexture)
		SHADER_PARAMETER(FUintVector2, finalpass_OutputDimensions)

		SHADER_PARAMETER(float, finalpass_aniso_control)	// 1
		SHADER_PARAMETER(float, finalpass_hardness)			// 8
		SHADER_PARAMETER(float, finalpass_sharpness)		// 8

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

	FRHISamplerState* PointClampSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();


	RDG_EVENT_SCOPE(GraphBuilder, "Naga_AnisoKuwaharaPass %dx%d", PrimaryViewRect.Width(), PrimaryViewRect.Height());

	
	FRDGTextureRef tex_eigenvector = GraphBuilder.CreateTexture(
		FRDGTextureDesc::Create2D(
			FIntPoint(PrimaryViewRect.Width(), PrimaryViewRect.Height()),
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
			Parameters->eigenvectorpass_SourceDimensions = WorkRect;
			Parameters->eigenvectorpass_SourceSampler = PointClampSampler;

			Parameters->eigenvectorpass_OutputTexture = WorkUav;
			Parameters->eigenvectorpass_OutputDimensions = WorkRect;
		}
	
		TShaderMapRef<FAnisotropicKuwaharaEigenvectorCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));

		FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(WorkRect.X, cs->THREADGROUPSIZE_X),
				FMath::DivideAndRoundUp(WorkRect.Y, cs->THREADGROUPSIZE_Y), 1);
	
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("AnisoKuwaharaEigenvector"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
	}

	// Blur.
	{
		FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(tex_eigenvector_blurwork);
		FAnisoKuwaharaBlurCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAnisoKuwaharaBlurCS::FParameters>();
		{
			Parameters->blurpass_SourceTexture = tex_eigenvector;
			Parameters->blurpass_SourceDimensions = WorkRect;
			Parameters->blurpass_SourceSampler = PointClampSampler;

			Parameters->blurpass_OutputTexture = WorkUav;
			Parameters->blurpass_OutputDimensions = WorkRect;
		}
	
		TShaderMapRef<FAnisoKuwaharaBlurCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(WorkRect.X, cs->THREADGROUPSIZE_X),
				FMath::DivideAndRoundUp(WorkRect.Y, cs->THREADGROUPSIZE_Y), 1);
	
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("AnisoKuwaharaBlur"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
	}

	// CalcAniso.
	{
		FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(tex_aniso);
		FAnisoKuwaharaCalcAnisoCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAnisoKuwaharaCalcAnisoCS::FParameters>();
		{
			Parameters->calcanisopass_SourceTexture = tex_eigenvector_blurwork;
			Parameters->calcanisopass_SourceDimensions = WorkRect;
			Parameters->calcanisopass_SourceSampler = PointClampSampler;

			Parameters->calcanisopass_OutputTexture = WorkUav;
			Parameters->calcanisopass_OutputDimensions = WorkRect;
		}
	
		TShaderMapRef<FAnisoKuwaharaCalcAnisoCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));

		FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(WorkRect.X, cs->THREADGROUPSIZE_X),
				FMath::DivideAndRoundUp(WorkRect.Y, cs->THREADGROUPSIZE_Y), 1);
	
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("AnisoKuwaharaBlur"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
	}


	FRDGTextureDesc TexSceneColorCopyDesc = {};;
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
	
	// Final.
	{
		FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(Inputs.SceneTextures->GetParameters()->SceneColorTexture);
		FAnisoKuwaharaFinalCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAnisoKuwaharaFinalCS::FParameters>();
		{
			Parameters->finalpass_SourceTexture = tex_aniso;
			Parameters->finalpass_SourceDimensions = WorkRect;
			Parameters->finalpass_SourceSampler = PointClampSampler;

			Parameters->finalpass_SceneTexture = tex_scene_color_copy;

			Parameters->finalpass_OutputTexture = WorkUav;
			Parameters->finalpass_OutputDimensions = WorkRect;

			Parameters->finalpass_aniso_control = Inputs.aniso_kuwahara_aniso_control;
			Parameters->finalpass_hardness = Inputs.aniso_kuwahara_hardness;
			Parameters->finalpass_sharpness = Inputs.aniso_kuwahara_sharpness;
		}
	
		TShaderMapRef<FAnisoKuwaharaFinalCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));

		FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(WorkRect.X, cs->THREADGROUPSIZE_X),
				FMath::DivideAndRoundUp(WorkRect.Y, cs->THREADGROUPSIZE_Y), 1);
	
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("AnisoKuwaharaEigen"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
	}

	
	/*
	 * ただコピーするだけ
	 * 
	auto& inView = static_cast<const FViewInfo&>(View);
	FIntPoint Resolution = inView.ViewRect.Size();

	FScreenPassTextureViewport viewport = FScreenPassTextureViewport(inView.ViewRect);
	FRDGTextureDesc desc = FRDGTextureDesc::Create2D(Inputs.AKTarget->Desc.Extent, PF_A32B32G32R32F, FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV);
	FRDGTextureRef outputTexture = GraphBuilder.CreateTexture(desc, TEXT("AKEigenvectorCSTexture"));

	FGlobalShaderMap* shaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FAnisotropicKuwaharaEigenvectorCS> computeShader(shaderMap);

	FAnisotropicKuwaharaEigenvectorCS::FParameters* parameters = GraphBuilder.AllocParameters<FAnisotropicKuwaharaEigenvectorCS::FParameters>();
	// parameters->View = View.ViewUniformBuffer;
	// parameters->Input = GetScreenPassTextureViewportParameters(viewport);
	parameters->eigenvectorpass_SourceTexture = Inputs.AKInputTexture;
	parameters->eigenvectorpass_OutputTexture = GraphBuilder.CreateUAV(Inputs.AKTarget);
	
	// レンダーパスを追加
	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("AnisoKuwaharaEigenvectorCS"),
		computeShader,
		parameters,
		FComputeShaderUtils::GetGroupCount(viewport.Rect.Size(),
		FIntPoint(16, 16)));
	*/
}