// Copyright 2024 kafues511. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OutlineSettings.h"
#include "OutlineControlConsoleActor.generated.h"

UCLASS()
class OUTLINERENDERPIPELINE_API AOutlineControlConsoleActor : public AActor
{
	GENERATED_BODY()
	
public:
	AOutlineControlConsoleActor();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

public:
	//~ Begin AActor interface
#if WITH_EDITOR
	virtual void RerunConstructionScripts() override;
	virtual void OnConstruction(const FTransform& Transform) override;
#endif
	//~ End AAcotr interface

public:
	/**  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline Settings")
	FOutlineSettings OutlineSettings;

	//
	// Anisotropic Kuwahara Filter
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AnisoKuwahara")
	bool enable_aniso_kuwahara = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AnisoKuwahara")
	float aniso_kuwahara_aniso_control = 1.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AnisoKuwahara")
	float aniso_kuwahara_hardness = 8.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AnisoKuwahara")
	float aniso_kuwahara_sharpness = 8.0f;

	//
	// Unity Anisotropic Kuwahara Filter
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="UnityAnisoKuwahara", meta = (ClampMin = "0", ClampMax = "10", UIMin = "0", UIMax = "10"))
	int unity_aniso_kuwahara_gauss_radius = 5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="UnityAnisoKuwahara", meta = (ClampMin = "0.1", ClampMax = "10.0", UIMin = "0.1", UIMax = "10.0"))
	float unity_aniso_kuwahara_gauss_sigma = 8.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="UnityAnisoKuwahara", meta = (ClampMin = "0.0", ClampMax = "10.0", UIMin = "0.0", UIMax = "10.0"))
	float unity_aniso_kuwahara_alpha = 1.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="UnityAnisoKuwahara", meta = (ClampMin = "0", ClampMax = "5", UIMin = "0", UIMax = "5"))
	int unity_aniso_kuwahara_radius = 2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="UnityAnisoKuwahara", meta = (ClampMin = "1", ClampMax = "20", UIMin = "1", UIMax = "20"))
	int unity_aniso_kuwahara_q = 8;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="UnityAnisoKuwahara", meta = (ClampMin = "0.1", ClampMax = "1.0", UIMin = "0.1", UIMax = "1.0"))
	float unity_aniso_kuwahara_resolution_scale = 1.0f;
};
