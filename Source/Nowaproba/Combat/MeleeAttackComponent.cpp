#include "MeleeAttackComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UMeleeAttackComponent::UMeleeAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMeleeAttackComponent::Attack()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || World->GetTimeSeconds() - LastAttackTime < Cooldown)
	{
		return;
	}
	LastAttackTime = World->GetTimeSeconds();

	const FVector Center = Owner->GetActorLocation() + Owner->GetActorForwardVector() * Reach;

	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectTypes.AddObjectTypesToQuery(ECC_Pawn);
	ObjectTypes.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectTypes.AddObjectTypesToQuery(ECC_Destructible);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MeleeAttack), false, Owner);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjectTypes, FCollisionShape::MakeSphere(Radius), Params);

	// One hit per actor even if several of its components overlap.
	TSet<AActor*> Hit;
	const APawn* Pawn = Cast<APawn>(Owner);
	AController* Instigator = Pawn ? Pawn->GetController() : nullptr;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (Target && !Hit.Contains(Target))
		{
			Hit.Add(Target);
			UGameplayStatics::ApplyDamage(Target, Damage, Instigator, Owner, nullptr);
		}
	}

	if (bDrawDebug)
	{
		DrawDebugSphere(World, Center, Radius, 12, Hit.Num() > 0 ? FColor::Red : FColor::White, false, 0.2f);
	}
}
