#include "MovementStateComponent.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** Safety net: restore the boosted jump velocity even if the jump never happened. */
	constexpr float MaxJumpBoostWait = 0.3f;
}

UMovementStateComponent::UMovementStateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UMovementStateComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ACharacter>(GetOwner());
	Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: owner is not a Character with CharacterMovement, movement states disabled."), *GetName());
		return;
	}

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

	if (bSliding)
	{
		EndSlide();
		if (TryStandUpNow())
		{
			Character->Jump();
		}
		return;
	}

	if (Character->bIsCrouched && Movement->IsMovingOnGround())
	{
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
			UpdateTickEnabled();
		}
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

	if (bSliding)
	{
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

	UpdateTickEnabled();
}

void UMovementStateComponent::StartSlide()
{
	SavedGroundFriction = Movement->GroundFriction;
	SavedBrakingDeceleration = Movement->BrakingDecelerationWalking;
	SavedMaxAcceleration = Movement->MaxAcceleration;

	Movement->GroundFriction = SlideGroundFriction;
	Movement->BrakingDecelerationWalking = SlideBrakingDeceleration;
	Movement->MaxAcceleration = SavedMaxAcceleration * SlideSteering;

	// Entry boost along the current direction; the slide then decays from the real entry speed.
	const FVector Planar(Movement->Velocity.X, Movement->Velocity.Y, 0.f);
	const float Speed = Planar.Size();
	const float NewSpeed = FMath::Max(Speed, FMath::Min(Speed + SlideImpulse, SlideMaxSpeed));
	Movement->Velocity = Planar.GetSafeNormal() * NewSpeed + FVector(0.f, 0.f, Movement->Velocity.Z);

	Character->Crouch();

	bSliding = true;
	SlideElapsed = 0.f;
	OnSlideStateChanged.Broadcast(true);
	UpdateTickEnabled();
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

void UMovementStateComponent::UpdateTickEnabled()
{
	SetComponentTickEnabled(bSliding || bJumpBoostActive);
}

void UMovementStateComponent::HandleLanded(const FHitResult& Hit)
{
	LastLandedTime = GetWorld()->GetTimeSeconds();
	if (bCrouchHeld)
	{
		Character->Crouch();
	}
}
