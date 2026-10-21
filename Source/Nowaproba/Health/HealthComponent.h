#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class AController;
class APawn;
class UAbilitySystemComponent;
class UAnimationAsset;
class UDamageType;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, CurrentHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

/**
 * Health pool on the owning actor. Takes damage from the engine damage system (UGameplayStatics::ApplyDamage /
 * OnTakeAnyDamage) or directly via ApplyHealthDamage. At 0 the owner dies once: OnDeath fires, character movement
 * stops and DeathAnimation (if set) plays and holds its last frame.
 * If the owner has a UCombatAbilitySystemComponent, health lives in UCombatAttributeSet (Health / MaxHealth, damage via
 * the Damage meta attribute, so gameplay effects and invulnerability apply); this component mirrors it.
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bDead; }

	/** Pawn behind the last engine damage taken (instigator's pawn, else the causer if it is a pawn); used for kill rewards. */
	UFUNCTION(BlueprintPure, Category = "Health")
	APawn* GetLastDamageInstigator() const { return LastDamageInstigator.Get(); }

	/** Ignored when dead or Amount <= 0. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyHealthDamage(float Amount);

	/** Ignored when dead or Amount <= 0. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);

	/** For items / level-ups later; current health is kept, clamped to the new maximum. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetMaxHealth(float NewMaxHealth);

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDeath OnDeath;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.f;

	/** Played on the owner's mesh at death (holds the last frame). */
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	TObjectPtr<UAnimationAsset> DeathAnimation;

private:
	float CurrentHealth = 0.f;
	bool bDead = false;
	TWeakObjectPtr<APawn> LastDamageInstigator;

	/** Set when health is stored in the owner's combat attributes. */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	void HandleHealthAttributeChanged(const FOnAttributeChangeData& Data);

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	/** Source = damage causer, so a block (UCombatAttributeSet) can tell where the hit came from. */
	void ApplyDamageFrom(float Amount, const AActor* Source);
	void SetHealth(float NewHealth);
	void Die();
};
