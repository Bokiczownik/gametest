#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class UDataTable;
class UEnemyAttackComponent;
class UHealthComponent;
class UStaticMeshComponent;

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

	/** Notices the player within this distance (needs line of sight). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0"))
	float AggroRange = 1200.f;

	/** Gives up the chase beyond this distance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0"))
	float LoseAggroRange = 2000.f;

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
	UFUNCTION()
	void HandleDeath();
};
