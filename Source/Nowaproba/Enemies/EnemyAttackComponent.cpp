#include "EnemyAttackComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UEnemyAttackComponent::UEnemyAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEnemyAttackComponent::SetTelegraph(UPrimitiveComponent* InTelegraph)
{
	Telegraph = InTelegraph;
	if (!Telegraph)
	{
		return;
	}

	// Owner origin is the capsule centre; the telegraph sits on the ground under the hit sphere.
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const float HalfHeight = Character ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
	Telegraph->SetRelativeLocation(FVector(Reach, 0.f, -HalfHeight + TelegraphGroundOffset));
	Telegraph->SetRelativeScale3D(FVector(Radius / 50.f, Radius / 50.f, Telegraph->GetRelativeScale3D().Z));
	SetTelegraphVisible(false);
}

void UEnemyAttackComponent::StartAttack(AActor* Target)
{
	AActor* Owner = GetOwner();
	if (bAttacking || !Target || !Owner)
	{
		return;
	}

	bAttacking = true;
	CurrentTarget = Target;

	// Commit to a direction: the hit area stays where it was telegraphed.
	FVector ToTarget = Target->GetActorLocation() - Owner->GetActorLocation();
	ToTarget.Z = 0.f;
	if (!ToTarget.IsNearlyZero())
	{
		Owner->SetActorRotation(ToTarget.Rotation());
	}

	SetTelegraphVisible(true);
	Owner->GetWorldTimerManager().SetTimer(AttackTimer, this, &UEnemyAttackComponent::Strike, WindupTime, false);
}

void UEnemyAttackComponent::CancelAttack()
{
	if (AActor* Owner = GetOwner())
	{
		Owner->GetWorldTimerManager().ClearTimer(AttackTimer);
	}
	SetTelegraphVisible(false);
	CurrentTarget.Reset();
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
	if (Owner && Target)
	{
		// Hit if the target's capsule touches the hit sphere (2D) and it is roughly at the same height.
		const FVector Center = Owner->GetActorLocation() + Owner->GetActorForwardVector() * Reach;
		float TargetRadius = 0.f;
		float TargetHalfHeight = 0.f;
		Target->GetSimpleCollisionCylinder(TargetRadius, TargetHalfHeight);
		const FVector Delta = Target->GetActorLocation() - Center;
		if (Delta.Size2D() <= Radius + TargetRadius && FMath::Abs(Delta.Z) <= TargetHalfHeight + Radius)
		{
			const APawn* Pawn = Cast<APawn>(Owner);
			UGameplayStatics::ApplyDamage(Target, Damage, Pawn ? Pawn->GetController() : nullptr, Owner, nullptr);
		}
	}

	if (Owner)
	{
		Owner->GetWorldTimerManager().SetTimer(AttackTimer, this, &UEnemyAttackComponent::FinishAttack, FMath::Max(RecoveryTime, KINDA_SMALL_NUMBER), false);
	}
}

void UEnemyAttackComponent::FinishAttack()
{
	CurrentTarget.Reset();
	bAttacking = false;
}

void UEnemyAttackComponent::SetTelegraphVisible(bool bVisible)
{
	if (Telegraph)
	{
		Telegraph->SetVisibility(bVisible);
	}
}
