#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BlockComponent.generated.h"

class UAbilitySystemComponent;
class UAnimSequenceBase;
class UPushableComponent;
struct FOnAttributeChangeData;

UENUM(BlueprintType)
enum class EBlockEndReason : uint8
{
	/** Lasted the full BlockDuration. */
	Completed,
	/** Block attribute dropped to 0. */
	Broken,
	/** StopBlock (stagger, death, ...). */
	Interrupted
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBlockEnded, EBlockEndReason, Reason);

/**
 * Timed block with its own pool of block points (UCombatAttributeSet Block / MaxBlock). StartBlock fills Block to MaxBlock
 * and adds the State.Blocking tag for BlockDuration: hits from within BlockHalfAngle of the owner's facing (by the damage
 * source location) lose DamageReduction and go into Block before Health, and cause no poise damage or knockback; hits
 * from behind are not blocked. Meanwhile the owner turns toward the target at TurnRate (the component ticks only while
 * blocking), loops BlockAnimation on DefaultSlot and is harder to push (UPushableComponent speed x PushSpeedScale).
 * The block ends when it runs out (Completed), Block reaches 0 (Broken) or StopBlock is called (Interrupted); then
 * Block is emptied and the next block waits BlockCooldown. MaxBlock 0 = the owner never blocks.
 * Needs an ability system with UCombatAttributeSet on the owner.
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UBlockComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBlockComponent();

	/** Ignored unless CanBlock(). Target = what the owner turns toward (may be null). */
	UFUNCTION(BlueprintCallable, Category = "Block")
	void StartBlock(AActor* Target);

	/** Ends a running block as Interrupted (still starts the cooldown). */
	UFUNCTION(BlueprintCallable, Category = "Block")
	void StopBlock();

	/** MaxBlock > 0, not blocking and not on cooldown. */
	UFUNCTION(BlueprintPure, Category = "Block")
	bool CanBlock() const;

	UFUNCTION(BlueprintPure, Category = "Block")
	bool IsBlocking() const { return bBlocking; }

	/** Blocking and SourceLocation is within BlockHalfAngle of the owner's facing (2D). */
	UFUNCTION(BlueprintPure, Category = "Block")
	bool IsBlockingFrom(const FVector& SourceLocation) const;

	AActor* GetBlockTarget() const { return BlockTarget.Get(); }

	UPROPERTY(BlueprintAssignable, Category = "Block")
	FOnBlockEnded OnBlockEnded;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Share of blocked damage removed before the rest goes into Block (0.5 = half). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block", meta = (ClampMin = "0", ClampMax = "1"))
	float DamageReduction = 0.5f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Block points per block; 0 disables blocking. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block", meta = (ClampMin = "0"))
	float MaxBlock = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block", meta = (ClampMin = "0.1"))
	float BlockDuration = 5.f;

	/** Time after a block ends (any reason) before the next one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block", meta = (ClampMin = "0"))
	float BlockCooldown = 10.f;

	/** Hits from within this angle of the facing are blocked (90 = whole front half). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block", meta = (ClampMin = "0", ClampMax = "180"))
	float BlockHalfAngle = 70.f;

	/** Yaw speed toward the target while blocking (deg/s); slow enough that the player can circle to the side. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block", meta = (ClampMin = "0"))
	float TurnRate = 120.f;

	/** Multiplier on UPushableComponent's push speed while blocking (lower = harder to shove). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block", meta = (ClampMin = "0", ClampMax = "1"))
	float PushSpeedScale = 0.2f;

	/** Looped on DefaultSlot while blocking (optional). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block")
	TObjectPtr<UAnimSequenceBase> BlockAnimation;

private:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UPushableComponent> Pushable;

	TWeakObjectPtr<AActor> BlockTarget;
	FTimerHandle BlockTimer;
	FTimerHandle CooldownTimer;
	bool bBlocking = false;

	void HandleBlockChanged(const FOnAttributeChangeData& Data);
	void CompleteBlock();
	void EndBlock(EBlockEndReason Reason);
};
