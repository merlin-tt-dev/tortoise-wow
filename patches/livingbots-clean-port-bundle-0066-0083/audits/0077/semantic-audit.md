# Semantic audit 0077

## Native RPG interaction movement pause

Replaces direct interaction-timer/waypoint control with Creature::PauseOutOfCombatMovement(). Also guards the Creature cast with TYPEID_UNIT to avoid undefined behavior for player RPG targets.

Core impact: **none**. This patch is Mod-only.
