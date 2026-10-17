#include "InteractableActor.h"

#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "InteractionComponent.h"

AInteractableActor::AInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRange"));
	InteractionRange->SetupAttachment(Root);
	InteractionRange->InitSphereRadius(250.f);
	InteractionRange->SetCollisionProfileName(TEXT("Trigger"));
}

void AInteractableActor::BeginPlay()
{
	Super::BeginPlay();

	InteractionRange->OnComponentBeginOverlap.AddUniqueDynamic(this, &AInteractableActor::HandleRangeBeginOverlap);
	InteractionRange->OnComponentEndOverlap.AddUniqueDynamic(this, &AInteractableActor::HandleRangeEndOverlap);
}

void AInteractableActor::HandleRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	UInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UInteractionComponent>() : nullptr;
	if (!Interaction)
	{
		return;
	}

	Interaction->SetTarget(this);
	OnPlayerEnteredRange(Pawn);
}

void AInteractableActor::HandleRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	UInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UInteractionComponent>() : nullptr;
	if (!Interaction)
	{
		return;
	}

	Interaction->ClearTarget(this);
	OnPlayerLeftRange(Pawn);
}
