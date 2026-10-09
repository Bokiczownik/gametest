#include "GoldCoin.h"

#include "CoinComboComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "../Currency/CurrencyComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/RotatingMovementComponent.h"

AGoldCoin::AGoldCoin()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(50.f);
	Collision->SetCollisionProfileName(TEXT("Trigger"));
	RootComponent = Collision;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);

	Rotation = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("Rotation"));
	Rotation->RotationRate = FRotator(0.f, 180.f, 0.f);
}

void AGoldCoin::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!IsValid(this) || IsActorBeingDestroyed() || !Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	if (UCoinComboComponent* Combo = Pawn->FindComponentByClass<UCoinComboComponent>())
	{
		Combo->RegisterCoin(Value);
	}
	else if (UCurrencyComponent* Wallet = Pawn->FindComponentByClass<UCurrencyComponent>())
	{
		Wallet->AddCurrency(Value);
	}
	else
	{
		return;
	}

	Destroy();
}
