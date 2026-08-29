# 0041 semantic audit — native trainer/RPG port

## Source model

Historical Playerbot reference:
- `cmangos/playerbots` commit `076045efa835da9aab7caa943bca752aebe1baad`
- historical `TrainerAction.cpp` expects fork-era `TrainerSpell::learnedSpell`, `isProvidedReqLevel`, and related fields.

Target host model:
- `merlin-tt-dev/tortoise-wow`, branch `livingbots/playerbots-clean-base`
- verified HEAD `769a18d3a2b57a67a15ebc2bf31d1c7631d7a519`

Penqle/Tortoise host facts used for the port:

1. `TrainerSpell` contains only:
   - `spell`
   - `spellCost`
   - `reqSkill`
   - `reqSkillValue`
   - `reqLevel`

2. `ObjectMgr::LoadTrainers()` rejects trainer rows whose spell is not a `SPELL_EFFECT_LEARN_SPELL` wrapper and normalizes `reqLevel` while loading.

3. `Player::GetTrainerSpellState(TrainerSpell const*)` resolves the wrapper's triggered learned spell and performs known-spell, class/race, level, chain, skill, and profession-limit checks.

4. `Creature::IsTrainerOf(Player*, bool)` already implements host-native class/pet/mount/tradeskill trainer eligibility including faction/exalted mount handling.

5. `Creature::GetTrainerSpells()` and `Creature::GetTrainerTemplateSpells()` are the native accessors for the two trainer spell sources.

## Mappings

### TrainerAction

- Removed Classic dependence on `TrainerSpell::learnedSpell` and `isProvidedReqLevel`.
- Uses `Player::GetTrainerSpellState(tSpell)` as the authority for trainability.
- When learning, resolves `SPELL_EFFECT_LEARN_SPELL` triggers from `TrainerSpell::spell` and calls native `Player::LearnSpell()`.
- Retains a wrapper-cast fallback for defensive compatibility, matching the semantic approach already used in patch 0039's PlayerbotFactory trainer work.
- `hasTrainable` is set after native GREEN-state and user spell-filter checks.
- `Creature::IsTrainer()` and `CreatureInfo::trainer_type` replace fork-era names.

### RpgTrainTrigger

- Deleted the Playerbot-local duplicate `RpgTrainTrigger::IsTrainerOf(CreatureInfo*, Player*)`.
- Resolves the actual spawned `Creature` and delegates to `Creature::IsTrainerOf(bot, false)`.
- Uses native trainer spell accessors and creature-based reputation discount.
- Uses native `GetTrainerSpellState(tSpell)`; no separate learned-spell reimplementation remains in the Classic path.

### TrainerValues

- Maps `CreatureInfo` fields to Penqle snake_case names.
- Trainer-spell equivalence now compares exactly the fields that exist in Penqle's `TrainerSpell` representation.
- For tradeskill grouping, when no explicit required skill is present, the learned spell is resolved through the trainer wrapper's `EffectTriggerSpell[EFFECT_INDEX_0]`, preserving the historical meaning of `learnedSpell`.
- Initial-profession filtering uses that same resolved learned-spell ID.

### AutoLearnSpellAction

- Uses Penqle casing: `LearnSpell()` and `LearnDefaultSpells()`.
- Uses native trainer field names and `GetTrainerSpellState(tSpell)`.
- Existing `LearnSpellFromSpell()` remains the semantic mechanism for Classic trainer learning wrappers.

### Other RpgTriggers compile blockers in the same TU

Because `RpgTriggers.cpp` must compile as a complete translation unit, the following direct host API mappings are included:

- `Unit::IsFriend` -> `Unit::IsFriendlyTo`
- `Player::isAFK` -> `Player::IsAFK`
- `CreatureInfo::GossipMenuId` -> `gossip_menu_id`
- `Creature::isGossip` -> `Creature::IsGossip`
- `Player::GetPlayerMenu()` -> `Player::PlayerTalkClass`

These are direct native mappings, not new compatibility shims.

## Expansion scope

The target is Tortoise/Classic (`MANGOSBOT_ZERO`). Non-Classic `learnedSpell` branches guarded outside `MANGOSBOT_ZERO` were intentionally left untouched rather than guessing Penqle semantics for unsupported expansion builds.

## Result

No core API was added. No new compatibility shim was added. A duplicated trainer eligibility implementation was removed. The Classic trainer path now delegates requirement/state logic to Penqle's native core.
