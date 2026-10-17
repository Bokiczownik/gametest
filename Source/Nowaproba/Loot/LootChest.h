#pragma once

#include "CoreMinimal.h"
#include "../Interaction/InteractableActor.h"
#include "LootChest.generated.h"

class UDataTable;
class UMaterialInterface;
class UStaticMeshComponent;

/**
 * Chest opened with the interact input: rolls RowName from LootTable once and pays it to the player.
 * Material follows the row (RowMaterials) and switches to OpenedMaterial after opening.
 * Fog chests are spawned hidden and shown with SetRevealed(true) when the fog reaches them.
 */
UCLASS()
class NOWAPROBA_API ALootChest : public AInteractableActor
{
	GENERATED_BODY()

public:
	ALootChest();

	virtual void Interact_Implementation(APawn* InstigatorPawn) override;

	UFUNCTION(BlueprintCallable, Category = "Loot")
	void SetLootRow(FName NewRowName);

	/** Hidden chests have no collision and cannot be interacted with. */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void SetRevealed(bool bRevealed);

	UFUNCTION(BlueprintPure, Category = "Loot")
	bool IsOpened() const { return bOpened; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UDataTable> LootTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FName RowName = TEXT("Chest_Common");

	/** Material per loot row (e.g. Chest_Rare -> blue); rows without an entry keep the mesh material. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TMap<FName, TObjectPtr<UMaterialInterface>> RowMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UMaterialInterface> OpenedMaterial;

private:
	bool bOpened = false;

	void ApplyMaterial(UMaterialInterface* Material);
};
