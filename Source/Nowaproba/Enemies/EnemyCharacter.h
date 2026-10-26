#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../Combat/BlockComponent.h"
#include "EnemyCharacter.generated.h"

class AFogHunterSpawner;
class UAnimSequenceBase;
class UCombatAbilitySystemComponent;
class UHitReactionComponent;
class UDataTable;
class UEnemyAttackComponent;
class UHealthComponent;
class UStaticMeshComponent;
class UWidgetComponent;

UENUM(BlueprintType)
enum class EEnemyBehaviour : uint8
{
	/** Stands still until it notices the player, then chases. */
	Chase,
	/** Walks PatrolPoints in a loop, chases when it notices the player, then resumes the route. */
	Patrol,
	/** Holds its spawn position; only fights while the player is within GuardRadius of it, then walks back. */
	Guard,
	/** Spawned in the fog by AFogHunterSpawner; knows where the player is and chases inside the spawner's hunt area, else walks back into the fog. */
	FogHunter
};

/** How a shield enemy (AEnemyCharacter::bShieldStances) fights right now; chosen by AEnemyAIController each update. */
UENUM(BlueprintType)
enum class EShieldStance : uint8
{
	/** Not fighting, or not a shield enemy: plain block on damage. */
	None,
	/** Screening a ranged ally (bBodyguard). */
	Bodyguard,
	/** Other non-shield allies fight nearby: holds the player in front, short blocks when the player swings, rare attacks. */
	Defensive,
	/** Alone: attacks whenever it can, short block only after being hit. */
	Aggressive
};

/**
 * Base enemy: Health + telegraphed melee Attack, driven by AEnemyAIController (auto-possessed when placed or spawned).
 * Detection/chase tuning lives here so each enemy BP can differ. On death: AI stops, the capsule stops blocking,
 * LootRow from LootTable is paid to the killer (UHealthComponent::GetLastDamageInstigator) and the corpse is removed
 * after CorpseLifetime. Mesh, anim BP and telegraph mesh/material are set in the BP child.
 * Block (UBlockComponent, off while its MaxBlock is 0): taking damage from the player starts a block when it is ready
 * and the enemy is not winding up an attack. A block that lasts its full duration is followed by the attack named
 * BlockCounterAttack; a broken block knocks the enemy down (KnockdownAnimation, no attacks or moving, no stagger);
 * a stagger interrupts it. HealthBar shows a small HP / block bar above the head.
 */
UCLASS()
class NOWAPROBA_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

	UHealthComponent* GetHealth() const { return Health; }
	UEnemyAttackComponent* GetAttack() const { return Attack; }
	UBlockComponent* GetBlock() const { return Block; }
	UHitReactionComponent* GetHitReaction() const { return HitReaction; }

	bool IsDead() const;

	/** Blocking or knocked down: the AI neither moves nor attacks. */
	bool IsActionLocked() const;

	/** For the anim BP: a slot animation drives the whole body (block, knockdown, full-body attack), so arm layers are dropped. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsPlayingFullBodyAnimation() const;

	/** Patrol points in world space (from PatrolPoints, fixed at BeginPlay). */
	const TArray<FVector>& GetPatrolRoute() const { return PatrolRoute; }

	/** Spawn location and facing; the guard's post. */
	const FVector& GetHomeLocation() const { return HomeLocation; }
	const FRotator& GetHomeRotation() const { return HomeRotation; }

	/** Set by AFogHunterSpawner before FinishSpawning. */
	void SetHunterSpawner(AFogHunterSpawner* InSpawner);
	AFogHunterSpawner* GetHunterSpawner() const;

	/** Uses ChaseSpeed while chasing, else PatrolWalkSpeed. */
	void SetChaseSpeed(bool bChasing);

	/** Set by AEnemyAIController while it runs at the player or is within reach to attack (anim BP raises the weapon). */
	void SetInCombat(bool bNewInCombat) { bInCombat = bNewInCombat; }

	EShieldStance GetShieldStance() const { return ShieldStance; }
	void SetShieldStance(EShieldStance NewStance) { ShieldStance = NewStance; }

	/** Defensive stance: the player Attacker started an attack; raise a short block if it is ready and the attacker is close. */
	void TryReactiveBlock(AActor* Attacker);

	/** Fog hunter chase: ChaseSpeed within HunterNearDistance of the player, HunterFarSpeed farther away. */
	void UpdateHunterChaseSpeed(float DistanceToPlayer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	EEnemyBehaviour Behaviour = EEnemyBehaviour::Chase;

	/** Notices the player within this distance (needs line of sight). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0"))
	float AggroRange = 1200.f;

	/** Gives up the chase beyond this distance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0"))
	float LoseAggroRange = 2000.f;

	/** Enemies with the same group (None = no group) start fighting together: when one engages the player, the others are alerted (and provoked). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	FName AggroGroup;

	/** After taking damage or a group alert the enemy keeps fighting this long, even beyond LoseAggroRange, its GuardRadius or the fog hunt area (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0"))
	float ProvokedDuration = 8.f;

	/** Half angle of the view cone for noticing the player (180 = all around). Taking damage always alerts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "1", ClampMax = "180"))
	float SightHalfAngle = 180.f;

	/** Route relative to the actor (drag the diamonds in the viewport); walked in a loop. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Patrol", meta = (MakeEditWidget, EditCondition = "Behaviour == EEnemyBehaviour::Patrol"))
	TArray<FVector> PatrolPoints;

	/** Pause at each patrol point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Patrol", meta = (ClampMin = "0"))
	float PatrolWaitTime = 1.5f;

	/** Walk speed while patrolling or returning to the post (chasing uses the movement component's MaxWalkSpeed). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0"))
	float PatrolWalkSpeed = 200.f;

	/** Fog hunter chase speed while farther than HunterNearDistance from the player. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Fog Hunter", meta = (ClampMin = "0"))
	float HunterFarSpeed = 250.f;

	/** Within this distance of the player a fog hunter chases at full speed (the movement component's MaxWalkSpeed). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Fog Hunter", meta = (ClampMin = "0"))
	float HunterNearDistance = 4000.f;

	/** Guard fights only while the player is within this distance of its post. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Guard", meta = (ClampMin = "0", EditCondition = "Behaviour == EEnemyBehaviour::Guard"))
	float GuardRadius = 800.f;

	/**
	 * Melee flanker: while 2+ chasing flankers are within FlankRadius of the player they spread evenly around it
	 * (2 = opposite sides, 3 = 120 deg apart; slots relative to each other, not to the player's facing). It still attacks
	 * as soon as the player is in reach and gives up circling after FlankTimeout (approaches directly as long again).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Flank")
	bool bFlank = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Flank", meta = (ClampMin = "0", EditCondition = "bFlank"))
	float FlankRadius = 800.f;

	/** Within this many degrees of its slot it closes in directly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Flank", meta = (ClampMin = "0", ClampMax = "90", EditCondition = "bFlank"))
	float FlankTolerance = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Flank", meta = (ClampMin = "0.5", EditCondition = "bFlank"))
	float FlankTimeout = 3.f;

	/**
	 * Bodyguard: while a chasing ranged ally (AEnemyRangedAIController, same AggroGroup) is within BodyguardRadius, it
	 * does not chase the player but stands GuardDistance in front of that ally on the line to the player, facing the
	 * player (several bodyguards split over the allies nearest the player). It only uses GuardShoveAttack when the player
	 * is in its reach and GuardChargeAttack when the player got past it and stands between it and the ally.
	 * Without such an ally it fights normally.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Bodyguard")
	bool bBodyguard = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Bodyguard", meta = (ClampMin = "0", EditCondition = "bBodyguard"))
	float BodyguardRadius = 1500.f;

	/** Distance from the guarded ally toward the player (at most half the way to the player). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Bodyguard", meta = (ClampMin = "0", EditCondition = "bBodyguard"))
	float GuardDistance = 350.f;

	/** Attack names (FEnemyAttack::Name, usually Weight 0 so the normal fight never picks them). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Bodyguard", meta = (EditCondition = "bBodyguard"))
	FName GuardShoveAttack = TEXT("GuardShove");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Bodyguard", meta = (EditCondition = "bBodyguard"))
	FName GuardChargeAttack = TEXT("ShieldCharge");

	/**
	 * Shield stances (EShieldStance) when not a bodyguard: Defensive while a chasing non-shield ally of the same AggroGroup
	 * is within DefensiveAllyRadius, else Aggressive. Defensive: faces the player at the edge of its reach, attacks at
	 * most every DefensiveAttackInterval, raises a ShortBlockDuration block when the player starts an attack within
	 * ReactiveBlockRange and follows a short block that stopped a hit with BlockShoveAttack; the long block on damage
	 * happens at most every LongBlockInterval. Aggressive: normal melee, damage raises a short block followed by
	 * BlockCounterAttack. Short blocks are followed by ShortBlockCooldown.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Shield")
	bool bShieldStances = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Shield", meta = (ClampMin = "0", EditCondition = "bShieldStances"))
	float DefensiveAllyRadius = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Shield", meta = (ClampMin = "0", EditCondition = "bShieldStances"))
	float DefensiveAttackInterval = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Shield", meta = (ClampMin = "0", EditCondition = "bShieldStances"))
	float ReactiveBlockRange = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Shield", meta = (ClampMin = "0.1", EditCondition = "bShieldStances"))
	float ShortBlockDuration = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Shield", meta = (ClampMin = "0", EditCondition = "bShieldStances"))
	float ShortBlockCooldown = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Shield", meta = (ClampMin = "0", EditCondition = "bShieldStances"))
	float LongBlockInterval = 15.f;

	/** Attack (FEnemyAttack::Name) after a defensive short block that stopped a hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Shield", meta = (EditCondition = "bShieldStances"))
	FName BlockShoveAttack = TEXT("BlockShove");

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Shield")
	EShieldStance ShieldStance = EShieldStance::None;

	/** True while the AI runs at the player or is within reach to attack. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy")
	bool bInCombat = false;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UHealthComponent> Health;

	/** Combat attributes (health storage) and abilities. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UCombatAbilitySystemComponent> AbilitySystem;

	/** Hit flash, knockback and stagger (poise break cancels the current attack). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UHitReactionComponent> HitReaction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UEnemyAttackComponent> Attack;

	/** Disabled unless MaxBlock > 0 (set in the BP). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UBlockComponent> Block;

	/** Small HP / block bar above the head (screen space). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UWidgetComponent> HealthBar;

	/** Height of the health bar above the top of the capsule (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0"))
	float HealthBarOffset = 25.f;

	/** Attack (FEnemyAttack::Name) started when a block lasts its full duration. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Block")
	FName BlockCounterAttack = TEXT("ShieldBash");

	/** Played on DefaultSlot when the block is broken; the enemy is down for its length. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Block")
	TObjectPtr<UAnimSequenceBase> KnockdownAnimation;

	/** Hit area shown during the attack windup (mesh/material in BP; hidden in game until an attack starts). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UStaticMeshComponent> AttackTelegraph;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Loot")
	TObjectPtr<UDataTable> LootTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Loot")
	FName LootRow = TEXT("Enemy");

	/** Seconds the body stays after death. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0.1"))
	float CorpseLifetime = 5.f;

private:
	TArray<FVector> PatrolRoute;
	FVector HomeLocation = FVector::ZeroVector;
	FRotator HomeRotation = FRotator::ZeroRotator;
	float ChaseSpeed = 0.f;
	TWeakObjectPtr<AFogHunterSpawner> HunterSpawner;
	float LastHealth = 0.f;
	bool bKnockedDown = false;
	bool bShortBlock = false;
	double NextLongBlockTime = 0.0;
	FTimerHandle KnockdownTimer;

	void StartShieldBlock(AActor* Target, bool bShort);
	void StartKnockdown();
	void EndKnockdown();

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);

	UFUNCTION()
	void HandleBlockEnded(EBlockEndReason Reason);

	UFUNCTION()
	void HandleDeath();

	UFUNCTION()
	void HandleStaggered();
};
