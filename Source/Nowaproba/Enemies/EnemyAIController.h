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
 * Damage or a group alert (same AggroGroup) alerts it and provokes it for ProvokedDuration (from the last hit / alert):
 * it then keeps chasing regardless of LoseAggroRange, GuardRadius and the hunt area. A chasing member that fights on its
 * own account (was hit, or the player is inside its own area and AggroRange) alerts its group every second, so the
 * group fights as long as one member does; members kept only by alerts do not renew them.
 * Stops when the enemy or the player is dead.
 */
UCLASS()
class NOWAPROBA_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	/** Group alert: notice the player on the next update (no sight check) and keep fighting for ProvokedDuration. */
	void Alert();

	bool IsChasing() const { return bChasing; }

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
	double ProvokedUntil = -1.0;
	double GroupAlertUntil = -1.0;
	double NextGroupAlertTime = 0.0;
	double FlankStartTime = -1.0;
	double FlankResumeTime = 0.0;

	void UpdateBehaviour();
	bool IsProvoked() const;
	void Provoke();
	void AlertGroup();
	bool IsEngagedOnOwn(const APawn* Player, float Distance) const;
	bool GetFlankPoint(const APawn* Player, float Distance, FVector& OutPoint);
	bool CanSee(const APawn* Player) const;
	bool CanEngage(const APawn* Player) const;
	bool CanEngageOnOwn(const APawn* Player) const;
	void SetChasing(bool bNewChasing);
	void UpdatePatrol();
	void ReturnHome();
	void StopChase();

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);
};
