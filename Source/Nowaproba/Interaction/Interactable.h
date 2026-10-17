#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class APawn;

UINTERFACE(MinimalAPI, BlueprintType)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Something the player can use with the interact key (E) while UInteractionComponent has it as target. */
class NOWAPROBA_API IInteractable
{
	GENERATED_BODY()

public:
	/** C++: override Interact_Implementation. Blueprint: implement the Interact event. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(APawn* InstigatorPawn);
};
