#include "ShopWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerController.h"

void UShopWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildLayout();
	SetIsFocusable(true);
	// Runs once per widget instance, so the button is bound exactly once.
	CloseButton->OnClicked.AddUniqueDynamic(this, &UShopWidget::HandleCloseClicked);
}

void UShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		bPreviousShowMouseCursor = PC->bShowMouseCursor;
		PC->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
}

void UShopWidget::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = bPreviousShowMouseCursor;

		// Game-only mode uses engine capture defaults; put back the project's viewport mouse settings.
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			const UInputSettings* Settings = GetDefault<UInputSettings>();
			Viewport->SetMouseCaptureMode(Settings->DefaultViewportMouseCaptureMode);
			Viewport->SetMouseLockMode(Settings->DefaultViewportMouseLockMode);
		}
	}

	Super::NativeDestruct();
}

FReply UShopWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Game input is paused while the shop has focus, so the interact key and Esc are handled here.
	// Ignore auto-repeat so holding the E that opened the shop does not close it again.
	if (!InKeyEvent.IsRepeat() && (InKeyEvent.GetKey() == EKeys::E || InKeyEvent.GetKey() == EKeys::Escape))
	{
		RemoveFromParent();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
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
