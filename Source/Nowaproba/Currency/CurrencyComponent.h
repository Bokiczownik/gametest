#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CurrencyComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCurrencyBalanceChanged, int32, NewBalance, int32, Delta);

/** Stores the owner's currency balance. Balance is always within [0, MAX_int32]. */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UCurrencyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCurrencyComponent();

	UFUNCTION(BlueprintPure, Category = "Currency")
	int32 GetBalance() const { return Balance; }

	/** Returns false if Amount is negative or the result would overflow. Adding 0 succeeds without change. */
	UFUNCTION(BlueprintCallable, Category = "Currency")
	bool AddCurrency(int32 Amount);

	/** Returns false if Amount is negative or exceeds the current balance. Removing 0 succeeds without change. */
	UFUNCTION(BlueprintCallable, Category = "Currency")
	bool RemoveCurrency(int32 Amount);

	/** Broadcast only when the balance actually changes. */
	UPROPERTY(BlueprintAssignable, Category = "Currency")
	FOnCurrencyBalanceChanged OnBalanceChanged;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Currency")
	int32 Balance = 0;

	void SetBalance(int32 NewBalance);
};
