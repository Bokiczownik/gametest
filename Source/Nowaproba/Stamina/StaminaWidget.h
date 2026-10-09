#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StaminaWidget.generated.h"

class APawn;
class UProgressBar;
class UStaminaComponent;
class UTextBlock;

/**
 * Bottom-center stamina bar with "current / max" text.
 * Layout is built in C++, so the class can be created directly without a UMG asset.
 * Tracks the stamina component on the owning player's pawn and follows pawn changes; updates only on stamina events.
 */
UCLASS()
class NOWAPROBA_API UStaminaWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY()
	TObjectPtr<UProgressBar> StaminaBar;

	UPROPERTY()
	TObjectPtr<UTextBlock> StaminaText;

	TWeakObjectPtr<UStaminaComponent> StaminaComponent;

	void BuildLayout();
	void BindToPawn(APawn* Pawn);
	void UnbindStamina();

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleStaminaChanged(float CurrentStamina, float MaxStamina);
};
