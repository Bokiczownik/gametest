#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class AFogHunterSpawner;
class UDataTable;
class UEnemyAttackComponent;
class UHealthComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EEnemyBehaviour : uint8
{
	/** Stands still until it notices the player, then chases. */
	Chase,
	/** Walks PatrolPoints in a loop, chases when it notices the player, then resumes the route. */
	Patrol,
	/** Holds its spawn position; only fights while the player is within GuardRadius of it, then walks back. */
	Guard,
	/** Spawned in the fog by AFogHunterSpawner; knows where the player is and chases inside the spawner's hunt area, else walks back into the fog. */
	FogHunter
};

/**
 * Base enemy: Health + telegraphed melee Attack, driven by AEnemyAIController (auto-possessed when placed or spawned).
 * Detection/chase tuning lives here so each enemy BP can differ. On death: AI stops, the capsule stops blocking,
 * LootRow from LootTable is paid to the killer (UHealthComponent::GetLastDamageInstigator) and the corpse is removed
 * after CorpseLifetime. Mesh, anim BP and telegraph mesh/material are set in the BP child.
 */
UCLASS()
class NOWAPROBA_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

	UHealthComponent* GetHealth() const { return Health; }
	UEnemyAttackComponent* GetAttack() const { return Attack; }

	bool IsDead() const;

	/** Patrol points in world space (from PatrolPoints, fixed at BeginPlay). */
	const TArray<FVector>& GetPatrolRoute() const { return PatrolRoute; }

	/** Spawn location and facing; the guard's post. */
	const FVector& GetHomeLocation() const { return HomeLocation; }
	const FRotator& GetHomeRotation() const { return HomeRotation; }

	/** Set by AFogHunterSpawner before FinishSpawning. */
	void SetHunterSpawner(AFogHunterSpawner* InSpawner);
	AFogHunterSpawner* GetHunterSpawner() const;

	/** Uses ChaseSpeed while chasing, else PatrolWalkSpeed. */
	void SetChaseSpeed(bool bChasing);

	/** Fog hunter chase: ChaseSpeed within HunterNearDistance of the player, HunterFarSpeed farther away. */
	void UpdateHunterChaseSpeed(float DistanceToPlayer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	EEnemyBehaviour Behaviour = EEnemyBehaviour::Chase;

	/** Notices the player within this distance (needs line of sight). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0"))
	float AggroRange = 1200.f;

	/** Gives up the chase beyond this distance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0"))
	float LoseAggroRange = 2000.f;

	/** Half angle of the view cone for noticing the player (180 = all around). Taking damage always alerts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "1", ClampMax = "180"))
	float SightHalfAngle = 180.f;

	/** Route relative to the actor (drag the diamonds in the viewport); walked in a loop. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Patrol", meta = (MakeEditWidget, EditCondition = "Behaviour == EEnemyBehaviour::Patrol"))
	TArray<FVector> PatrolPoints;

	/** Pause at each patrol point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Patrol", meta = (ClampMin = "0"))
	float PatrolWaitTime = 1.5f;

	/** Walk speed while patrolling or returning to the post (chasing uses the movement component's MaxWalkSpeed). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0"))
	float PatrolWalkSpeed = 200.f;

	/** Fog hunter chase speed while farther than HunterNearDistance from the player. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Fog Hunter", meta = (ClampMin = "0"))
	float HunterFarSpeed = 250.f;

	/** Within this distance of the player a fog hunter chases at full speed (the movement component's MaxWalkSpeed). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Fog Hunter", meta = (ClampMin = "0"))
	float HunterNearDistance = 4000.f;

	/** Guard fights only while the player is within this distance of its post. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Guard", meta = (ClampMin = "0", EditCondition = "Behaviour == EEnemyBehaviour::Guard"))
	float GuardRadius = 800.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UEnemyAttackComponent> Attack;

	/** Hit area shown during the attack windup (mesh/material in BP; hidden in game until an attack starts). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UStaticMeshComponent> AttackTelegraph;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Loot")
	TObjectPtr<UDataTable> LootTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Loot")
	FName LootRow = TEXT("Enemy");

	/** Seconds the body stays after death. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0.1"))
	float CorpseLifetime = 5.f;

private:
	TArray<FVector> PatrolRoute;
	FVector HomeLocation = FVector::ZeroVector;
	FRotator HomeRotation = FRotator::ZeroRotator;
	float ChaseSpeed = 0.f;
	TWeakObjectPtr<AFogHunterSpawner> HunterSpawner;

	UFUNCTION()
	void HandleDeath();
};
