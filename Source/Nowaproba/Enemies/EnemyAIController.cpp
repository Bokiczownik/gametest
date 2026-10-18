#include "EnemyAIController.h"

#include "EnemyAttackComponent.h"
#include "EnemyCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "../Health/HealthComponent.h"

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	Enemy = Cast<AEnemyCharacter>(InPawn);
	if (Enemy.IsValid())
	{
		// Random first delay so a group does not think in the same frame.
		GetWorldTimerManager().SetTimer(UpdateTimer, this, &AEnemyAIController::UpdateBehaviour, UpdateInterval, true, FMath::FRandRange(0.f, UpdateInterval));
	}
}

void AEnemyAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(UpdateTimer);
	StopChase();
	Enemy.Reset();
	Super::OnUnPossess();
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

	const float Distance = FVector::Dist(Self->GetActorLocation(), Player->GetActorLocation());
	if (!bChasing)
	{
		if (Distance > Self->AggroRange || !LineOfSightTo(Player))
		{
			return;
		}
		bChasing = true;
	}
	else if (Distance > Self->LoseAggroRange)
	{
		StopChase();
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

void AEnemyAIController::StopChase()
{
	if (bChasing)
	{
		StopMovement();
	}
	bChasing = false;
}
