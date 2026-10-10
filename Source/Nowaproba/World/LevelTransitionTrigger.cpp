#include "LevelTransitionTrigger.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

ALevelTransitionTrigger::ALevelTransitionTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->InitBoxExtent(FVector(100.f, 100.f, 100.f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	RootComponent = Trigger;

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Trigger);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetGenerateOverlapEvents(false);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Trigger);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
}

void ALevelTransitionTrigger::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (bTransitionRequested || !Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	if (DestinationLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: DestinationLevel is not set, ignoring."), *GetName());
		return;
	}

	bTransitionRequested = true;
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, DestinationLevel, true, Options);
}
