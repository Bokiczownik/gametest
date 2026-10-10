#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShopWidget.generated.h"

class UButton;

/**
 * Placeholder shop window (title, "coming soon" text, Close button). Gear and upgrades will be listed here later.
 * Layout is built in C++, so the class can be created directly without a UMG asset.
 */
UCLASS()
class NOWAPROBA_API UShopWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY()
	TObjectPtr<UButton> CloseButton;

	void BuildLayout();

	UFUNCTION()
	void HandleCloseClicked();
};
