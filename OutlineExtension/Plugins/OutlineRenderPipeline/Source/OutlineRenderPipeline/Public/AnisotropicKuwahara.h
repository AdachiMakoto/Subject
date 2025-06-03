#pragma once


#include "CoreMinimal.h"
#include "ScreenPass.h"


/*
 * PrePostProcessPass_RenderThreadで呼び出し
 */
struct FAnisotropicKuwaharaCSInput
{
	TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures = nullptr;
	FRDGTextureRef AKTarget;
	FRDGTextureRef AKInputTexture;
	FRDGTextureRef AKOutputTexture;
	float aniso_kuwahara_aniso_control	= 1.0f;
	float aniso_kuwahara_hardness		= 8.0f;
	float aniso_kuwahara_sharpness		= 8.0f;

	// Unity版異方性桑原フィルタの実装で追加するパラメータ
	int AnisoKuwaharaGaussRadius		= 5;
	float AnisoKuwaharaGaussSigma		= 8.0f;
	float AnisoKuwaharaAlpha			= 1.0f;
	int AnisoKuwaharaRadius				= 2;
	int AnisoKuwaharaQ					= 8;
	float AnisoKuwaharaResolutionScale	= 1.0f;
};

void AnisotropicKuwaharaPass(FRDGBuilder& GraphBuilder, const FSceneView& View, const FAnisotropicKuwaharaCSInput& Inputs);