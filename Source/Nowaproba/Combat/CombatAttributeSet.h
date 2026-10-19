#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CombatAttributeSet.generated.h"

#define COMBAT_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Combat attributes shared by the player and enemies (GAS).
 * Damage / PoiseDamage are meta attributes: effects (ApplyDamage) write incoming damage there and PostGameplayEffectExecute moves it
 * into Health / Poise. Damage is ignored while the owner has the State.Invulnerable tag (e.g. during a dash).
 * UHealthComponent / UStaminaComponent keep their own API and use these attributes as storage when the owner has an ASC.
 */
UCLASS()
class NOWAPROBA_API UCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Health")
	FGameplayAttributeData Health = 100.f;
	COMBAT_ATTRIBUTE_ACCESSORS(UCombatAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Health")
	FGameplayAttributeData MaxHealth = 100.f;
	COMBAT_ATTRIBUTE_ACCESSORS(UCombatAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Stamina")
	FGameplayAttributeData Stamina = 30.f;
	COMBAT_ATTRIBUTE_ACCESSORS(UCombatAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Stamina")
	FGameplayAttributeData MaxStamina = 30.f;
	COMBAT_ATTRIBUTE_ACCESSORS(UCombatAttributeSet, MaxStamina)

	/** Stagger resistance; reaching 0 staggers the owner. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Poise")
	FGameplayAttributeData Poise = 50.f;
	COMBAT_ATTRIBUTE_ACCESSORS(UCombatAttributeSet, Poise)

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Poise")
	FGameplayAttributeData MaxPoise = 50.f;
	COMBAT_ATTRIBUTE_ACCESSORS(UCombatAttributeSet, MaxPoise)

	/** Meta: incoming health damage. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Meta")
	FGameplayAttributeData Damage = 0.f;
	COMBAT_ATTRIBUTE_ACCESSORS(UCombatAttributeSet, Damage)

	/** Meta: incoming poise damage. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Meta")
	FGameplayAttributeData PoiseDamage = 0.f;
	COMBAT_ATTRIBUTE_ACCESSORS(UCombatAttributeSet, PoiseDamage)

	/**
	 * Applies Damage / PoiseDamage to Target through an instant gameplay effect, so PostGameplayEffectExecute runs
	 * (invulnerability, clamping). Use this instead of ApplyModToAttribute, which only sets base values.
	 */
	static void ApplyDamage(UAbilitySystemComponent* Target, float InDamage, float InPoiseDamage);

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

private:
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};
