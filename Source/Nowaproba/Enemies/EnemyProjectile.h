#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
struct FEnemyAttack;

/**
 * Straight-flying enemy shot fired by UEnemyAttackComponent (FEnemyAttack::Projectile). Flies through enemies (an archer
 * may shoot past its allies), stops at world static geometry and hits the first other pawn with a UHealthComponent:
 * engine damage, poise damage, knockback and stun of the attack that fired it. Expires after the attack's Reach.
 * Mesh is set in the BP child.
 */
UCLASS()
class NOWAPROBA_API AEnemyProjectile : public AActor
{
	GENERATED_BODY()

public:
	AEnemyProjectile();

	/** Call before FinishSpawning (SpawnActorDeferred): hit values, collision radius and velocity from Attack. */
	void Launch(const FEnemyAttack& Attack, const FVector& Direction);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> Movement;

private:
	float Damage = 0.f;
	float PoiseDamage = 0.f;
	float Knockback = 0.f;
	float StunDuration = 0.f;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleStop(const FHitResult& ImpactResult);
};
