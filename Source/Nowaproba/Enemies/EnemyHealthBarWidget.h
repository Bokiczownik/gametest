#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHealthBarWidget.generated.h"

class UAbilitySystemComponent;
class UHealthComponent;
class UProgressBar;
struct FOnAttributeChangeData;

/**
 * Small bar above an enemy's head (shown by AEnemyCharacter's HealthBar widget component): thin HP bar, with a block
 * bar (UCombatAttributeSet Block / MaxBlock) above it only while Block > 0. Layout is built in C++.
 */
UCLASS()
class NOWAPROBA_API UEnemyHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Binds to the enemy's health and (if any) combat attributes; call once after the widget is created. */
	void Bind(UHealthComponent* InHealth, UAbilitySystemComponent* InAbilitySystem);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY()
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY()
	TObjectPtr<UProgressBar> BlockBar;

	TWeakObjectPtr<UHealthComponent> Health;
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
	FDelegateHandle BlockChangedHandle;

	void BuildLayout();
	void UpdateBlock();
	void HandleBlockChanged(const FOnAttributeChangeData& Data);

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);
};
