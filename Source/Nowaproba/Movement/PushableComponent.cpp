#include "PushableComponent.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

UPushableComponent::UPushableComponent()
{
	// Ticks only while overlapping other pawns.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UPushableComponent::BeginPlay()
{
	Super::BeginPlay();

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	if (!Capsule)
	{
		return;
	}
	Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Capsule->SetGenerateOverlapEvents(true);
	Capsule->OnComponentBeginOverlap.AddUniqueDynamic(this, &UPushableComponent::HandleBeginOverlap);
	Capsule->OnComponentEndOverlap.AddUniqueDynamic(this, &UPushableComponent::HandleEndOverlap);
}

void UPushableComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn && Pawn != GetOwner() && OtherComp == Pawn->GetRootComponent())
	{
		OverlappingPawns.AddUnique(Pawn);
		SetComponentTickEnabled(true);
	}
}

void UPushableComponent::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	OverlappingPawns.Remove(Cast<APawn>(OtherActor));
	if (OverlappingPawns.Num() == 0)
	{
		SetComponentTickEnabled(false);
	}
}

void UPushableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner || !Capsule)
	{
		return;
	}

	const FVector Location = Owner->GetActorLocation();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	FVector Push = FVector::ZeroVector;
	for (const TWeakObjectPtr<APawn>& Other : OverlappingPawns)
	{
		const UCapsuleComponent* OtherCapsule = Other.IsValid() ? Cast<UCapsuleComponent>(Other->GetRootComponent()) : nullptr;
		if (!OtherCapsule || OtherCapsule->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
		{
			continue;
		}
		FVector Away = Location - Other->GetActorLocation();
		Away.Z = 0.f;
		const float Distance = Away.Size();
		const float Penetration = Radius + OtherCapsule->GetScaledCapsuleRadius() - Distance;
		if (Penetration <= 0.f)
		{
			continue;
		}
		// Exactly on top of each other: pick a side from the other pawn's facing.
		const FVector Direction = Distance > KINDA_SMALL_NUMBER ? Away / Distance : Other->GetActorRightVector();
		const float Share = Other->FindComponentByClass<UPushableComponent>() ? 0.5f : 1.f;
		Push += Direction * Penetration * Share;
	}

	const float MaxStep = PushSpeed * PushSpeedScale * DeltaTime;
	if (Push.SizeSquared() > FMath::Square(MaxStep))
	{
		Push = Push.GetSafeNormal() * MaxStep;
	}
	if (!Push.IsNearlyZero())
	{
		Owner->AddActorWorldOffset(Push, true);
	}
}
