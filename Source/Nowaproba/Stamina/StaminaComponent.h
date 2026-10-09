#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float, CurrentStamina, float, MaxStamina);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaDepleted);

/**
 * Stamina pool on the player pawn, used by sprint.
 * While draining (and the owner is actually moving) stamina drops by DrainPerSecond; once draining stops,
 * regeneration starts after RegenDelay seconds. Max stamina = (BaseMaxStamina + MaxStaminaBonus) * MaxStaminaMultiplier,
 * so items / character level can change it through SetMaxStaminaBonus / SetMaxStaminaMultiplier.
 * Ticks only while draining or regenerating.
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStaminaComponent();

	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetStamina() const { return CurrentStamina; }

	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetMaxStamina() const { return MaxStamina; }

	UFUNCTION(BlueprintPure, Category = "Stamina")
	bool HasStamina() const { return CurrentStamina > 0.f; }

	/** Start/stop continuous drain (e.g. sprint held). Stopping starts the regen delay. */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void SetDraining(bool bNewDraining);

	UFUNCTION(BlueprintPure, Category = "Stamina")
	bool IsDraining() const { return bDraining; }

	/** Flat max stamina bonus from items / character level. */
	UFUNCTION(BlueprintCallable, Category = "Stamina|Max")
	void SetMaxStaminaBonus(float NewBonus);

	/** Multiplier on (base + bonus) max stamina, e.g. from talents. */
	UFUNCTION(BlueprintCallable, Category = "Stamina|Max")
	void SetMaxStaminaMultiplier(float NewMultiplier);

	UPROPERTY(BlueprintAssignable, Category = "Stamina")
	FOnStaminaChanged OnStaminaChanged;

	/** Broadcast when stamina hits 0 while draining; draining is stopped automatically. */
	UPROPERTY(BlueprintAssignable, Category = "Stamina")
	FOnStaminaDepleted OnStaminaDepleted;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Stamina|Max", meta = (ClampMin = "1.0"))
	float BaseMaxStamina = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stamina|Max")
	float MaxStaminaBonus = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stamina|Max", meta = (ClampMin = "0.0"))
	float MaxStaminaMultiplier = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
	float DrainPerSecond = 25.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
	float RegenPerSecond = 20.f;

	/** Seconds after the last drain before regeneration starts. */
	UPROPERTY(EditDefaultsOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
	float RegenDelay = 3.f;

	/** Drain only while the owner moves faster than this (cm/s), so holding sprint while standing is free. */
	UPROPERTY(EditDefaultsOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
	float MinMoveSpeedToDrain = 10.f;

private:
	float CurrentStamina = 0.f;
	float MaxStamina = 0.f;
	float RegenDelayRemaining = 0.f;
	bool bDraining = false;

	void RecalculateMaxStamina();
	void SetStamina(float NewStamina);
	void UpdateTickEnabled();
};
