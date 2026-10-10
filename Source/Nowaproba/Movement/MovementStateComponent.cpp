#include "MovementStateComponent.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** Safety net: restore the boosted jump velocity even if the jump never happened. */
	constexpr float MaxJumpBoostWait = 0.3f;

	/** Below this ground speed (cm/s) there is no meaningful movement direction for slope checks. */
	constexpr float MinSpeedForSlope = 10.f;
}

UMovementStateComponent::UMovementStateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UMovementStateComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ACharacter>(GetOwner());
	Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: owner is not a Character with CharacterMovement, movement states disabled."), *GetName());
		SetComponentTickEnabled(false);
		return;
	}

	BaseWalkSpeed = Movement->MaxWalkSpeed;
	LastWrittenWalkSpeed = BaseWalkSpeed;
	Character->LandedDelegate.AddUniqueDynamic(this, &UMovementStateComponent::HandleLanded);
}

void UMovementStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Character)
	{
		Character->LandedDelegate.RemoveDynamic(this, &UMovementStateComponent::HandleLanded);
	}
	if (bSliding)
	{
		EndSlide();
	}
	RestoreJumpBoost();
	if (Movement && FMath::IsNearlyEqual(Movement->MaxWalkSpeed, LastWrittenWalkSpeed))
	{
		Movement->MaxWalkSpeed = BaseWalkSpeed;
	}

	Super::EndPlay(EndPlayReason);
}

void UMovementStateComponent::HandleCrouchPressed()
{
	bCrouchHeld = true;
	if (!Movement || bSliding || !Movement->IsMovingOnGround())
	{
		// Airborne: HandleLanded crouches on touchdown if Ctrl is still held.
		return;
	}

	if (Movement->Velocity.Size2D() >= SprintSpeedThreshold && Character->CanCrouch())
	{
		StartSlide();
	}
	else
	{
		Character->Crouch();
	}
}

void UMovementStateComponent::HandleCrouchReleased()
{
	bCrouchHeld = false;
	if (Character && !bSliding)
	{
		// Engine uncrouch keeps retrying until there is headroom.
		Character->UnCrouch();
	}
}

void UMovementStateComponent::HandleJumpPressed()
{
	if (!Movement)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Movement->IsFalling())
	{
		// Remembered so a slightly early press still counts as a hop on touchdown.
		LastAirJumpPressTime = Now;
	}
	// A buffered early press is on time by definition, whatever the frame rate.
	const bool bInBhopWindow = Movement->IsMovingOnGround()
		&& (bBufferedJumpPending || (LastLandedTime >= 0.f && Now - LastLandedTime <= BhopWindow));

	if (bSliding)
	{
		EndSlide();
		if (TryStandUpNow())
		{
			if (bInBhopWindow)
			{
				RegisterBhop();
			}
			Character->Jump();
		}
		return;
	}

	if (Character->bIsCrouched && Movement->IsMovingOnGround())
	{
		// Read the slope before standing up changes anything.
		const FVector Planar(Movement->Velocity.X, Movement->Velocity.Y, 0.f);
		const float SlopeAngle = Planar.Size() > MinSpeedForSlope ? GetSlopeAngleAlong(Planar.GetSafeNormal()) : 0.f;

		if (!TryStandUpNow())
		{
			return;
		}
		if (!bJumpBoostActive)
		{
			SavedJumpZVelocity = Movement->JumpZVelocity;
			// Jump height scales with velocity squared.
			Movement->JumpZVelocity = SavedJumpZVelocity * FMath::Sqrt(CrouchJumpHeightMultiplier);
			bJumpBoostActive = true;
			JumpBoostElapsed = 0.f;

			// One-time forward push on slopes; it fades on its own after landing through normal ground friction.
			const float Influence = GetSlopeInfluence(SlopeAngle);
			if (Influence > 0.f)
			{
				const float Scale = SlopeAngle < 0.f ? 1.f : CrouchJumpUphillBoostScale;
				const float Speed = Planar.Size();
				const float NewSpeed = FMath::Max(Speed, FMath::Min(Speed + CrouchJumpSlopeBoost * Influence * Scale, CrouchJumpMaxHorizontalSpeed));
				const FVector Boosted = Planar.GetSafeNormal() * NewSpeed;
				Movement->Velocity = FVector(Boosted.X, Boosted.Y, Movement->Velocity.Z);
			}
		}
	}

	if (bInBhopWindow)
	{
		RegisterBhop();
	}
	Character->Jump();
}

ECharacterMoveState UMovementStateComponent::GetMoveState() const
{
	if (!Movement)
	{
		return ECharacterMoveState::Walking;
	}
	if (bSliding)
	{
		return ECharacterMoveState::Sliding;
	}
	if (Movement->IsFalling())
	{
		return Movement->Velocity.Z > 0.f ? ECharacterMoveState::Jumping : ECharacterMoveState::Falling;
	}
	if (LastLandedTime >= 0.f && GetWorld()->GetTimeSeconds() - LastLandedTime < LandingStateDuration)
	{
		return ECharacterMoveState::Landing;
	}
	if (Character->bIsCrouched)
	{
		return ECharacterMoveState::Crouching;
	}
	return Movement->Velocity.Size2D() > SprintSpeedThreshold ? ECharacterMoveState::Sprinting : ECharacterMoveState::Walking;
}

void UMovementStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bJumpBoostActive)
	{
		JumpBoostElapsed += DeltaTime;
		if (Movement->IsFalling() || JumpBoostElapsed >= MaxJumpBoostWait)
		{
			RestoreJumpBoost();
		}
	}

	// Buffered early press: Landed fires before the movement mode switches to walking, so jump on the next tick.
	if (bBufferedJumpPending && Movement->IsMovingOnGround())
	{
		HandleJumpPressed();
		bBufferedJumpPending = false;
	}

	UpdateBhop();

	if (bSliding)
	{
		ApplySlideSlopeGravity(DeltaTime);

		SlideElapsed += DeltaTime;
		if (!Movement->IsMovingOnGround() || Movement->Velocity.Size2D() <= SlideEndSpeed || SlideElapsed >= SlideMaxDuration)
		{
			EndSlide();
			if (!bCrouchHeld)
			{
				Character->UnCrouch();
			}
		}
	}

	UpdateSlopeSpeed(DeltaTime);
}

void UMovementStateComponent::StartSlide()
{
	SavedGroundFriction = Movement->GroundFriction;
	SavedBrakingDeceleration = Movement->BrakingDecelerationWalking;
	SavedMaxAcceleration = Movement->MaxAcceleration;

	Movement->GroundFriction = SlideGroundFriction;
	Movement->BrakingDecelerationWalking = SlideBrakingDeceleration;
	Movement->MaxAcceleration = SavedMaxAcceleration * SlideSteering;

	// Entry boost along the current direction (bigger downhill, applied once); the slide then decays from the real entry speed.
	const FVector Planar(Movement->Velocity.X, Movement->Velocity.Y, 0.f);
	const float Speed = Planar.Size();
	const float SlopeAngle = GetSlopeAngleAlong(Planar.GetSafeNormal());
	const float DownhillBoost = SlopeAngle < 0.f ? SlideSlopeEntryBoost * GetSlopeInfluence(SlopeAngle) : 0.f;
	const float NewSpeed = FMath::Max(Speed, FMath::Min(Speed + SlideImpulse + DownhillBoost, SlideMaxSpeed));
	Movement->Velocity = Planar.GetSafeNormal() * NewSpeed + FVector(0.f, 0.f, Movement->Velocity.Z);
	SlideSpeedCap = FMath::Max(SlideMaxSpeed, NewSpeed);

	Character->Crouch();

	bSliding = true;
	SlideElapsed = 0.f;
	OnSlideStateChanged.Broadcast(true);
}

void UMovementStateComponent::EndSlide()
{
	if (!bSliding)
	{
		return;
	}

	Movement->GroundFriction = SavedGroundFriction;
	Movement->BrakingDecelerationWalking = SavedBrakingDeceleration;
	Movement->MaxAcceleration = SavedMaxAcceleration;

	bSliding = false;
	OnSlideStateChanged.Broadcast(false);
}

bool UMovementStateComponent::TryStandUpNow()
{
	Character->UnCrouch();
	if (Character->bIsCrouched)
	{
		// Character::UnCrouch only defers to the next movement update; jumping needs the full capsule now.
		// Movement's UnCrouch does the headroom check and leaves us crouched if blocked.
		Movement->UnCrouch(false);
	}
	return !Character->bIsCrouched;
}

void UMovementStateComponent::RestoreJumpBoost()
{
	if (!bJumpBoostActive)
	{
		return;
	}

	Movement->JumpZVelocity = SavedJumpZVelocity;
	bJumpBoostActive = false;
}

void UMovementStateComponent::UpdateSlopeSpeed(float DeltaTime)
{
	// Someone else (e.g. sprint) changed the walk speed since our last write: that is the new base.
	if (!FMath::IsNearlyEqual(Movement->MaxWalkSpeed, LastWrittenWalkSpeed))
	{
		BaseWalkSpeed = Movement->MaxWalkSpeed;
	}

	float Target = 1.f;
	const FVector Planar(Movement->Velocity.X, Movement->Velocity.Y, 0.f);
	if (!bSliding && Movement->IsMovingOnGround() && Planar.Size() > MinSpeedForSlope)
	{
		const float SlopeAngle = GetSlopeAngleAlong(Planar.GetSafeNormal());
		const float Influence = GetSlopeInfluence(SlopeAngle);
		Target = SlopeAngle > 0.f ? 1.f - UphillSpeedPenalty * Influence : 1.f + DownhillSpeedBonus * Influence;
	}

	SlopeSpeedMultiplier = FMath::FInterpTo(SlopeSpeedMultiplier, Target, DeltaTime, SlopeSpeedInterpSpeed);
	if (FMath::IsNearlyEqual(SlopeSpeedMultiplier, Target, 0.001f))
	{
		// Snap so flat ground ends up exactly at the base speed.
		SlopeSpeedMultiplier = Target;
	}

	LastWrittenWalkSpeed = BaseWalkSpeed * SlopeSpeedMultiplier * GetBhopMultiplier();
	Movement->MaxWalkSpeed = LastWrittenWalkSpeed;
}

void UMovementStateComponent::RegisterBhop()
{
	if (bBhopUsedThisLanding || Movement->Velocity.Size2D() < BhopMinSpeed)
	{
		return;
	}
	bBhopUsedThisLanding = true;
	SetBhopStacks(FMath::Min(BhopStacks + 1, BhopMaxStacks));

	// Raise the speed cap now and give an instant push toward it, so each hop is felt immediately.
	UpdateSlopeSpeed(0.f);
	const FVector Planar(Movement->Velocity.X, Movement->Velocity.Y, 0.f);
	const float Speed = Planar.Size();
	const float NewSpeed = FMath::Max(Speed, FMath::Min(Speed + BaseWalkSpeed * BhopSpeedBonusPerStack, Movement->MaxWalkSpeed));
	const FVector Boosted = Planar.GetSafeNormal() * NewSpeed;
	Movement->Velocity = FVector(Boosted.X, Boosted.Y, Movement->Velocity.Z);
}

void UMovementStateComponent::SetBhopStacks(int32 NewStacks)
{
	if (NewStacks == BhopStacks)
	{
		return;
	}

	BhopStacks = NewStacks;
	OnBhopStacksChanged.Broadcast(BhopStacks);
}

void UMovementStateComponent::UpdateBhop()
{
	if (BhopStacks == 0)
	{
		return;
	}

	const bool bTooSlow = Movement->Velocity.Size2D() < BhopMinSpeed;
	const bool bMissedWindow = Movement->IsMovingOnGround() && !bBufferedJumpPending
		&& (LastLandedTime < 0.f || GetWorld()->GetTimeSeconds() - LastLandedTime > BhopWindow);
	if (bTooSlow || bMissedWindow)
	{
		// The higher speed cap goes away; ground friction then removes the extra speed.
		SetBhopStacks(0);
	}
}

void UMovementStateComponent::ApplySlideSlopeGravity(float DeltaTime)
{
	if (!Movement->IsMovingOnGround() || !Movement->CurrentFloor.IsWalkableFloor())
	{
		return;
	}

	// Gravity component along the floor plane: length is sin(slope), direction is straight downhill.
	const FVector Normal = Movement->CurrentFloor.HitResult.ImpactNormal;
	const FVector Down = FVector::DownVector - Normal * FVector::DotProduct(FVector::DownVector, Normal);
	const float SinSlope = Down.Size();
	if (SinSlope <= FMath::Sin(FMath::DegreesToRadians(SlopeDeadZoneAngle)))
	{
		return;
	}

	const float Acceleration = FMath::Abs(Movement->GetGravityZ()) * SinSlope * SlideSlopeGravityScale;
	Movement->Velocity += Down / SinSlope * Acceleration * DeltaTime;
	Movement->Velocity = Movement->Velocity.GetClampedToMaxSize(SlideSpeedCap);
}

float UMovementStateComponent::GetSlopeAngleAlong(const FVector& Direction) const
{
	if (!Movement->IsMovingOnGround() || !Movement->CurrentFloor.IsWalkableFloor())
	{
		return 0.f;
	}

	// Rise per horizontal unit when moving along Direction on a plane with this normal.
	const FVector Normal = Movement->CurrentFloor.HitResult.ImpactNormal;
	const float Rise = -(Normal.X * Direction.X + Normal.Y * Direction.Y) / FMath::Max(Normal.Z, KINDA_SMALL_NUMBER);
	return FMath::RadiansToDegrees(FMath::Atan(Rise));
}

float UMovementStateComponent::GetSlopeInfluence(float SlopeAngle) const
{
	const float Range = FMath::Max(SlopeMaxInfluenceAngle - SlopeDeadZoneAngle, KINDA_SMALL_NUMBER);
	return FMath::Clamp((FMath::Abs(SlopeAngle) - SlopeDeadZoneAngle) / Range, 0.f, 1.f);
}

void UMovementStateComponent::HandleLanded(const FHitResult& Hit)
{
	LastLandedTime = GetWorld()->GetTimeSeconds();
	bBhopUsedThisLanding = false;
	if (LastAirJumpPressTime >= 0.f && LastLandedTime - LastAirJumpPressTime <= BhopBufferTime)
	{
		bBufferedJumpPending = true;
	}
	LastAirJumpPressTime = -1.f;

	if (bCrouchHeld)
	{
		Character->Crouch();
	}
}
