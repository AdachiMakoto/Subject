#pragma once


#include "CoreMinimal.h"
#include "ScreenPass.h"


struct FAddMyShaderCSInput
{
	FRDGTextureRef Target;
	FRDGTextureRef InputTexture;
	FRDGTextureRef OutputTexture;
	// FRDGTextureRef OtherTexture;
	// TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures;
	FScreenPassTexture SceneColor;
	FIntPoint ViewRect;
	// FScreenPassTexture& InOutSceneColor;

	// FAddMyShaderCSInput() {}
};

struct FAddMyShaderPSInput
{
	FRDGTextureRef OutputTexture;
	TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures;
};

FRDGTextureRef AddComputePass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FAddMyShaderCSInput& Inputs);
void AddPixelPass(FRDGBuilder& GraphBuilder, const FViewInfo& Vuew, const FAddMyShaderPSInput& Inputs);