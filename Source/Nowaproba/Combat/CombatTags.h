#pragma once

#include "NativeGameplayTags.h"

/** Native gameplay tags used by combat code. */
namespace CombatTags
{
	/** Owner takes no damage / poise damage (dash i-frames). */
	NOWAPROBA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invulnerable);

	/** Granted by the dash cooldown effect. */
	NOWAPROBA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Dash);

	/** Weapon attacks (cancelled by the dash). */
	NOWAPROBA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack);

	/** Owner is staggered (poise broken): cannot attack or dash. */
	NOWAPROBA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Staggered);
}
