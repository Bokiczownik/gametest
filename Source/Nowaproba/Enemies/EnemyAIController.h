#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class AEnemyCharacter;

/**
 * Brain for AEnemyCharacter, evaluated on a timer (UpdateInterval, no tick). Notices the player pawn within AggroRange
 * inside the SightHalfAngle cone with line of sight (or when damaged), chases it on the NavMesh, attacks inside the
 * attack range and gives up beyond LoseAggroRange. While not chasing it follows the enemy's Behaviour: Chase stands,
 * Patrol walks the route in a loop with a pause at each point, Guard returns to its post and faces the spawn direction
 * (and only engages while the player is within GuardRadius of the post), FogHunter needs no sight or range check, chases
 * while its spawner's IsInHuntArea allows it and otherwise walks back to its spawn point in the fog.
 * Stops when the enemy or the player is dead.
 */
UCLASS()
class NOWAPROBA_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(EditAnywhere, Category = "Enemy AI", meta = (ClampMin = "0.05"))
	float UpdateInterval = 0.2f;

	/** 2D distance at which a patrol point or the guard post counts as reached. */
	UPROPERTY(EditAnywhere, Category = "Enemy AI", meta = (ClampMin = "10"))
	float ArriveDistance = 100.f;

	/** One update while chasing an engaged, reachable player (not while attacking, blocking or staggered); Distance is 2D. Melee: close in and attack within the attack range. */
	virtual void UpdateCombat(APawn* Player, float Distance);

	/** Called when the chase starts or ends. */
	virtual void OnChaseChanged(bool bNowChasing) {}

	AEnemyCharacter* GetEnemy() const { return Enemy.Get(); }

private:
	TWeakObjectPtr<AEnemyCharacter> Enemy;
	TWeakObjectPtr<APawn> Target;
	FTimerHandle UpdateTimer;
	bool bChasing = false;
	bool bAlerted = false;
	float LastHealth = 0.f;
	int32 PatrolIndex = 0;
	double PatrolWaitEnd = -1.0;

	void UpdateBehaviour();
	bool CanSee(const APawn* Player) const;
	bool CanEngage(const APawn* Player) const;
	void SetChasing(bool bNewChasing);
	void UpdatePatrol();
	void ReturnHome();
	void StopChase();

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);
};
