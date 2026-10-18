#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyAttackComponent.generated.h"

class UPrimitiveComponent;

/**
 * Telegraphed enemy melee attack: StartAttack turns the owner to the target and shows the telegraph (hit area on the
 * ground) for WindupTime, then the strike applies engine damage (UGameplayStatics::ApplyDamage) to the target if it is
 * still inside the sphere of Radius at Reach in front of the owner, then the owner recovers for RecoveryTime.
 * No tick; timers only. The telegraph is scaled to Radius (engine Cylinder: 100 cm wide at scale 1).
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UEnemyAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyAttackComponent();

	/** Ignored while an attack is running or Target is null. */
	UFUNCTION(BlueprintCallable, Category = "Enemy Attack")
	void StartAttack(AActor* Target);

	/** Stops the current attack without hitting (e.g. on death). */
	UFUNCTION(BlueprintCallable, Category = "Enemy Attack")
	void CancelAttack();

	/** True from StartAttack until the recovery ends. */
	UFUNCTION(BlueprintPure, Category = "Enemy Attack")
	bool IsAttacking() const { return bAttacking; }

	/** Distance from the owner within which starting an attack can hit (Reach + Radius). */
	UFUNCTION(BlueprintPure, Category = "Enemy Attack")
	float GetAttackRange() const { return Reach + Radius; }

	/** Shown during the windup; positioned at Reach and scaled to Radius. */
	void SetTelegraph(UPrimitiveComponent* InTelegraph);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Attack", meta = (ClampMin = "0"))
	float Damage = 15.f;

	/** Distance from the owner to the hit sphere centre, along the owner's forward vector. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Attack", meta = (ClampMin = "0"))
	float Reach = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Attack", meta = (ClampMin = "1"))
	float Radius = 90.f;

	/** Telegraph time before the strike; the player's window to step out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Attack", meta = (ClampMin = "0.05"))
	float WindupTime = 0.8f;

	/** Pause after the strike before the next attack or move. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Attack", meta = (ClampMin = "0"))
	float RecoveryTime = 1.f;

	/** Telegraph height above the owner's feet (avoids z-fighting with the ground). */
	UPROPERTY(EditAnywhere, Category = "Enemy Attack")
	float TelegraphGroundOffset = 2.f;

private:
	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> Telegraph;

	TWeakObjectPtr<AActor> CurrentTarget;
	FTimerHandle AttackTimer;
	bool bAttacking = false;

	void Strike();
	void FinishAttack();
	void SetTelegraphVisible(bool bVisible);
};
