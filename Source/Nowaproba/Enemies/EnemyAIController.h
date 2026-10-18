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
 * (and only engages while the player is within GuardRadius of the post). Stops when the enemy or the player is dead.
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
	void UpdateGuard();
	void StopChase();

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);
};
