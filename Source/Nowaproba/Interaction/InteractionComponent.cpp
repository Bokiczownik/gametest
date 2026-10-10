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
	if (IInteractable* Interactable = Cast<IInteractable>(Current))
	{
		Interactable->Interact(Cast<APawn>(GetOwner()));
	}
}

void UInteractionComponent::SetTarget(AActor* NewTarget)
{
	if (!Cast<IInteractable>(NewTarget) || Target.Get() == NewTarget)
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
