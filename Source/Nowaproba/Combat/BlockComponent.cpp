#include "BlockComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "CombatAttributeSet.h"
#include "CombatTags.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "../Movement/PushableComponent.h"

namespace
{
	const FName BlockSlot = TEXT("DefaultSlot");
}

UBlockComponent::UBlockComponent()
{
	// Ticks only while blocking (turning toward the target).
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UBlockComponent::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (AbilitySystem && AbilitySystem->HasAttributeSetForAttribute(UCombatAttributeSet::GetBlockAttribute()))
	{
		AbilitySystem->SetNumericAttributeBase(UCombatAttributeSet::GetMaxBlockAttribute(), MaxBlock);
		AbilitySystem->GetGameplayAttributeValueChangeDelegate(UCombatAttributeSet::GetBlockAttribute()).AddUObject(this, &UBlockComponent::HandleBlockChanged);
	}
	else
	{
		AbilitySystem = nullptr;
	}
	Pushable = GetOwner()->FindComponentByClass<UPushableComponent>();
}

void UBlockComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	Super::EndPlay(EndPlayReason);
}

bool UBlockComponent::CanBlock() const
{
	return AbilitySystem && MaxBlock > 0.f && !bBlocking && !GetWorld()->GetTimerManager().IsTimerActive(CooldownTimer);
}

bool UBlockComponent::IsBlockingFrom(const FVector& SourceLocation) const
{
	if (!bBlocking)
	{
		return false;
	}
	const AActor* Owner = GetOwner();
	const FVector ToSource = (SourceLocation - Owner->GetActorLocation()).GetSafeNormal2D();
	return FVector::DotProduct(Owner->GetActorForwardVector().GetSafeNormal2D(), ToSource) >= FMath::Cos(FMath::DegreesToRadians(BlockHalfAngle));
}

void UBlockComponent::StartBlock(AActor* Target)
{
	StartBlockFor(Target, BlockDuration, BlockCooldown);
}

void UBlockComponent::StartBlockFor(AActor* Target, float Duration, float Cooldown)
{
	if (!CanBlock())
	{
		return;
	}

	bBlocking = true;
	bBlockedHit = false;
	CurrentCooldown = Cooldown;
	BlockTarget = Target;
	AbilitySystem->SetNumericAttributeBase(UCombatAttributeSet::GetBlockAttribute(), MaxBlock);
	AbilitySystem->AddLooseGameplayTag(CombatTags::State_Blocking);
	if (Pushable)
	{
		Pushable->SetPushSpeedScale(PushSpeedScale);
	}

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (BlockAnimation && AnimInstance)
	{
		// Loops until EndBlock stops the slot.
		AnimInstance->PlaySlotAnimationAsDynamicMontage(BlockAnimation, BlockSlot, 0.15f, 0.2f, 1.f, 1000000);
	}

	SetComponentTickEnabled(Target != nullptr);
	GetWorld()->GetTimerManager().SetTimer(BlockTimer, this, &UBlockComponent::CompleteBlock, FMath::Max(Duration, 0.1f));
}

void UBlockComponent::StopBlock()
{
	if (bBlocking)
	{
		EndBlock(EBlockEndReason::Interrupted);
	}
}

void UBlockComponent::CompleteBlock()
{
	EndBlock(EBlockEndReason::Completed);
}

void UBlockComponent::HandleBlockChanged(const FOnAttributeChangeData& Data)
{
	if (!bBlocking)
	{
		return;
	}
	if (Data.NewValue < Data.OldValue)
	{
		bBlockedHit = true;
	}
	if (Data.NewValue <= 0.f)
	{
		EndBlock(EBlockEndReason::Broken);
	}
}

void UBlockComponent::EndBlock(EBlockEndReason Reason)
{
	// Cleared first: emptying Block below re-enters HandleBlockChanged.
	bBlocking = false;
	SetComponentTickEnabled(false);
	GetWorld()->GetTimerManager().ClearTimer(BlockTimer);
	if (CurrentCooldown > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(CooldownTimer, CurrentCooldown, false);
	}

	AbilitySystem->RemoveLooseGameplayTag(CombatTags::State_Blocking);
	AbilitySystem->SetNumericAttributeBase(UCombatAttributeSet::GetBlockAttribute(), 0.f);
	if (Pushable)
	{
		Pushable->SetPushSpeedScale(1.f);
	}

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (BlockAnimation && AnimInstance)
	{
		AnimInstance->StopSlotAnimation(0.2f, BlockSlot);
	}

	OnBlockEnded.Broadcast(Reason);
	BlockTarget.Reset();
}

void UBlockComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	const AActor* Target = BlockTarget.Get();
	if (!Target)
	{
		return;
	}
	FVector ToTarget = Target->GetActorLocation() - Owner->GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		return;
	}
	FRotator Rotation = Owner->GetActorRotation();
	Rotation.Yaw = FMath::FixedTurn(Rotation.Yaw, ToTarget.Rotation().Yaw, TurnRate * DeltaTime);
	Owner->SetActorRotation(Rotation);
}
