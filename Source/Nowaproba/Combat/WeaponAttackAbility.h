#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "WeaponAttackAbility.generated.h"

class UWeaponComponent;
struct FWeaponAttack;

/**
 * Performs the attack chosen by the owner's UWeaponComponent (GetPendingAttack): faces the cursor, spends stamina,
 * plays the attack's animation range on DefaultSlot (upper body in ABP_Unarmed), steps forward (LungeDistance, ground
 * only) and keeps that facing while the owner can still walk (orient-to-movement is off until it ends). Retriggering (next combo hit) ends the running attack first. Tagged Ability.Attack (cancelled
 * by the dash); cannot start while State.Invulnerable (during a dash).
 */
UCLASS()
class NOWAPROBA_API UWeaponAttackAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UWeaponAttackAbility();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	TWeakObjectPtr<UWeaponComponent> Weapon;
	bool bRestoreOrientToMovement = false;
	FTimerHandle EndTimer;

	static UWeaponComponent* FindWeapon(const FGameplayAbilityActorInfo* ActorInfo);
	/** Turns the owner towards the cursor (plus the weapon's AttackFacingYawOffset); returns the attack direction. */
	FVector FaceCursor(ACharacter* Character, float YawOffset) const;

	UFUNCTION()
	void HandleAttackFinished();
};
