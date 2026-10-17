#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoinComboComponent.generated.h"

class UCurrencyComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboMultiplierChanged, float, NewMultiplier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboProgressChanged, float, NewProgress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCoinRewarded, int32, Amount, float, Multiplier);

/**
 * Coin combo on the player pawn. Each coin collected while the combo bar is not empty raises the
 * multiplier by MultiplierStep (up to MaxMultiplier) and refills the bar; the bar drains over
 * ComboDuration seconds and the multiplier falls back to x1 when it empties.
 * Rewards are paid into the owner's UCurrencyComponent. Ticks only while a combo is running.
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UCoinComboComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCoinComboComponent();

	/** Applies the combo to BaseValue, pays the result into the wallet and returns it (0 if there is no wallet). */
	UFUNCTION(BlueprintCallable, Category = "Combo")
	int32 RegisterCoin(int32 BaseValue);

	UFUNCTION(BlueprintPure, Category = "Combo")
	float GetMultiplier() const { return Multiplier; }

	/** Remaining combo bar, 1 = just collected, 0 = combo over. */
	UFUNCTION(BlueprintPure, Category = "Combo")
	float GetComboProgress() const { return Progress; }

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool IsComboActive() const { return Progress > 0.f; }

	UPROPERTY(BlueprintAssignable, Category = "Combo")
	FOnComboMultiplierChanged OnMultiplierChanged;

	/** Broadcast every frame while the bar drains, and when it is refilled. */
	UPROPERTY(BlueprintAssignable, Category = "Combo")
	FOnComboProgressChanged OnProgressChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combo")
	FOnCoinRewarded OnCoinRewarded;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	/** Seconds for a full bar to drain to zero. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo", meta = (ClampMin = "0.1"))
	float ComboDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Combo", meta = (ClampMin = "0.0"))
	float MultiplierStep = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Combo", meta = (ClampMin = "1.0"))
	float MaxMultiplier = 5.f;

private:
	float Multiplier = 1.f;
	float Progress = 0.f;

	TWeakObjectPtr<UCurrencyComponent> Currency;

	void SetMultiplier(float NewMultiplier);
};
