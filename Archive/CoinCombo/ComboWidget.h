#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ComboWidget.generated.h"

class APawn;
class UCoinComboComponent;
class UProgressBar;
class UTextBlock;

/**
 * Top-center combo display: multiplier text and a draining combo bar.
 * Layout is built in C++, so the class can be created directly without a UMG asset.
 * Tracks the combo component on the owning player's pawn and follows pawn changes.
 */
UCLASS()
class NOWAPROBA_API UComboWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY()
	TObjectPtr<UTextBlock> MultiplierText;

	UPROPERTY()
	TObjectPtr<UProgressBar> ComboBar;

	TWeakObjectPtr<UCoinComboComponent> ComboComponent;

	void BuildLayout();
	void BindToPawn(APawn* Pawn);
	void UnbindCombo();
	void RefreshMultiplier();

	UFUNCTION()
	void HandleProgressChanged(float NewProgress);

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleMultiplierChanged(float NewMultiplier);
};
