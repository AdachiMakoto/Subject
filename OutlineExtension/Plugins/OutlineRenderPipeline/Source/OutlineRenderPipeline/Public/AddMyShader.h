#pragma once


#include "CoreMinimal.h"
#include "ScreenPass.h"


struct FAddMyShaderInput
{
	FRDGTextureRef Target;
	FRDGTextureRef SceneDepth;
	FRDGTextureRef LineTexture;
	FLinearColor LineColor;
	int32 LineWidth;
};

void AddPixelPass(FRDGBuilder& GraphBuilder, const FViewInfo& Vuew, const FAddMyShaderInput& Inputs);