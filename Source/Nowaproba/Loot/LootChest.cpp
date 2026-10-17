#include "LootChest.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "LootLibrary.h"

ALootChest::ALootChest()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ALootChest::BeginPlay()
{
	Super::BeginPlay();
	SetLootRow(RowName);
}

void ALootChest::SetLootRow(FName NewRowName)
{
	RowName = NewRowName;
	if (const TObjectPtr<UMaterialInterface>* Material = RowMaterials.Find(RowName))
	{
		ApplyMaterial(*Material);
	}
}

void ALootChest::SetRevealed(bool bRevealed)
{
	SetActorHiddenInGame(!bRevealed);
	SetActorEnableCollision(bRevealed);
}

void ALootChest::Interact_Implementation(APawn* InstigatorPawn)
{
	if (bOpened)
	{
		return;
	}
	bOpened = true;

	ULootLibrary::RollAndGrantLoot(LootTable, RowName, InstigatorPawn);
	ApplyMaterial(OpenedMaterial);
	// Leaving the range clears the player's interaction target.
	InteractionRange->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALootChest::ApplyMaterial(UMaterialInterface* Material)
{
	if (Material)
	{
		for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
		{
			Mesh->SetMaterial(Index, Material);
		}
	}
}
