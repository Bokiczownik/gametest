#include "EnemyCharacter.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "../Combat/CombatAbilitySystemComponent.h"
#include "../Combat/CombatTags.h"
#include "../Combat/HitReactionComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnemyAIController.h"
#include "EnemyAttackComponent.h"
#include "EnemyHealthBarWidget.h"
#include "FogHunterSpawner.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../Health/HealthComponent.h"
#include "../Loot/LootLibrary.h"
#include "TimerManager.h"

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
	Block = CreateDefaultSubobject<UBlockComponent>(TEXT("Block"));

	HealthBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	HealthBar->SetupAttachment(GetCapsuleComponent());
	HealthBar->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBar->SetWidgetClass(UEnemyHealthBarWidget::StaticClass());
	HealthBar->SetDrawAtDesiredSize(true);
	HealthBar->SetPivot(FVector2D(0.5f, 1.f));
	HealthBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HealthBar->SetGenerateOverlapEvents(false);

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

bool AEnemyCharacter::IsActionLocked() const
{
	return bKnockedDown || Block->IsBlocking();
}

bool AEnemyCharacter::IsPlayingFullBodyAnimation() const
{
	return bKnockedDown || Block->IsBlocking() || Attack->IsFullBodyAttack();
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
	LastHealth = Health->GetHealth();
	Health->OnHealthChanged.AddUniqueDynamic(this, &AEnemyCharacter::HandleHealthChanged);
	Health->OnDeath.AddUniqueDynamic(this, &AEnemyCharacter::HandleDeath);
	HitReaction->OnStaggered.AddUniqueDynamic(this, &AEnemyCharacter::HandleStaggered);
	Block->OnBlockEnded.AddUniqueDynamic(this, &AEnemyCharacter::HandleBlockEnded);

	HealthBar->SetRelativeLocation(FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + HealthBarOffset));
	if (UEnemyHealthBarWidget* Bar = Cast<UEnemyHealthBarWidget>(HealthBar->GetUserWidgetObject()))
	{
		Bar->Bind(Health, AbilitySystem);
	}
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
	Block->StopBlock();
}

void AEnemyCharacter::HandleHealthChanged(float CurrentHealth, float MaxHealth)
{
	const bool bDamaged = CurrentHealth < LastHealth;
	LastHealth = CurrentHealth;

	APawn* Attacker = Health->GetLastDamageInstigator();
	if (!bDamaged || IsDead() || !Attacker || !Attacker->IsPlayerControlled() || !Block->CanBlock() || bKnockedDown
		|| HitReaction->IsStaggered() || HitReaction->IsStunned() || Attack->IsWindingUp())
	{
		return;
	}

	if (ShieldStance == EShieldStance::Aggressive)
	{
		StartShieldBlock(Attacker, true);
		return;
	}
	if (ShieldStance == EShieldStance::Defensive)
	{
		// Short blocks do most of the work; the long one is rare.
		const double Now = GetWorld()->GetTimeSeconds();
		if (Now < NextLongBlockTime)
		{
			return;
		}
		NextLongBlockTime = Now + LongBlockInterval;
	}
	StartShieldBlock(Attacker, false);
}

void AEnemyCharacter::TryReactiveBlock(AActor* Attacker)
{
	if (ShieldStance != EShieldStance::Defensive || !Attacker || IsDead() || bKnockedDown || !Block->CanBlock()
		|| Attack->IsAttacking() || HitReaction->IsStaggered() || HitReaction->IsStunned()
		|| FVector::Dist2D(Attacker->GetActorLocation(), GetActorLocation()) > ReactiveBlockRange)
	{
		return;
	}
	StartShieldBlock(Attacker, true);
}

void AEnemyCharacter::StartShieldBlock(AActor* Target, bool bShort)
{
	// Raise the shield: drop the attack recovery and stand still.
	Attack->CancelAttack();
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}
	SetInCombat(true);
	bShortBlock = bShort;
	if (bShort)
	{
		Block->StartBlockFor(Target, ShortBlockDuration, ShortBlockCooldown);
	}
	else
	{
		Block->StartBlock(Target);
	}
}

void AEnemyCharacter::HandleBlockEnded(EBlockEndReason Reason)
{
	if (IsDead())
	{
		return;
	}
	if (Reason == EBlockEndReason::Completed)
	{
		if (bShortBlock && ShieldStance == EShieldStance::Defensive)
		{
			// Push the player off after a parried swing; nothing if the block caught nothing.
			if (Block->DidBlockHit())
			{
				Attack->StartNamedAttack(Block->GetBlockTarget(), BlockShoveAttack);
			}
		}
		else
		{
			Attack->StartNamedAttack(Block->GetBlockTarget(), BlockCounterAttack);
		}
	}
	else if (Reason == EBlockEndReason::Broken)
	{
		StartKnockdown();
	}
}

void AEnemyCharacter::StartKnockdown()
{
	bKnockedDown = true;
	Attack->CancelAttack();
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}
	// No stagger or knockback while down.
	AbilitySystem->AddLooseGameplayTag(CombatTags::State_StaggerImmune);

	float Duration = 1.f;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (KnockdownAnimation && AnimInstance)
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(KnockdownAnimation, TEXT("DefaultSlot"), 0.1f, 0.2f);
		Duration = KnockdownAnimation->GetPlayLength();
	}
	GetWorldTimerManager().SetTimer(KnockdownTimer, this, &AEnemyCharacter::EndKnockdown, Duration);
}

void AEnemyCharacter::EndKnockdown()
{
	bKnockedDown = false;
	AbilitySystem->RemoveLooseGameplayTag(CombatTags::State_StaggerImmune);
}

void AEnemyCharacter::HandleDeath()
{
	Attack->CancelAttack();
	Block->StopBlock();
	GetWorldTimerManager().ClearTimer(KnockdownTimer);
	HealthBar->SetVisibility(false);

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
