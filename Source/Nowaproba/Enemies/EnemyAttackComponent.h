#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyAttackComponent.generated.h"

class UAnimSequenceBase;
class UPrimitiveComponent;

/** One telegraphed enemy melee attack. */
USTRUCT(BlueprintType)
struct FEnemyAttack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float Damage = 15.f;

	/** Poise damage to the target (staggers it at 0, see UHitReactionComponent). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float PoiseDamage = 20.f;

	/** Horizontal launch speed given to the target on hit (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float Knockback = 300.f;

	/** Distance from the owner to the hit sphere centre, along the owner's forward vector. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float Reach = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "1"))
	float Radius = 90.f;

	/** Telegraph time before the strike; the player's window to step out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.05"))
	float WindupTime = 0.8f;

	/** Pause after the strike before the next attack or move (the player's punish window). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float RecoveryTime = 1.f;

	/** Relative chance among the attacks that can reach the target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float Weight = 1.f;

	/** Optional, played on DefaultSlot so that its StrikeAnimTime lines up with the end of the windup. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Animation")
	TObjectPtr<UAnimSequenceBase> Animation;

	/** Animation time of the hit frame (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Animation", meta = (ClampMin = "0"))
	float StrikeAnimTime = 0.f;

	float GetRange() const { return Reach + Radius; }
};

/**
 * Telegraphed enemy melee attacks: StartAttack picks one of Attacks that can reach the target (weighted random), turns
 * the owner to the target, plays its animation and shows the telegraph (hit area on the ground, sized to the attack)
 * for WindupTime; the strike then hits the target if it is still inside the sphere of Radius at Reach in front of the
 * owner: engine damage (UGameplayStatics::ApplyDamage), PoiseDamage (UCombatAttributeSet) and knockback
 * (UHitReactionComponent); then the owner recovers for RecoveryTime. Timers drive the attack; with bTrackTarget the
 * component ticks during the windup only, turning the owner (and the telegraph) toward the target at TrackTurnRate.
 * The telegraph is scaled to Radius (engine Cylinder: 100 cm wide at scale 1).
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UEnemyAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyAttackComponent();

	/** Ignored while an attack is running, Target is null or no attack can reach it. */
	UFUNCTION(BlueprintCallable, Category = "Enemy Attack")
	void StartAttack(AActor* Target);

	/** Stops the current attack without hitting (e.g. on death or stagger). */
	UFUNCTION(BlueprintCallable, Category = "Enemy Attack")
	void CancelAttack();

	/** True from StartAttack until the recovery ends. */
	UFUNCTION(BlueprintPure, Category = "Enemy Attack")
	bool IsAttacking() const { return bAttacking; }

	/** True while the owner must stay in place: windup and strike, and the recovery unless bMoveDuringRecovery. */
	UFUNCTION(BlueprintPure, Category = "Enemy Attack")
	bool IsHoldingPosition() const { return bAttacking && !(bRecovering && bMoveDuringRecovery); }

	/** Distance from the owner within which some attack can hit (largest Reach + Radius). */
	UFUNCTION(BlueprintPure, Category = "Enemy Attack")
	float GetAttackRange() const;

	/** Shown during the windup; positioned and scaled per attack. */
	void SetTelegraph(UPrimitiveComponent* InTelegraph);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Attack")
	TArray<FEnemyAttack> Attacks;

	/** Keep turning toward the target during the windup (the owner stays in place); off = the facing is committed at the start. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Attack")
	bool bTrackTarget = false;

	/** Yaw speed while tracking (deg/s); a target circling faster than this can still get out of the hit area. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Attack", meta = (ClampMin = "0", EditCondition = "bTrackTarget"))
	float TrackTurnRate = 360.f;

	/** Let the owner move (keep chasing) during RecoveryTime; it still cannot start the next attack until it ends. Off = it stands still (punish window). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Attack")
	bool bMoveDuringRecovery = false;

	/** Telegraph height above the owner's feet (avoids z-fighting with the ground). */
	UPROPERTY(EditAnywhere, Category = "Enemy Attack")
	float TelegraphGroundOffset = 2.f;

private:
	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> Telegraph;

	TWeakObjectPtr<AActor> CurrentTarget;
	int32 CurrentAttackIndex = INDEX_NONE;
	FTimerHandle AttackTimer;
	bool bAttacking = false;
	bool bRecovering = false;

	int32 PickAttack(float Distance) const;
	void Strike();
	void FinishAttack();
	void SetTelegraphVisible(bool bVisible);
	void StopAttackAnimation();
	void FaceTarget(float MaxYawDelta);
};
