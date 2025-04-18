#pragma once


#include "CoreMinimal.h"
#include "ScreenPass.h"


struct FAddMyShaderInput
{
	FRDGTextureRef Target;
	FRDGTextureRef SceneDepth;
	FRDGTextureRef InputTexture;
	FRDGTextureRef OutputTexture;
	TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures;
	FScreenPassTexture SceneColor;
};

void AddComputePass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FAddMyShaderInput& Inputs);
void AddPixelPass(FRDGBuilder& GraphBuilder, const FViewInfo& Vuew, const FAddMyShaderInput& Inputs);