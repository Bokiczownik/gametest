#include "CombatTags.h"

namespace CombatTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable, "State.Invulnerable", "Owner takes no damage (dash i-frames).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Dash, "Cooldown.Dash", "Dash is on cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack, "Ability.Attack", "Weapon attack.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Staggered, "State.Staggered", "Poise broken: cannot attack or dash.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_StaggerImmune, "State.StaggerImmune", "Stagger cooldown: no poise damage or knockback.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Blocking, "State.Blocking", "Blocking: frontal hits reduced and absorbed by the Block attribute.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Stunned, "State.Stunned", "Stunned: cannot move, attack or dash.");
}
