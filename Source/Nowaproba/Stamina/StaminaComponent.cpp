#include "StaminaComponent.h"

#include "AbilitySystemComponent.h"
#include "../Combat/CombatAttributeSet.h"
#include "GameFramework/Actor.h"

UStaminaComponent::UStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UStaminaComponent::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystem = GetOwner()->FindComponentByClass<UAbilitySystemComponent>();
	if (AbilitySystem && AbilitySystem->HasAttributeSetForAttribute(UCombatAttributeSet::GetStaminaAttribute()))
	{
		AbilitySystem->SetNumericAttributeBase(UCombatAttributeSet::GetStaminaAttribute(), 0.f);
		AbilitySystem->GetGameplayAttributeValueChangeDelegate(UCombatAttributeSet::GetStaminaAttribute()).AddUObject(this, &UStaminaComponent::HandleStaminaAttributeChanged);
	}
	else
	{
		AbilitySystem = nullptr;
	}

	RecalculateMaxStamina();
	SetStamina(MaxStamina);
}

void UStaminaComponent::SetDraining(bool bNewDraining)
{
	if (bNewDraining && !HasStamina())
	{
		bNewDraining = false;
	}
	if (bNewDraining == bDraining)
	{
		return;
	}

	bDraining = bNewDraining;
	if (!bDraining)
	{
		RegenDelayRemaining = RegenDelay;
	}
	UpdateTickEnabled();
}

void UStaminaComponent::SetMaxStaminaBonus(float NewBonus)
{
	MaxStaminaBonus = NewBonus;
	RecalculateMaxStamina();
}

void UStaminaComponent::SetMaxStaminaMultiplier(float NewMultiplier)
{
	MaxStaminaMultiplier = FMath::Max(0.f, NewMultiplier);
	RecalculateMaxStamina();
}

void UStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDraining)
	{
		// Holding sprint without moving costs nothing, but still counts as sprinting for the regen delay.
		RegenDelayRemaining = RegenDelay;
		if (GetOwner()->GetVelocity().Size2D() > MinMoveSpeedToDrain)
		{
			SetStamina(CurrentStamina - DrainPerSecond * DeltaTime);
			if (!HasStamina())
			{
				SetDraining(false);
				OnStaminaDepleted.Broadcast();
			}
		}
		return;
	}

	if (RegenDelayRemaining > 0.f)
	{
		RegenDelayRemaining -= DeltaTime;
		return;
	}

	SetStamina(CurrentStamina + RegenPerSecond * DeltaTime);
	UpdateTickEnabled();
}

void UStaminaComponent::RecalculateMaxStamina()
{
	MaxStamina = FMath::Max(1.f, (BaseMaxStamina + MaxStaminaBonus) * MaxStaminaMultiplier);
	if (AbilitySystem)
	{
		AbilitySystem->SetNumericAttributeBase(UCombatAttributeSet::GetMaxStaminaAttribute(), MaxStamina);
	}
	SetStamina(CurrentStamina);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
	UpdateTickEnabled();
}

void UStaminaComponent::SetStamina(float NewStamina)
{
	NewStamina = FMath::Clamp(NewStamina, 0.f, MaxStamina);
	if (NewStamina == CurrentStamina)
	{
		return;
	}

	if (AbilitySystem)
	{
		// HandleStaminaAttributeChanged updates CurrentStamina and broadcasts.
		TGuardValue<bool> Writing(bWritingStaminaAttribute, true);
		AbilitySystem->SetNumericAttributeBase(UCombatAttributeSet::GetStaminaAttribute(), NewStamina);
		return;
	}

	CurrentStamina = NewStamina;
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

void UStaminaComponent::HandleStaminaAttributeChanged(const FOnAttributeChangeData& Data)
{
	CurrentStamina = Data.NewValue;
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);

	if (!bWritingStaminaAttribute && Data.NewValue < Data.OldValue)
	{
		RegenDelayRemaining = RegenDelay;
		UpdateTickEnabled();
	}
}

void UStaminaComponent::UpdateTickEnabled()
{
	SetComponentTickEnabled(bDraining || CurrentStamina < MaxStamina);
}
