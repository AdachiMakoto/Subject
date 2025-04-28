#pragma once


#include "CoreMinimal.h"
#include "ScreenPass.h"


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