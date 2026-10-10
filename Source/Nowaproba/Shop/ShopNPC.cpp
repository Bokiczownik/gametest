#include "ShopNPC.h"

#include "../Interaction/InteractionComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ShopWidget.h"

AShopNPC::AShopNPC()
{
	PrimaryActorTick.bCanEverTick = false;

	Body = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Body"));
	Body->InitCapsuleSize(42.f, 96.f);
	Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = Body;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Body);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));

	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRange"));
	InteractionRange->SetupAttachment(Body);
	InteractionRange->InitSphereRadius(250.f);
	InteractionRange->SetCollisionProfileName(TEXT("Trigger"));

	NameLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameLabel"));
	NameLabel->SetupAttachment(Body);
	NameLabel->SetHorizontalAlignment(EHTA_Center);
	NameLabel->SetText(NSLOCTEXT("Shop", "Name", "SHOP"));

	PromptLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PromptLabel"));
	PromptLabel->SetupAttachment(Body);
	PromptLabel->SetHorizontalAlignment(EHTA_Center);
	PromptLabel->SetText(NSLOCTEXT("Shop", "Prompt", "Press E"));
	PromptLabel->SetHiddenInGame(true);

	ShopWidgetClass = UShopWidget::StaticClass();
}

void AShopNPC::BeginPlay()
{
	Super::BeginPlay();

	InteractionRange->OnComponentBeginOverlap.AddUniqueDynamic(this, &AShopNPC::HandleRangeBeginOverlap);
	InteractionRange->OnComponentEndOverlap.AddUniqueDynamic(this, &AShopNPC::HandleRangeEndOverlap);
}

void AShopNPC::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CloseShop();
	Super::EndPlay(EndPlayReason);
}

void AShopNPC::Interact(APawn* InstigatorPawn)
{
	if (IsShopOpen())
	{
		CloseShop();
	}
	else
	{
		OpenShop(InstigatorPawn);
	}
}

bool AShopNPC::IsShopOpen() const
{
	return ShopWidget && ShopWidget->IsInViewport();
}

void AShopNPC::HandleRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	UInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UInteractionComponent>() : nullptr;
	if (!Interaction)
	{
		return;
	}

	Interaction->SetTarget(this);
	PromptLabel->SetHiddenInGame(false);
}

void AShopNPC::HandleRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	UInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UInteractionComponent>() : nullptr;
	if (!Interaction)
	{
		return;
	}

	Interaction->ClearTarget(this);
	PromptLabel->SetHiddenInGame(true);
	CloseShop();
}

void AShopNPC::OpenShop(APawn* InstigatorPawn)
{
	APlayerController* PC = InstigatorPawn ? Cast<APlayerController>(InstigatorPawn->GetController()) : nullptr;
	if (!PC || !ShopWidgetClass)
	{
		return;
	}

	if (!ShopWidget)
	{
		ShopWidget = CreateWidget<UUserWidget>(PC, ShopWidgetClass);
	}
	if (ShopWidget && !ShopWidget->IsInViewport())
	{
		ShopWidget->AddToViewport(10);
	}
}

void AShopNPC::CloseShop()
{
	if (ShopWidget)
	{
		ShopWidget->RemoveFromParent();
	}
}
