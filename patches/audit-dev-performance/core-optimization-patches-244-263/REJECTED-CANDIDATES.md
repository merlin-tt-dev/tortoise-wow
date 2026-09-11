# Rejected / deliberately retained candidates during 244–263

- `Player::DismountCheck` AuraList copy: retained because the loop calls removal paths that mutate the source Aura container.
- `Spell::DoSpellHitOnUnit` AttackerSet copy: retained because `AttackStop()` can mutate the target attacker set during traversal.
- Broad `Item::GetProto()` caching: rejected where it would combine otherwise separate global-template lookups because `item_template` is runtime-reloadable.
- Equivalent broad GameObjectInfo caching: treated with the same reload caution.
- `Quest::GetRewOrReqMoney()` caching: rejected; positive rewards read the current `CONFIG_FLOAT_RATE_DROP_MONEY` each call, so it is not a pure stable getter.
- Other global reloadable-map hash conversions: intentionally avoided in this block after the 224–243 adversarial audit.
