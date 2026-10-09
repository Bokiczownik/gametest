#include "StaminaComponent.h"

#include "GameFramework/Actor.h"

UStaminaComponent::UStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UStaminaComponent::BeginPlay()
{
	Super::BeginPlay();

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

	CurrentStamina = NewStamina;
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

void UStaminaComponent::UpdateTickEnabled()
{
	SetComponentTickEnabled(bDraining || CurrentStamina < MaxStamina);
}
