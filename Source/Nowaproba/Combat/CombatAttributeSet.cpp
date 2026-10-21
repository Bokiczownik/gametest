#include "CombatAttributeSet.h"

#include "CombatTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

void UCombatAttributeSet::ApplyDamage(UAbilitySystemComponent* Target, float InDamage, float InPoiseDamage)
{
	if (!Target || (InDamage <= 0.f && InPoiseDamage <= 0.f))
	{
		return;
	}

	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage());
	Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
	auto AddModifier = [Effect](const FGameplayAttribute& Attribute, float Magnitude)
	{
		if (Magnitude > 0.f)
		{
			FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
			Modifier.Attribute = Attribute;
			Modifier.ModifierOp = EGameplayModOp::Additive;
			Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Magnitude));
		}
	};
	AddModifier(GetDamageAttribute(), InDamage);
	AddModifier(GetPoiseDamageAttribute(), InPoiseDamage);

	Target->ApplyGameplayEffectSpecToSelf(FGameplayEffectSpec(Effect, Target->MakeEffectContext(), 1.f));
}

void UCombatAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UCombatAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	else if (Attribute == GetPoiseAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxPoise());
	}
	else if (Attribute == GetMaxHealthAttribute() || Attribute == GetMaxStaminaAttribute() || Attribute == GetMaxPoiseAttribute())
	{
		NewValue = FMath::Max(1.f, NewValue);
	}
}

void UCombatAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// Keep current values within a lowered maximum.
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (Attribute == GetMaxHealthAttribute() && GetHealth() > NewValue)
	{
		ASC->SetNumericAttributeBase(GetHealthAttribute(), NewValue);
	}
	else if (Attribute == GetMaxStaminaAttribute() && GetStamina() > NewValue)
	{
		ASC->SetNumericAttributeBase(GetStaminaAttribute(), NewValue);
	}
	else if (Attribute == GetMaxPoiseAttribute() && GetPoise() > NewValue)
	{
		ASC->SetNumericAttributeBase(GetPoiseAttribute(), NewValue);
	}
}

void UCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const bool bInvulnerable = Data.Target.HasMatchingGameplayTag(CombatTags::State_Invulnerable);

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float Amount = GetDamage();
		SetDamage(0.f);
		if (Amount > 0.f && !bInvulnerable)
		{
			SetHealth(FMath::Clamp(GetHealth() - Amount, 0.f, GetMaxHealth()));
		}
	}
	else if (Data.EvaluatedData.Attribute == GetPoiseDamageAttribute())
	{
		const float Amount = GetPoiseDamage();
		SetPoiseDamage(0.f);
		if (Amount > 0.f && !bInvulnerable && !Data.Target.HasMatchingGameplayTag(CombatTags::State_StaggerImmune))
		{
			SetPoise(FMath::Clamp(GetPoise() - Amount, 0.f, GetMaxPoise()));
		}
	}
}
