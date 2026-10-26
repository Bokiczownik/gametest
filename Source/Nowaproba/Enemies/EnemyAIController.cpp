#include "EnemyAIController.h"

#include "EnemyAttackComponent.h"
#include "EnemyCharacter.h"
#include "FogHunterSpawner.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "../Combat/HitReactionComponent.h"
#include "../Health/HealthComponent.h"

namespace
{
	/** How often a group member fighting on its own account renews the group alert (s). */
	constexpr double GroupAlertInterval = 1.0;
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	Enemy = Cast<AEnemyCharacter>(InPawn);
	if (AEnemyCharacter* Self = Enemy.Get())
	{
		LastHealth = Self->GetHealth()->GetHealth();
		Self->GetHealth()->OnHealthChanged.AddUniqueDynamic(this, &AEnemyAIController::HandleHealthChanged);

		// Random first delay so a group does not think in the same frame.
		GetWorldTimerManager().SetTimer(UpdateTimer, this, &AEnemyAIController::UpdateBehaviour, UpdateInterval, true, FMath::FRandRange(0.f, UpdateInterval));
	}
}

void AEnemyAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(UpdateTimer);
	StopChase();
	if (AEnemyCharacter* Self = Enemy.Get())
	{
		Self->GetHealth()->OnHealthChanged.RemoveDynamic(this, &AEnemyAIController::HandleHealthChanged);
	}
	Enemy.Reset();
	Super::OnUnPossess();
}

void AEnemyAIController::HandleHealthChanged(float CurrentHealth, float MaxHealth)
{
	if (CurrentHealth < LastHealth)
	{
		bAlerted = true;
		Provoke();
	}
	LastHealth = CurrentHealth;
}

void AEnemyAIController::Alert()
{
	bAlerted = true;
	if (const AEnemyCharacter* Self = Enemy.Get())
	{
		GroupAlertUntil = GetWorld()->GetTimeSeconds() + Self->ProvokedDuration;
	}
}

void AEnemyAIController::Provoke()
{
	if (const AEnemyCharacter* Self = Enemy.Get())
	{
		ProvokedUntil = GetWorld()->GetTimeSeconds() + Self->ProvokedDuration;
	}
}

bool AEnemyAIController::IsProvoked() const
{
	const double Now = GetWorld()->GetTimeSeconds();
	return Now < ProvokedUntil || Now < GroupAlertUntil;
}

bool AEnemyAIController::IsEngagedOnOwn(const APawn* Player, float Distance) const
{
	// Hit, or the player is within its AggroRange and its own area: not kept in the fight only by the group's alerts
	// (LoseAggroRange would let alerted members that ran after the player keep renewing each other).
	const AEnemyCharacter* Self = Enemy.Get();
	return GetWorld()->GetTimeSeconds() < ProvokedUntil
		|| (CanEngageOnOwn(Player) && (Self->Behaviour == EEnemyBehaviour::FogHunter || Distance <= Self->AggroRange));
}

void AEnemyAIController::AlertGroup()
{
	const AEnemyCharacter* Self = Enemy.Get();
	if (!Self || Self->AggroGroup.IsNone())
	{
		return;
	}
	for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (*It != Self && It->AggroGroup == Self->AggroGroup && !It->IsDead())
		{
			if (AEnemyAIController* MemberAI = Cast<AEnemyAIController>(It->GetController()))
			{
				MemberAI->Alert();
			}
		}
	}
}

void AEnemyAIController::UpdateBehaviour()
{
	AEnemyCharacter* Self = Enemy.Get();
	if (!Self || Self->IsDead())
	{
		StopChase();
		return;
	}

	if (!Target.IsValid())
	{
		Target = UGameplayStatics::GetPlayerPawn(this, 0);
	}
	APawn* Player = Target.Get();
	const UHealthComponent* PlayerHealth = Player ? Player->FindComponentByClass<UHealthComponent>() : nullptr;
	if (!Player || (PlayerHealth && PlayerHealth->IsDead()))
	{
		StopChase();
		return;
	}

	// 2D like the attack's own range and hit tests: capsule centres of differently sized pawns sit at different heights.
	const float Distance = FVector::Dist2D(Self->GetActorLocation(), Player->GetActorLocation());

	// A member fighting on its own account keeps its group in the fight (also while attacking or blocking).
	const double Now = GetWorld()->GetTimeSeconds();
	if (bChasing && Now >= NextGroupAlertTime && IsEngagedOnOwn(Player, Distance))
	{
		NextGroupAlertTime = Now + GroupAlertInterval;
		AlertGroup();
	}

	UEnemyAttackComponent* Attack = Self->GetAttack();
	if (Attack->IsHoldingPosition() || Self->IsActionLocked() || Self->GetHitReaction()->IsStaggered() || Self->GetHitReaction()->IsStunned())
	{
		return;
	}

	const bool bHunter = Self->Behaviour == EEnemyBehaviour::FogHunter;
	if (!bChasing)
	{
		if ((bHunter || bAlerted || CanSee(Player)) && CanEngage(Player))
		{
			SetChasing(true);
		}
	}
	else if ((!bHunter && Distance > Self->LoseAggroRange && !IsProvoked()) || !CanEngage(Player))
	{
		SetChasing(false);
	}
	bAlerted = false;

	if (!bChasing)
	{
		switch (Self->Behaviour)
		{
		case EEnemyBehaviour::Patrol: UpdatePatrol(); break;
		case EEnemyBehaviour::Guard:
		case EEnemyBehaviour::FogHunter: ReturnHome(); break;
		default: break;
		}
		return;
	}

	UpdateCombat(Player, Distance);
}

void AEnemyAIController::UpdateCombat(APawn* Player, float Distance)
{
	AEnemyCharacter* Self = Enemy.Get();
	UEnemyAttackComponent* Attack = Self->GetAttack();

	// Weapon raised only while running at the player or about to attack, not while stuck (e.g. no path yet).
	Self->SetInCombat(Distance <= Attack->GetAttackRange() * 1.5f || Self->GetVelocity().Size2D() > 50.f);

	if (Distance <= Attack->GetAttackRange())
	{
		StopMovement();
		Attack->StartAttack(Player);
		return;
	}

	if (Self->Behaviour == EEnemyBehaviour::FogHunter)
	{
		Self->UpdateHunterChaseSpeed(Distance);
	}

	FVector FlankPoint;
	if (Self->bFlank && GetFlankPoint(Player, Distance, FlankPoint))
	{
		MoveToLocation(FlankPoint, 30.f);
		return;
	}

	// Stop a bit inside the attack range so the next update can attack.
	MoveToActor(Player, Attack->GetAttackRange() * 0.6f);
}

bool AEnemyAIController::GetFlankPoint(const APawn* Player, float Distance, FVector& OutPoint)
{
	AEnemyCharacter* Self = Enemy.Get();
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < FlankResumeTime || Distance > Self->FlankRadius)
	{
		FlankStartTime = -1.0;
		return false;
	}

	// Chasing flankers near the player, ordered by their angle around it.
	const FVector PlayerLocation = Player->GetActorLocation();
	const auto AngleAround = [&PlayerLocation](const AActor* Actor)
	{
		const FVector Offset = Actor->GetActorLocation() - PlayerLocation;
		return FMath::RadiansToDegrees(FMath::Atan2(Offset.Y, Offset.X));
	};
	TArray<TPair<float, const AEnemyCharacter*>, TInlineAllocator<8>> Members;
	for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
	{
		const AEnemyAIController* MemberAI = Cast<AEnemyAIController>(It->GetController());
		if (It->bFlank && !It->IsDead() && MemberAI && MemberAI->IsChasing()
			&& FVector::Dist2D(It->GetActorLocation(), PlayerLocation) <= It->FlankRadius)
		{
			Members.Emplace(AngleAround(*It), *It);
		}
	}
	const int32 SelfIndex = Members.IndexOfByPredicate([Self](const TPair<float, const AEnemyCharacter*>& Member) { return Member.Value == Self; });
	if (Members.Num() < 2 || SelfIndex == INDEX_NONE)
	{
		FlankStartTime = -1.0;
		return false;
	}
	Members.Sort([](const TPair<float, const AEnemyCharacter*>& A, const TPair<float, const AEnemyCharacter*>& B) { return A.Key < B.Key; });

	// Evenly spaced slots in the members' current order, rotated to stay closest to where they stand (circular mean).
	const float SlotStep = 360.f / Members.Num();
	FVector2D Sum = FVector2D::ZeroVector;
	int32 MyIndex = 0;
	for (int32 i = 0; i < Members.Num(); ++i)
	{
		const float Offset = FMath::DegreesToRadians(Members[i].Key - i * SlotStep);
		Sum += FVector2D(FMath::Cos(Offset), FMath::Sin(Offset));
		if (Members[i].Value == Self)
		{
			MyIndex = i;
		}
	}
	const float MyAngle = Members[MyIndex].Key;
	const float SlotAngle = FMath::RadiansToDegrees(FMath::Atan2(Sum.Y, Sum.X)) + MyIndex * SlotStep;
	const float Delta = FMath::FindDeltaAngleDegrees(MyAngle, SlotAngle);
	if (FMath::Abs(Delta) <= Self->FlankTolerance)
	{
		FlankStartTime = -1.0;
		return false;
	}

	// Circling too long (e.g. the player keeps turning with it): approach directly for a while.
	if (FlankStartTime < 0.0)
	{
		FlankStartTime = Now;
	}
	else if (Now - FlankStartTime > Self->FlankTimeout)
	{
		FlankStartTime = -1.0;
		FlankResumeTime = Now + Self->FlankTimeout;
		return false;
	}

	// Walk around the player just outside its reach, at most 60 deg per step; farther ones close in while circling.
	const float AttackRange = Self->GetAttack()->GetAttackRange();
	const float Radius = FMath::Clamp(Distance, AttackRange + 60.f, AttackRange + 250.f);
	const float StepAngle = FMath::DegreesToRadians(MyAngle + FMath::Clamp(Delta, -60.f, 60.f));
	const FVector Point = FVector(PlayerLocation.X + FMath::Cos(StepAngle) * Radius, PlayerLocation.Y + FMath::Sin(StepAngle) * Radius, Self->GetNavAgentLocation().Z);

	const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation PointNav;
	if (!NavSys || !NavSys->ProjectPointToNavigation(Point, PointNav))
	{
		return false;
	}
	OutPoint = PointNav.Location;
	return true;
}

bool AEnemyAIController::CanSee(const APawn* Player) const
{
	const AEnemyCharacter* Self = Enemy.Get();
	const FVector ToPlayer = Player->GetActorLocation() - Self->GetActorLocation();
	if (ToPlayer.Size() > Self->AggroRange)
	{
		return false;
	}
	if (Self->SightHalfAngle < 180.f)
	{
		const float CosAngle = FVector::DotProduct(Self->GetActorForwardVector().GetSafeNormal2D(), ToPlayer.GetSafeNormal2D());
		if (CosAngle < FMath::Cos(FMath::DegreesToRadians(Self->SightHalfAngle)))
		{
			return false;
		}
	}
	return LineOfSightTo(Player);
}

bool AEnemyAIController::CanEngage(const APawn* Player) const
{
	return IsProvoked() || CanEngageOnOwn(Player);
}

bool AEnemyAIController::CanEngageOnOwn(const APawn* Player) const
{
	const AEnemyCharacter* Self = Enemy.Get();
	switch (Self->Behaviour)
	{
	case EEnemyBehaviour::Guard:
		return FVector::Dist2D(Player->GetActorLocation(), Self->GetHomeLocation()) <= Self->GuardRadius;
	case EEnemyBehaviour::FogHunter:
	{
		const AFogHunterSpawner* Spawner = Self->GetHunterSpawner();
		return !Spawner || Spawner->IsInHuntArea(Player->GetActorLocation());
	}
	default:
		return true;
	}
}

void AEnemyAIController::SetChasing(bool bNewChasing)
{
	if (bChasing == bNewChasing)
	{
		return;
	}
	bChasing = bNewChasing;
	StopMovement();
	PatrolWaitEnd = -1.0;
	if (AEnemyCharacter* Self = Enemy.Get())
	{
		Self->SetChaseSpeed(bChasing || Self->Behaviour == EEnemyBehaviour::Chase);
		Self->SetInCombat(bChasing);
	}
	NextGroupAlertTime = 0.0;
	OnChaseChanged(bChasing);
}

void AEnemyAIController::UpdatePatrol()
{
	const AEnemyCharacter* Self = Enemy.Get();
	const TArray<FVector>& Route = Self->GetPatrolRoute();
	if (Route.Num() == 0)
	{
		return;
	}
	PatrolIndex %= Route.Num();

	const double Now = GetWorld()->GetTimeSeconds();
	if (FVector::Dist2D(Self->GetActorLocation(), Route[PatrolIndex]) <= ArriveDistance)
	{
		if (PatrolWaitEnd < 0.0)
		{
			PatrolWaitEnd = Now + Self->PatrolWaitTime;
		}
		if (Now < PatrolWaitEnd)
		{
			return;
		}
		PatrolWaitEnd = -1.0;
		PatrolIndex = (PatrolIndex + 1) % Route.Num();
	}

	if (GetMoveStatus() != EPathFollowingStatus::Moving)
	{
		// Unreachable point: skip it instead of retrying forever.
		if (MoveToLocation(Route[PatrolIndex], ArriveDistance * 0.5f) == EPathFollowingRequestResult::Failed)
		{
			PatrolIndex = (PatrolIndex + 1) % Route.Num();
		}
	}
}

void AEnemyAIController::ReturnHome()
{
	AEnemyCharacter* Self = Enemy.Get();
	if (FVector::Dist2D(Self->GetActorLocation(), Self->GetHomeLocation()) > ArriveDistance)
	{
		if (GetMoveStatus() != EPathFollowingStatus::Moving)
		{
			MoveToLocation(Self->GetHomeLocation(), ArriveDistance * 0.5f);
		}
		return;
	}

	if (GetMoveStatus() != EPathFollowingStatus::Moving)
	{
		Self->SetActorRotation(Self->GetHomeRotation());
	}
}

void AEnemyAIController::StopChase()
{
	const bool bWasChasing = bChasing;
	if (bChasing)
	{
		StopMovement();
	}
	bChasing = false;
	if (bWasChasing)
	{
		OnChaseChanged(false);
	}
	if (AEnemyCharacter* Self = Enemy.Get())
	{
		Self->SetInCombat(false);
	}
}
