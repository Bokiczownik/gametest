#include "MapFogVisualComponent.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "TimerManager.h"

UMapFogVisualComponent::UMapFogVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	LevelLooks = {
		{ 0.25f, 0.f, 300.f, 1.f },
		{ 0.4f, 10.f, 300.f, 1.f },
		{ 0.6f, 20.f, 280.f, 0.8f },
		{ 0.8f, 35.f, 250.f, 0.6f },
		{ 1.f, 50.f, 200.f, 0.4f },
	};
}

void UMapFogVisualComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentLook = LookForLevel(0);
	FromLook = CurrentLook;
}

const FFogLevelLook& UMapFogVisualComponent::LookForLevel(int32 Level) const
{
	static const FFogLevelLook Default;
	if (LevelLooks.Num() == 0)
	{
		return Default;
	}
	return LevelLooks[FMath::Clamp(Level, 0, LevelLooks.Num() - 1)];
}

void UMapFogVisualComponent::SetFogState(float HalfExtent, int32 FogLevel)
{
	CurrentHalfExtent = HalfExtent;
	if (!bLookInitialized)
	{
		bLookInitialized = true;
		CurrentLevel = FogLevel;
		CurrentLook = LookForLevel(FogLevel);
		FromLook = CurrentLook;
	}

	if (FogLevel != CurrentLevel)
	{
		CurrentLevel = FogLevel;
		FromLook = CurrentLook;
		BlendAlpha = 0.f;
		if (GrassAreaClass && CurrentLevel >= GrassHideLevel && !GrassTimer.IsValid())
		{
			HideGrassInFog();
			GetWorld()->GetTimerManager().SetTimer(GrassTimer, this, &UMapFogVisualComponent::HideGrassInFog, 1.f, true);
		}
	}
}

void UMapFogVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bLookInitialized)
	{
		return;
	}

	// Blend from the look at the moment of the level change to the new level's look over LevelBlendTime.
	BlendAlpha = FMath::Min(BlendAlpha + DeltaTime / LevelBlendTime, 1.f);
	const FFogLevelLook& Target = LookForLevel(CurrentLevel);
	CurrentLook.Opacity = FMath::Lerp(FromLook.Opacity, Target.Opacity, BlendAlpha);
	CurrentLook.FullHeight = FMath::Lerp(FromLook.FullHeight, Target.FullHeight, BlendAlpha);
	CurrentLook.FadeHeight = FMath::Lerp(FromLook.FadeHeight, Target.FadeHeight, BlendAlpha);
	CurrentLook.CloudStrength = FMath::Lerp(FromLook.CloudStrength, Target.CloudStrength, BlendAlpha);

	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	bool bPlayerInFog = false;
	if (Pawn)
	{
		const FVector Offset = Pawn->GetActorLocation() - GetOwner()->GetActorLocation();
		bPlayerInFog = FMath::Max(FMath::Abs(Offset.X), FMath::Abs(Offset.Y)) > CurrentHalfExtent;
	}
	PlayerInFog = FMath::FInterpConstantTo(PlayerInFog, bPlayerInFog ? 1.f : 0.f, DeltaTime, 2.f);

	PushToCollection();
}

void UMapFogVisualComponent::PushToCollection() const
{
	const FVector Center = GetOwner()->GetActorLocation();
	const FLinearColor Area(Center.X, Center.Y, CurrentHalfExtent, BaseZ);
	const float GrassHide = CurrentLevel >= GrassHideLevel ? 1.f : 0.f;

	if (UMaterialParameterCollectionInstance* Grass = GrassCollection ? GetWorld()->GetParameterCollectionInstance(GrassCollection) : nullptr)
	{
		Grass->SetVectorParameterValue(TEXT("MapFogArea"), Area);
		Grass->SetVectorParameterValue(TEXT("MapFogState"), FLinearColor(0.f, GrassHide, 0.f, 0.f));
	}

	UMaterialParameterCollectionInstance* Instance = Collection ? GetWorld()->GetParameterCollectionInstance(Collection) : nullptr;
	if (!Instance)
	{
		return;
	}

	Instance->SetVectorParameterValue(TEXT("FogArea"), Area);
	Instance->SetVectorParameterValue(TEXT("FogLook"), FLinearColor(CurrentLook.Opacity, CurrentLook.FullHeight, CurrentLook.FadeHeight, CurrentLook.CloudStrength));
	Instance->SetVectorParameterValue(TEXT("FogColor"), FogColor);
	Instance->SetVectorParameterValue(TEXT("FogStyle"), FLinearColor(EdgeWidth, Steps, CloudScale, CloudSpeed));
	Instance->SetVectorParameterValue(TEXT("FogState"), FLinearColor(PlayerInFog, GrassHide, VignetteStrength, 0.f));
}

void UMapFogVisualComponent::HideGrassInFog()
{
	const FVector Center = GetOwner()->GetActorLocation();
	const FBox Safe(FVector(Center.X - CurrentHalfExtent, Center.Y - CurrentHalfExtent, -UE_BIG_NUMBER),
		FVector(Center.X + CurrentHalfExtent, Center.Y + CurrentHalfExtent, UE_BIG_NUMBER));

	for (TActorIterator<AActor> It(GetWorld(), GrassAreaClass); It; ++It)
	{
		AActor* Area = *It;
		if (Area->IsHidden())
		{
			continue;
		}

		FVector Origin, Extent;
		Area->GetActorBounds(false, Origin, Extent);
		if (!Safe.Intersect(FBox(Origin - Extent, Origin + Extent)))
		{
			// The safe square only shrinks, so hidden areas stay hidden.
			Area->SetActorHiddenInGame(true);
		}
	}
}
