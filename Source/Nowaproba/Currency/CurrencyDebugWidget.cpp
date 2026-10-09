#include "CurrencyDebugWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "CurrencyComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UCurrencyDebugWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildLayout();

	// Runs once per widget instance, so buttons are bound exactly once.
	AddButton->OnClicked.AddUniqueDynamic(this, &UCurrencyDebugWidget::HandleAddClicked);
	RemoveButton->OnClicked.AddUniqueDynamic(this, &UCurrencyDebugWidget::HandleRemoveClicked);
}

void UCurrencyDebugWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &UCurrencyDebugWidget::HandlePossessedPawnChanged);
		BindToPawn(PC->GetPawn());
	}
	else
	{
		RefreshBalance();
	}
}

void UCurrencyDebugWidget::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &UCurrencyDebugWidget::HandlePossessedPawnChanged);
	}
	UnbindCurrency();

	Super::NativeDestruct();
}

void UCurrencyDebugWidget::BuildLayout()
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

	// Top-right corner, sized to content.
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(1.f, 0.f));
	PanelSlot->SetAlignment(FVector2D(1.f, 0.f));
	PanelSlot->SetPosition(FVector2D(-20.f, 20.f));
	PanelSlot->SetAutoSize(true);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Panel->SetContent(Column);

	auto MakeText = [this](FName Name, const FText& Text, int32 Size)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TextBlock->SetText(Text);
		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = Size;
		TextBlock->SetFont(Font);
		return TextBlock;
	};

	auto MakeButton = [this, &MakeText](FName Name, const FText& Label)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Button->AddChild(MakeText(*(Name.ToString() + TEXT("Label")), Label, 14));
		return Button;
	};

	Column->AddChildToVerticalBox(MakeText(TEXT("WalletLabel"), NSLOCTEXT("Currency", "Wallet", "Wallet"), 18));

	BalanceText = MakeText(TEXT("BalanceText"), FText::AsNumber(0), 24);
	Column->AddChildToVerticalBox(BalanceText)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

	// TEMPORARY debug controls.
	AddButton = MakeButton(TEXT("DebugAddButton"), NSLOCTEXT("Currency", "DebugAdd", "+100 Currency"));
	Column->AddChildToVerticalBox(AddButton)->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));

	RemoveButton = MakeButton(TEXT("DebugRemoveButton"), NSLOCTEXT("Currency", "DebugRemove", "-100 Currency"));
	Column->AddChildToVerticalBox(RemoveButton);
}

void UCurrencyDebugWidget::BindToPawn(APawn* Pawn)
{
	UnbindCurrency();

	CurrencyComponent = Pawn ? Pawn->FindComponentByClass<UCurrencyComponent>() : nullptr;
	if (UCurrencyComponent* Currency = CurrencyComponent.Get())
	{
		Currency->OnBalanceChanged.AddUniqueDynamic(this, &UCurrencyDebugWidget::HandleBalanceChanged);
	}

	// Reflects any balance change that happened before the widget appeared.
	RefreshBalance();
}

void UCurrencyDebugWidget::UnbindCurrency()
{
	if (UCurrencyComponent* Currency = CurrencyComponent.Get())
	{
		Currency->OnBalanceChanged.RemoveDynamic(this, &UCurrencyDebugWidget::HandleBalanceChanged);
	}
	CurrencyComponent.Reset();
}

void UCurrencyDebugWidget::RefreshBalance()
{
	if (BalanceText)
	{
		const UCurrencyComponent* Currency = CurrencyComponent.Get();
		BalanceText->SetText(FText::AsNumber(Currency ? Currency->GetBalance() : 0));
	}
}

void UCurrencyDebugWidget::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	BindToPawn(NewPawn);
}

void UCurrencyDebugWidget::HandleBalanceChanged(int32 NewBalance, int32 Delta)
{
	RefreshBalance();
}

void UCurrencyDebugWidget::HandleAddClicked()
{
	if (UCurrencyComponent* Currency = CurrencyComponent.Get())
	{
		Currency->AddCurrency(DebugAmount);
	}
}

void UCurrencyDebugWidget::HandleRemoveClicked()
{
	if (UCurrencyComponent* Currency = CurrencyComponent.Get())
	{
		Currency->RemoveCurrency(DebugAmount);
	}
}
