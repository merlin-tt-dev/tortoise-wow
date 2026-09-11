# Semantic audit — 244–263

## Patch notes

### 244 — Nightmare dragon fixed permutation
A local four-element vector becomes `std::array<uint32,4>`. `std::shuffle` still receives the same engine and four-element random-access range. Vector-vs-array output was differential-tested for 250,000 seeds with exact permutation equality.

### 245 — Shaman armor aura-list reference
`GetAurasByType(SPELL_AURA_DUMMY)` already returns `AuraList const&`; the old `auto` made a full list copy. The audited loop only reads aura IDs/items and updates a local accumulator, so a const reference preserves traversal/order without invalidation risk.

### 246 — Shaman armor direct spell dispatch
A fixed three-entry hash table (`45951/45952/45953`) becomes a switch with identical values and unknown-ID continuation behavior. This removes one-time hash allocation/static-init synchronization; successful behavior is unchanged.

### 247 — PlayerAI fixed skip-spell arrays
Fixed Priest/Hunter ID vectors become constexpr arrays. Entry ordering and `std::find` membership semantics remain unchanged. Startup heap allocations are removed.

### 248 — Player challenge direct spell dispatch
A fixed Challenge-enum-to-spell hash becomes a switch. All valid mappings are preserved, unknown values remain false, and `HasSpell` is called once for a valid mapping.

### 249 — Hardcore reward direct level dispatch
A fixed level-to-item map for levels 10–60 becomes a switch. Unknown levels still return immediately; all six mappings are identical.

### 250 — First attacker accessor reuse
Caches the existing `AttackerSet const&`, uses `empty()`, and still returns `*begin()`. The ordered set and first-element semantics are untouched.

### 251 — Condition aura SpellProto reuse
Within an aura-holder check, reuses the already-stored `SpellProto` pointer. No holder/container mutation occurs in the expression.

### 252 — Melee-position bounding-radius reuse
Caches the attacker's bounding radius within the calculation. RNG call count/order and arithmetic expression ordering remain unchanged.

### 253 — Arena aura SpellProto reuse
Caches each holder's SpellProto only until the existing possible aura removal. No pointer is retained across the container mutation.

### 254 — Paladin healing aura SpellProto reuse
Reuses the stable SpellProto pointer during a read-only aura iteration.

### 255 — Invisibility modifier reuse
Caches the modifier pointer returned from the Aura member. The loop only reads `m_miscvalue`/`m_amount`; no Aura mutation occurs.

### 256 — Food emote SpellProto reuse
Reuses SpellProto per Aura. Packet emission conditions, break conditions and traversal order remain unchanged.

### 257 — Health regen modifier reuse
Reuses the Aura modifier pointer; amount/periodic-time arithmetic is unchanged.

### 258 — Weapon aura cast-GUID reuse
Reuses the holder-owned cast-item GUID reference. No holder mutation occurs between the relevant checks.

### 259 — Escort point-list reference
`GetPointMoveList` already returns a const reference (or a static empty vector); the old local `auto` copied it. `LoadEscortData` only reads the point list and mutates a separate escort structure, so source-vector snapshot semantics are unnecessary.

### 260 — Stalked aura-list reference
`BuildValuesUpdate` used to copy the `SPELL_AURA_MOD_STALKED` AuraList before a read-only `find_if`. The audited predicate only reads caster GUIDs and does not mutate the Aura container. Wire field selection/order is unchanged.

### 261 — Pet owner attacker-set reference
The owner AttackerSet copy becomes a const reference. The audited loop performs read-only target predicates; none of `IsInMap`, `IsValidAttackTarget`, or `HasAuraPetShouldAvoidBreaking` mutates the owner's attacker set. Set ordering is retained.

### 262 — PlayerAI spell-map reference
The `PlayerControlledAI` constructor reads the PlayerSpellMap to build its separate usable-spell data and does not mutate the player's spell map during traversal. Therefore the old full map copy is unnecessary. The map type/order itself remains unchanged.

### 263 — EventAI victim/value reuse
TARGET_HP/TARGET_MANA event checks retain the original logical order: in-combat check, victim fetch/null check, max-value zero check, percentage arithmetic. Victim and max values are reused instead of repeatedly loaded. Existing uint32 multiplication/wrap/division behavior is preserved and differential-tested.

## Snapshot distinction

Copy removal was not mechanical. Two close counterexamples were deliberately left unchanged:

- `Player::DismountCheck`: the loop can remove Aura types, mutating the source AuraList. Its old copy is a required traversal snapshot.
- `Spell::DoSpellHitOnUnit`: the loop can call `AttackStop()`, which mutates the target attacker set. Its copy is likewise a required snapshot.

This distinction was used as an adversarial check for 259–262.

## Reload-sensitive accessors

Tempting broader `Item::GetProto()` caches were rejected because `item_template` has runtime reload support; collapsing repeated store lookups can change the reload/race observation window. Similar caution applies to reloadable GameObject info. `Quest::GetRewOrReqMoney()` was also rejected as a generic getter-cache candidate because positive rewards read the current money-rate configuration each call.

## Allocation/failure timing

Patches 244 and 246–249 remove dynamic storage/static hash allocation for genuinely fixed bounded data. This intentionally removes those allocation/OOM points. No dynamic untrusted sizing is introduced. Other patches predominantly remove already-unnecessary copies or repeated stable member reads.

## Proprietary-client boundary

No patch changes packet schema, opcode layout, serialization, GUID packing, packet presence/order or target RNG mapping. Patch 260 alters only how an existing AuraList is traversed before the same update-field decision is made.
