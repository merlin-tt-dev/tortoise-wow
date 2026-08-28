# LivingBots clean Playerbot port: provenance and backport matrix

This document is the review ledger for the Playerbot rebuild on top of Tortoise `1181dev`.

## Invariants

- Base branch: `1181dev` (never modified by this work).
- Working branch: `livingbots/playerbots-clean-base`.
- No Shyalya history merge and no whole-commit cherry-picks by default.
- Random teleport is not an acceptable progression, idle, population, travel, or dungeon recovery policy.
- Bug fixes and policy changes are split into separate decisions and commits.
- Persistent LivingBots start at level 1 and progress through world/quest/travel semantics.
- Turtle races 9 (Goblin) and 10 (High Elf) must be first-class races throughout creation, start positions, class/spec selection, questing, travel, and social logic.

## Provenance

### Playerbot lineage

The Playerbot AI used by the historical Tortoise port belongs to the ike3 line:

`ike3/mangosbot* -> celguar/mangosbot-bots -> cmangos/playerbots`

The CMaNGOS repository describes itself as the Bot AI core from ike3 and is the upstream reference for Playerbot semantics. AzerothCore is not used as a semantic reference for this port.

### Historical Tortoise port

Penqle PR #79 (`Add cmangos/playerbots subsystem`) targeted an older `1181dev` snapshot and was closed without merge. Its final head is:

- Tortoise PR head: `087425c7d2a64074e3b66a19dc7c22174b902cb0`
- PR base: `32a04a77947e2201359108226b2bd8f0bcd404a0`

PR #79 is useful as a compatibility-port reference, not as an authoritative implementation. Its own description separates vendored Playerbot code from an authored compatibility shim/host hooks, but audit has already found semantic compatibility edits and stubs inside the ported surface. Those must be reviewed individually.

### Upstream snapshot reference

The historical vendor tree matches `cmangos/playerbots` from early May 2026. The current pinned comparison reference is:

- `cmangos/playerbots@c33dfac220eb624761b6737324071ad7fae5b39f` (2026-05-04)

Examples verified against this pin include the root vendor metadata and `playerbot/strategy/Value.h`; port differences there are Tortoise API reconciliation rather than a different Playerbot design. Continue per-file verification when a semantic difference matters.

## Porting losses / suspicious compatibility stubs already identified

These are not accepted merely because PR #79 compiled:

| Surface | Historical port behavior | Risk / lost semantics | Decision |
|---|---|---|---|
| historical `IsPreventingDeath()` | old call-site queried AI state that Penqle does not expose | bot can ignore scripted death prevention / encounter semantics | **resolved module-only:** query Penqle `Unit::GetInvincibilityHpThreshold()`, the state used by the native lethal-damage path |
| historical `UnitAI::IsRangedUnit()` | fake `UnitAI = CreatureAI` alias pretended incompatible AI models were equivalent | caster/ranged target classification degraded | **resolved module-only:** use Penqle's dynamic `CreatureAI::IsMeleeAttackEnabled()` state; main-ranged-spell mode disables melee |
| historical `DistanceCalculation` / `DIST_CALC_*` | integer compatibility enum was passed into Penqle APIs; several CMaNGOS squared-distance call-sites still applied `sqrt()` | ranges collapse (e.g. 25 yd becomes 5 yd), old 2D/3D bool overloads have wrong type/meaning | **resolved module-only:** use Penqle `SizeFactor`, explicit `GetDistance2d()` for historical 2D calls, and remove only the obsolete CMaNGOS square-root conversions |
| historical `Taxi::Map` / `Player::GetTaxiPathSpline()` | compatibility shim always returned an empty pseudo-spline | following a player on taxi never knew the real flight endpoint; `go` could not report the destination | **resolved module-only:** read Penqle's active `FlightPathMovementGenerator` and its native `TaxiPathNodeList`; stop at `GetPathAtMapEnd()` to preserve CMaNGOS' map-local spline semantics (and work for both single- and multi-leg flights) |
| historical `TransportAnimation` compatibility types | shim declared an in-memory path shape but Penqle never populated it; transport generation also read `moTransport` fields from `GAMEOBJECT_TYPE_TRANSPORT` union entries | elevator/tram graph generation was either non-functional or based on the wrong GO union member | **resolved module-only:** load the genuine client `TransportAnimation.dbc` through Tortoise `DBCStorage`, index it inside playerbots, and only read `moTransport` for `GAMEOBJECT_TYPE_MO_TRANSPORT`; boats/zeppelins keep Penqle's native TaxiPath data |
| fake `InstanceTemplate` | zero-filled struct plus `WorldPosition::getInstanceTemplate() -> nullptr` | generated instance tests were silently skipped and any future field read would consume fabricated metadata | **resolved native:** use Penqle `MapEntry::maxPlayers` and `sObjectMgr.GetMapEntranceTrigger(mapId)->requiredLevel`; explicitly skip test generation if authoritative metadata is absent |
| fake `AuctionHouseType` | pseudo enum `0/1/2` was passed to an API that expects `AuctionHouseEntry const*` | AH mirror used the wrong API contract and could duplicate the same native AH object | **resolved native:** iterate `sAuctionHouseStore`, resolve with `sAuctionMgr.GetAuctionsMap(entry)`, and deduplicate native `AuctionHouseObject*` mappings |
| EmotesTextSound no-op | shim always returned `nullptr` | playerbot text-emote sounds were silently disabled | **resolved module-only:** load genuine `EmotesTextSound.dbc` through Tortoise `DBCStorage` and preserve the historical exact `(emote,race,gender)` lookup |
| debug/diagnostic fake semantics | empty spell-result formatter, persistent-aura helper always false plus invented cast-flag name, and fake formation objects | diagnostics lied or pretended unsupported core behavior existed | **resolved:** report numeric native `SpellCastResult`; leave Penqle's native spell-go flags untouched; formation debug command explicitly reports unsupported rather than invoking a fake API |
| historical `SEC_GAMEMASTER` hierarchy / staff command trust | shim mapped CMaNGOS GM directly to Penqle `SEC_ADMINISTRATOR`; CLI dispatch used unchecked `ChatHandler* -> CliHandler*` casts; `.bot` source implicitly granted moderator debug rights | developer-rank staff lost intended Playerbot privileges, observer could inherit old `> SEC_PLAYER` gameplay bypasses, `NullChatHandler` could hit undefined behavior, and generic `.bot do ... cdebug` inherited moderator-only debug paths | **resolved native:** map historical GM-only semantics to Tortoise `SEC_DEVELOPER`, preserve admin-only selection as `> SEC_DEVELOPER`, start gameplay staff bypasses at `SEC_MODERATOR`, use virtual `ChatHandler::GetAccessLevel()/GetAccountId()`, and emit a privileged debug event source only after explicit `debug`/`c` command authorization |
| remaining `ScriptDevAIMgr` shim | fake manager exposed only an always-false `OnNpcSpellClick`; three source files included a compatibility-only ScriptDev2 header | shim pretended a script dispatcher existed; the only remaining click hook is in the inactive `MANGOSBOT_TWO` path while Tortoise is built as `MANGOSBOT_ZERO` | **resolved:** remove fake manager/header/includes and dead click hook; active gossip/RPG dispatch uses Penqle native `sScriptMgr.OnGossipHello()` directly |
| historical `AreaTableEntry` / `sAreaStore` / Area free-function compatibility | shim made CMaNGOS field/API names compile over Penqle `AreaEntry`; `WorldPosition::GetArea()` then passed an ExploreFlag to an Area-ID lookup | valid positions could resolve to a completely unrelated valid area, while Turtle self-parenting `area_template` rows could recurse indefinitely in area-level derivation and stale generated cache rows preserved the bad result | **resolved native:** use Penqle `AreaEntry`/`sAreaStorage` directly, resolve positions through `TerrainManager::GetAreaId`, guard parent cycles, ignore non-positive derived levels, build creature-level fallbacks in one pass, version the generated area-level cache, and leave unavailable WMO subzone-name semantics explicitly empty instead of fabricating a replacement |
| Auction compatibility methods | fields/defaults and no-op methods added for missing CMaNGOS API | AH bot can reason over fabricated state | audit before enabling AH bot; do not treat stubs as real state |
| BG flag carrier base accessor | default empty GUID | BG tactics can silently lose objective state | map to real per-BG Tortoise state |
| `GetTransports()` | later Shyalya state was effectively empty in the broken port | transport travel impossible | restore real transport enumeration from Tortoise containers |
| BattleGround queue ACE guards | mutex type converted to `std::recursive_mutex`, guards left commented | concurrent queue map/list access | restore std locks |
| Bot anticheat session | bot session can have null anticheat | movement/knockback paths dereference null | instantiate `NullSessionAnticheat` |
| `UnitCalculatedValue` | caches raw `Unit*` across ticks | dangling pointer / UAF after despawn/death | cache GUID and resolve on access |
| Race start fallback | race 9 behaves like Orc; race 10 like Human | wrong level-1 world position and downstream home/quest assumptions | implement Turtle races as first-class mappings |

## Backport matrix

Status vocabulary: `TAKE` = small correctness fix to reproduce; `REIMPLEMENT` = preserve intent but write against current `1181dev`; `REVIEW` = evidence promising but dependencies/policy mixed; `REJECT` = intentionally excluded.

| Function / concern | Upstream / origin | Shyalya SHA | Files / surface | Root cause | Dependencies | Risk | Decision |
|---|---|---|---|---|---|---|---|
| Safe cached unit value | CMaNGOS value engine + Tortoise lifetime fix | `72144787c3311f0eb69548fc4019e902171efa8d` | `playerbot/strategy/Value.{h,cpp}` | cached raw `Unit*` outlives world object | ObjectGuid + `PlayerbotAI::GetUnit` | low | TAKE |
| Bot anticheat object | Tortoise session lifecycle | `4c09563a1f00a05b6e9f8f5248cc0bcdec7dd12f` | `src/game/WorldSession.cpp` | socketless bot sessions bypass normal anticheat init | existing `NullSessionAnticheat` | low | TAKE |
| Battleground queue locking | Tortoise ACE -> std migration repair | `bf6b1d4dbacf85100d3bd23d60d10f2011c2e779` | `src/game/Battlegrounds/BattleGroundMgr.cpp` | five ACE guards commented while mutex survived | `std::recursive_mutex` already present | low/medium: lock ordering | TAKE after current-core lock-order check |
| Channel broadcaster busyloop | Tortoise runtime fix | `ab1b4f88581e2174e07250f1bfefad0479323cc1` | ChannelBroadcaster | zero-work loop burns CPU | current broadcaster lifecycle | low | TAKE after exact diff check |
| `NextAction::getName()` copies | Playerbot perf fix | `0d663f0117471f15ba6311243d73f96ee36f702c` | strategy engine | hot-path `std::string` copies | callers must not retain mutable alias | low | TAKE after signature/caller audit |
| Falling state recovery | Tortoise movement semantics | `e65be1a91313cc3b550c11e14d80faf7514188a3` | movement/follow actions | synthetic fall-land packet can be dropped, leaving server fall state set | current Player movement fields/API | medium | REIMPLEMENT minimal state repair |
| Party/raid command scope | CMaNGOS command semantics / Tortoise fix | `e65be1a91313cc3b550c11e14d80faf7514188a3` | RandomPlayerbotMgr command dispatch | realm-wide same-team bots consumed party/raid command | Group membership | low | TAKE |
| Real transport enumeration | CMaNGOS travel semantics + Tortoise transport model | multiple | Travel/WorldPosition/compat surface | compatibility stub returned empty transport set | current Tortoise `Transport`/MapManager containers | medium | REIMPLEMENT |
| Quest area / turn-in resolution | CMaNGOS travel semantics + Tortoise quest model | multiple | TravelMgr / quest destinations | port API mismatch lost valid turn-in areas | quest giver/object lookup, area hierarchy | medium/high | REIMPLEMENT per proven mismatch |
| CustomStrategy negative-result cache | Shyalya perf fix | `c2bdc1e055cef98fed6c0638b39058141f650d69` | CustomStrategy | repeated negative construction/lookups | strategy lifetime | medium | REVIEW |
| QueryResult / pet query lifetime | DB API port repair | `c2bdc1e055cef98fed6c0638b39058141f650d69` | pet/query path | result lifetime/API mismatch | current DB wrappers | low/medium | REVIEW then TAKE if still present |
| Bag swap retry bound | Playerbot behavior fix | `3908a522a055236c76b5c2d3e77ffbdf49e7a24b` | inventory action | failed swap retried indefinitely | item/bag state invalidation | low | REVIEW |
| Death-loop counter persistence | recovery behavior | `ee63bf5603ec847d3378b9e3012e38909f5b9407` | revive/death state | revive zeroed history, hiding loops | later configurable policy | medium | REIMPLEMENT counter mechanics only; no hardcoded policy |
| Random-bot composite DB index | DB performance | `c63906de24bb0310f937c33b458a0b591eda5ecd` | `ai_playerbot_random_bots` migration | hot lookup lacks `(owner,bot,event)` index | migration framework/current schema | low | REVIEW/TAKE if query plan matches |
| Weighted spec off-by-one | Playerbot math | `7a8604cd7ce09903b566de5aa8e32db5cec21ff0` | spec selection | inclusive/exclusive weighted range error | Turtle race/class/spec matrix | low | TAKE math only |
| Unsigned underflow repairs | mixed fix commit | `aa8f15c148659b058861fbf3e7125dcc` | mixed | unsigned subtraction wraps | inspect each arithmetic hunk | low individually | TAKE only proven arithmetic hunks |
| Immediate Engine `Init()/Reset()` suppression | Shyalya strategy perf work | `d886c5c240497f696897f60981ce68bb7a0933a7` | Engine / strategy mutations | excessive rebuilds, but synchronous rebuild can invalidate current action/queue | AI tick lifecycle | high | REIMPLEMENT as deferred dirty-flag rebuild |
| Strategy removal callback iterator invalidation | CMaNGOS strategy lifecycle + Shyalya runtime diagnosis | `0cec7f93d8fa169ffb2bc24c997ed2dfe0db6f0a` | `playerbot/strategy/Engine.cpp` | `OnStrategyRemoved()` can recursively call `ChangeStrategy()` and mutate the same map while `removeStrategy()` still holds its iterator | current raw-pointer strategy ownership; `RpgStrategy::OnStrategyRemoved()` reproduces the nested mutation | low | TAKE detach-before-callback ordering only; reject unrelated Shyalya strategy-gate/hash changes |
| Idle/progress random teleport | Shyalya fallback policy | various | RandomPlayerbotMgr / Travel | masks navigation/progression failures | none desired | high semantic damage | REJECT |
| Dungeon seed/fill teleport | Shyalya fallback policy | various | dungeon population | substitutes teleport for travel/group formation | none desired | high semantic damage | REJECT |
| Hardcoded vanilla “dumb quest” blacklist | Shyalya policy | various | quest selection | content-specific suppression instead of fixing semantics | world DB/content drift | high | REJECT |
| Blanket level-5 breadcrumb suppression | Shyalya policy | various | quest selection | policy changes organic progression | content-specific | high | REJECT |

## Strategy rebuild design target

Do not call `Engine::Init()` / `Reset()` synchronously from a strategy-changing action.

Target lifecycle:

1. strategy mutation updates the strategy set;
2. mutation marks the owning engine `dirty`;
3. current action/tick completes without rebuilding containers it may still reference;
4. at the AI tick boundary, if `dirty`, rebuild exactly once;
5. clear `dirty` only after a successful rebuild.

Coalesce multiple mutations within one tick. No mid-action queue invalidation.

## Turtle race 9 / 10 acceptance criteria

At minimum, audit and support race IDs in all of these paths before declaring race support complete:

- character creation / race-class validation;
- random or generated race/class selection;
- start map and coordinates;
- home bind / home area assumptions;
- faction/team mapping;
- initial spells/skills/languages if Playerbot code synthesizes them;
- trainer/vendor/quest filters;
- travel start-region / area graph seeding;
- spec/class weighting tables keyed by race;
- social/PvP faction checks.

Expected level-1 spawn evidence supplied for this project:

- Goblin (race 9): map 1, approximately `x=-619, y=-4252`.
- High Elf (race 10): map 0, approximately `x=-8950, y=-132`.

Do not implement these as `race 9 -> Orc` or `race 10 -> Human` aliases.
