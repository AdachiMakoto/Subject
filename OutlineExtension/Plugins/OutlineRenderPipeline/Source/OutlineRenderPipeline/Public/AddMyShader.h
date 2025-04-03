#pragma once


#include "CoreMinimal.h"
#include "ScreenPass.h"


struct FAddMyShaderInput
{
	FRDGTextureRef Target;
	FRDGTextureRef SceneDepth;
	FRDGTextureRef LineTexture;
	FRDGTextureRef OutputTexture;
	TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures;
	FScreenPassTexture SceneColor;
	FLinearColor LineColor;
	int32 LineWidth;
	// Test
	// FScreenPassTexture SceneColor;
	// FScreenPassRenderTarget OverrideOutput;
};

void AddPixelPass(FRDGBuilder& GraphBuilder, const FViewInfo& Vuew, const FAddMyShaderInput& Inputs);