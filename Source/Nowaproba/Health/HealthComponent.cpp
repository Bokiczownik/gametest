#include "HealthComponent.h"

#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	SetHealth(MaxHealth);
	GetOwner()->OnTakeAnyDamage.AddUniqueDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
}

void UHealthComponent::ApplyHealthDamage(float Amount)
{
	if (bDead || Amount <= 0.f)
	{
		return;
	}

	SetHealth(CurrentHealth - Amount);
	if (CurrentHealth <= 0.f)
	{
		Die();
	}
}

void UHealthComponent::Heal(float Amount)
{
	if (!bDead && Amount > 0.f)
	{
		SetHealth(CurrentHealth + Amount);
	}
}

void UHealthComponent::SetMaxHealth(float NewMaxHealth)
{
	MaxHealth = FMath::Max(1.f, NewMaxHealth);
	SetHealth(CurrentHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (bDead || Damage <= 0.f)
	{
		return;
	}

	APawn* InstigatorPawn = InstigatedBy ? InstigatedBy->GetPawn() : nullptr;
	LastDamageInstigator = InstigatorPawn ? InstigatorPawn : Cast<APawn>(DamageCauser);
	ApplyHealthDamage(Damage);
}

void UHealthComponent::SetHealth(float NewHealth)
{
	NewHealth = FMath::Clamp(NewHealth, 0.f, MaxHealth);
	if (NewHealth == CurrentHealth)
	{
		return;
	}

	CurrentHealth = NewHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UHealthComponent::Die()
{
	bDead = true;

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCharacterMovement()->StopMovementImmediately();
		Character->GetCharacterMovement()->DisableMovement();
		if (DeathAnimation)
		{
			Character->GetMesh()->PlayAnimation(DeathAnimation, false);
		}
	}

	OnDeath.Broadcast();
}
