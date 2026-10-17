#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LootSpot.generated.h"

class UArrowComponent;
class UBillboardComponent;

UENUM(BlueprintType)
enum class ELootSpotType : uint8
{
	Chest,
	FogChest,
	Barrel,
	ResourceNode
};

/** Hand-placed candidate location for ALootSpawner. Editor-only visuals; arrow colour shows the type. */
UCLASS()
class NOWAPROBA_API ALootSpot : public AActor
{
	GENERATED_BODY()

public:
	ALootSpot();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	ELootSpotType SpotType = ELootSpotType::Chest;

#if WITH_EDITORONLY_DATA
private:
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;

	UPROPERTY()
	TObjectPtr<UArrowComponent> Arrow;
#endif
};
