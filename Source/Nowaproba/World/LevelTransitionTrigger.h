#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelTransitionTrigger.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UWorld;

/**
 * Trigger zone that opens DestinationLevel when a player-controlled pawn walks in.
 * Fires once per instance (no repeated requests while the level loads). A pawn that spawns inside the box does not
 * trigger it: it is not player-controlled yet when the initial overlap happens, so it has to walk out and back in.
 * Visual mesh/material are set in the Blueprint subclass (BP_LevelTransition).
 */
UCLASS()
class NOWAPROBA_API ALevelTransitionTrigger : public AActor
{
	GENERATED_BODY()

public:
	ALevelTransitionTrigger();

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Level Transition")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, Category = "Level Transition")
	TObjectPtr<UStaticMeshComponent> Visual;

	UPROPERTY(VisibleAnywhere, Category = "Level Transition")
	TObjectPtr<UTextRenderComponent> Label;

	/** Level opened when the player enters. Must also be listed in Project Settings > Packaging > Maps to Cook. */
	UPROPERTY(EditAnywhere, Category = "Level Transition")
	TSoftObjectPtr<UWorld> DestinationLevel;

	/** Optional URL options passed to OpenLevel (e.g. "?listen"). */
	UPROPERTY(EditAnywhere, Category = "Level Transition", AdvancedDisplay)
	FString Options;

private:
	bool bTransitionRequested = false;
};
