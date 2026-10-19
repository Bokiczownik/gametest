#include "WeaponAttackAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "CombatAttributeSet.h"
#include "CombatTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "WeaponComponent.h"

UWeaponAttackAbility::UWeaponAttackAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bRetriggerInstancedAbility = true;
	SetAssetTags(FGameplayTagContainer(CombatTags::Ability_Attack));
	ActivationBlockedTags.AddTag(CombatTags::State_Invulnerable);
}

UWeaponComponent* UWeaponAttackAbility::FindWeapon(const FGameplayAbilityActorInfo* ActorInfo)
{
	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	return Avatar ? Avatar->FindComponentByClass<UWeaponComponent>() : nullptr;
}

bool UWeaponAttackAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	const UWeaponComponent* WeaponComponent = FindWeapon(ActorInfo);
	const FWeaponAttack* Attack = WeaponComponent ? WeaponComponent->GetPendingAttack() : nullptr;
	if (!Character || !Attack || Character->GetCharacterMovement()->MovementMode == MOVE_None)
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	return Attack->StaminaCost <= 0.f || ASC->GetNumericAttribute(UCombatAttributeSet::GetStaminaAttribute()) >= Attack->StaminaCost;
}

void UWeaponAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UWeaponComponent* WeaponComponent = FindWeapon(ActorInfo);
	const FWeaponAttack* Attack = WeaponComponent ? WeaponComponent->GetPendingAttack() : nullptr;
	if (!Attack || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Weapon = WeaponComponent;
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	ACharacter* Character = CastChecked<ACharacter>(ActorInfo->AvatarActor.Get());

	if (Attack->StaminaCost > 0.f)
	{
		ASC->ApplyModToAttribute(UCombatAttributeSet::GetStaminaAttribute(), EGameplayModOp::Additive, -Attack->StaminaCost);
	}

	const FVector AttackDirection = FaceCursor(Character, WeaponComponent->GetWeaponData()->AttackFacingYawOffset);
	if (AController* Controller = Character->GetController())
	{
		Controller->SetIgnoreMoveInput(true);
		bIgnoringMoveInput = true;
	}

	const float AnimLength = Attack->Animation->GetPlayLength();
	const float EndTime = Attack->EndTime > Attack->StartTime ? FMath::Min(Attack->EndTime, AnimLength) : AnimLength;
	UAnimMontage* Montage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(Attack->Animation, TEXT("DefaultSlot"), 0.1f, 0.2f, 1.f);
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, TEXT("Attack"), Montage, Attack->PlayRate, NAME_None, true, 1.f, Attack->StartTime);
	MontageTask->OnCompleted.AddDynamic(this, &UWeaponAttackAbility::HandleAttackFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &UWeaponAttackAbility::HandleAttackFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UWeaponAttackAbility::HandleAttackFinished);
	MontageTask->OnCancelled.AddDynamic(this, &UWeaponAttackAbility::HandleAttackFinished);
	MontageTask->ReadyForActivation();

	// The attack usually ends before the animation does.
	Character->GetWorldTimerManager().SetTimer(EndTimer, this, &UWeaponAttackAbility::HandleAttackFinished, (EndTime - Attack->StartTime) / Attack->PlayRate);

	if (Attack->LungeDistance > 0.f && !Character->GetCharacterMovement()->IsFalling())
	{
		UAbilityTask_ApplyRootMotionConstantForce* Lunge = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
			this, TEXT("Lunge"), AttackDirection, Attack->LungeDistance / Attack->LungeDuration, Attack->LungeDuration,
			false, nullptr, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.f, true);
		Lunge->ReadyForActivation();
	}

	WeaponComponent->NotifyAttackStarted(*Attack);
}

void UWeaponAttackAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (const AActor* Avatar = ActorInfo->AvatarActor.Get())
	{
		Avatar->GetWorldTimerManager().ClearTimer(EndTimer);
	}

	if (bIgnoringMoveInput)
	{
		bIgnoringMoveInput = false;
		if (const APawn* Pawn = Cast<APawn>(ActorInfo->AvatarActor.Get()))
		{
			if (AController* Controller = Pawn->GetController())
			{
				Controller->SetIgnoreMoveInput(false);
			}
		}
	}

	if (UWeaponComponent* WeaponComponent = Weapon.Get())
	{
		Weapon.Reset();
		WeaponComponent->NotifyAttackEnded();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FVector UWeaponAttackAbility::FaceCursor(ACharacter* Character, float YawOffset) const
{
	FVector Direction = Character->GetActorForwardVector().RotateAngleAxis(-YawOffset, FVector::UpVector).GetSafeNormal2D();

	const APlayerController* PC = Cast<APlayerController>(Character->GetController());
	FHitResult Hit;
	if (PC && PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		const FVector ToCursor = (Hit.ImpactPoint - Character->GetActorLocation()).GetSafeNormal2D();
		if (!ToCursor.IsNearlyZero())
		{
			Direction = ToCursor;
		}
	}

	Character->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw + YawOffset, 0.f));
	return Direction;
}

void UWeaponAttackAbility::HandleAttackFinished()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}
