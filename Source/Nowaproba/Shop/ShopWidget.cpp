#include "ShopWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UShopWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildLayout();
	// Runs once per widget instance, so the button is bound exactly once.
	CloseButton->OnClicked.AddUniqueDynamic(this, &UShopWidget::HandleCloseClicked);
}

void UShopWidget::BuildLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	auto MakeText = [this](const TCHAR* Name, const FText& Text, int32 Size)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TextBlock->SetText(Text);
		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = Size;
		TextBlock->SetFont(Font);
		TextBlock->SetJustification(ETextJustify::Center);
		return TextBlock;
	};

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f));
	Panel->SetPadding(FMargin(24.f));

	// Screen center, sized to content.
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetAutoSize(true);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Panel->SetContent(Column);

	Column->AddChildToVerticalBox(MakeText(TEXT("Title"), NSLOCTEXT("Shop", "Title", "SHOP"), 28))
		->SetHorizontalAlignment(HAlign_Center);

	UVerticalBoxSlot* InfoSlot = Column->AddChildToVerticalBox(
		MakeText(TEXT("Info"), NSLOCTEXT("Shop", "Info", "Gear and upgrades coming soon."), 16));
	InfoSlot->SetHorizontalAlignment(HAlign_Center);
	InfoSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 16.f));

	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
	CloseButton->AddChild(MakeText(TEXT("CloseLabel"), NSLOCTEXT("Shop", "Close", "Close"), 16));
	Column->AddChildToVerticalBox(CloseButton)->SetHorizontalAlignment(HAlign_Center);
}

void UShopWidget::HandleCloseClicked()
{
	RemoveFromParent();
}
