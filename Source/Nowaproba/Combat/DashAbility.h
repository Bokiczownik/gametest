#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "DashAbility.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

/**
 * Dash with dodge properties: a short burst of DashDistance over DashDuration (root motion, no animation needed) in the
 * movement input direction, or towards the cursor when there is no input. While dashing the owner has
 * State.Invulnerable (no damage) and its capsule ignores pawns (passes through enemies). Cancels a running weapon attack.
 * Costs StaminaCost; cooldown CooldownDuration starts when the dash ends and is a runtime gameplay effect granting Cooldown.Dash (read by
 * GetDashCooldownProgress for the HUD). Grant via UCombatAbilitySystemComponent::DefaultAbilities and activate with
 * TryActivateAbilityByClass.
 */
UCLASS()
class NOWAPROBA_API UDashAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UDashAbility();

	/** 0..1 cooldown progress of the dash on this ASC (1 = ready). */
	UFUNCTION(BlueprintPure, Category = "Dash")
	static float GetDashCooldownProgress(const UAbilitySystemComponent* AbilitySystem);

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Dash", meta = (ClampMin = "0"))
	float DashDistance = 330.f;

	UPROPERTY(EditDefaultsOnly, Category = "Dash", meta = (ClampMin = "0.05"))
	float DashDuration = 0.2f;

	/** Seconds; items will lower it later. */
	UPROPERTY(EditDefaultsOnly, Category = "Dash", meta = (ClampMin = "0"))
	float CooldownDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Dash", meta = (ClampMin = "0"))
	float StaminaCost = 10.f;

private:
	UPROPERTY(Transient)
	mutable TObjectPtr<UGameplayEffect> CooldownEffect;

	TEnumAsByte<ECollisionResponse> SavedPawnResponse = ECR_Block;
	bool bInvulnerabilityAdded = false;

	FVector GetDashDirection(const FGameplayAbilityActorInfo* ActorInfo) const;

	UFUNCTION()
	void HandleDashFinished();
};
