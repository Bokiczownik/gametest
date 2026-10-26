#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyAttackComponent.generated.h"

class AEnemyProjectile;
class UAnimSequenceBase;
class UPrimitiveComponent;

/** One telegraphed enemy attack: melee (hit sphere), ranged (Projectile) or charge (ChargeSpeed). */
USTRUCT(BlueprintType)
struct FEnemyAttack
{
	GENERATED_BODY()

	/** For StartNamedAttack (e.g. a counter after a block). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FName Name;

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

	/** Stuns the target on hit for this long (s, UHitReactionComponent::ApplyStun); 0 = no stun. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float StunDuration = 0.f;

	/** Only picked while the target is at least this far (2D). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float MinRange = 0.f;

	/** Not picked again until this long after its recovery ends (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float Cooldown = 0.f;

	/** Relative chance among the attacks that can reach the target; 0 = only started by name, not counted in the attack range. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float Weight = 1.f;

	/** Optional, played on DefaultSlot so that its StrikeAnimTime lines up with the end of the windup. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Animation")
	TObjectPtr<UAnimSequenceBase> Animation;

	/** Animation time of the hit frame (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Animation", meta = (ClampMin = "0"))
	float StrikeAnimTime = 0.f;

	/** The animation drives the whole body (the anim BP drops its arm layers, e.g. the shield arm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Animation")
	bool bFullBody = false;

	/** Set = ranged: fired at the strike along the owner's facing instead of the hit sphere; Reach is its max distance, Radius its hit radius. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Ranged")
	TSubclassOf<AEnemyProjectile> Projectile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Ranged", meta = (ClampMin = "1"))
	float ProjectileSpeed = 1800.f;

	/** Random yaw error of the shot, +- degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Ranged", meta = (ClampMin = "0", ClampMax = "45"))
	float AimSpread = 0.f;

	/** Hop this far away from the target at the start (only where the navmesh continues behind the owner); 0 = none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Ranged", meta = (ClampMin = "0"))
	float BackstepDistance = 0.f;

	/**
	 * > 0 = charge: at the strike the owner rushes along its facing at this speed (cm/s) up to Reach, ending just past the
	 * target's position at the strike (walls stop it). The first time the target touches the Radius sphere in front of
	 * the owner it is hit and knocked sideways off the charge line; then the charge ends and RecoveryTime follows.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charge", meta = (ClampMin = "0"))
	float ChargeSpeed = 0.f;

	float GetRange() const { return Reach + Radius; }
	bool IsRanged() const;
	bool IsCharge() const { return ChargeSpeed > 0.f; }
};

/**
 * Telegraphed enemy attacks: StartAttack picks one of Attacks that can reach the target and is off cooldown (weighted random), turns
 * the owner to the target, plays its animation and shows the telegraph (hit area on the ground, sized to the attack)
 * for WindupTime; the strike then hits the target if it is still inside the sphere of Radius at Reach in front of the
 * owner: engine damage (UGameplayStatics::ApplyDamage), PoiseDamage (UCombatAttributeSet) and knockback
 * (UHitReactionComponent); then the owner recovers for RecoveryTime. Timers drive the attack; with bTrackTarget the
 * component ticks during the windup only, turning the owner (and the telegraph) toward the target at TrackTurnRate.
 * The telegraph is scaled to Radius (engine Cylinder: 100 cm wide at scale 1).
 * Ranged attacks (FEnemyAttack::Projectile) show a lane that grows from the owner toward the target during the windup
 * (the component ticks) and fire the projectile at the strike; the optional backstep hops away first.
 * Charge attacks (FEnemyAttack::ChargeSpeed) show the same lane, then move the owner with a constant-force root motion
 * source and hit the target on contact (the component ticks during the charge).
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

	/** Starts the attack called Name regardless of range or Weight (it may miss). Ignored while an attack is running. */
	UFUNCTION(BlueprintCallable, Category = "Enemy Attack")
	void StartNamedAttack(AActor* Target, FName Name);

	/** Stops the current attack without hitting (e.g. on death or stagger). */
	UFUNCTION(BlueprintCallable, Category = "Enemy Attack")
	void CancelAttack();

	/** True from StartAttack until the recovery ends. */
	UFUNCTION(BlueprintPure, Category = "Enemy Attack")
	bool IsAttacking() const { return bAttacking; }

	/** True from StartAttack until the strike (telegraph shown). */
	UFUNCTION(BlueprintPure, Category = "Enemy Attack")
	bool IsWindingUp() const { return bAttacking && !bRecovering; }

	/** True while the running attack's animation is full body (FEnemyAttack::bFullBody). */
	UFUNCTION(BlueprintPure, Category = "Enemy Attack")
	bool IsFullBodyAttack() const { return bAttacking && Attacks.IsValidIndex(CurrentAttackIndex) && Attacks[CurrentAttackIndex].bFullBody; }

	/** True while the owner must stay in place: windup and strike, and the recovery unless bMoveDuringRecovery. */
	UFUNCTION(BlueprintPure, Category = "Enemy Attack")
	bool IsHoldingPosition() const { return bAttacking && !(bRecovering && bMoveDuringRecovery); }

	/** True if no attack is running and the attack called Name exists and is off cooldown (range not checked). */
	UFUNCTION(BlueprintPure, Category = "Enemy Attack")
	bool CanStartNamedAttack(FName Name) const;

	/** The attack called Name, or null. */
	const FEnemyAttack* FindAttack(FName Name) const;

	/** True if StartAttack(Target) would start an attack now (none running, one off cooldown covers the 2D distance). */
	UFUNCTION(BlueprintPure, Category = "Enemy Attack")
	bool CanStartAttack(const AActor* Target) const;

	/** True when SharedCooldown and the own cooldown of some attack with Weight > 0 have run out (range not checked). */
	UFUNCTION(BlueprintPure, Category = "Enemy Attack")
	bool IsOffCooldown() const;

	const FVector& GetProjectileSpawnOffset() const { return ProjectileSpawnOffset; }

	/** Distance from the owner within which some attack can hit (largest Reach + Radius of attacks with Weight > 0). */
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

	/** No attack is picked until this long after any attack's recovery ends (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Attack", meta = (ClampMin = "0"))
	float SharedCooldown = 0.f;

	/** Projectile spawn point relative to the owner (X along its facing, Z up). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Attack")
	FVector ProjectileSpawnOffset = FVector(50.f, 0.f, 20.f);

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
	TArray<double> ReadyTimes;
	double SharedReadyTime = 0.0;
	bool bCharging = false;
	uint16 ChargeRootMotionID = 0;
	FVector ChargeDirection = FVector::ForwardVector;
	FVector ChargeCheckedLocation = FVector::ZeroVector;

	bool IsAttackReady(int32 AttackIndex) const;
	int32 PickAttack(float Distance) const;
	void BeginAttack(AActor* Target, int32 AttackIndex);
	void Strike();
	void BeginRecovery();
	void FinishAttack();
	void ApplyHit(const FEnemyAttack& Attack, AActor& Target, const FVector& KnockbackDirection);
	void StartCharge(const FEnemyAttack& Attack, const AActor& Target);
	/** Hits the target if the front of the owner passed it since the last check. */
	bool CheckChargeHit(const FEnemyAttack& Attack);
	void EndCharge();
	void SetTelegraphVisible(bool bVisible);
	void StopAttackAnimation();
	void FaceTarget(float MaxYawDelta);
	void UpdateLaneTelegraph(const FEnemyAttack& Attack);
	void FireProjectile(const FEnemyAttack& Attack, const AActor& Target);
	void TryBackstep(float Distance);
};
