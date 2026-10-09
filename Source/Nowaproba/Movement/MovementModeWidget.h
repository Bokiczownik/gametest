#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MovementModeComponent.h"
#include "MovementModeWidget.generated.h"

class UButton;

/**
 * Bottom-left panel with "WASD" / "Point & Click" buttons; the active mode is highlighted.
 * Layout is built in C++, so the class can be created directly without a UMG asset.
 * Talks to the UMovementModeComponent on the owning player controller.
 */
UCLASS()
class NOWAPROBA_API UMovementModeWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY()
	TObjectPtr<UButton> WASDButton;

	UPROPERTY()
	TObjectPtr<UButton> PointAndClickButton;

	TWeakObjectPtr<UMovementModeComponent> MovementMode;

	void BuildLayout();
	UButton* MakeButton(const FString& Name, const FText& Label);
	void RefreshHighlight();
	void SetMode(EPlayerMovementMode NewMode);

	UFUNCTION()
	void HandleModeChanged(EPlayerMovementMode NewMode);

	UFUNCTION()
	void HandleWASDClicked();

	UFUNCTION()
	void HandlePointAndClickClicked();
};
