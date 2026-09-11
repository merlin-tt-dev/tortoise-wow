# Rejected / deliberately retained candidates during the 224–243 re-audit

These are recorded to prevent the same unsafe mechanical modernization from being proposed again without new evidence.

- Runtime-reloadable SpellMgr maps including `spell_affect`, `spell_elixir`, `spell_proc_item_enchant`, `spell_group_stack_rules` and `spell_pet_auras`: rejected hash conversions. Rehash would add a different invalidation/race surface around reloads.
- `SpellEnchantChargesMap` and `SpellLearnSkillMap`: also not converted. Their loaders explicitly contain reload-case clearing even though no convenient command-path proof was found; that uncertainty is enough to keep the ordered implementation.
- AccountMgr security/fingerprint maps from the withdrawn first bundle: rejected. Runtime updates plus asynchronous/socket readers make a changed rehash/race surface insufficiently proven.
- `SpellThreatMap`: retained ordered; stored iterator/lifetime behavior makes hash rehash unsafe.
- `TalentBitSize`: retained ordered because traversal order defines client-visible talent bit positions.
- Pet-family inner sets: retained ordered because traversal feeds AddSpell side effects.
- `pool2event`: retained ordered because traversal/diagnostic order was not proven unobservable.
- Any other ordered structure tied to RNG indexing, deterministic packing/renumbering, first-element selection, logging or mutation order was left unchanged unless the exact rule could be reconstructed explicitly.
