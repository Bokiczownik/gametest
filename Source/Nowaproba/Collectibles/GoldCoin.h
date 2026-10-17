#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GoldCoin.generated.h"

class URotatingMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * Spinning coin picked up by a player-controlled pawn on overlap.
 * Pays Value into the pawn's wallet.
 * Mesh and material are set in the Blueprint subclass (BP_GoldCoin).
 */
UCLASS()
class NOWAPROBA_API AGoldCoin : public AActor
{
	GENERATED_BODY()

public:
	AGoldCoin();

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Coin")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Coin")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Coin")
	TObjectPtr<URotatingMovementComponent> Rotation;

	UPROPERTY(EditAnywhere, Category = "Coin", meta = (ClampMin = "1"))
	int32 Value = 5;
};
