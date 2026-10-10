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

/**
 * Crouch / slide / crouch-jump logic for the owning ACharacter, built on the engine's crouch and jump.
 * One input flow: Ctrl while moving faster than SprintSpeedThreshold starts a slide, otherwise crouches;
 * jump while sliding cancels the slide, jump while crouched gives a small height boost.
 * The slide only swaps friction/braking/acceleration on the movement component for its duration, so its length
 * follows the real entry speed and speed limits stay in force. Ticks only while sliding or a jump boost is pending.
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

	void StartSlide();
	void EndSlide();
	bool TryStandUpNow();
	void RestoreJumpBoost();
	void UpdateTickEnabled();

	UFUNCTION()
	void HandleLanded(const FHitResult& Hit);
};
