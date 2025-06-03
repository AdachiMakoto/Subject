// Copyright 2024 kafues511. All Rights Reserved.

#include "OutlineControlConsoleActor.h"
#include "OutlineSubsystem.h"

AOutlineControlConsoleActor::AOutlineControlConsoleActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AOutlineControlConsoleActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (UOutlineSubsystem* Subsystem = UOutlineSubsystem::GetCurrent(GetWorld()))
	{
		Subsystem->OverrideOutlineSettings(OutlineSettings);
	}
}

void AOutlineControlConsoleActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

#if WITH_EDITOR
void AOutlineControlConsoleActor::RerunConstructionScripts()
{
	Super::RerunConstructionScripts();
}

void AOutlineControlConsoleActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (UOutlineSubsystem* Subsystem = UOutlineSubsystem::GetCurrent(GetWorld()))
	{
		Subsystem->OverrideOutlineSettings(OutlineSettings);

		Subsystem->EnableAnisoKuwahara = enable_aniso_kuwahara;
		Subsystem->AnisoKuwahara_AnisoControl = aniso_kuwahara_aniso_control;
		Subsystem->AnisoKuwahara_Hardness = aniso_kuwahara_hardness;
		Subsystem->AnisoKuwahara_Sharpness = aniso_kuwahara_sharpness;

		Subsystem->UnityAnisoKuwaharaGaussRadius = unity_aniso_kuwahara_gauss_radius;
		Subsystem->UnityAnisoKuwaharaGaussSigma = unity_aniso_kuwahara_gauss_sigma;
		Subsystem->UnityAnisoKuwaharaAlpha = unity_aniso_kuwahara_alpha;
		Subsystem->UnityAnisoKuwaharaRadius = unity_aniso_kuwahara_radius;
		Subsystem->UnityAnisoKuwaharaQ = unity_aniso_kuwahara_q;
		Subsystem->UnityAnisoKuwaharaResolutionScale = unity_aniso_kuwahara_resolution_scale;
	}
}
#endif
