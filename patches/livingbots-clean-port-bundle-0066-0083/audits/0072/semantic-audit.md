# Semantic audit 0072

## Native melee combat state

Replaces old MeleeAttackStart/Stop calls with Penqle Unit::Attack(target, true) and AttackStop().

Core impact: **none**. This patch is Mod-only.
