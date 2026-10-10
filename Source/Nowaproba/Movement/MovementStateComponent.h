#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MovementStateComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;
struct FHitResult;

/** High-level movement state, derived from the character each time it is queried. */
UENUM(BlueprintType)
enum class ECharacterMoveState : uint8
{
	Walking,
	Sprinting,
	Crouching,
	Sliding,
	Jumping,
	Falling,
	Landing
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlideStateChanged, bool, bIsSliding);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBhopStacksChanged, int32, NewStacks);

/**
 * Crouch / slide / crouch-jump logic for the owning ACharacter, built on the engine's crouch and jump.
 * One input flow: Ctrl while moving faster than SprintSpeedThreshold starts a slide, otherwise crouches;
 * jump while sliding cancels the slide, jump while crouched gives a small height boost.
 * The slide only swaps friction/braking/acceleration on the movement component for its duration, so its length
 * follows the real entry speed and speed limits stay in force.
 * Slopes (from the movement component's current floor, measured along the movement direction): running speed is
 * scaled smoothly up/down hill, slides get a one-time downhill entry boost plus gravity along the slope, and the
 * crouch-jump gets a one-time forward push. Everything is capped; ticks every frame for the smooth speed scaling.
 * Bunny hop: jumping within BhopWindow after landing (or BhopBufferTime before it) adds a stack of extra speed;
 * missing the window on the ground or slowing below BhopMinSpeed clears all stacks.
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UMovementStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMovementStateComponent();

	UFUNCTION(BlueprintCallable, Category = "Movement State")
	void HandleCrouchPressed();

	UFUNCTION(BlueprintCallable, Category = "Movement State")
	void HandleCrouchReleased();

	/** Use instead of calling Jump directly: cancels a slide and applies the crouch-jump boost. */
	UFUNCTION(BlueprintCallable, Category = "Movement State")
	void HandleJumpPressed();

	UFUNCTION(BlueprintPure, Category = "Movement State")
	ECharacterMoveState GetMoveState() const;

	UFUNCTION(BlueprintPure, Category = "Movement State")
	bool IsSliding() const { return bSliding; }

	UPROPERTY(BlueprintAssignable, Category = "Movement State")
	FOnSlideStateChanged OnSlideStateChanged;

	UFUNCTION(BlueprintPure, Category = "Movement State|Bunny Hop")
	int32 GetBhopStacks() const { return BhopStacks; }

	UPROPERTY(BlueprintAssignable, Category = "Movement State|Bunny Hop")
	FOnBhopStacksChanged OnBhopStacksChanged;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Ground speed (cm/s) above which the character counts as sprinting; Ctrl above it starts a slide. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State", meta = (ClampMin = "0.0"))
	float SprintSpeedThreshold = 650.f;

	/** How long after touching down the state reports Landing. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State", meta = (ClampMin = "0.0"))
	float LandingStateDuration = 0.15f;

	/** Jump height multiplier when jumping out of a crouch (1.25 = 25% higher). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Crouch Jump", meta = (ClampMin = "1.0"))
	float CrouchJumpHeightMultiplier = 1.25f;

	/** Speed added along the movement direction when a slide starts. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slide", meta = (ClampMin = "0.0"))
	float SlideImpulse = 150.f;

	/** The entry impulse never pushes speed above this (an already faster character keeps its speed). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slide", meta = (ClampMin = "0.0"))
	float SlideMaxSpeed = 1300.f;

	/** Ground friction while sliding (normal ground friction is restored afterwards). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slide", meta = (ClampMin = "0.0"))
	float SlideGroundFriction = 0.5f;

	/** Constant braking deceleration while sliding (cm/s^2). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slide", meta = (ClampMin = "0.0"))
	float SlideBrakingDeceleration = 350.f;

	/** Fraction of normal MaxAcceleration available for steering while sliding. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slide", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SlideSteering = 0.35f;

	/** The slide ends when ground speed drops to this value. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slide", meta = (ClampMin = "0.0"))
	float SlideEndSpeed = 300.f;

	/** Hard cap on slide time in seconds. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slide", meta = (ClampMin = "0.1"))
	float SlideMaxDuration = 1.5f;

	/** Slopes flatter than this (degrees, along the movement direction) count as flat ground. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slope", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float SlopeDeadZoneAngle = 3.f;

	/** Slope angle (degrees) at which slope effects reach full strength; steeper slopes are not stronger. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slope", meta = (ClampMin = "1.0", ClampMax = "89.0"))
	float SlopeMaxInfluenceAngle = 25.f;

	/** Running speed reduction at full uphill influence (0.1 = 10% slower). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slope", meta = (ClampMin = "0.0", ClampMax = "0.9"))
	float UphillSpeedPenalty = 0.1f;

	/** Running speed increase at full downhill influence (0.1 = 10% faster). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slope", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DownhillSpeedBonus = 0.1f;

	/** How fast the running speed multiplier eases toward its slope target (higher = snappier). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slope", meta = (ClampMin = "0.1"))
	float SlopeSpeedInterpSpeed = 5.f;

	/** Extra slide entry speed at full downhill influence (none uphill). Still capped by SlideMaxSpeed. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slope", meta = (ClampMin = "0.0"))
	float SlideSlopeEntryBoost = 200.f;

	/** Fraction of gravity pulling along the slope while sliding (speeds up downhill, slows uphill). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slope", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SlideSlopeGravityScale = 0.5f;

	/** One-time forward speed added to a crouch-jump at full downhill influence. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slope", meta = (ClampMin = "0.0"))
	float CrouchJumpSlopeBoost = 150.f;

	/** Fraction of CrouchJumpSlopeBoost applied when jumping uphill. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slope", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CrouchJumpUphillBoostScale = 0.35f;

	/** The crouch-jump slope boost never pushes horizontal speed above this. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Slope", meta = (ClampMin = "0.0"))
	float CrouchJumpMaxHorizontalSpeed = 1200.f;

	/** Seconds after landing in which a jump counts as a bunny hop. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Bunny Hop", meta = (ClampMin = "0.0"))
	float BhopWindow = 0.2f;

	/** A jump pressed this many seconds before landing is performed on touchdown and counts as a hop. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Bunny Hop", meta = (ClampMin = "0.0"))
	float BhopBufferTime = 0.1f;

	/** Extra speed per stack, as a fraction of the current walk/sprint speed (0.08 = +8%). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Bunny Hop", meta = (ClampMin = "0.0"))
	float BhopSpeedBonusPerStack = 0.08f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Bunny Hop", meta = (ClampMin = "0"))
	int32 BhopMaxStacks = 5;

	/** Dropping below this ground speed (cm/s) clears all stacks. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement State|Bunny Hop", meta = (ClampMin = "0.0"))
	float BhopMinSpeed = 200.f;

private:
	UPROPERTY()
	TObjectPtr<ACharacter> Character;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> Movement;

	bool bCrouchHeld = false;

	bool bSliding = false;
	float SlideElapsed = 0.f;
	float SavedGroundFriction = 0.f;
	float SavedBrakingDeceleration = 0.f;
	float SavedMaxAcceleration = 0.f;

	bool bJumpBoostActive = false;
	float JumpBoostElapsed = 0.f;
	float SavedJumpZVelocity = 0.f;

	float LastLandedTime = -1.f;

	float SlideSpeedCap = 0.f;

	/** Walk speed as set by others (e.g. sprint); the slope multiplier is applied on top of it. */
	float BaseWalkSpeed = 0.f;
	float LastWrittenWalkSpeed = 0.f;
	float SlopeSpeedMultiplier = 1.f;

	int32 BhopStacks = 0;
	float LastAirJumpPressTime = -1.f;
	bool bBufferedJumpPending = false;

	/** One stack per landing, even if jump is pressed several times before takeoff. */
	bool bBhopUsedThisLanding = false;

	void StartSlide();
	void RegisterBhop();
	void SetBhopStacks(int32 NewStacks);
	void UpdateBhop();
	float GetBhopMultiplier() const { return 1.f + BhopSpeedBonusPerStack * BhopStacks; }
	void EndSlide();
	bool TryStandUpNow();
	void RestoreJumpBoost();
	void UpdateSlopeSpeed(float DeltaTime);
	void ApplySlideSlopeGravity(float DeltaTime);

	/** Signed floor slope in degrees along Direction (positive = uphill); 0 when not on walkable ground. */
	float GetSlopeAngleAlong(const FVector& Direction) const;

	/** 0..1 strength of a slope angle after the dead zone, reaching 1 at SlopeMaxInfluenceAngle. */
	float GetSlopeInfluence(float SlopeAngle) const;

	UFUNCTION()
	void HandleLanded(const FHitResult& Hit);
};
