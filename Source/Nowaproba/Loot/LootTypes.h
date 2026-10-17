#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "../Resources/ResourceComponent.h"
#include "LootTypes.generated.h"

/** Shared rarity scale for chests and (later) items. */
UENUM(BlueprintType)
enum class ERarity : uint8
{
	None,
	Common,
	Rare,
	Epic,
	Legendary
};

/** Which resource a loot source gives. Random picks one of the three on every roll. */
UENUM(BlueprintType)
enum class ELootResource : uint8
{
	Random,
	Resource1 UMETA(DisplayName = "Resource 1"),
	Resource2 UMETA(DisplayName = "Resource 2"),
	Resource3 UMETA(DisplayName = "Resource 3")
};

/** One loot source (barrel, chest category, resource node). Edited in Content/Data/Loot/DT_Loot.csv. */
USTRUCT(BlueprintType)
struct NOWAPROBA_API FLootTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Weight in the random chest pool (PickWeightedRow). 0 = never picked from the pool. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	float SpawnWeight = 0.f;

	/** Chance (0..1) that anything drops at all. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0", ClampMax = "1"))
	float DropChance = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	int32 CoinsMin = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	int32 CoinsMax = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0", ClampMax = "1"))
	float ItemChance = 0.f;

	/** Item rarity is rolled uniformly between Min and Max. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	ERarity ItemRarityMin = ERarity::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	ERarity ItemRarityMax = ERarity::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0", ClampMax = "1"))
	float ResourceChance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	ELootResource ResourceType = ELootResource::Random;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	int32 ResourceMin = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	int32 ResourceMax = 0;
};

/** Outcome of one roll. Item is only a rarity until an item system exists. */
USTRUCT(BlueprintType)
struct NOWAPROBA_API FLootResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	int32 Coins = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	bool bHasResource = false;

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	EResourceType Resource = EResourceType::Resource1;

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	int32 ResourceAmount = 0;

	/** None = no item. */
	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	ERarity ItemRarity = ERarity::None;
};
