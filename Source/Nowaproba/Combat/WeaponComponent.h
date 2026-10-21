#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponData.h"
#include "WeaponComponent.generated.h"

class UGameplayAbility;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EWeaponAttackType : uint8
{
	Light,
	Heavy,
	Sprint,
	Jump
};

/**
 * Holds the player's weapon (UWeaponData): attaches its mesh to the owner's skeletal mesh, turns attack input into
 * attacks (tap = light combo, hold = heavy, while sprinting = sprint attack, in the air = jump attack), buffers input
 * pressed during an attack until its ChainTime, ignores attacks during the weapon's AttackCooldown after one ends, and sweeps the blade during the hit window (ticks only then).
 * Each attack is performed by AttackAbilityClass (UWeaponAttackAbility) on the owner's ability system.
 * Hits use the engine damage system (UGameplayStatics::ApplyDamage, so UHealthComponent and loot breakables react),
 * apply PoiseDamage to targets with combat attributes, knock back targets with a UHitReactionComponent and freeze the
 * game for the attack's HitStop (global time dilation). Each actor is hit once per attack.
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponComponent();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AttackPressed();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AttackReleased();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsAttacking() const { return bAttacking; }

	const UWeaponData* GetWeaponData() const { return WeaponData; }

	/** Attack to be performed by the activating attack ability (valid only during its activation). */
	const FWeaponAttack* GetPendingAttack() const { return PendingAttack; }

	/** Called by the attack ability. */
	void NotifyAttackStarted(const FWeaponAttack& Attack);
	void NotifyAttackEnded();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponData> WeaponData;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<UGameplayAbility> AttackAbilityClass;

	/** The hit area reaches this far below the blade (cm), so the swing also hits short enemies in the top-down view. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0"))
	float HitReachDown = 80.f;

	/** Draws the blade sweeps (green) and hits (red). */
	UPROPERTY(EditAnywhere, Category = "Weapon|Debug")
	bool bDrawDebugHits = false;

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	const FWeaponAttack* PendingAttack = nullptr;
	const FWeaponAttack* CurrentAttack = nullptr;
	TOptional<EWeaponAttackType> BufferedAttack;
	TArray<TWeakObjectPtr<AActor>> HitActors;
	FVector PreviousTip = FVector::ZeroVector;
	int32 ComboIndex = 0;
	float LastAttackEndTime = -1000.f;
	bool bAttacking = false;
	bool bCanChain = false;
	bool bHitWindowOpen = false;

	FTimerHandle HeavyHoldTimer;
	FTimerHandle HitStartTimer;
	FTimerHandle HitEndTimer;
	FTimerHandle ChainTimer;
	FTimerHandle HitStopTimer;

	void RequestAttack(EWeaponAttackType Type);
	void StartAttack(EWeaponAttackType Type);
	void HandleHeavyHoldReached();
	void OpenHitWindow();
	void CloseHitWindow();
	void OpenChainWindow();
	void SweepBlade();
	void ApplyHit(AActor* Target);
	void StartHitStop(float Duration);
	void EndHitStop();
	void GetBlade(FVector& OutStart, FVector& OutEnd) const;
};
