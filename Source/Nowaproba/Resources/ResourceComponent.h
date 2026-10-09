#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceComponent.generated.h"

/** Temporary resource identifiers. */
UENUM(BlueprintType)
enum class EResourceType : uint8
{
	Resource1 UMETA(DisplayName = "Resource 1"),
	Resource2 UMETA(DisplayName = "Resource 2"),
	Resource3 UMETA(DisplayName = "Resource 3"),
	Count UMETA(Hidden)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnResourceChanged, EResourceType, Resource, int32, NewValue, int32, Delta);

/** Stores three independent resource balances. Each balance is always within [0, MAX_int32]. */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UResourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	static constexpr int32 NumResources = static_cast<int32>(EResourceType::Count);

	UResourceComponent();

	/** Returns 0 for an invalid resource type. */
	UFUNCTION(BlueprintPure, Category = "Resources")
	int32 GetResource(EResourceType Resource) const;

	/** Returns false if Resource is invalid, Amount is negative or the result would overflow. Adding 0 succeeds without change. */
	UFUNCTION(BlueprintCallable, Category = "Resources")
	bool AddResource(EResourceType Resource, int32 Amount);

	/** Returns false if Resource is invalid, Amount is negative or exceeds the current balance. Removing 0 succeeds without change. */
	UFUNCTION(BlueprintCallable, Category = "Resources")
	bool RemoveResource(EResourceType Resource, int32 Amount);

	/** Broadcast only when a balance actually changes. */
	UPROPERTY(BlueprintAssignable, Category = "Resources")
	FOnResourceChanged OnResourceChanged;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Resources")
	int32 Balances[NumResources] = {};

	static bool IsValidResource(EResourceType Resource);
	void SetResource(EResourceType Resource, int32 NewValue);
};
