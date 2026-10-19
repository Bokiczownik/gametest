#include "CombatAbilitySystemComponent.h"

#include "Abilities/GameplayAbility.h"
#include "CombatAttributeSet.h"
#include "GameplayEffect.h"

UCombatAbilitySystemComponent::UCombatAbilitySystemComponent()
{
	bWantsInitializeComponent = true;
}

void UCombatAbilitySystemComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (!GetSet<UCombatAttributeSet>())
	{
		AddSet<UCombatAttributeSet>();
	}
}

void UCombatAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!IsOwnerActorAuthoritative())
	{
		return;
	}

	for (const TSubclassOf<UGameplayAbility>& Ability : DefaultAbilities)
	{
		if (Ability)
		{
			GiveAbility(FGameplayAbilitySpec(Ability));
		}
	}

	for (const TSubclassOf<UGameplayEffect>& Effect : DefaultEffects)
	{
		if (Effect)
		{
			ApplyGameplayEffectToSelf(Effect->GetDefaultObject<UGameplayEffect>(), 1.f, MakeEffectContext());
		}
	}
}
