#include "EnemyAttackComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnemyProjectile.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/RootMotionSource.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "../Combat/CombatAttributeSet.h"
#include "../Combat/HitReactionComponent.h"

namespace
{
	const FName AttackSlot = TEXT("DefaultSlot");

	/** Vertical launch speed of a backstep hop; its horizontal speed is set so the hop covers BackstepDistance. */
	constexpr float BackstepUpSpeed = 300.f;

	const FName ChargeRootMotionName = TEXT("EnemyCharge");

	/** The target's capsule touches the sphere (2D) and is roughly at the same height. */
	bool IsTouchingSphere(const AActor& Target, const FVector& Center, float Radius)
	{
		float TargetRadius = 0.f;
		float TargetHalfHeight = 0.f;
		Target.GetSimpleCollisionCylinder(TargetRadius, TargetHalfHeight);
		const FVector Delta = Target.GetActorLocation() - Center;
		return Delta.Size2D() <= Radius + TargetRadius && FMath::Abs(Delta.Z) <= TargetHalfHeight + Radius;
	}
}

UEnemyAttackComponent::UEnemyAttackComponent()
{
	// Ticks only during a tracked windup.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

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
		if (Attack.Weight > 0.f)
		{
			Range = FMath::Max(Range, Attack.GetRange());
		}
	}
	return Range;
}

void UEnemyAttackComponent::SetTelegraph(UPrimitiveComponent* InTelegraph)
{
	Telegraph = InTelegraph;
	SetTelegraphVisible(false);
}

bool FEnemyAttack::IsRanged() const
{
	return Projectile.Get() != nullptr;
}

bool UEnemyAttackComponent::IsAttackReady(int32 AttackIndex) const
{
	const double Now = GetWorld()->GetTimeSeconds();
	return Now >= SharedReadyTime && (!ReadyTimes.IsValidIndex(AttackIndex) || Now >= ReadyTimes[AttackIndex]);
}

bool UEnemyAttackComponent::IsOffCooldown() const
{
	for (int32 i = 0; i < Attacks.Num(); ++i)
	{
		if (Attacks[i].Weight > 0.f && IsAttackReady(i))
		{
			return true;
		}
	}
	return false;
}

const FEnemyAttack* UEnemyAttackComponent::FindAttack(FName Name) const
{
	return Attacks.FindByPredicate([Name](const FEnemyAttack& Attack) { return Attack.Name == Name; });
}

bool UEnemyAttackComponent::CanStartNamedAttack(FName Name) const
{
	const int32 Index = Attacks.IndexOfByPredicate([Name](const FEnemyAttack& Attack) { return Attack.Name == Name; });
	return !bAttacking && Index != INDEX_NONE && IsAttackReady(Index);
}

bool UEnemyAttackComponent::CanStartAttack(const AActor* Target) const
{
	const AActor* Owner = GetOwner();
	return !bAttacking && Target && Owner && PickAttack(FVector::Dist2D(Target->GetActorLocation(), Owner->GetActorLocation())) != INDEX_NONE;
}

int32 UEnemyAttackComponent::PickAttack(float Distance) const
{
	const auto IsUsable = [this, Distance](int32 i)
	{
		const FEnemyAttack& Attack = Attacks[i];
		return Attack.Weight > 0.f && Distance <= Attack.GetRange() && Distance >= Attack.MinRange && IsAttackReady(i);
	};

	float TotalWeight = 0.f;
	for (int32 i = 0; i < Attacks.Num(); ++i)
	{
		if (IsUsable(i))
		{
			TotalWeight += Attacks[i].Weight;
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
		if (IsUsable(i))
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
	BeginAttack(Target, PickAttack(ToTarget.Size()));
}

void UEnemyAttackComponent::StartNamedAttack(AActor* Target, FName Name)
{
	if (bAttacking || !Target || !Cast<ACharacter>(GetOwner()))
	{
		return;
	}
	BeginAttack(Target, Attacks.IndexOfByPredicate([Name](const FEnemyAttack& Attack) { return Attack.Name == Name; }));
}

void UEnemyAttackComponent::BeginAttack(AActor* Target, int32 AttackIndex)
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Attacks.IsValidIndex(AttackIndex))
	{
		return;
	}
	CurrentAttackIndex = AttackIndex;
	const FEnemyAttack& Attack = Attacks[CurrentAttackIndex];

	bAttacking = true;
	CurrentTarget = Target;

	// Face the target; without tracking this commits the hit area to where it was telegraphed.
	FaceTarget(180.f);
	SetComponentTickEnabled(bTrackTarget || Attack.IsRanged() || Attack.IsCharge());

	if (Attack.BackstepDistance > 0.f)
	{
		TryBackstep(Attack.BackstepDistance);
	}

	if (Attack.IsRanged() || Attack.IsCharge())
	{
		UpdateLaneTelegraph(Attack);
	}
	else if (Telegraph)
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

void UEnemyAttackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Attacks.IsValidIndex(CurrentAttackIndex))
	{
		return;
	}
	const FEnemyAttack& Attack = Attacks[CurrentAttackIndex];
	if (bCharging)
	{
		if (CheckChargeHit(Attack))
		{
			EndCharge();
		}
		return;
	}
	if (bTrackTarget)
	{
		FaceTarget(TrackTurnRate * DeltaTime);
	}
	if (Attack.IsRanged() || Attack.IsCharge())
	{
		UpdateLaneTelegraph(Attack);
	}
}

void UEnemyAttackComponent::UpdateLaneTelegraph(const FEnemyAttack& Attack)
{
	const ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Telegraph || !Owner)
	{
		return;
	}

	// A lane along the facing that reaches the target (or the max distance) when the shot fires.
	float Length = Attack.Reach;
	if (const AActor* Target = CurrentTarget.Get())
	{
		float TargetRadius = 0.f;
		float TargetHalfHeight = 0.f;
		Target->GetSimpleCollisionCylinder(TargetRadius, TargetHalfHeight);
		Length = FMath::Min(Length, FVector::Dist2D(Target->GetActorLocation(), Owner->GetActorLocation()) + TargetRadius);
	}
	const float Elapsed = Owner->GetWorldTimerManager().GetTimerElapsed(AttackTimer);
	Length = FMath::Max(1.f, Length * FMath::Clamp(Elapsed / Attack.WindupTime, 0.f, 1.f));

	const float HalfHeight = Owner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Telegraph->SetRelativeLocation(FVector(Length * 0.5f, 0.f, -HalfHeight + TelegraphGroundOffset));
	Telegraph->SetRelativeScale3D(FVector(Length / 100.f, Attack.Radius / 50.f, Telegraph->GetRelativeScale3D().Z));
}

void UEnemyAttackComponent::FireProjectile(const FEnemyAttack& Attack, const AActor& Target)
{
	APawn* Owner = Cast<APawn>(GetOwner());
	if (!Owner)
	{
		return;
	}

	// Yaw from the facing the lane showed, pitch toward the target's centre, plus the attack's aim error.
	const FVector Forward = Owner->GetActorForwardVector().GetSafeNormal2D();
	const FVector Start = Owner->GetActorLocation() + Forward * ProjectileSpawnOffset.X + FVector(0.f, 0.f, ProjectileSpawnOffset.Z);
	const FVector ToTarget = Target.GetActorLocation() - Start;
	FVector Direction = Forward * ToTarget.Size2D();
	Direction.Z = ToTarget.Z;
	Direction = Direction.GetSafeNormal().RotateAngleAxis(FMath::FRandRange(-Attack.AimSpread, Attack.AimSpread), FVector::UpVector);

	// Deferred so the projectile carries its damage before it can overlap anything.
	const FTransform SpawnTransform(Direction.Rotation(), Start);
	AEnemyProjectile* Projectile = GetWorld()->SpawnActorDeferred<AEnemyProjectile>(Attack.Projectile, SpawnTransform, Owner, Owner,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Projectile)
	{
		Projectile->Launch(Attack, Direction);
		Projectile->FinishSpawning(SpawnTransform);
	}
}

void UEnemyAttackComponent::TryBackstep(float Distance)
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	const FVector Away = -Owner->GetActorForwardVector().GetSafeNormal2D();
	const FVector Feet = Owner->GetNavAgentLocation();

	// Only where the navmesh continues behind (no wall, corner or ledge); otherwise shoot from where it stands.
	FVector HitLocation;
	if (UNavigationSystemV1::NavigationRaycast(Owner, Feet, Feet + Away * Distance, HitLocation))
	{
		return;
	}

	const float AirTime = 2.f * BackstepUpSpeed / FMath::Max(1.f, FMath::Abs(Owner->GetCharacterMovement()->GetGravityZ()));
	Owner->LaunchCharacter(Away * (Distance / AirTime) + FVector(0.f, 0.f, BackstepUpSpeed), true, true);
}

void UEnemyAttackComponent::StartCharge(const FEnemyAttack& Attack, const AActor& Target)
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());

	// Same length as the lane shown during the windup; the charge itself runs with the locomotion animation.
	float TargetRadius = 0.f;
	float TargetHalfHeight = 0.f;
	Target.GetSimpleCollisionCylinder(TargetRadius, TargetHalfHeight);
	const float Length = FMath::Min(Attack.Reach, FVector::Dist2D(Target.GetActorLocation(), Owner->GetActorLocation()) + TargetRadius);
	const float Duration = FMath::Max(Length / Attack.ChargeSpeed, KINDA_SMALL_NUMBER);
	StopAttackAnimation();

	TSharedPtr<FRootMotionSource_ConstantForce> Force = MakeShared<FRootMotionSource_ConstantForce>();
	Force->InstanceName = ChargeRootMotionName;
	Force->AccumulateMode = ERootMotionAccumulateMode::Override;
	Force->Priority = 5;
	ChargeDirection = Owner->GetActorForwardVector().GetSafeNormal2D();
	Force->Force = ChargeDirection * Attack.ChargeSpeed;
	Force->Duration = Duration;
	Force->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);
	Force->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	Force->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
	ChargeRootMotionID = Owner->GetCharacterMovement()->ApplyRootMotionSource(Force);

	bCharging = true;
	ChargeCheckedLocation = Owner->GetActorLocation();
	SetComponentTickEnabled(true);
	Owner->GetWorldTimerManager().SetTimer(AttackTimer, this, &UEnemyAttackComponent::EndCharge, Duration, false);
}

bool UEnemyAttackComponent::CheckChargeHit(const FEnemyAttack& Attack)
{
	const ACharacter* Owner = Cast<ACharacter>(GetOwner());
	AActor* Target = CurrentTarget.Get();
	if (!Owner || !Target)
	{
		return false;
	}

	// Swept from the last check: a fast charge can pass the target between two frames.
	const FVector Front = ChargeDirection * Owner->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const FVector From = ChargeCheckedLocation + Front;
	const FVector To = Owner->GetActorLocation() + Front;
	ChargeCheckedLocation = Owner->GetActorLocation();

	float TargetRadius = 0.f;
	float TargetHalfHeight = 0.f;
	Target->GetSimpleCollisionCylinder(TargetRadius, TargetHalfHeight);
	const FVector TargetLocation = Target->GetActorLocation();
	const float Distance = FMath::PointDistToSegment(FVector(TargetLocation.X, TargetLocation.Y, 0.f), FVector(From.X, From.Y, 0.f), FVector(To.X, To.Y, 0.f));
	if (Distance > Attack.Radius + TargetRadius || FMath::Abs(TargetLocation.Z - To.Z) > TargetHalfHeight + Attack.Radius)
	{
		return false;
	}

	// Off the charge line, to the side the target is on.
	const FVector Side = FVector::CrossProduct(FVector::UpVector, ChargeDirection);
	const float SideSign = FVector::DotProduct(TargetLocation - Owner->GetActorLocation(), Side) >= 0.f ? 1.f : -1.f;
	ApplyHit(Attack, *Target, Side * SideSign);
	return true;
}

void UEnemyAttackComponent::EndCharge()
{
	if (bCharging)
	{
		// The timer can end the charge before a tick has checked its last stretch.
		if (Attacks.IsValidIndex(CurrentAttackIndex))
		{
			CheckChargeHit(Attacks[CurrentAttackIndex]);
		}
		bCharging = false;
		if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
		{
			Owner->GetCharacterMovement()->RemoveRootMotionSourceByID(ChargeRootMotionID);
			Owner->GetCharacterMovement()->StopMovementImmediately();
		}
	}
	SetComponentTickEnabled(false);
	BeginRecovery();
}

void UEnemyAttackComponent::FaceTarget(float MaxYawDelta)
{
	AActor* Owner = GetOwner();
	const AActor* Target = CurrentTarget.Get();
	if (!Owner || !Target)
	{
		return;
	}
	FVector ToTarget = Target->GetActorLocation() - Owner->GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		return;
	}
	FRotator Rotation = Owner->GetActorRotation();
	Rotation.Yaw = FMath::FixedTurn(Rotation.Yaw, ToTarget.Rotation().Yaw, MaxYawDelta);
	Owner->SetActorRotation(Rotation);
}

void UEnemyAttackComponent::CancelAttack()
{
	SetComponentTickEnabled(false);
	if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
	{
		Owner->GetWorldTimerManager().ClearTimer(AttackTimer);
		if (bCharging)
		{
			Owner->GetCharacterMovement()->RemoveRootMotionSourceByID(ChargeRootMotionID);
		}
	}
	bCharging = false;
	if (bAttacking)
	{
		StopAttackAnimation();
	}
	SetTelegraphVisible(false);
	CurrentTarget.Reset();
	CurrentAttackIndex = INDEX_NONE;
	bAttacking = false;
	bRecovering = false;
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
	SetComponentTickEnabled(false);

	AActor* Target = CurrentTarget.Get();
	if (!Owner || !Target || !Attacks.IsValidIndex(CurrentAttackIndex))
	{
		FinishAttack();
		return;
	}

	const FEnemyAttack& Attack = Attacks[CurrentAttackIndex];
	if (Attack.IsCharge())
	{
		// Recovery starts when the charge ends.
		StartCharge(Attack, *Target);
		return;
	}

	if (Attack.IsRanged())
	{
		FireProjectile(Attack, *Target);
	}
	else if (IsTouchingSphere(*Target, Owner->GetActorLocation() + Owner->GetActorForwardVector() * Attack.Reach, Attack.Radius))
	{
		ApplyHit(Attack, *Target, (Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D());
	}
	BeginRecovery();
}

void UEnemyAttackComponent::ApplyHit(const FEnemyAttack& Attack, AActor& Target, const FVector& KnockbackDirection)
{
	AActor* Owner = GetOwner();
	const APawn* Pawn = Cast<APawn>(Owner);
	UGameplayStatics::ApplyDamage(&Target, Attack.Damage, Pawn ? Pawn->GetController() : nullptr, Owner, nullptr);

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(&Target);
	if (TargetASC && TargetASC->HasAttributeSetForAttribute(UCombatAttributeSet::GetPoiseDamageAttribute()))
	{
		UCombatAttributeSet::ApplyDamage(TargetASC, 0.f, Attack.PoiseDamage, Owner);
	}

	if (UHitReactionComponent* HitReaction = Target.FindComponentByClass<UHitReactionComponent>())
	{
		HitReaction->ApplyKnockback(KnockbackDirection * Attack.Knockback);
		HitReaction->ApplyStun(Attack.StunDuration);
	}
}

void UEnemyAttackComponent::BeginRecovery()
{
	if (!Attacks.IsValidIndex(CurrentAttackIndex))
	{
		FinishAttack();
		return;
	}
	const FEnemyAttack& Attack = Attacks[CurrentAttackIndex];
	const double RecoveryEnd = GetWorld()->GetTimeSeconds() + Attack.RecoveryTime;
	ReadyTimes.SetNumZeroed(Attacks.Num());
	ReadyTimes[CurrentAttackIndex] = RecoveryEnd + Attack.Cooldown;
	SharedReadyTime = RecoveryEnd + SharedCooldown;

	bRecovering = true;
	GetOwner()->GetWorldTimerManager().SetTimer(AttackTimer, this, &UEnemyAttackComponent::FinishAttack, FMath::Max(Attack.RecoveryTime, KINDA_SMALL_NUMBER), false);
}

void UEnemyAttackComponent::FinishAttack()
{
	CurrentTarget.Reset();
	CurrentAttackIndex = INDEX_NONE;
	bAttacking = false;
	bRecovering = false;
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
