#include "ResourceDebugWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UResourceDebugWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildLayout();

	// Runs once per widget instance, so buttons are bound exactly once.
	AddButtons[0]->OnClicked.AddUniqueDynamic(this, &UResourceDebugWidget::HandleAdd1Clicked);
	RemoveButtons[0]->OnClicked.AddUniqueDynamic(this, &UResourceDebugWidget::HandleRemove1Clicked);
	AddButtons[1]->OnClicked.AddUniqueDynamic(this, &UResourceDebugWidget::HandleAdd2Clicked);
	RemoveButtons[1]->OnClicked.AddUniqueDynamic(this, &UResourceDebugWidget::HandleRemove2Clicked);
	AddButtons[2]->OnClicked.AddUniqueDynamic(this, &UResourceDebugWidget::HandleAdd3Clicked);
	RemoveButtons[2]->OnClicked.AddUniqueDynamic(this, &UResourceDebugWidget::HandleRemove3Clicked);
}

void UResourceDebugWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &UResourceDebugWidget::HandlePossessedPawnChanged);
		BindToPawn(PC->GetPawn());
	}
	else
	{
		RefreshAll();
	}
}

void UResourceDebugWidget::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &UResourceDebugWidget::HandlePossessedPawnChanged);
	}
	UnbindResources();

	Super::NativeDestruct();
}

void UResourceDebugWidget::BuildLayout()
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

	// Top-left corner, sized to content.
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetPosition(FVector2D(20.f, 20.f));
	PanelSlot->SetAutoSize(true);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Panel->SetContent(Column);

	for (int32 Index = 0; Index < UResourceComponent::NumResources; ++Index)
	{
		AddRow(Column, Index);
	}
}

void UResourceDebugWidget::AddRow(UVerticalBox* Column, int32 Index)
{
	auto MakeText = [this](const FString& Name, const FText& Text)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *Name);
		TextBlock->SetText(Text);
		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = 16;
		TextBlock->SetFont(Font);
		return TextBlock;
	};

	auto MakeButton = [this, &MakeText](const FString& Name, const FText& Label)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *Name);
		Button->AddChild(MakeText(Name + TEXT("Label"), Label));
		return Button;
	};

	const FString Suffix = FString::FromInt(Index + 1);
	const EResourceType Resource = static_cast<EResourceType>(Index);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(TEXT("Row") + Suffix));
	Column->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));

	const FText Label = FText::Format(NSLOCTEXT("Resources", "RowLabel", "{0}:"), UEnum::GetDisplayValueAsText(Resource));
	Row->AddChildToHorizontalBox(MakeText(TEXT("Label") + Suffix, Label))->SetVerticalAlignment(VAlign_Center);

	ValueTexts[Index] = MakeText(TEXT("Value") + Suffix, FText::AsNumber(0));
	UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(ValueTexts[Index]);
	ValueSlot->SetVerticalAlignment(VAlign_Center);
	ValueSlot->SetPadding(FMargin(8.f, 0.f, 16.f, 0.f));

	AddButtons[Index] = MakeButton(TEXT("AddButton") + Suffix, NSLOCTEXT("Resources", "DebugAdd", "+10"));
	Row->AddChildToHorizontalBox(AddButtons[Index])->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));

	RemoveButtons[Index] = MakeButton(TEXT("RemoveButton") + Suffix, NSLOCTEXT("Resources", "DebugRemove", "-10"));
	Row->AddChildToHorizontalBox(RemoveButtons[Index]);
}

void UResourceDebugWidget::BindToPawn(APawn* Pawn)
{
	UnbindResources();

	ResourceComponent = Pawn ? Pawn->FindComponentByClass<UResourceComponent>() : nullptr;
	if (UResourceComponent* Resources = ResourceComponent.Get())
	{
		Resources->OnResourceChanged.AddUniqueDynamic(this, &UResourceDebugWidget::HandleResourceChanged);
	}

	// Reflects any change that happened before the widget appeared.
	RefreshAll();
}

void UResourceDebugWidget::UnbindResources()
{
	if (UResourceComponent* Resources = ResourceComponent.Get())
	{
		Resources->OnResourceChanged.RemoveDynamic(this, &UResourceDebugWidget::HandleResourceChanged);
	}
	ResourceComponent.Reset();
}

void UResourceDebugWidget::RefreshValue(EResourceType Resource)
{
	const int32 Index = static_cast<int32>(Resource);
	if (Index < UResourceComponent::NumResources && ValueTexts[Index])
	{
		const UResourceComponent* Resources = ResourceComponent.Get();
		ValueTexts[Index]->SetText(FText::AsNumber(Resources ? Resources->GetResource(Resource) : 0));
	}
}

void UResourceDebugWidget::RefreshAll()
{
	for (int32 Index = 0; Index < UResourceComponent::NumResources; ++Index)
	{
		RefreshValue(static_cast<EResourceType>(Index));
	}
}

void UResourceDebugWidget::ChangeResource(EResourceType Resource, int32 Amount)
{
	if (UResourceComponent* Resources = ResourceComponent.Get())
	{
		if (Amount >= 0)
		{
			Resources->AddResource(Resource, Amount);
		}
		else
		{
			Resources->RemoveResource(Resource, -Amount);
		}
	}
}

void UResourceDebugWidget::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	BindToPawn(NewPawn);
}

void UResourceDebugWidget::HandleResourceChanged(EResourceType Resource, int32 NewValue, int32 Delta)
{
	RefreshValue(Resource);
}

void UResourceDebugWidget::HandleAdd1Clicked()
{
	ChangeResource(EResourceType::Resource1, DebugAmount);
}

void UResourceDebugWidget::HandleRemove1Clicked()
{
	ChangeResource(EResourceType::Resource1, -DebugAmount);
}

void UResourceDebugWidget::HandleAdd2Clicked()
{
	ChangeResource(EResourceType::Resource2, DebugAmount);
}

void UResourceDebugWidget::HandleRemove2Clicked()
{
	ChangeResource(EResourceType::Resource2, -DebugAmount);
}

void UResourceDebugWidget::HandleAdd3Clicked()
{
	ChangeResource(EResourceType::Resource3, DebugAmount);
}

void UResourceDebugWidget::HandleRemove3Clicked()
{
	ChangeResource(EResourceType::Resource3, -DebugAmount);
}
