#include "StaminaWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "StaminaComponent.h"

void UStaminaWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildLayout();
}

void UStaminaWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &UStaminaWidget::HandlePossessedPawnChanged);
		BindToPawn(PC->GetPawn());
	}
	else
	{
		HandleStaminaChanged(0.f, 1.f);
	}
}

void UStaminaWidget::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &UStaminaWidget::HandlePossessedPawnChanged);
	}
	UnbindStamina();

	Super::NativeDestruct();
}

void UStaminaWidget::BuildLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	Panel->SetPadding(FMargin(8.f));

	// Bottom-center, sized to content.
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.5f, 1.f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 1.f));
	PanelSlot->SetPosition(FVector2D(0.f, -20.f));
	PanelSlot->SetAutoSize(true);

	USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BarSize"));
	BarSize->SetWidthOverride(300.f);
	BarSize->SetHeightOverride(20.f);
	Panel->SetContent(BarSize);

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay"));
	BarSize->SetContent(Overlay);

	StaminaBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("StaminaBar"));
	StaminaBar->SetFillColorAndOpacity(FLinearColor(0.25f, 0.85f, 0.3f, 1.f));
	UOverlaySlot* BarSlot = Overlay->AddChildToOverlay(StaminaBar);
	BarSlot->SetHorizontalAlignment(HAlign_Fill);
	BarSlot->SetVerticalAlignment(VAlign_Fill);

	StaminaText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StaminaText"));
	FSlateFontInfo Font = StaminaText->GetFont();
	Font.Size = 12;
	StaminaText->SetFont(Font);
	StaminaText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	StaminaText->SetShadowOffset(FVector2D(1.f, 1.f));
	UOverlaySlot* TextSlot = Overlay->AddChildToOverlay(StaminaText);
	TextSlot->SetHorizontalAlignment(HAlign_Center);
	TextSlot->SetVerticalAlignment(VAlign_Center);
}

void UStaminaWidget::BindToPawn(APawn* Pawn)
{
	UnbindStamina();

	StaminaComponent = Pawn ? Pawn->FindComponentByClass<UStaminaComponent>() : nullptr;
	if (UStaminaComponent* Stamina = StaminaComponent.Get())
	{
		Stamina->OnStaminaChanged.AddUniqueDynamic(this, &UStaminaWidget::HandleStaminaChanged);
		HandleStaminaChanged(Stamina->GetStamina(), Stamina->GetMaxStamina());
	}
	else
	{
		HandleStaminaChanged(0.f, 1.f);
	}
}

void UStaminaWidget::UnbindStamina()
{
	if (UStaminaComponent* Stamina = StaminaComponent.Get())
	{
		Stamina->OnStaminaChanged.RemoveDynamic(this, &UStaminaWidget::HandleStaminaChanged);
	}
	StaminaComponent.Reset();
}

void UStaminaWidget::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	BindToPawn(NewPawn);
}

void UStaminaWidget::HandleStaminaChanged(float CurrentStamina, float MaxStamina)
{
	StaminaBar->SetPercent(MaxStamina > 0.f ? CurrentStamina / MaxStamina : 0.f);
	StaminaText->SetText(FText::Format(NSLOCTEXT("Stamina", "Value", "Stamina {0} / {1}"),
		FText::AsNumber(FMath::CeilToInt(CurrentStamina)), FText::AsNumber(FMath::RoundToInt(MaxStamina))));
}
