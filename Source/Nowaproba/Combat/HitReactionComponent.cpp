#include "HitReactionComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "BlockComponent.h"
#include "CombatAttributeSet.h"
#include "CombatTags.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "../Health/HealthComponent.h"

void UHitReactionComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = GetOwner()->FindComponentByClass<UHealthComponent>();
	if (Health)
	{
		LastHealth = Health->GetHealth();
		Health->OnHealthChanged.AddUniqueDynamic(this, &UHitReactionComponent::HandleHealthChanged);
	}

	Block = GetOwner()->FindComponentByClass<UBlockComponent>();

	AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (AbilitySystem && AbilitySystem->HasAttributeSetForAttribute(UCombatAttributeSet::GetPoiseAttribute()))
	{
		AbilitySystem->GetGameplayAttributeValueChangeDelegate(UCombatAttributeSet::GetPoiseAttribute()).AddUObject(this, &UHitReactionComponent::HandlePoiseChanged);
	}
	else
	{
		AbilitySystem = nullptr;
	}
}

void UHitReactionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	Super::EndPlay(EndPlayReason);
}

void UHitReactionComponent::ApplyKnockback(const FVector& Velocity)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || KnockbackScale <= 0.f || (Health && Health->IsDead())
		|| (AbilitySystem && (AbilitySystem->HasMatchingGameplayTag(CombatTags::State_Invulnerable)
			|| AbilitySystem->HasMatchingGameplayTag(CombatTags::State_StaggerImmune)))
		|| (Block && Block->IsBlockingFrom(Character->GetActorLocation() - Velocity)))
	{
		return;
	}
	const FVector Push = FVector(Velocity.X, Velocity.Y, 0.f) * KnockbackScale;
	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (bGroundedKnockback && Movement->IsMovingOnGround())
	{
		Movement->AddImpulse(Push, true);
	}
	else
	{
		Character->LaunchCharacter(Push, true, false);
	}
}

void UHitReactionComponent::HandleHealthChanged(float CurrentHealth, float MaxHealth)
{
	const bool bDamaged = CurrentHealth < LastHealth;
	LastHealth = CurrentHealth;

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!bDamaged || !HitFlashMaterial || !Character)
	{
		return;
	}

	Character->GetMesh()->SetOverlayMaterial(HitFlashMaterial);
	GetWorld()->GetTimerManager().SetTimer(FlashTimer, this, &UHitReactionComponent::EndFlash, HitFlashDuration);
}

void UHitReactionComponent::EndFlash()
{
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetMesh()->SetOverlayMaterial(nullptr);
	}
}

void UHitReactionComponent::HandlePoiseChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue >= Data.OldValue)
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(PoiseRegenTimer, this, &UHitReactionComponent::RefillPoise, PoiseRegenDelay);
	if (Data.NewValue <= 0.f && !bStaggered && !(Health && Health->IsDead()))
	{
		StartStagger();
	}
}

void UHitReactionComponent::StartStagger()
{
	bStaggered = true;
	GetWorld()->GetTimerManager().ClearTimer(PoiseRegenTimer);

	AbilitySystem->AddLooseGameplayTag(CombatTags::State_Staggered);
	Incapacitate();
	GetWorld()->GetTimerManager().SetTimer(StaggerTimer, this, &UHitReactionComponent::EndStagger, StaggerDuration);
}

void UHitReactionComponent::ApplyStun(float Duration)
{
	if (bStunned || Duration <= 0.f || !AbilitySystem || (Health && Health->IsDead())
		|| AbilitySystem->HasMatchingGameplayTag(CombatTags::State_Invulnerable))
	{
		return;
	}

	bStunned = true;
	AbilitySystem->AddLooseGameplayTag(CombatTags::State_Stunned);
	Incapacitate();
	GetWorld()->GetTimerManager().SetTimer(StunTimer, this, &UHitReactionComponent::EndStun, Duration);
}

void UHitReactionComponent::EndStun()
{
	bStunned = false;
	AbilitySystem->RemoveLooseGameplayTag(CombatTags::State_Stunned);
	ReleaseMoveInput();
}

void UHitReactionComponent::Incapacitate()
{
	const FGameplayTagContainer AttackTags(CombatTags::Ability_Attack);
	AbilitySystem->CancelAbilities(&AttackTags);

	// Before the stagger animation, so owners can stop their own montages (e.g. an enemy attack) first.
	OnStaggered.Broadcast();

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Character)
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			PC->SetIgnoreMoveInput(true);
			++MoveInputLocks;
		}
		else if (AAIController* AI = Cast<AAIController>(Character->GetController()))
		{
			AI->StopMovement();
		}

		UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
		if (StaggerAnimation && AnimInstance)
		{
			AnimInstance->PlaySlotAnimationAsDynamicMontage(StaggerAnimation, TEXT("DefaultSlot"), 0.1f, 0.2f);
		}
	}
}

void UHitReactionComponent::ReleaseMoveInput()
{
	if (MoveInputLocks <= 0)
	{
		return;
	}
	--MoveInputLocks;
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (AController* Controller = Pawn->GetController())
		{
			Controller->SetIgnoreMoveInput(false);
		}
	}
}

void UHitReactionComponent::EndStagger()
{
	bStaggered = false;
	AbilitySystem->RemoveLooseGameplayTag(CombatTags::State_Staggered);

	ReleaseMoveInput();
	RefillPoise();
	if (StaggerCooldown > 0.f)
	{
		AbilitySystem->AddLooseGameplayTag(CombatTags::State_StaggerImmune);
		GetWorld()->GetTimerManager().SetTimer(StaggerCooldownTimer, this, &UHitReactionComponent::EndStaggerCooldown, StaggerCooldown);
	}
}

void UHitReactionComponent::EndStaggerCooldown()
{
	AbilitySystem->RemoveLooseGameplayTag(CombatTags::State_StaggerImmune);
}

void UHitReactionComponent::RefillPoise()
{
	if (AbilitySystem)
	{
		AbilitySystem->SetNumericAttributeBase(UCombatAttributeSet::GetPoiseAttribute(), AbilitySystem->GetNumericAttribute(UCombatAttributeSet::GetMaxPoiseAttribute()));
	}
}
