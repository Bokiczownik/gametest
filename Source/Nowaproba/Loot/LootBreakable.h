#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LootBreakable.generated.h"

class UDataTable;
class UStaticMeshComponent;

/**
 * Barrel / resource node: takes engine damage (UGameplayStatics::ApplyDamage), and when HitPoints reach 0 rolls
 * RowName from LootTable, pays it to the pawn that dealt the last hit and destroys itself.
 */
UCLASS()
class NOWAPROBA_API ALootBreakable : public AActor
{
	GENERATED_BODY()

public:
	ALootBreakable();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UDataTable> LootTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FName RowName = TEXT("Barrel");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	float HitPoints = 1.f;

private:
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	bool bBroken = false;
};
