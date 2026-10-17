#include "InteractionComponent.h"

#include "GameFramework/Pawn.h"
#include "Interactable.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractionComponent::Interact()
{
	AActor* Current = Target.Get();
	if (Current && Current->Implements<UInteractable>())
	{
		IInteractable::Execute_Interact(Current, Cast<APawn>(GetOwner()));
	}
}

void UInteractionComponent::SetTarget(AActor* NewTarget)
{
	if (!NewTarget || !NewTarget->Implements<UInteractable>() || Target.Get() == NewTarget)
	{
		return;
	}

	Target = NewTarget;
	OnTargetChanged.Broadcast(NewTarget);
}

void UInteractionComponent::ClearTarget(AActor* OldTarget)
{
	if (Target.Get() != OldTarget)
	{
		return;
	}

	Target.Reset();
	OnTargetChanged.Broadcast(nullptr);
}
