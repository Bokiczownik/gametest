#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractionTargetChanged, AActor*, NewTarget);

/**
 * Lives on the player pawn. Interactable actors register themselves while the pawn is in their range
 * (SetTarget / ClearTarget); Interact() uses the current target. No traces or ticking.
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

	/** Bound to the interact input (E). Does nothing without a target. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Interact();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetTarget() const { return Target.Get(); }

	/** Called by interactables when the owner enters their range. Target must implement IInteractable. */
	void SetTarget(AActor* NewTarget);

	/** Called by interactables when the owner leaves their range; ignored if another target took over. */
	void ClearTarget(AActor* OldTarget);

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractionTargetChanged OnTargetChanged;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Interaction")
	TWeakObjectPtr<AActor> Target;
};
