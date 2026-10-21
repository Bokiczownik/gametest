#include "EnemyHealthBarWidget.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "../Combat/CombatAttributeSet.h"
#include "../Health/HealthComponent.h"

namespace
{
	const FVector2D EnemyBarSize(50.f, 4.f);
}

void UEnemyHealthBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
}

void UEnemyHealthBarWidget::BuildLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	auto AddBar = [this, Root](const TCHAR* Name, const FLinearColor& Fill) -> UProgressBar*
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Size->SetWidthOverride(EnemyBarSize.X);
		Size->SetHeightOverride(EnemyBarSize.Y);
		UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), Name);
		Bar->SetFillColorAndOpacity(Fill);
		Size->SetContent(Bar);
		UVerticalBoxSlot* BarSlot = Root->AddChildToVerticalBox(Size);
		BarSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 1.f));
		return Bar;
	};
	BlockBar = AddBar(TEXT("BlockBar"), FLinearColor(0.55f, 0.75f, 1.f, 1.f));
	HealthBar = AddBar(TEXT("HealthBar"), FLinearColor(0.85f, 0.12f, 0.12f, 1.f));
	// Hidden (not collapsed) keeps the HP bar in place while there is no block.
	BlockBar->GetParent()->SetVisibility(ESlateVisibility::Hidden);
}

void UEnemyHealthBarWidget::Bind(UHealthComponent* InHealth, UAbilitySystemComponent* InAbilitySystem)
{
	Health = InHealth;
	if (InHealth)
	{
		InHealth->OnHealthChanged.AddUniqueDynamic(this, &UEnemyHealthBarWidget::HandleHealthChanged);
		HandleHealthChanged(InHealth->GetHealth(), InHealth->GetMaxHealth());
	}

	if (InAbilitySystem && InAbilitySystem->HasAttributeSetForAttribute(UCombatAttributeSet::GetBlockAttribute()))
	{
		AbilitySystem = InAbilitySystem;
		BlockChangedHandle = InAbilitySystem->GetGameplayAttributeValueChangeDelegate(UCombatAttributeSet::GetBlockAttribute()).AddUObject(this, &UEnemyHealthBarWidget::HandleBlockChanged);
	}
	UpdateBlock();
}

void UEnemyHealthBarWidget::NativeDestruct()
{
	if (UHealthComponent* HealthComponent = Health.Get())
	{
		HealthComponent->OnHealthChanged.RemoveDynamic(this, &UEnemyHealthBarWidget::HandleHealthChanged);
	}
	if (UAbilitySystemComponent* ASC = AbilitySystem.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UCombatAttributeSet::GetBlockAttribute()).Remove(BlockChangedHandle);
	}
	Super::NativeDestruct();
}

void UEnemyHealthBarWidget::HandleHealthChanged(float CurrentHealth, float MaxHealth)
{
	HealthBar->SetPercent(MaxHealth > 0.f ? CurrentHealth / MaxHealth : 0.f);
}

void UEnemyHealthBarWidget::HandleBlockChanged(const FOnAttributeChangeData& Data)
{
	UpdateBlock();
}

void UEnemyHealthBarWidget::UpdateBlock()
{
	const UAbilitySystemComponent* ASC = AbilitySystem.Get();
	const float Block = ASC ? ASC->GetNumericAttribute(UCombatAttributeSet::GetBlockAttribute()) : 0.f;
	const float MaxBlock = ASC ? ASC->GetNumericAttribute(UCombatAttributeSet::GetMaxBlockAttribute()) : 0.f;
	BlockBar->GetParent()->SetVisibility(Block > 0.f && MaxBlock > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	BlockBar->SetPercent(MaxBlock > 0.f ? Block / MaxBlock : 0.f);
}
