#include "EnemyAttackComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "../Combat/CombatAttributeSet.h"
#include "../Combat/HitReactionComponent.h"

namespace
{
	const FName AttackSlot = TEXT("DefaultSlot");
}

UEnemyAttackComponent::UEnemyAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Quick jab and a slow, wide heavy blow with a long recovery to punish.
	FEnemyAttack& Quick = Attacks.AddDefaulted_GetRef();
	Quick.Damage = 10.f;
	Quick.PoiseDamage = 15.f;
	Quick.Knockback = 250.f;
	Quick.Reach = 90.f;
	Quick.Radius = 75.f;
	Quick.WindupTime = 0.45f;
	Quick.RecoveryTime = 0.6f;
	Quick.Weight = 2.f;

	FEnemyAttack& Heavy = Attacks.AddDefaulted_GetRef();
	Heavy.Damage = 25.f;
	Heavy.PoiseDamage = 45.f;
	Heavy.Knockback = 650.f;
	Heavy.Reach = 130.f;
	Heavy.Radius = 120.f;
	Heavy.WindupTime = 1.1f;
	Heavy.RecoveryTime = 1.6f;
	Heavy.Weight = 1.f;
}

float UEnemyAttackComponent::GetAttackRange() const
{
	float Range = 0.f;
	for (const FEnemyAttack& Attack : Attacks)
	{
		Range = FMath::Max(Range, Attack.GetRange());
	}
	return Range;
}

void UEnemyAttackComponent::SetTelegraph(UPrimitiveComponent* InTelegraph)
{
	Telegraph = InTelegraph;
	SetTelegraphVisible(false);
}

int32 UEnemyAttackComponent::PickAttack(float Distance) const
{
	float TotalWeight = 0.f;
	for (const FEnemyAttack& Attack : Attacks)
	{
		if (Distance <= Attack.GetRange())
		{
			TotalWeight += Attack.Weight;
		}
	}
	if (TotalWeight <= 0.f)
	{
		return INDEX_NONE;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	int32 Last = INDEX_NONE;
	for (int32 i = 0; i < Attacks.Num(); ++i)
	{
		if (Distance <= Attacks[i].GetRange() && Attacks[i].Weight > 0.f)
		{
			Last = i;
			Roll -= Attacks[i].Weight;
			if (Roll <= 0.f)
			{
				return i;
			}
		}
	}
	return Last;
}

void UEnemyAttackComponent::StartAttack(AActor* Target)
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (bAttacking || !Target || !Owner)
	{
		return;
	}

	FVector ToTarget = Target->GetActorLocation() - Owner->GetActorLocation();
	ToTarget.Z = 0.f;
	CurrentAttackIndex = PickAttack(ToTarget.Size());
	if (CurrentAttackIndex == INDEX_NONE)
	{
		return;
	}
	const FEnemyAttack& Attack = Attacks[CurrentAttackIndex];

	bAttacking = true;
	CurrentTarget = Target;

	// Commit to a direction: the hit area stays where it was telegraphed.
	if (!ToTarget.IsNearlyZero())
	{
		Owner->SetActorRotation(ToTarget.Rotation());
	}

	if (Telegraph)
	{
		// Owner origin is the capsule centre; the telegraph sits on the ground under the hit sphere.
		const float HalfHeight = Owner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Telegraph->SetRelativeLocation(FVector(Attack.Reach, 0.f, -HalfHeight + TelegraphGroundOffset));
		Telegraph->SetRelativeScale3D(FVector(Attack.Radius / 50.f, Attack.Radius / 50.f, Telegraph->GetRelativeScale3D().Z));
	}
	SetTelegraphVisible(true);

	UAnimInstance* AnimInstance = Owner->GetMesh()->GetAnimInstance();
	if (Attack.Animation && AnimInstance)
	{
		// Start early enough (or late, if the windup is shorter) that the hit frame lands on the strike.
		const float StartTime = FMath::Max(0.f, Attack.StrikeAnimTime - Attack.WindupTime);
		const float PlayRate = FMath::Max(0.1f, (Attack.StrikeAnimTime - StartTime) / Attack.WindupTime);
		AnimInstance->PlaySlotAnimationAsDynamicMontage(Attack.Animation, AttackSlot, 0.1f, 0.2f, PlayRate, 1, -1.f, StartTime);
	}

	Owner->GetWorldTimerManager().SetTimer(AttackTimer, this, &UEnemyAttackComponent::Strike, Attack.WindupTime, false);
}

void UEnemyAttackComponent::CancelAttack()
{
	if (AActor* Owner = GetOwner())
	{
		Owner->GetWorldTimerManager().ClearTimer(AttackTimer);
	}
	if (bAttacking)
	{
		StopAttackAnimation();
	}
	SetTelegraphVisible(false);
	CurrentTarget.Reset();
	CurrentAttackIndex = INDEX_NONE;
	bAttacking = false;
}

void UEnemyAttackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelAttack();
	Super::EndPlay(EndPlayReason);
}

void UEnemyAttackComponent::Strike()
{
	AActor* Owner = GetOwner();
	SetTelegraphVisible(false);

	AActor* Target = CurrentTarget.Get();
	if (Owner && Target && Attacks.IsValidIndex(CurrentAttackIndex))
	{
		const FEnemyAttack& Attack = Attacks[CurrentAttackIndex];

		// Hit if the target's capsule touches the hit sphere (2D) and it is roughly at the same height.
		const FVector Center = Owner->GetActorLocation() + Owner->GetActorForwardVector() * Attack.Reach;
		float TargetRadius = 0.f;
		float TargetHalfHeight = 0.f;
		Target->GetSimpleCollisionCylinder(TargetRadius, TargetHalfHeight);
		const FVector Delta = Target->GetActorLocation() - Center;
		if (Delta.Size2D() <= Attack.Radius + TargetRadius && FMath::Abs(Delta.Z) <= TargetHalfHeight + Attack.Radius)
		{
			const APawn* Pawn = Cast<APawn>(Owner);
			UGameplayStatics::ApplyDamage(Target, Attack.Damage, Pawn ? Pawn->GetController() : nullptr, Owner, nullptr);

			UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
			if (TargetASC && TargetASC->HasAttributeSetForAttribute(UCombatAttributeSet::GetPoiseDamageAttribute()))
			{
				UCombatAttributeSet::ApplyDamage(TargetASC, 0.f, Attack.PoiseDamage);
			}

			if (UHitReactionComponent* HitReaction = Target->FindComponentByClass<UHitReactionComponent>())
			{
				const FVector Direction = (Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
				HitReaction->ApplyKnockback(Direction * Attack.Knockback);
			}
		}

		Owner->GetWorldTimerManager().SetTimer(AttackTimer, this, &UEnemyAttackComponent::FinishAttack, FMath::Max(Attack.RecoveryTime, KINDA_SMALL_NUMBER), false);
		return;
	}

	FinishAttack();
}

void UEnemyAttackComponent::FinishAttack()
{
	CurrentTarget.Reset();
	CurrentAttackIndex = INDEX_NONE;
	bAttacking = false;
}

void UEnemyAttackComponent::SetTelegraphVisible(bool bVisible)
{
	if (Telegraph)
	{
		Telegraph->SetVisibility(bVisible);
	}
}

void UEnemyAttackComponent::StopAttackAnimation()
{
	const ACharacter* Owner = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = Owner ? Owner->GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInstance)
	{
		AnimInstance->StopSlotAnimation(0.1f, AttackSlot);
	}
}
