#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HitReactionComponent.generated.h"

class UAbilitySystemComponent;
class UAnimSequenceBase;
class UHealthComponent;
class UMaterialInterface;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaggered);

/**
 * How the owner reacts to being hit: overlays HitFlashMaterial on its mesh for a moment when it loses health, takes
 * knockback (ApplyKnockback, called by the attacker) and staggers when its Poise attribute (UCombatAttributeSet) hits 0.
 * Staggered = tag State.Staggered (blocks attack / dash abilities), running Ability.Attack abilities are cancelled,
 * StaggerAnimation plays, a player cannot move and an AI stops moving, for StaggerDuration; OnStaggered lets the owner
 * cancel its own actions. Poise refills to MaxPoise after the stagger, or PoiseRegenDelay after the last poise damage.
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Launches the owner (character) horizontally by Velocity * KnockbackScale. Ignored when dead. */
	UFUNCTION(BlueprintCallable, Category = "Hit Reaction")
	void ApplyKnockback(const FVector& Velocity);

	UFUNCTION(BlueprintPure, Category = "Hit Reaction")
	bool IsStaggered() const { return bStaggered; }

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

	/** Played on DefaultSlot when staggered (optional). */
	UPROPERTY(EditAnywhere, Category = "Hit Reaction|Stagger")
	TObjectPtr<UAnimSequenceBase> StaggerAnimation;

	UPROPERTY(EditAnywhere, Category = "Hit Reaction|Stagger", meta = (ClampMin = "0.1"))
	float StaggerDuration = 1.2f;

	/** Poise refills to max this long after the last poise damage (s). */
	UPROPERTY(EditAnywhere, Category = "Hit Reaction|Stagger", meta = (ClampMin = "0"))
	float PoiseRegenDelay = 3.f;

private:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UHealthComponent> Health;

	float LastHealth = 0.f;
	bool bStaggered = false;
	bool bIgnoringMoveInput = false;
	FTimerHandle FlashTimer;
	FTimerHandle StaggerTimer;
	FTimerHandle PoiseRegenTimer;

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);

	void HandlePoiseChanged(const FOnAttributeChangeData& Data);
	void StartStagger();
	void EndStagger();
	void RefillPoise();
	void EndFlash();
};
