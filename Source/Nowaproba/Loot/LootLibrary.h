#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LootTypes.h"
#include "LootLibrary.generated.h"

class UDataTable;

/** Rolls loot from a FLootTableRow data table and pays it to the player. */
UCLASS()
class NOWAPROBA_API ULootLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Rolls one drop for RowName. Returns an empty result (and logs) if the row is missing. */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	static FLootResult RollLoot(const UDataTable* Table, FName RowName);

	/** Picks a row by SpawnWeight (rows with weight 0 are skipped); OnlyRows limits the candidates (empty = all rows). Returns None if no candidate has weight. */
	UFUNCTION(BlueprintCallable, Category = "Loot", meta = (AutoCreateRefTerm = "OnlyRows"))
	static FName PickWeightedRow(const UDataTable* Table, const TArray<FName>& OnlyRows);

	/** Adds coins and resources to the pawn's Currency/Resources components; items are only shown on screen for now. */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	static void GrantLoot(APawn* Pawn, const FLootResult& Loot);

	/** RollLoot + GrantLoot. */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	static FLootResult RollAndGrantLoot(const UDataTable* Table, FName RowName, APawn* Pawn);
};
