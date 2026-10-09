#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CurrencyDebugWidget.generated.h"

class APawn;
class UButton;
class UCurrencyComponent;
class UTextBlock;

/**
 * Wallet display with TEMPORARY debug buttons (+100 / -100).
 * Layout is built in C++, so the class can be created directly without a UMG asset.
 * Tracks the currency component on the owning player's pawn and follows pawn changes.
 */
UCLASS()
class NOWAPROBA_API UCurrencyDebugWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	static constexpr int32 DebugAmount = 100;

	UPROPERTY()
	TObjectPtr<UTextBlock> BalanceText;

	UPROPERTY()
	TObjectPtr<UButton> AddButton;

	UPROPERTY()
	TObjectPtr<UButton> RemoveButton;

	TWeakObjectPtr<UCurrencyComponent> CurrencyComponent;

	void BuildLayout();
	void BindToPawn(APawn* Pawn);
	void UnbindCurrency();
	void RefreshBalance();

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleBalanceChanged(int32 NewBalance, int32 Delta);

	UFUNCTION()
	void HandleAddClicked();

	UFUNCTION()
	void HandleRemoveClicked();
};
