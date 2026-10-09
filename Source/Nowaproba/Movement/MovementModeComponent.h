#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MovementModeComponent.generated.h"

class APlayerController;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/** How the player steers the character. */
UENUM(BlueprintType)
enum class EPlayerMovementMode : uint8
{
	WASD UMETA(DisplayName = "WASD"),
	PointAndClick UMETA(DisplayName = "Point & Click")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerMovementModeChanged, EPlayerMovementMode, NewMode);

/**
 * Lives on the player controller. Switches between WASD and point-and-click movement
 * by swapping the matching input mapping contexts, and handles the WASD move action
 * (camera-relative, so W always moves "up" on screen).
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UMovementModeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMovementModeComponent();

	UFUNCTION(BlueprintPure, Category = "Movement Mode")
	EPlayerMovementMode GetMovementMode() const { return CurrentMode; }

	/** Activates the mapping context of NewMode. Leaving point-and-click cancels any path in progress. */
	UFUNCTION(BlueprintCallable, Category = "Movement Mode")
	void SetMovementMode(EPlayerMovementMode NewMode);

	/** Broadcast only when the mode actually changes. */
	UPROPERTY(BlueprintAssignable, Category = "Movement Mode")
	FOnPlayerMovementModeChanged OnMovementModeChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Movement Mode")
	EPlayerMovementMode DefaultMode = EPlayerMovementMode::WASD;

	UPROPERTY(EditDefaultsOnly, Category = "Movement Mode|Input")
	TObjectPtr<UInputMappingContext> WASDContext;

	UPROPERTY(EditDefaultsOnly, Category = "Movement Mode|Input")
	TObjectPtr<UInputMappingContext> PointAndClickContext;

	/** Axis2D action: X = forward, Y = left. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement Mode|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Movement Mode|Input")
	int32 ContextPriority = 0;

private:
	EPlayerMovementMode CurrentMode = EPlayerMovementMode::WASD;

	APlayerController* GetLocalPlayerController() const;
	void ApplyMappingContexts() const;
	void HandleMove(const FInputActionValue& Value);
};
