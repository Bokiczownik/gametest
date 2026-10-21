#include "CombatAttributeSet.h"

#include "BlockComponent.h"
#include "CombatTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

void UCombatAttributeSet::ApplyDamage(UAbilitySystemComponent* Target, float InDamage, float InPoiseDamage, const AActor* Source)
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

	FGameplayEffectContextHandle Context = Target->MakeEffectContext();
	if (Source)
	{
		Context.AddOrigin(Source->GetActorLocation());
	}
	Target->ApplyGameplayEffectSpecToSelf(FGameplayEffectSpec(Effect, Context, 1.f));
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
	else if (Attribute == GetBlockAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxBlock());
	}
	else if (Attribute == GetMaxBlockAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
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
	else if (Attribute == GetMaxBlockAttribute() && GetBlock() > NewValue)
	{
		ASC->SetNumericAttributeBase(GetBlockAttribute(), NewValue);
	}
}

void UCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const bool bInvulnerable = Data.Target.HasMatchingGameplayTag(CombatTags::State_Invulnerable);

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		float Amount = GetDamage();
		SetDamage(0.f);
		if (Amount > 0.f && !bInvulnerable)
		{
			if (IsBlockedHit(Data))
			{
				const UBlockComponent* Blocker = GetOwningActor()->FindComponentByClass<UBlockComponent>();
				Amount *= 1.f - Blocker->DamageReduction;
				const float Absorbed = FMath::Min(Amount, GetBlock());
				SetBlock(GetBlock() - Absorbed);
				Amount -= Absorbed;
			}
			if (Amount > 0.f)
			{
				SetHealth(FMath::Clamp(GetHealth() - Amount, 0.f, GetMaxHealth()));
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetPoiseDamageAttribute())
	{
		const float Amount = GetPoiseDamage();
		SetPoiseDamage(0.f);
		if (Amount > 0.f && !bInvulnerable && !Data.Target.HasMatchingGameplayTag(CombatTags::State_StaggerImmune) && !IsBlockedHit(Data))
		{
			SetPoise(FMath::Clamp(GetPoise() - Amount, 0.f, GetMaxPoise()));
		}
	}
}

bool UCombatAttributeSet::IsBlockedHit(const FGameplayEffectModCallbackData& Data) const
{
	const FGameplayEffectContextHandle& Context = Data.EffectSpec.GetContext();
	if (!Data.Target.HasMatchingGameplayTag(CombatTags::State_Blocking) || !Context.HasOrigin())
	{
		return false;
	}
	const UBlockComponent* Blocker = GetOwningActor()->FindComponentByClass<UBlockComponent>();
	return Blocker && Blocker->IsBlockingFrom(Context.GetOrigin());
}
