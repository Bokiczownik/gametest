#include "EnemyCharacter.h"

#include "AIController.h"
#include "../Combat/CombatAbilitySystemComponent.h"
#include "../Combat/HitReactionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnemyAIController.h"
#include "EnemyAttackComponent.h"
#include "FogHunterSpawner.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../Health/HealthComponent.h"
#include "../Loot/LootLibrary.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Turn toward the walk direction; attacks snap the facing themselves.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 450.f;

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	AbilitySystem = CreateDefaultSubobject<UCombatAbilitySystemComponent>(TEXT("AbilitySystem"));
	Attack = CreateDefaultSubobject<UEnemyAttackComponent>(TEXT("Attack"));
	HitReaction = CreateDefaultSubobject<UHitReactionComponent>(TEXT("HitReaction"));

	AttackTelegraph = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttackTelegraph"));
	AttackTelegraph->SetupAttachment(GetCapsuleComponent());
	AttackTelegraph->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttackTelegraph->SetGenerateOverlapEvents(false);
	AttackTelegraph->SetCastShadow(false);
	AttackTelegraph->SetRelativeScale3D(FVector(1.f, 1.f, 0.02f));
}

bool AEnemyCharacter::IsDead() const
{
	return Health->IsDead();
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	HomeLocation = GetActorLocation();
	HomeRotation = GetActorRotation();
	for (const FVector& Point : PatrolPoints)
	{
		PatrolRoute.Add(GetActorTransform().TransformPosition(Point));
	}
	ChaseSpeed = GetCharacterMovement()->MaxWalkSpeed;
	SetChaseSpeed(Behaviour == EEnemyBehaviour::Chase);

	Attack->SetTelegraph(AttackTelegraph);
	Health->OnDeath.AddUniqueDynamic(this, &AEnemyCharacter::HandleDeath);
	HitReaction->OnStaggered.AddUniqueDynamic(this, &AEnemyCharacter::HandleStaggered);
}

void AEnemyCharacter::SetHunterSpawner(AFogHunterSpawner* InSpawner)
{
	HunterSpawner = InSpawner;
}

AFogHunterSpawner* AEnemyCharacter::GetHunterSpawner() const
{
	return HunterSpawner.Get();
}

void AEnemyCharacter::SetChaseSpeed(bool bChasing)
{
	GetCharacterMovement()->MaxWalkSpeed = bChasing ? ChaseSpeed : PatrolWalkSpeed;
}

void AEnemyCharacter::UpdateHunterChaseSpeed(float DistanceToPlayer)
{
	GetCharacterMovement()->MaxWalkSpeed = DistanceToPlayer <= HunterNearDistance ? ChaseSpeed : HunterFarSpeed;
}

void AEnemyCharacter::HandleStaggered()
{
	Attack->CancelAttack();
}

void AEnemyCharacter::HandleDeath()
{
	Attack->CancelAttack();

	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}
	DetachFromControllerPendingDestroy();

	// The body no longer blocks the player or takes hits.
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ULootLibrary::RollAndGrantLoot(LootTable, LootRow, Health->GetLastDamageInstigator());
	SetLifeSpan(CorpseLifetime);
}
