#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MeleeAttackComponent.generated.h"

/**
 * Temporary melee attack until real combat exists: Attack() applies engine damage (UGameplayStatics::ApplyDamage)
 * once to every actor overlapping a sphere in front of the owner, then waits Cooldown. No animation; the hit sphere
 * is drawn briefly when bDrawDebug is on. Damage receivers react via OnTakeAnyDamage (e.g. UHealthComponent, loot).
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UMeleeAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMeleeAttackComponent();

	/** Bound to the attack input. Does nothing while on cooldown. */
	UFUNCTION(BlueprintCallable, Category = "Melee Attack")
	void Attack();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Attack", meta = (ClampMin = "0"))
	float Damage = 25.f;

	/** Distance from the owner to the sphere centre, along the owner's forward vector. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Attack", meta = (ClampMin = "0"))
	float Reach = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Attack", meta = (ClampMin = "1"))
	float Radius = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Attack", meta = (ClampMin = "0"))
	float Cooldown = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Melee Attack")
	bool bDrawDebug = true;

private:
	double LastAttackTime = -1.e9;
};
