#pragma once


#include "CoreMinimal.h"
#include "ScreenPass.h"


struct FAddMyShaderCSInput
{
	FRDGTextureRef Target;
	FRDGTextureRef InputTexture;
	FRDGTextureRef OutputTexture;
};

struct FAddMyShaderPSInput
{
	FRDGTextureRef OutputTexture;
	TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures;
};

FRDGTextureRef AddComputePass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FAddMyShaderCSInput& Inputs);
void AddPixelPass(FRDGBuilder& GraphBuilder, const FViewInfo& Vuew, const FAddMyShaderPSInput& Inputs);