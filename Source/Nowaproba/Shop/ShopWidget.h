#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShopWidget.generated.h"

class UButton;

/**
 * Placeholder shop window (title, "coming soon" text, Close button). Gear and upgrades will be listed here later.
 * While open it takes UI-only input with the mouse cursor shown (character controls pause); Close, E or Esc
 * close it, and the previous cursor state and the project's default viewport mouse capture are restored.
 * Layout is built in C++, so the class can be created directly without a UMG asset.
 */
UCLASS()
class NOWAPROBA_API UShopWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	UPROPERTY()
	TObjectPtr<UButton> CloseButton;

	bool bPreviousShowMouseCursor = false;

	void BuildLayout();

	UFUNCTION()
	void HandleCloseClicked();
};
