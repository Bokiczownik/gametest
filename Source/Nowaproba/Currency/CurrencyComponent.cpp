#include "CurrencyComponent.h"

UCurrencyComponent::UCurrencyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UCurrencyComponent::AddCurrency(int32 Amount)
{
	if (Amount < 0 || Amount > MAX_int32 - Balance)
	{
		return false;
	}

	SetBalance(Balance + Amount);
	return true;
}

bool UCurrencyComponent::RemoveCurrency(int32 Amount)
{
	if (Amount < 0 || Amount > Balance)
	{
		return false;
	}

	SetBalance(Balance - Amount);
	return true;
}

void UCurrencyComponent::SetBalance(int32 NewBalance)
{
	if (NewBalance == Balance)
	{
		return;
	}

	const int32 Delta = NewBalance - Balance;
	Balance = NewBalance;
	OnBalanceChanged.Broadcast(Balance, Delta);
}
