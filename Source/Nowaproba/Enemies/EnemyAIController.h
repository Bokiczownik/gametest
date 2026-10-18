#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class AEnemyCharacter;

/**
 * Basic chase brain for AEnemyCharacter, evaluated on a timer (UpdateInterval, no tick): notices the player pawn within
 * AggroRange with line of sight, chases it on the NavMesh, attacks when inside the attack range and gives up beyond
 * LoseAggroRange. Stops when the enemy or the player is dead. Behaviour types (patrol, guard, ambush) come later.
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

private:
	TWeakObjectPtr<AEnemyCharacter> Enemy;
	TWeakObjectPtr<APawn> Target;
	FTimerHandle UpdateTimer;
	bool bChasing = false;

	void UpdateBehaviour();
	void StopChase();
};
