#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FogHunterSpawner.generated.h"

class AEnemyCharacter;
class UMapFogVisualComponent;

/** Fog hunter limits at one fog level. */
USTRUCT(BlueprintType)
struct NOWAPROBA_API FFogHunterLevel
{
	GENERATED_BODY()

	/** Hunters alive at the same time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog Hunter", meta = (ClampMin = "0"))
	int32 MaxHunters = 1;

	/** How far (cm) out of the fog a hunter still chases the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog Hunter", meta = (ClampMin = "0"))
	float HuntRange = 0.f;
};

/**
 * One per map. Every SpawnInterval it removes dead or far-away idle hunters and, while fewer than the current level's
 * MaxHunters are alive, spawns one HunterClass enemy (Behaviour FogHunter) at a NavMesh point in the fog
 * SpawnDistanceMin..Max from the player and at least MinFogDepth past the safe circle's edge. Hunters know where the
 * player is and chase while the player is in the fog or less than the level's HuntRange outside it (IsInHuntArea),
 * otherwise they walk back into the fog. Fog level and circle come from Fog's UMapFogVisualComponent.
 */
UCLASS()
class NOWAPROBA_API AFogHunterSpawner : public AActor
{
	GENERATED_BODY()

public:
	AFogHunterSpawner();

	/** True if a hunter may chase a player standing at Location (in the fog or within the level's HuntRange of it). */
	bool IsInHuntArea(const FVector& Location) const;

protected:
	virtual void BeginPlay() override;

	/** Fog actor (BP_MapFog) with a UMapFogVisualComponent. Nothing spawns without it. */
	UPROPERTY(EditAnywhere, Category = "Fog Hunter")
	TObjectPtr<AActor> Fog;

	UPROPERTY(EditAnywhere, Category = "Fog Hunter")
	TSubclassOf<AEnemyCharacter> HunterClass;

	/** One entry per fog level (index = fog level); the last entry is used for higher levels. */
	UPROPERTY(EditAnywhere, Category = "Fog Hunter")
	TArray<FFogHunterLevel> Levels;

	/** Seconds between spawn/cleanup checks (at most one spawn per check). */
	UPROPERTY(EditAnywhere, Category = "Fog Hunter", meta = (ClampMin = "0.2"))
	float SpawnInterval = 3.f;

	UPROPERTY(EditAnywhere, Category = "Fog Hunter", meta = (ClampMin = "0"))
	float SpawnDistanceMin = 3000.f;

	UPROPERTY(EditAnywhere, Category = "Fog Hunter", meta = (ClampMin = "0"))
	float SpawnDistanceMax = 4500.f;

	/** Spawn points lie at least this far past the safe circle's edge. */
	UPROPERTY(EditAnywhere, Category = "Fog Hunter", meta = (ClampMin = "0"))
	float MinFogDepth = 300.f;

	/** Hunters not chasing and farther than this from the player are removed (frees the slot). */
	UPROPERTY(EditAnywhere, Category = "Fog Hunter", meta = (ClampMin = "0"))
	float DespawnDistance = 7000.f;

	/** Random points tried per spawn before giving up until the next check. */
	UPROPERTY(EditAnywhere, Category = "Fog Hunter", meta = (ClampMin = "1"))
	int32 SpawnAttempts = 12;

private:
	UPROPERTY()
	TObjectPtr<UMapFogVisualComponent> FogVisual;

	TArray<TWeakObjectPtr<AEnemyCharacter>> Hunters;
	FTimerHandle SpawnTimer;

	const FFogHunterLevel* GetLevel() const;
	void UpdateHunters();
	bool FindSpawnPoint(const FVector& PlayerLocation, FVector& OutLocation) const;
};
