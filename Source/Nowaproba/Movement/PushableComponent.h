#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PushableComponent.generated.h"

class APawn;
class UCapsuleComponent;
class UPrimitiveComponent;
struct FHitResult;

/**
 * Lets other pawns walk through the owner character and shove it aside instead of being stopped by it (small enemies
 * the player can push past). At BeginPlay the owner's capsule switches its Pawn channel response from Block to Overlap;
 * while it overlaps other pawns the component ticks and moves the owner out of them in 2D (swept, so walls still block),
 * at most PushSpeed cm/s. Two pushable pawns each take half of the separation. Only the position changes: running
 * attacks, AI and facing are left alone. Weapon sweeps by object type still hit the capsule.
 */
UCLASS(ClassGroup = (Movement), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UPushableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPushableComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Temporary multiplier on PushSpeed (e.g. while blocking); 1 = normal. */
	UFUNCTION(BlueprintCallable, Category = "Pushable")
	void SetPushSpeedScale(float Scale) { PushSpeedScale = FMath::Max(0.f, Scale); }

protected:
	virtual void BeginPlay() override;

	/** Fastest the owner is moved out of an overlapping pawn (cm/s); lower = the owner feels heavier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pushable", meta = (ClampMin = "0"))
	float PushSpeed = 600.f;

private:
	UPROPERTY()
	TObjectPtr<UCapsuleComponent> Capsule;

	TArray<TWeakObjectPtr<APawn>> OverlappingPawns;
	float PushSpeedScale = 1.f;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
