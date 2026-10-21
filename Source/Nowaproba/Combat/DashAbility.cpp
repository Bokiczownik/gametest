#include "DashAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "CombatAim.h"
#include "CombatAttributeSet.h"
#include "CombatTags.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UDashAbility::UDashAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	CancelAbilitiesWithTag.AddTag(CombatTags::Ability_Attack);
	ActivationBlockedTags.AddTag(CombatTags::State_Staggered);
	ActivationBlockedTags.AddTag(CombatTags::State_Stunned);
}

float UDashAbility::GetDashCooldownProgress(const UAbilitySystemComponent* AbilitySystem)
{
	if (!AbilitySystem)
	{
		return 1.f;
	}

	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(CombatTags::Cooldown_Dash));
	float Progress = 1.f;
	for (const TPair<float, float>& RemainingAndDuration : AbilitySystem->GetActiveEffectsTimeRemainingAndDuration(Query))
	{
		if (RemainingAndDuration.Value > 0.f)
		{
			Progress = FMath::Min(Progress, 1.f - RemainingAndDuration.Key / RemainingAndDuration.Value);
		}
	}
	return FMath::Clamp(Progress, 0.f, 1.f);
}

bool UDashAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character || Character->GetCharacterMovement()->MovementMode == MOVE_None)
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	return StaminaCost <= 0.f || ASC->GetNumericAttribute(UCombatAttributeSet::GetStaminaAttribute()) >= StaminaCost;
}

void UDashAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// Cooldown is committed in EndAbility, so it counts from the end of the dash.
	if (!CommitAbilityCost(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	ACharacter* Character = CastChecked<ACharacter>(ActorInfo->AvatarActor.Get());

	if (StaminaCost > 0.f)
	{
		ASC->ApplyModToAttribute(UCombatAttributeSet::GetStaminaAttribute(), EGameplayModOp::Additive, -StaminaCost);
	}

	ASC->AddLooseGameplayTag(CombatTags::State_Invulnerable);
	bInvulnerabilityAdded = true;

	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	SavedPawnResponse = Capsule->GetCollisionResponseToChannel(ECC_Pawn);
	Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	const FVector Direction = GetDashDirection(ActorInfo);
	Character->SetActorRotation(Direction.Rotation());

	UAbilityTask_ApplyRootMotionConstantForce* Task = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this, TEXT("Dash"), Direction, DashDistance / DashDuration, DashDuration, false, nullptr,
		ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, Character->GetCharacterMovement()->MaxWalkSpeed, false);
	Task->OnFinish.AddDynamic(this, &UDashAbility::HandleDashFinished);
	Task->ReadyForActivation();
}

void UDashAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bInvulnerabilityAdded) // the dash actually started
	{
		CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, true);
		bInvulnerabilityAdded = false;
		ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(CombatTags::State_Invulnerable);
		if (const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
		{
			Character->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, SavedPawnResponse);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

const FGameplayTagContainer* UDashAbility::GetCooldownTags() const
{
	static const FGameplayTagContainer Tags(CombatTags::Cooldown_Dash);
	return &Tags;
}

void UDashAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (CooldownDuration <= 0.f)
	{
		return;
	}

	if (!CooldownEffect)
	{
		CooldownEffect = NewObject<UGameplayEffect>(const_cast<UDashAbility*>(this), TEXT("GE_DashCooldown"));
		CooldownEffect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
		FInheritedTagContainer GrantedTags;
		GrantedTags.Added.AddTag(CombatTags::Cooldown_Dash);
		CooldownEffect->FindOrAddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(GrantedTags);
	}
	CooldownEffect->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(CooldownDuration));

	// Not ApplyGameplayEffectToOwner: it builds the spec from the effect's class (CDO), which would drop this runtime instance.
	const FGameplayEffectSpecHandle Spec(new FGameplayEffectSpec(CooldownEffect, MakeEffectContext(Handle, ActorInfo), GetAbilityLevel(Handle, ActorInfo)));
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
}

FVector UDashAbility::GetDashDirection(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const ACharacter* Character = CastChecked<ACharacter>(ActorInfo->AvatarActor.Get());

	FVector Direction = Character->GetLastMovementInputVector().GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		CombatAim::GetCursorDirection(Cast<APlayerController>(Character->GetController()), Character->GetActorLocation(), Direction);
	}
	return Direction.IsNearlyZero() ? Character->GetActorForwardVector().GetSafeNormal2D() : Direction;
}

void UDashAbility::HandleDashFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
