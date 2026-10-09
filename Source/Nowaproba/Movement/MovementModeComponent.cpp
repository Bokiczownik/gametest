#include "MovementModeComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"

UMovementModeComponent::UMovementModeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMovementModeComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentMode = DefaultMode;

	APlayerController* PC = GetLocalPlayerController();
	if (!PC)
	{
		return;
	}

	ApplyMappingContexts();

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PC->InputComponent))
	{
		if (MoveAction)
		{
			Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &UMovementModeComponent::HandleMove);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: owner has no EnhancedInputComponent, WASD movement disabled."), *GetName());
	}
}

void UMovementModeComponent::SetMovementMode(EPlayerMovementMode NewMode)
{
	if (NewMode == CurrentMode)
	{
		return;
	}

	CurrentMode = NewMode;

	APlayerController* PC = GetLocalPlayerController();
	if (PC && NewMode != EPlayerMovementMode::PointAndClick)
	{
		// Cancel a click destination that is still being followed.
		PC->StopMovement();
	}

	ApplyMappingContexts();
	OnMovementModeChanged.Broadcast(CurrentMode);
}

APlayerController* UMovementModeComponent::GetLocalPlayerController() const
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	return PC && PC->IsLocalController() ? PC : nullptr;
}

void UMovementModeComponent::ApplyMappingContexts() const
{
	const APlayerController* PC = GetLocalPlayerController();
	const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return;
	}

	const bool bWASD = CurrentMode == EPlayerMovementMode::WASD;
	UInputMappingContext* Active = bWASD ? WASDContext.Get() : PointAndClickContext.Get();
	UInputMappingContext* Inactive = bWASD ? PointAndClickContext.Get() : WASDContext.Get();

	if (Inactive)
	{
		Subsystem->RemoveMappingContext(Inactive);
	}
	if (Active)
	{
		Subsystem->AddMappingContext(Active, ContextPriority);
	}
}

void UMovementModeComponent::HandleMove(const FInputActionValue& Value)
{
	const APlayerController* PC = GetLocalPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	const float Yaw = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation().Yaw : PC->GetControlRotation().Yaw;
	const FRotator YawRotation(0.f, Yaw, 0.f);
	const FVector2D Axis = Value.Get<FVector2D>();

	Pawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Axis.X);
	// Mapping uses +Y for "left" (A), so negate for the right vector.
	Pawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), -Axis.Y);
}
