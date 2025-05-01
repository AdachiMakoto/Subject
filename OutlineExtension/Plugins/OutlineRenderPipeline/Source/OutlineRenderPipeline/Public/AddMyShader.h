#pragma once


#include "CoreMinimal.h"
#include "ScreenPass.h"


/*
 * PrePostProcessPass_RenderThreadで呼び出し
 */
struct FAddMyShaderCSInput
{
	FRDGTextureRef Target;
	FRDGTextureRef InputTexture;
	FRDGTextureRef OutputTexture;
};

struct FCopyShaderCSInput
{
	FRDGTextureRef Target;
	FRDGTextureRef InputTexture;
};

struct FAddMyShaderPSInput
{
	FRDGTextureRef Target;
	FRDGTextureRef OutputTexture;
	TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures;
};

FRDGTextureRef AddComputePass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FAddMyShaderCSInput& Inputs);
void CopyComputePass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FCopyShaderCSInput& Inputs);
void AddPixelPass(FRDGBuilder& GraphBuilder, const FViewInfo& Vuew, const FAddMyShaderPSInput& Inputs);

/*
 * PostRenderBasePassDeferred_RenderThreadで呼び出し
 */
struct FPostAddMyShaderCSInput
{
	FRDGTextureRef Target;
	FRDGTextureRef InputTexture;
};

struct FPostAddMyShaderPSInput
{
	FRDGTextureRef Target;
	FRDGTextureRef InputTexture;
	TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures;
};

void PostAddMyCS(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FPostAddMyShaderCSInput& Inputs);
void PostAddMyPS(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FPostAddMyShaderPSInput& Inputs);