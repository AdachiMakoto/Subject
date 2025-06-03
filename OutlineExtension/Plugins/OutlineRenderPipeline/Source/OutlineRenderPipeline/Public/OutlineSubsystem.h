// Copyright 2024 kafues511. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OutlineSettings.h"
#include "OutlineSubsystem.generated.h"

class FOutlineViewExtension;

/**
 * 
 */
UCLASS()
class OUTLINERENDERPIPELINE_API UOutlineSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	/**  */
	static UOutlineSubsystem* GetCurrent(const UWorld* World);

	/**  */
	UFUNCTION(BlueprintCallable, Category = "Outline Settings")
	const FOutlineSettings& GetOutlineSettings() const;

	/**  */
	UFUNCTION(BlueprintCallable, Category = "Outline Settings")
	void SetOutlineSettings(const FOutlineSettings NewValue);

	/**  */
	UFUNCTION(BlueprintCallable, Category = "Outline Settings")
	void OverrideOutlineSettings(const FOutlineSettings& NewValue);

public:
	/**  */
	const FOutlineSettings GetOutlineSettingsForLock() const;

	/** AnosptropicKuwaharaFilter */
	bool EnableAnisoKuwahara = false;
	float AnisoKuwahara_AnisoControl = 1.0f;
	float AnisoKuwahara_Hardness = 8.0f;
	float AnisoKuwahara_Sharpness = 8.0f;

	/** AnosptropicKuwaharaFilter */
	int UnityAnisoKuwaharaGaussRadius = 5;
	float UnityAnisoKuwaharaGaussSigma = 8.0f;
	float UnityAnisoKuwaharaAlpha = 1.0f;
	int UnityAnisoKuwaharaRadius = 2;
	int UnityAnisoKuwaharaQ = 8;
	float UnityAnisoKuwaharaResolutionScale = 1.0f;
	
private:
	/**  */
	mutable FCriticalSection CriticalSection;

	/**  */
	TSharedPtr<FOutlineViewExtension, ESPMode::ThreadSafe> ViewExtension;

	/**  */
	FOutlineSettings OutlineSettings;
};
