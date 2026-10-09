#include "ResourceComponent.h"

UResourceComponent::UResourceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UResourceComponent::GetResource(EResourceType Resource) const
{
	return IsValidResource(Resource) ? Balances[static_cast<int32>(Resource)] : 0;
}

bool UResourceComponent::AddResource(EResourceType Resource, int32 Amount)
{
	if (!IsValidResource(Resource))
	{
		return false;
	}

	const int32 Current = Balances[static_cast<int32>(Resource)];
	if (Amount < 0 || Amount > MAX_int32 - Current)
	{
		return false;
	}

	SetResource(Resource, Current + Amount);
	return true;
}

bool UResourceComponent::RemoveResource(EResourceType Resource, int32 Amount)
{
	if (!IsValidResource(Resource))
	{
		return false;
	}

	const int32 Current = Balances[static_cast<int32>(Resource)];
	if (Amount < 0 || Amount > Current)
	{
		return false;
	}

	SetResource(Resource, Current - Amount);
	return true;
}

bool UResourceComponent::IsValidResource(EResourceType Resource)
{
	return static_cast<int32>(Resource) < NumResources;
}

void UResourceComponent::SetResource(EResourceType Resource, int32 NewValue)
{
	int32& Balance = Balances[static_cast<int32>(Resource)];
	if (NewValue == Balance)
	{
		return;
	}

	const int32 Delta = NewValue - Balance;
	Balance = NewValue;
	OnResourceChanged.Broadcast(Resource, Balance, Delta);
}
