#include "EnemyAIController.h"

#include "EnemyAttackComponent.h"
#include "EnemyCharacter.h"
#include "FogHunterSpawner.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"
#include "../Health/HealthComponent.h"

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
	}
	LastHealth = CurrentHealth;
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

	UEnemyAttackComponent* Attack = Self->GetAttack();
	if (Attack->IsAttacking())
	{
		return;
	}

	const bool bHunter = Self->Behaviour == EEnemyBehaviour::FogHunter;
	const float Distance = FVector::Dist(Self->GetActorLocation(), Player->GetActorLocation());
	if (!bChasing)
	{
		if ((bHunter || bAlerted || CanSee(Player)) && CanEngage(Player))
		{
			SetChasing(true);
		}
	}
	else if ((!bHunter && Distance > Self->LoseAggroRange) || !CanEngage(Player))
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

	if (Distance <= Attack->GetAttackRange())
	{
		StopMovement();
		Attack->StartAttack(Player);
		return;
	}

	// Stop a bit inside the attack range so the next update can attack.
	MoveToActor(Player, Attack->GetAttackRange() * 0.6f);
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
	}
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
	if (bChasing)
	{
		StopMovement();
	}
	bChasing = false;
}
