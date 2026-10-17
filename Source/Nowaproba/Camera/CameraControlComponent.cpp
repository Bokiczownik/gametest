#include "CameraControlComponent.h"

#include "EnhancedInputComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"

UCameraControlComponent::UCameraControlComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Ticks for look-ahead; turns itself off when look-ahead is disabled and no zoom is running.
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UCameraControlComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetLocalPlayerController();
	if (!PC)
	{
		return;
	}

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PC->InputComponent);
	if (!Input)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: owner has no EnhancedInputComponent, camera control disabled."), *GetName());
		return;
	}

	if (ZoomAction)
	{
		Input->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &UCameraControlComponent::HandleZoom);
	}
	if (RotateAction)
	{
		Input->BindAction(RotateAction, ETriggerEvent::Started, this, &UCameraControlComponent::HandleRotateStarted);
		Input->BindAction(RotateAction, ETriggerEvent::Completed, this, &UCameraControlComponent::HandleRotateCompleted);
		Input->BindAction(RotateAction, ETriggerEvent::Canceled, this, &UCameraControlComponent::HandleRotateCompleted);
	}
	if (LookAction)
	{
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &UCameraControlComponent::HandleLook);
	}
}

void UCameraControlComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Keeps ticking without a pawn when look-ahead is on, so it resumes after respawn.
	USpringArmComponent* Arm = GetSpringArm();
	if (!Arm)
	{
		if (LookAheadDistance <= 0.f)
		{
			SetComponentTickEnabled(false);
		}
		return;
	}

	UpdateLookAhead(Arm, DeltaTime);

	if (!FMath::IsNearlyEqual(Arm->TargetArmLength, TargetArmLength, 1.f))
	{
		Arm->TargetArmLength = FMath::FInterpTo(Arm->TargetArmLength, TargetArmLength, DeltaTime, ZoomInterpSpeed);
	}
	else
	{
		Arm->TargetArmLength = TargetArmLength;
		if (LookAheadDistance <= 0.f)
		{
			SetComponentTickEnabled(false);
		}
	}
}

void UCameraControlComponent::UpdateLookAhead(USpringArmComponent* Arm, float DeltaTime)
{
	if (LookAheadDistance <= 0.f)
	{
		return;
	}

	FVector Velocity = Arm->GetOwner()->GetVelocity();
	Velocity.Z = 0.f;
	const float Scale = FMath::Min(Velocity.Size() / LookAheadFullSpeed, 1.f);
	const FVector Desired = Velocity.GetSafeNormal() * LookAheadDistance * Scale;

	LookAheadOffset = FMath::VInterpTo(LookAheadOffset, Desired, DeltaTime, LookAheadInterpSpeed);
	Arm->TargetOffset = BaseTargetOffset + LookAheadOffset;
}

APlayerController* UCameraControlComponent::GetLocalPlayerController() const
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	return PC && PC->IsLocalController() ? PC : nullptr;
}

USpringArmComponent* UCameraControlComponent::GetSpringArm()
{
	const APlayerController* PC = GetLocalPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return nullptr;
	}

	if (!CachedArm.IsValid() || CachedArm->GetOwner() != Pawn)
	{
		CachedArm = Pawn->FindComponentByClass<USpringArmComponent>();
		if (CachedArm.IsValid())
		{
			TargetArmLength = FMath::Clamp(CachedArm->TargetArmLength, MinArmLength, MaxArmLength);
			// Base from the template, not the live value: a re-created arm keeps the old look-ahead in TargetOffset.
			const USpringArmComponent* Template = Cast<USpringArmComponent>(CachedArm->GetArchetype());
			BaseTargetOffset = Template ? Template->TargetOffset : FVector::ZeroVector;
			LookAheadOffset = FVector::ZeroVector;
		}
	}
	return CachedArm.Get();
}

void UCameraControlComponent::HandleZoom(const FInputActionValue& Value)
{
	USpringArmComponent* Arm = GetSpringArm();
	const float Notches = Value.Get<float>();
	if (!Arm || FMath::IsNearlyZero(Notches))
	{
		return;
	}

	TargetArmLength = FMath::Clamp(TargetArmLength - Notches * ZoomStep, MinArmLength, MaxArmLength);
	if (ZoomInterpSpeed <= 0.f)
	{
		Arm->TargetArmLength = TargetArmLength;
		return;
	}
	SetComponentTickEnabled(true);
}

void UCameraControlComponent::HandleRotateStarted(const FInputActionValue& Value)
{
	bRotating = true;
}

void UCameraControlComponent::HandleRotateCompleted(const FInputActionValue& Value)
{
	bRotating = false;
}

void UCameraControlComponent::HandleLook(const FInputActionValue& Value)
{
	if (!bRotating)
	{
		return;
	}

	USpringArmComponent* Arm = GetSpringArm();
	if (!Arm)
	{
		return;
	}

	const FVector2D Delta = Value.Get<FVector2D>();
	FRotator Rotation = Arm->GetComponentRotation();
	Rotation.Yaw += Delta.X * RotateSpeed * YawSpeedMultiplier;
	Rotation.Pitch = FMath::Clamp(Rotation.Pitch + Delta.Y * RotateSpeed, FMath::Min(MinPitch, MaxPitch), FMath::Max(MinPitch, MaxPitch));
	Rotation.Roll = 0.f;
	Arm->SetWorldRotation(Rotation);
}
