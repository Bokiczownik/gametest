#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class APawn;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Something the player can use with the interact key (E) while UInteractionComponent has it as target. */
class NOWAPROBA_API IInteractable
{
	GENERATED_BODY()

public:
	virtual void Interact(APawn* InstigatorPawn) = 0;
};
