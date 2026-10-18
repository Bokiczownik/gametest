#include "FogHunterSpawner.h"

#include "Components/CapsuleComponent.h"
#include "EnemyCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "../Fog/MapFogVisualComponent.h"

AFogHunterSpawner::AFogHunterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Levels = { {1, 0.f}, {2, 300.f}, {3, 600.f}, {5, 1000.f}, {8, 1500.f} };
}

void AFogHunterSpawner::BeginPlay()
{
	Super::BeginPlay();

	FogVisual = Fog ? Fog->FindComponentByClass<UMapFogVisualComponent>() : nullptr;
	if (FogVisual && HunterClass)
	{
		GetWorldTimerManager().SetTimer(SpawnTimer, this, &AFogHunterSpawner::UpdateHunters, SpawnInterval, true);
	}
}

const FFogHunterLevel* AFogHunterSpawner::GetLevel() const
{
	if (!FogVisual || Levels.Num() == 0)
	{
		return nullptr;
	}
	return &Levels[FMath::Clamp(FogVisual->GetFogLevel(), 0, Levels.Num() - 1)];
}

bool AFogHunterSpawner::IsInHuntArea(const FVector& Location) const
{
	const FFogHunterLevel* Level = GetLevel();
	return Level && FogVisual->GetDistanceIntoFog(Location) >= -Level->HuntRange;
}

void AFogHunterSpawner::UpdateHunters()
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const FFogHunterLevel* Level = GetLevel();
	if (!Player || !Level)
	{
		return;
	}
	const FVector PlayerLocation = Player->GetActorLocation();

	// Dead hunters keep their corpse lifetime but no longer take a slot.
	for (int32 i = Hunters.Num() - 1; i >= 0; --i)
	{
		AEnemyCharacter* Hunter = Hunters[i].Get();
		if (!Hunter || Hunter->IsDead())
		{
			Hunters.RemoveAtSwap(i);
		}
		else if (FVector::Dist2D(Hunter->GetActorLocation(), PlayerLocation) > DespawnDistance && !IsInHuntArea(PlayerLocation))
		{
			Hunter->Destroy();
			Hunters.RemoveAtSwap(i);
		}
	}

	FVector SpawnLocation;
	if (Hunters.Num() >= Level->MaxHunters || !FindSpawnPoint(PlayerLocation, SpawnLocation))
	{
		return;
	}

	const float HalfHeight = HunterClass->GetDefaultObject<AEnemyCharacter>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector ToPlayer = PlayerLocation - SpawnLocation;
	ToPlayer.Z = 0.f;
	const FTransform Transform(ToPlayer.Rotation(), SpawnLocation + FVector(0.f, 0.f, HalfHeight));

	AEnemyCharacter* Hunter = GetWorld()->SpawnActorDeferred<AEnemyCharacter>(HunterClass, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
	if (!Hunter)
	{
		return;
	}
	Hunter->Behaviour = EEnemyBehaviour::FogHunter;
	Hunter->SetHunterSpawner(this);
	Hunter->FinishSpawning(Transform);
	if (IsValid(Hunter))
	{
		Hunters.Add(Hunter);
	}
}

bool AFogHunterSpawner::FindSpawnPoint(const FVector& PlayerLocation, FVector& OutLocation) const
{
	const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		return false;
	}

	for (int32 Attempt = 0; Attempt < SpawnAttempts; ++Attempt)
	{
		const float Angle = FMath::FRandRange(0.f, UE_TWO_PI);
		const float Distance = FMath::FRandRange(SpawnDistanceMin, SpawnDistanceMax);
		const FVector Candidate = PlayerLocation + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Distance;
		if (FogVisual->GetDistanceIntoFog(Candidate) < MinFogDepth)
		{
			continue;
		}

		FNavLocation NavLocation;
		if (NavSys->ProjectPointToNavigation(Candidate, NavLocation, FVector(200.f, 200.f, 1000.f))
			&& FogVisual->GetDistanceIntoFog(NavLocation.Location) >= MinFogDepth)
		{
			OutLocation = NavLocation.Location;
			return true;
		}
	}
	return false;
}
