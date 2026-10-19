#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "CombatAbilitySystemComponent.generated.h"

class UGameplayAbility;
class UGameplayEffect;

/**
 * Ability System Component for the player and enemies. Add it to a pawn (BP component or C++ subobject):
 * it creates UCombatAttributeSet on InitializeComponent (before any BeginPlay, so UHealthComponent /
 * UStaminaComponent can initialise the attributes), then on BeginPlay grants DefaultAbilities and applies
 * DefaultEffects to itself.
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UCombatAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UCombatAbilitySystemComponent();

	virtual void InitializeComponent() override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TArray<TSubclassOf<UGameplayEffect>> DefaultEffects;
};
