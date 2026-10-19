#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DashCooldownWidget.generated.h"

/**
 * Small pie above the HP bar (bottom-center) that fills while the dash recharges; full and bright = dash ready.
 * Reads UDashAbility::GetDashCooldownProgress from the owning player's pawn ASC. Drawn in NativePaint, so the class
 * can be created directly without a UMG asset (full-screen, hit-test invisible).
 */
UCLASS()
class NOWAPROBA_API UDashCooldownWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	UPROPERTY(EditAnywhere, Category = "Dash")
	float Radius = 14.f;

	/** Distance from the bottom of the screen to the bottom of the pie (HP bar panel top is at 98). */
	UPROPERTY(EditAnywhere, Category = "Dash")
	float BottomOffset = 104.f;

	UPROPERTY(EditAnywhere, Category = "Dash")
	FLinearColor BackgroundColor = FLinearColor(0.f, 0.f, 0.f, 0.55f);

	UPROPERTY(EditAnywhere, Category = "Dash")
	FLinearColor ChargingColor = FLinearColor(0.25f, 0.55f, 0.8f, 0.8f);

	UPROPERTY(EditAnywhere, Category = "Dash")
	FLinearColor ReadyColor = FLinearColor(0.45f, 0.9f, 1.f, 1.f);

private:
	float Progress = 1.f;
};
