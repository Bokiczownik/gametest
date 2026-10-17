#include "LootSpawner.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "LootChest.h"
#include "LootLibrary.h"
#include "LootSpot.h"
#include "TimerManager.h"

namespace
{
	/** Removes and returns a random spot, or nullptr when none are left. */
	ALootSpot* TakeRandom(TArray<ALootSpot*>& Spots)
	{
		if (Spots.Num() == 0)
		{
			return nullptr;
		}
		const int32 Index = FMath::RandRange(0, Spots.Num() - 1);
		ALootSpot* Spot = Spots[Index];
		Spots.RemoveAtSwap(Index);
		return Spot;
	}
}

ALootSpawner::ALootSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ALootSpawner::BeginPlay()
{
	Super::BeginPlay();

	TMap<ELootSpotType, TArray<ALootSpot*>> Spots;
	for (ALootSpot* Spot : TActorRange<ALootSpot>(GetWorld()))
	{
		Spots.FindOrAdd(Spot->SpotType).Add(Spot);
	}

	TArray<ALootSpot*>& ChestSpots = Spots.FindOrAdd(ELootSpotType::Chest);
	auto SpawnChest = [this](ALootSpot* Spot, FName Row) -> ALootChest*
	{
		ALootChest* Chest = Cast<ALootChest>(SpawnAtSpot(ChestClass, Spot));
		if (Chest)
		{
			Chest->SetLootRow(Row);
		}
		return Chest;
	};

	for (int32 i = 0; i < GuaranteedChestCount; ++i)
	{
		SpawnChest(TakeRandom(ChestSpots), ULootLibrary::PickWeightedRow(LootTable, GuaranteedChestRows));
	}
	for (int32 i = 0; i < ChestCount; ++i)
	{
		SpawnChest(TakeRandom(ChestSpots), ULootLibrary::PickWeightedRow(LootTable, {}));
	}

	TArray<ALootSpot*>& FogSpots = Spots.FindOrAdd(ELootSpotType::FogChest);
	for (int32 i = 0; i < FogChestCount; ++i)
	{
		if (ALootChest* Chest = SpawnChest(TakeRandom(FogSpots), FogChestRow))
		{
			Chest->SetRevealed(false);
			HiddenFogChests.Add(Chest);
		}
	}

	TArray<ALootSpot*>& BarrelSpots = Spots.FindOrAdd(ELootSpotType::Barrel);
	const int32 Barrels = FMath::RandRange(FMath::Min(BarrelCountMin, BarrelCountMax), FMath::Max(BarrelCountMin, BarrelCountMax));
	for (int32 i = 0; i < Barrels; ++i)
	{
		SpawnAtSpot(BarrelClass, TakeRandom(BarrelSpots));
	}

	TArray<ALootSpot*>& NodeSpots = Spots.FindOrAdd(ELootSpotType::ResourceNode);
	for (const TSubclassOf<AActor>& NodeClass : ResourceNodeClasses)
	{
		const int32 Nodes = FMath::RandRange(FMath::Min(NodesPerTypeMin, NodesPerTypeMax), FMath::Max(NodesPerTypeMin, NodesPerTypeMax));
		for (int32 i = 0; i < Nodes; ++i)
		{
			SpawnAtSpot(NodeClass, TakeRandom(NodeSpots));
		}
	}

	if (HiddenFogChests.Num() > 0)
	{
		CheckFogChests();
		GetWorldTimerManager().SetTimer(FogCheckTimer, this, &ALootSpawner::CheckFogChests, FogCheckInterval, true);
	}
}

AActor* ALootSpawner::SpawnAtSpot(TSubclassOf<AActor> Class, const ALootSpot* Spot)
{
	if (!Class || !Spot)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return GetWorld()->SpawnActor<AActor>(Class, Spot->GetActorTransform(), Params);
}

void ALootSpawner::CheckFogChests()
{
	FVector Center = FVector::ZeroVector;
	FVector Extent = FVector(UE_BIG_NUMBER);
	if (Fog)
	{
		// Bounds of all components = the square of fog walls.
		Fog->GetActorBounds(false, Center, Extent);
	}

	for (int32 i = HiddenFogChests.Num() - 1; i >= 0; --i)
	{
		ALootChest* Chest = HiddenFogChests[i].Get();
		if (!Chest)
		{
			HiddenFogChests.RemoveAtSwap(i);
			continue;
		}

		const FVector Offset = Chest->GetActorLocation() - Center;
		const bool bInFog = !Fog || FMath::Abs(Offset.X) > Extent.X || FMath::Abs(Offset.Y) > Extent.Y;
		if (bInFog)
		{
			Chest->SetRevealed(true);
			HiddenFogChests.RemoveAtSwap(i);
		}
	}

	if (HiddenFogChests.Num() == 0)
	{
		GetWorldTimerManager().ClearTimer(FogCheckTimer);
	}
}
