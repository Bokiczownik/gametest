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
		{ 0.5f, 35.f },
		{ 0.65f, 40.f },
		{ 0.8f, 45.f },
		{ 0.9f, 50.f },
		{ 1.f, 50.f },
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

bool UMapFogVisualComponent::IsInFog(const FVector& Location) const
{
	return bLookInitialized && FVector::DistSquared2D(Location, GetOwner()->GetActorLocation()) > FMath::Square(CurrentRadius);
}

float UMapFogVisualComponent::GetDistanceIntoFog(const FVector& Location) const
{
	return bLookInitialized ? FVector::Dist2D(Location, GetOwner()->GetActorLocation()) - CurrentRadius : -MAX_flt;
}

void UMapFogVisualComponent::SetFogState(float HalfExtent, int32 FogLevel)
{
	CurrentRadius = HalfExtent;
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
	CurrentLook.MaxHeight = FMath::Lerp(FromLook.MaxHeight, Target.MaxHeight, BlendAlpha);

	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	bool bPlayerInFog = false;
	if (Pawn)
	{
		PlayerLocation = Pawn->GetActorLocation();
		bPlayerInFog = IsInFog(PlayerLocation);
	}
	PlayerInFog = FMath::FInterpConstantTo(PlayerInFog, bPlayerInFog ? 1.f : 0.f, DeltaTime, 2.f);

	PushToCollection();
}

void UMapFogVisualComponent::PushToCollection() const
{
	const FVector Center = GetOwner()->GetActorLocation();
	const FLinearColor Area(Center.X, Center.Y, CurrentRadius, BaseZ);
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
	Instance->SetVectorParameterValue(TEXT("FogLook"), FLinearColor(CurrentLook.Opacity, CurrentLook.MaxHeight, 0.f, 0.f));
	Instance->SetVectorParameterValue(TEXT("FogColor"), FogColor);
	Instance->SetVectorParameterValue(TEXT("FogStyle"), FLinearColor(EdgeWidth, EdgeDensity, MistScale, MistSpeed));
	Instance->SetVectorParameterValue(TEXT("FogState"), FLinearColor(PlayerInFog, GrassHide, VignetteStrength, HazeStrength));
	Instance->SetVectorParameterValue(TEXT("FogPlayer"), FLinearColor(PlayerLocation.X, PlayerLocation.Y, PlayerLocation.Z, 0.f));
}

void UMapFogVisualComponent::HideGrassInFog()
{
	const FVector Center = GetOwner()->GetActorLocation();

	for (TActorIterator<AActor> It(GetWorld(), GrassAreaClass); It; ++It)
	{
		AActor* Area = *It;
		if (Area->IsHidden())
		{
			continue;
		}

		FVector Origin, Extent;
		Area->GetActorBounds(false, Origin, Extent);
		// Area box fully outside the safe circle = its closest point to the centre lies beyond the radius.
		const FVector Closest = FBox(Origin - Extent, Origin + Extent).GetClosestPointTo(Center);
		if (FVector::DistSquared2D(Closest, Center) > FMath::Square(CurrentRadius))
		{
			// The safe circle only shrinks, so hidden areas stay hidden.
			Area->SetActorHiddenInGame(true);
		}
	}
}
