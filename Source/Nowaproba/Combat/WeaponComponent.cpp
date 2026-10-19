#include "WeaponComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "CombatAttributeSet.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "HitReactionComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "WeaponAttackAbility.h"
#include "../Stamina/StaminaComponent.h"

UWeaponComponent::UWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	AttackAbilityClass = UWeaponAttackAbility::StaticClass();
}

void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !WeaponData)
	{
		return;
	}

	WeaponMesh = NewObject<UStaticMeshComponent>(Character, TEXT("WeaponMesh"));
	WeaponMesh->SetStaticMesh(WeaponData->Mesh);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetupAttachment(Character->GetMesh(), WeaponData->AttachSocket);
	WeaponMesh->SetRelativeTransform(WeaponData->AttachTransform);
	WeaponMesh->RegisterComponent();

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Character);
	if (ASC && AttackAbilityClass && !ASC->FindAbilitySpecFromClass(AttackAbilityClass))
	{
		ASC->GiveAbility(FGameplayAbilitySpec(AttackAbilityClass));
	}
}

void UWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld()->GetTimerManager().IsTimerActive(HitStopTimer))
	{
		EndHitStop();
	}
	Super::EndPlay(EndPlayReason);
}

void UWeaponComponent::AttackPressed()
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !WeaponData)
	{
		return;
	}

	if (Character->GetCharacterMovement()->IsFalling())
	{
		RequestAttack(EWeaponAttackType::Jump);
		return;
	}

	const UStaminaComponent* Stamina = Character->FindComponentByClass<UStaminaComponent>();
	if (Stamina && Stamina->IsDraining())
	{
		RequestAttack(EWeaponAttackType::Sprint);
		return;
	}

	// Light or heavy is decided on release / when the hold time is reached.
	GetWorld()->GetTimerManager().SetTimer(HeavyHoldTimer, this, &UWeaponComponent::HandleHeavyHoldReached, WeaponData->HeavyHoldTime);
}

void UWeaponComponent::AttackReleased()
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	if (Timers.IsTimerActive(HeavyHoldTimer))
	{
		Timers.ClearTimer(HeavyHoldTimer);
		RequestAttack(EWeaponAttackType::Light);
	}
}

void UWeaponComponent::HandleHeavyHoldReached()
{
	RequestAttack(EWeaponAttackType::Heavy);
}

void UWeaponComponent::RequestAttack(EWeaponAttackType Type)
{
	if (!bAttacking && GetWorld()->GetTimeSeconds() - LastAttackEndTime < WeaponData->AttackCooldown)
	{
		return;
	}
	if (bAttacking && !bCanChain)
	{
		BufferedAttack = Type;
		return;
	}
	StartAttack(Type);
}

void UWeaponComponent::StartAttack(EWeaponAttackType Type)
{
	BufferedAttack.Reset();

	const FWeaponAttack* Attack = nullptr;
	switch (Type)
	{
	case EWeaponAttackType::Light:
		if (WeaponData->LightCombo.Num() > 0)
		{
			// Chaining from a running attack continues the combo; otherwise it restarts after a pause.
			if (!bAttacking && GetWorld()->GetTimeSeconds() - LastAttackEndTime > WeaponData->ComboResetTime)
			{
				ComboIndex = 0;
			}
			Attack = &WeaponData->LightCombo[ComboIndex % WeaponData->LightCombo.Num()];
			ComboIndex = (ComboIndex + 1) % WeaponData->LightCombo.Num();
		}
		break;
	case EWeaponAttackType::Heavy:
		Attack = &WeaponData->HeavyAttack;
		ComboIndex = 0;
		break;
	case EWeaponAttackType::Sprint:
		Attack = &WeaponData->SprintAttack;
		ComboIndex = 0;
		break;
	case EWeaponAttackType::Jump:
		Attack = &WeaponData->JumpAttack;
		ComboIndex = 0;
		break;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!Attack || !Attack->Animation || !ASC || !AttackAbilityClass)
	{
		return;
	}

	PendingAttack = Attack;
	ASC->TryActivateAbilityByClass(AttackAbilityClass);
	PendingAttack = nullptr;
}

void UWeaponComponent::NotifyAttackStarted(const FWeaponAttack& Attack)
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(HitStartTimer);
	Timers.ClearTimer(HitEndTimer);
	Timers.ClearTimer(ChainTimer);
	CloseHitWindow();

	CurrentAttack = &Attack;
	bAttacking = true;
	bCanChain = false;
	HitActors.Reset();

	auto SetAnimTimer = [&](FTimerHandle& Handle, float AnimTime, void (UWeaponComponent::*Callback)())
	{
		const float Delay = (AnimTime - Attack.StartTime) / Attack.PlayRate;
		if (Delay > 0.f)
		{
			Timers.SetTimer(Handle, this, Callback, Delay);
		}
		else
		{
			(this->*Callback)();
		}
	};
	SetAnimTimer(HitStartTimer, Attack.HitStartTime, &UWeaponComponent::OpenHitWindow);
	SetAnimTimer(HitEndTimer, Attack.HitEndTime, &UWeaponComponent::CloseHitWindow);
	SetAnimTimer(ChainTimer, Attack.ChainTime, &UWeaponComponent::OpenChainWindow);
}

void UWeaponComponent::NotifyAttackEnded()
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(HitStartTimer);
	Timers.ClearTimer(HitEndTimer);
	Timers.ClearTimer(ChainTimer);
	CloseHitWindow();

	CurrentAttack = nullptr;
	bAttacking = false;
	bCanChain = false;
	LastAttackEndTime = GetWorld()->GetTimeSeconds();
}

void UWeaponComponent::OpenHitWindow()
{
	FVector BladeStart;
	GetBlade(BladeStart, PreviousTip);
	bHitWindowOpen = true;
	SetComponentTickEnabled(true);
	SweepBlade();
}

void UWeaponComponent::CloseHitWindow()
{
	bHitWindowOpen = false;
	SetComponentTickEnabled(false);
}

void UWeaponComponent::OpenChainWindow()
{
	bCanChain = true;
	if (BufferedAttack.IsSet())
	{
		StartAttack(BufferedAttack.GetValue());
	}
}

void UWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bHitWindowOpen)
	{
		SweepBlade();
	}
}

void UWeaponComponent::GetBlade(FVector& OutStart, FVector& OutEnd) const
{
	const FTransform& Transform = WeaponMesh->GetComponentTransform();
	OutStart = Transform.TransformPosition(FVector(WeaponData->BladeStart, 0.f, 0.f));
	OutEnd = Transform.TransformPosition(FVector(WeaponData->BladeEnd, 0.f, 0.f));
}

void UWeaponComponent::SweepBlade()
{
	if (!WeaponMesh || !CurrentAttack)
	{
		return;
	}

	FVector BladeStart, Tip;
	GetBlade(BladeStart, Tip);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectParams.AddObjectTypesToQuery(ECC_Destructible);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponSweep), false, GetOwner());
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(WeaponData->HitRadius);

	// Along the blade, and along the path of the tip since the last frame (fast swings).
	TArray<FHitResult> Hits;
	TArray<FHitResult> TipHits;
	GetWorld()->SweepMultiByObjectType(Hits, BladeStart, Tip, FQuat::Identity, ObjectParams, Sphere, QueryParams);
	GetWorld()->SweepMultiByObjectType(TipHits, PreviousTip, Tip, FQuat::Identity, ObjectParams, Sphere, QueryParams);
	Hits.Append(TipHits);

	if (bDrawDebugHits)
	{
		DrawDebugLine(GetWorld(), BladeStart, Tip, FColor::Green, false, 1.f, 0, 2.f);
		DrawDebugLine(GetWorld(), PreviousTip, Tip, FColor::Green, false, 1.f, 0, 2.f);
	}
	PreviousTip = Tip;

	for (const FHitResult& Hit : Hits)
	{
		AActor* Target = Hit.GetActor();
		if (Target && !HitActors.Contains(Target))
		{
			HitActors.Add(Target);
			if (bDrawDebugHits)
			{
				DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 25.f, 8, FColor::Red, false, 1.f);
			}
			ApplyHit(Target);
		}
	}
}

void UWeaponComponent::ApplyHit(AActor* Target)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	UGameplayStatics::ApplyDamage(Target, CurrentAttack->Damage, OwnerPawn ? OwnerPawn->GetController() : nullptr, GetOwner(), nullptr);

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (TargetASC && TargetASC->HasAttributeSetForAttribute(UCombatAttributeSet::GetPoiseDamageAttribute()))
	{
		UCombatAttributeSet::ApplyDamage(TargetASC, 0.f, CurrentAttack->PoiseDamage);
	}

	if (UHitReactionComponent* HitReaction = Target->FindComponentByClass<UHitReactionComponent>())
	{
		const FVector Direction = (Target->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal2D();
		HitReaction->ApplyKnockback(Direction * CurrentAttack->Knockback);
	}

	StartHitStop(CurrentAttack->HitStop);
}

void UWeaponComponent::StartHitStop(float Duration)
{
	if (Duration <= 0.f)
	{
		return;
	}

	// Timers run in dilated game time, so the real-time duration is scaled down by the dilation.
	constexpr float Dilation = 0.05f;
	UGameplayStatics::SetGlobalTimeDilation(this, Dilation);
	GetWorld()->GetTimerManager().SetTimer(HitStopTimer, this, &UWeaponComponent::EndHitStop, Duration * Dilation);
}

void UWeaponComponent::EndHitStop()
{
	GetWorld()->GetTimerManager().ClearTimer(HitStopTimer);
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
}
