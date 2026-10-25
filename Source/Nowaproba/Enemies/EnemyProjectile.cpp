#include "EnemyProjectile.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnemyAttackComponent.h"
#include "EnemyCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "../Combat/CombatAttributeSet.h"
#include "../Combat/HitReactionComponent.h"
#include "../Health/HealthComponent.h"

AEnemyProjectile::AEnemyProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// Walls stop it; pawns only overlap (enemies are skipped in HandleOverlap). Other projectiles are ignored.
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(15.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	RootComponent = Collision;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCastShadow(false);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->ProjectileGravityScale = 0.f;
	Movement->bRotationFollowsVelocity = true;
	Movement->bInitialVelocityInLocalSpace = false;
}

void AEnemyProjectile::Launch(const FEnemyAttack& Attack, const FVector& Direction)
{
	Damage = Attack.Damage;
	PoiseDamage = Attack.PoiseDamage;
	Knockback = Attack.Knockback;
	StunDuration = Attack.StunDuration;

	Collision->SetSphereRadius(Attack.Radius);
	Movement->InitialSpeed = Attack.ProjectileSpeed;
	Movement->MaxSpeed = Attack.ProjectileSpeed;
	Movement->Velocity = Direction * Attack.ProjectileSpeed;
	SetLifeSpan(Attack.GetRange() / Attack.ProjectileSpeed);

	Collision->OnComponentBeginOverlap.AddUniqueDynamic(this, &AEnemyProjectile::HandleOverlap);
	Movement->OnProjectileStop.AddUniqueDynamic(this, &AEnemyProjectile::HandleStop);
}

void AEnemyProjectile::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetInstigator() || OtherActor->IsA<AEnemyCharacter>())
	{
		return;
	}
	const UHealthComponent* Health = OtherActor->FindComponentByClass<UHealthComponent>();
	if (!Health || Health->IsDead())
	{
		return;
	}

	UGameplayStatics::ApplyDamage(OtherActor, Damage, GetInstigatorController(), this, nullptr);

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor);
	if (TargetASC && TargetASC->HasAttributeSetForAttribute(UCombatAttributeSet::GetPoiseDamageAttribute()))
	{
		UCombatAttributeSet::ApplyDamage(TargetASC, 0.f, PoiseDamage, this);
	}

	if (UHitReactionComponent* HitReaction = OtherActor->FindComponentByClass<UHitReactionComponent>())
	{
		HitReaction->ApplyKnockback(Movement->Velocity.GetSafeNormal2D() * Knockback);
		HitReaction->ApplyStun(StunDuration);
	}
	Destroy();
}

void AEnemyProjectile::HandleStop(const FHitResult& ImpactResult)
{
	Destroy();
}
