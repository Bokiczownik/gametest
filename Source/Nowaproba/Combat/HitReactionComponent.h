#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HitReactionComponent.generated.h"

class UAbilitySystemComponent;
class UAnimSequenceBase;
class UBlockComponent;
class UHealthComponent;
class UMaterialInterface;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaggered);

/**
 * How the owner reacts to being hit: overlays HitFlashMaterial on its mesh for a moment when it loses health, takes
 * knockback (ApplyKnockback, called by the attacker; ignored while State.Invulnerable) and staggers when its Poise attribute (UCombatAttributeSet) hits 0.
 * Staggered = tag State.Staggered (blocks attack / dash abilities), running Ability.Attack abilities are cancelled,
 * StaggerAnimation plays, a player cannot move and an AI stops moving, for StaggerDuration; OnStaggered lets the owner
 * cancel its own actions. Poise refills to MaxPoise after the stagger, or PoiseRegenDelay after the last poise damage.
 * For StaggerCooldown after a stagger ends the owner has tag State.StaggerImmune: no poise damage and no knockback.
 * ApplyStun works like a stagger of a given length without touching poise (tag State.Stunned; StaggerAnimation plays).
 * Knockback from the front is ignored while the owner blocks (UBlockComponent).
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Pushes the owner (character) horizontally by Velocity * KnockbackScale (launch, or a walking impulse with bGroundedKnockback). Ignored when dead. */
	UFUNCTION(BlueprintCallable, Category = "Hit Reaction")
	void ApplyKnockback(const FVector& Velocity);

	UFUNCTION(BlueprintPure, Category = "Hit Reaction")
	bool IsStaggered() const { return bStaggered; }

	/** Owner cannot move, attack or dash for Duration (s). Ignored when dead, invulnerable or already stunned. */
	UFUNCTION(BlueprintCallable, Category = "Hit Reaction")
	void ApplyStun(float Duration);

	UFUNCTION(BlueprintPure, Category = "Hit Reaction")
	bool IsStunned() const { return bStunned; }

	/** Staggered or stunned: lets the owner cancel its own actions. */
	UPROPERTY(BlueprintAssignable, Category = "Hit Reaction")
	FOnStaggered OnStaggered;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Overlay material shown briefly on the owner's mesh when it takes damage. */
	UPROPERTY(EditAnywhere, Category = "Hit Reaction|Flash")
	TObjectPtr<UMaterialInterface> HitFlashMaterial;

	UPROPERTY(EditAnywhere, Category = "Hit Reaction|Flash", meta = (ClampMin = "0"))
	float HitFlashDuration = 0.12f;

	/** Multiplier on incoming knockback (0 = immovable, e.g. bosses). */
	UPROPERTY(EditAnywhere, Category = "Hit Reaction|Knockback", meta = (ClampMin = "0"))
	float KnockbackScale = 1.f;

	/** Knockback as an impulse that keeps the owner walking (input and ground attacks keep working) instead of a launch into falling. */
	UPROPERTY(EditAnywhere, Category = "Hit Reaction|Knockback")
	bool bGroundedKnockback = false;

	/** Played on DefaultSlot when staggered (optional). */
	UPROPERTY(EditAnywhere, Category = "Hit Reaction|Stagger")
	TObjectPtr<UAnimSequenceBase> StaggerAnimation;

	UPROPERTY(EditAnywhere, Category = "Hit Reaction|Stagger", meta = (ClampMin = "0.1"))
	float StaggerDuration = 1.2f;

	/** After a stagger ends, the owner ignores poise damage and knockback for this long (s). */
	UPROPERTY(EditAnywhere, Category = "Hit Reaction|Stagger", meta = (ClampMin = "0"))
	float StaggerCooldown = 0.f;

	/** Poise refills to max this long after the last poise damage (s). */
	UPROPERTY(EditAnywhere, Category = "Hit Reaction|Stagger", meta = (ClampMin = "0"))
	float PoiseRegenDelay = 3.f;

private:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UHealthComponent> Health;

	UPROPERTY()
	TObjectPtr<UBlockComponent> Block;

	float LastHealth = 0.f;
	bool bStaggered = false;
	bool bStunned = false;
	/** Move input locks held (stagger and stun each take one). */
	int32 MoveInputLocks = 0;
	FTimerHandle FlashTimer;
	FTimerHandle StaggerTimer;
	FTimerHandle PoiseRegenTimer;
	FTimerHandle StaggerCooldownTimer;
	FTimerHandle StunTimer;

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);

	void HandlePoiseChanged(const FOnAttributeChangeData& Data);
	void StartStagger();
	void EndStagger();
	void EndStun();
	/** Stops the owner (move input / AI movement), cancels its attacks and plays StaggerAnimation. */
	void Incapacitate();
	void ReleaseMoveInput();
	void EndStaggerCooldown();
	void RefillPoise();
	void EndFlash();
};
