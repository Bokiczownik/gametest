#include "CoinComboComponent.h"

#include "../Currency/CurrencyComponent.h"

UCoinComboComponent::UCoinComboComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCoinComboComponent::BeginPlay()
{
	Super::BeginPlay();

	Currency = GetOwner()->FindComponentByClass<UCurrencyComponent>();
}

int32 UCoinComboComponent::RegisterCoin(int32 BaseValue)
{
	UCurrencyComponent* Wallet = Currency.Get();
	if (!Wallet || BaseValue <= 0)
	{
		return 0;
	}

	if (IsComboActive())
	{
		// Rounded to one decimal so repeated steps do not drift (x1.1, x1.2, ...).
		SetMultiplier(FMath::Min(MaxMultiplier, FMath::RoundToFloat((Multiplier + MultiplierStep) * 10.f) / 10.f));
	}

	Progress = 1.f;
	SetComponentTickEnabled(true);
	OnProgressChanged.Broadcast(Progress);

	const int32 Reward = FMath::RoundToInt(BaseValue * Multiplier);
	Wallet->AddCurrency(Reward);
	OnCoinRewarded.Broadcast(Reward, Multiplier);
	return Reward;
}

void UCoinComboComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Progress = FMath::Max(0.f, Progress - DeltaTime / ComboDuration);
	OnProgressChanged.Broadcast(Progress);
	if (Progress <= 0.f)
	{
		SetComponentTickEnabled(false);
		SetMultiplier(1.f);
	}
}

void UCoinComboComponent::SetMultiplier(float NewMultiplier)
{
	if (FMath::IsNearlyEqual(NewMultiplier, Multiplier))
	{
		return;
	}

	Multiplier = NewMultiplier;
	OnMultiplierChanged.Broadcast(Multiplier);
}
