#include "MovementModeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

namespace
{
	const FLinearColor ActiveColor(0.2f, 0.6f, 0.25f, 1.f);
	const FLinearColor InactiveColor(1.f, 1.f, 1.f, 1.f);
}

void UMovementModeWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildLayout();

	// Runs once per widget instance, so buttons are bound exactly once.
	WASDButton->OnClicked.AddUniqueDynamic(this, &UMovementModeWidget::HandleWASDClicked);
	PointAndClickButton->OnClicked.AddUniqueDynamic(this, &UMovementModeWidget::HandlePointAndClickClicked);
}

void UMovementModeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	const APlayerController* PC = GetOwningPlayer();
	MovementMode = PC ? PC->FindComponentByClass<UMovementModeComponent>() : nullptr;
	if (UMovementModeComponent* Mode = MovementMode.Get())
	{
		Mode->OnMovementModeChanged.AddUniqueDynamic(this, &UMovementModeWidget::HandleModeChanged);
	}

	RefreshHighlight();
}

void UMovementModeWidget::NativeDestruct()
{
	if (UMovementModeComponent* Mode = MovementMode.Get())
	{
		Mode->OnMovementModeChanged.RemoveDynamic(this, &UMovementModeWidget::HandleModeChanged);
	}
	MovementMode.Reset();

	Super::NativeDestruct();
}

void UMovementModeWidget::BuildLayout()
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

	// Bottom-left corner, sized to content.
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.f, 1.f));
	PanelSlot->SetAlignment(FVector2D(0.f, 1.f));
	PanelSlot->SetPosition(FVector2D(20.f, -20.f));
	PanelSlot->SetAutoSize(true);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
	Panel->SetContent(Row);

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Label"));
	Label->SetText(NSLOCTEXT("MovementMode", "Label", "Movement:"));
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = 16;
	Label->SetFont(Font);
	UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
	LabelSlot->SetVerticalAlignment(VAlign_Center);
	LabelSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

	WASDButton = MakeButton(TEXT("WASDButton"), NSLOCTEXT("MovementMode", "WASD", "WASD"));
	Row->AddChildToHorizontalBox(WASDButton)->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));

	PointAndClickButton = MakeButton(TEXT("PointAndClickButton"), NSLOCTEXT("MovementMode", "PointAndClick", "Point & Click"));
	Row->AddChildToHorizontalBox(PointAndClickButton);
}

UButton* UMovementModeWidget::MakeButton(const FString& Name, const FText& Label)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name + TEXT("Label")));
	Text->SetText(Label);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 16;
	Text->SetFont(Font);

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *Name);
	Button->AddChild(Text);
	return Button;
}

void UMovementModeWidget::RefreshHighlight()
{
	const UMovementModeComponent* Mode = MovementMode.Get();
	const bool bHasMode = Mode != nullptr;
	const bool bWASD = bHasMode && Mode->GetMovementMode() == EPlayerMovementMode::WASD;

	WASDButton->SetIsEnabled(bHasMode);
	PointAndClickButton->SetIsEnabled(bHasMode);
	WASDButton->SetBackgroundColor(bHasMode && bWASD ? ActiveColor : InactiveColor);
	PointAndClickButton->SetBackgroundColor(bHasMode && !bWASD ? ActiveColor : InactiveColor);
}

void UMovementModeWidget::SetMode(EPlayerMovementMode NewMode)
{
	if (UMovementModeComponent* Mode = MovementMode.Get())
	{
		Mode->SetMovementMode(NewMode);
	}
}

void UMovementModeWidget::HandleModeChanged(EPlayerMovementMode NewMode)
{
	RefreshHighlight();
}

void UMovementModeWidget::HandleWASDClicked()
{
	SetMode(EPlayerMovementMode::WASD);
}

void UMovementModeWidget::HandlePointAndClickClicked()
{
	SetMode(EPlayerMovementMode::PointAndClick);
}
