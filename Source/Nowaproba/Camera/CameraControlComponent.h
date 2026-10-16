#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraControlComponent.generated.h"

class APlayerController;
class UInputAction;
class USpringArmComponent;
struct FInputActionValue;

/**
 * Lives on the player controller. Zooms the possessed pawn's spring arm with the mouse wheel
 * and orbits it (yaw, clamped pitch) while the rotate action (right mouse button) is held.
 * Movement stays camera-relative because UMovementModeComponent reads the camera yaw.
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UCameraControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCameraControlComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	/** Axis1D action (mouse wheel): positive = zoom in. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Control|Input")
	TObjectPtr<UInputAction> ZoomAction;

	/** Boolean action (right mouse button): orbit while held. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Control|Input")
	TObjectPtr<UInputAction> RotateAction;

	/** Axis2D action (mouse delta): X = yaw, Y = pitch, used only while RotateAction is held. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Control|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Camera Control|Zoom", meta = (ClampMin = "100"))
	float MinArmLength = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera Control|Zoom", meta = (ClampMin = "100"))
	float MaxArmLength = 2400.f;

	/** Arm length change per wheel notch (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Control|Zoom", meta = (ClampMin = "1"))
	float ZoomStep = 200.f;

	/** How fast the arm length follows the target; 0 = instant. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Control|Zoom", meta = (ClampMin = "0"))
	float ZoomInterpSpeed = 10.f;

	/** Degrees per unit of mouse delta. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Control|Rotation", meta = (ClampMin = "0"))
	float RotateSpeed = 0.3f;

	/** Pitch limits (degrees, negative = looking down). Set both equal to lock pitch. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Control|Rotation", meta = (ClampMin = "-89", ClampMax = "0"))
	float MinPitch = -75.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera Control|Rotation", meta = (ClampMin = "-89", ClampMax = "0"))
	float MaxPitch = -30.f;

private:
	TWeakObjectPtr<USpringArmComponent> CachedArm;
	float TargetArmLength = 0.f;
	bool bRotating = false;

	APlayerController* GetLocalPlayerController() const;
	USpringArmComponent* GetSpringArm();

	void HandleZoom(const FInputActionValue& Value);
	void HandleRotateStarted(const FInputActionValue& Value);
	void HandleRotateCompleted(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
};
