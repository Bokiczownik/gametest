#include "ComboWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CoinComboComponent.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UComboWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildLayout();
}

void UComboWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &UComboWidget::HandlePossessedPawnChanged);
		BindToPawn(PC->GetPawn());
	}
	else
	{
		RefreshMultiplier();
	}
}

void UComboWidget::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &UComboWidget::HandlePossessedPawnChanged);
	}
	UnbindCombo();

	Super::NativeDestruct();
}

void UComboWidget::BuildLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	Panel->SetPadding(FMargin(12.f));

	// Top-center, sized to content.
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.f));
	PanelSlot->SetPosition(FVector2D(0.f, 20.f));
	PanelSlot->SetAutoSize(true);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Panel->SetContent(Column);

	MultiplierText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MultiplierText"));
	FSlateFontInfo Font = MultiplierText->GetFont();
	Font.Size = 18;
	MultiplierText->SetFont(Font);
	MultiplierText->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* TextSlot = Column->AddChildToVerticalBox(MultiplierText);
	TextSlot->SetHorizontalAlignment(HAlign_Center);
	TextSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BarSize"));
	BarSize->SetWidthOverride(220.f);
	BarSize->SetHeightOverride(14.f);
	Column->AddChildToVerticalBox(BarSize);

	ComboBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ComboBar"));
	ComboBar->SetFillColorAndOpacity(FLinearColor(1.f, 0.75f, 0.1f, 1.f));
	ComboBar->SetPercent(0.f);
	BarSize->SetContent(ComboBar);
}

void UComboWidget::BindToPawn(APawn* Pawn)
{
	UnbindCombo();

	ComboComponent = Pawn ? Pawn->FindComponentByClass<UCoinComboComponent>() : nullptr;
	if (UCoinComboComponent* Combo = ComboComponent.Get())
	{
		Combo->OnMultiplierChanged.AddUniqueDynamic(this, &UComboWidget::HandleMultiplierChanged);
		Combo->OnProgressChanged.AddUniqueDynamic(this, &UComboWidget::HandleProgressChanged);
	}

	RefreshMultiplier();
	HandleProgressChanged(ComboComponent.IsValid() ? ComboComponent->GetComboProgress() : 0.f);
}

void UComboWidget::UnbindCombo()
{
	if (UCoinComboComponent* Combo = ComboComponent.Get())
	{
		Combo->OnMultiplierChanged.RemoveDynamic(this, &UComboWidget::HandleMultiplierChanged);
		Combo->OnProgressChanged.RemoveDynamic(this, &UComboWidget::HandleProgressChanged);
	}
	ComboComponent.Reset();
}

void UComboWidget::RefreshMultiplier()
{
	const UCoinComboComponent* Combo = ComboComponent.Get();
	const float Multiplier = Combo ? Combo->GetMultiplier() : 1.f;

	FNumberFormattingOptions Format;
	Format.MinimumFractionalDigits = 1;
	Format.MaximumFractionalDigits = 1;
	MultiplierText->SetText(FText::Format(NSLOCTEXT("Combo", "Multiplier", "Combo x{0}"), FText::AsNumber(Multiplier, &Format)));
}

void UComboWidget::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	BindToPawn(NewPawn);
}

void UComboWidget::HandleMultiplierChanged(float NewMultiplier)
{
	RefreshMultiplier();
}

void UComboWidget::HandleProgressChanged(float NewProgress)
{
	ComboBar->SetPercent(NewProgress);
}
