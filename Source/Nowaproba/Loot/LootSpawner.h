#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LootSpawner.generated.h"

class ALootChest;
class ALootSpot;
class UDataTable;

/**
 * One per map. On BeginPlay picks random ALootSpots of each type and spawns the map's chests, barrels and
 * resource nodes (counts below; missing spots just mean fewer spawns). Fog chests stay hidden until the Fog
 * actor's bounds (the fog square) no longer contain them, checked every FogCheckInterval seconds.
 */
UCLASS()
class NOWAPROBA_API ALootSpawner : public AActor
{
	GENERATED_BODY()

public:
	ALootSpawner();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Loot")
	TObjectPtr<UDataTable> LootTable;

	UPROPERTY(EditAnywhere, Category = "Loot")
	TSubclassOf<ALootChest> ChestClass;

	UPROPERTY(EditAnywhere, Category = "Loot")
	TSubclassOf<AActor> BarrelClass;

	/** One class per resource colour; each gets NodesPerTypeMin..Max spawns. */
	UPROPERTY(EditAnywhere, Category = "Loot")
	TArray<TSubclassOf<AActor>> ResourceNodeClasses;

	/** Random chests: row picked by SpawnWeight over all rows. */
	UPROPERTY(EditAnywhere, Category = "Loot|Counts", meta = (ClampMin = "0"))
	int32 ChestCount = 10;

	/** Guaranteed rarer chests: row picked by SpawnWeight among GuaranteedChestRows. Use the same Chest spots. */
	UPROPERTY(EditAnywhere, Category = "Loot|Counts", meta = (ClampMin = "0"))
	int32 GuaranteedChestCount = 2;

	UPROPERTY(EditAnywhere, Category = "Loot|Counts")
	TArray<FName> GuaranteedChestRows = { TEXT("Chest_Rare"), TEXT("Chest_Epic") };

	UPROPERTY(EditAnywhere, Category = "Loot|Counts", meta = (ClampMin = "0"))
	int32 FogChestCount = 5;

	UPROPERTY(EditAnywhere, Category = "Loot|Counts")
	FName FogChestRow = TEXT("Chest_Fog");

	UPROPERTY(EditAnywhere, Category = "Loot|Counts", meta = (ClampMin = "0"))
	int32 BarrelCountMin = 70;

	UPROPERTY(EditAnywhere, Category = "Loot|Counts", meta = (ClampMin = "0"))
	int32 BarrelCountMax = 90;

	UPROPERTY(EditAnywhere, Category = "Loot|Counts", meta = (ClampMin = "0"))
	int32 NodesPerTypeMin = 1;

	UPROPERTY(EditAnywhere, Category = "Loot|Counts", meta = (ClampMin = "0"))
	int32 NodesPerTypeMax = 3;

	/** Fog actor (BP_MapFog) whose bounds are the safe square. Without it fog chests appear at once. */
	UPROPERTY(EditAnywhere, Category = "Loot|Fog")
	TObjectPtr<AActor> Fog;

	UPROPERTY(EditAnywhere, Category = "Loot|Fog", meta = (ClampMin = "0.1"))
	float FogCheckInterval = 1.f;

private:
	TArray<TWeakObjectPtr<ALootChest>> HiddenFogChests;
	FTimerHandle FogCheckTimer;

	void CheckFogChests();
	AActor* SpawnAtSpot(TSubclassOf<AActor> Class, const ALootSpot* Spot);
};
