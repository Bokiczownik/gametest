#include "LootBreakable.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "LootLibrary.h"

ALootBreakable::ALootBreakable()
{
	PrimaryActorTick.bCanEverTick = false;

	// Scene root so the mesh can be offset (engine basic shapes are centred, spots sit on the ground).
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ALootBreakable::BeginPlay()
{
	Super::BeginPlay();
	OnTakeAnyDamage.AddUniqueDynamic(this, &ALootBreakable::HandleTakeAnyDamage);
}

void ALootBreakable::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (bBroken || Damage <= 0.f)
	{
		return;
	}

	HitPoints -= Damage;
	if (HitPoints > 0.f)
	{
		return;
	}
	bBroken = true;

	APawn* Pawn = InstigatedBy ? InstigatedBy->GetPawn() : nullptr;
	if (!Pawn)
	{
		Pawn = Cast<APawn>(DamageCauser);
	}
	ULootLibrary::RollAndGrantLoot(LootTable, RowName, Pawn);
	Destroy();
}
