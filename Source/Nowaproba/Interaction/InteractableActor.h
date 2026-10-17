#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "InteractableActor.generated.h"

class USphereComponent;

/**
 * Base for Blueprint interactables (exit, chests, quest NPCs): while the player pawn is inside InteractionRange
 * this actor is its interaction target. Blueprint children implement the Interact event and may react to
 * OnPlayerEnteredRange / OnPlayerLeftRange (e.g. show a prompt).
 */
UCLASS(Abstract)
class NOWAPROBA_API AInteractableActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AInteractableActor();

	/** Empty by default; Blueprint children implement the Interact event. */
	virtual void Interact_Implementation(APawn* InstigatorPawn) override {}

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USceneComponent> Root;

	/** Player inside this sphere can interact. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USphereComponent> InteractionRange;

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnPlayerEnteredRange(APawn* PlayerPawn);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnPlayerLeftRange(APawn* PlayerPawn);

private:
	UFUNCTION()
	void HandleRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);
};
