#include "CameraControlComponent.h"

#include "EnhancedInputComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"

UCameraControlComponent::UCameraControlComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
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

	USpringArmComponent* Arm = GetSpringArm();
	if (!Arm)
	{
		SetComponentTickEnabled(false);
		return;
	}

	Arm->TargetArmLength = FMath::FInterpTo(Arm->TargetArmLength, TargetArmLength, DeltaTime, ZoomInterpSpeed);
	if (FMath::IsNearlyEqual(Arm->TargetArmLength, TargetArmLength, 1.f))
	{
		Arm->TargetArmLength = TargetArmLength;
		SetComponentTickEnabled(false);
	}
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
	Rotation.Yaw += Delta.X * RotateSpeed;
	Rotation.Pitch = FMath::Clamp(Rotation.Pitch + Delta.Y * RotateSpeed, FMath::Min(MinPitch, MaxPitch), FMath::Max(MinPitch, MaxPitch));
	Rotation.Roll = 0.f;
	Arm->SetWorldRotation(Rotation);
}
