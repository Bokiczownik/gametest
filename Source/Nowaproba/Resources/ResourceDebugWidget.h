#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ResourceComponent.h"
#include "ResourceDebugWidget.generated.h"

class APawn;
class UButton;
class UTextBlock;
class UVerticalBox;

/**
 * Shows all resource values with debug buttons (+10 / -10) per resource.
 * Layout is built in C++, so the class can be created directly without a UMG asset.
 * Tracks the resource component on the owning player's pawn and follows pawn changes.
 */
UCLASS()
class NOWAPROBA_API UResourceDebugWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	static constexpr int32 DebugAmount = 10;

	UPROPERTY()
	TObjectPtr<UTextBlock> ValueTexts[UResourceComponent::NumResources];

	UPROPERTY()
	TObjectPtr<UButton> AddButtons[UResourceComponent::NumResources];

	UPROPERTY()
	TObjectPtr<UButton> RemoveButtons[UResourceComponent::NumResources];

	TWeakObjectPtr<UResourceComponent> ResourceComponent;

	void BuildLayout();
	void AddRow(UVerticalBox* Column, int32 Index);
	void BindToPawn(APawn* Pawn);
	void UnbindResources();
	void RefreshValue(EResourceType Resource);
	void RefreshAll();
	void ChangeResource(EResourceType Resource, int32 Amount);

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleResourceChanged(EResourceType Resource, int32 NewValue, int32 Delta);

	UFUNCTION()
	void HandleAdd1Clicked();

	UFUNCTION()
	void HandleRemove1Clicked();

	UFUNCTION()
	void HandleAdd2Clicked();

	UFUNCTION()
	void HandleRemove2Clicked();

	UFUNCTION()
	void HandleAdd3Clicked();

	UFUNCTION()
	void HandleRemove3Clicked();
};
