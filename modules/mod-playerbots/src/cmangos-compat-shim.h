// cmangos/playerbots → Penqle/tortoise-wow compatibility shim.
//
// Provides the cmangos-side names/constants the vendored bot module references
// but Penqle either names differently or doesn't expose. Included by botpch.h
// as the first header in the PCH chain so all bot TUs see it.
//
// What's here:
//   - Type renames / typedef forwards
//   - Define mappings (cmangos constants → Penqle equivalents)
//   - Standard-library headers cmangos uses without explicit include
//
// What's NOT here (handled by per-call-site rewrites because they need
// contextual changes, not name remapping):
//   - DBC-store globals (sMapStore ↔ sMapStorage architecture)
//   - WorldPacket move-only assignment sites
//   - CreatureData::id (single field) vs Penqle's creature_id (array)
//   - PlayerbotAI internal signature mismatches
//   - GuidPosition diamond-inheritance ambiguity

#pragma once

// === Standard library headers the bot module uses without explicit includes ===
// PlayerbotAI.h declares methods taking std::future<...> but doesn't #include
// <future>. Penqle's botpch.h already pulls in many std headers but not this one.
#include <future>
#include <chrono>
#include <random>

// === Type renames ===
// cmangos's Transport class is called GenericTransport in WotLK builds and
// Transport in Classic. Penqle uses Transport. Provide both names.
class Transport;
typedef Transport GenericTransport;

// cmangos uses GuidSet typedef. Penqle uses ObjectGuidSet.
// Pull ObjectGuid header transitively to ensure the typedef target is visible
// before the alias is used.
#include "ObjectGuid.h"
typedef ObjectGuidSet GuidSet;

// === Define mappings ===
// cmangos's ItemClass enum has ITEM_CLASS_JUNK at value 15. Penqle renamed
// this to ITEM_CLASS_JUNK (also at 15). The bot module's ahbot/Category.h
// uses the cmangos name.
#ifndef ITEM_CLASS_JUNK
#define ITEM_CLASS_JUNK ITEM_CLASS_JUNK
#endif

// cmangos defines DEFAULT_MAX_LEVEL per-expansion (60 for Classic). The bot
// module's PlayerbotAIConfig.h and PlayerbotLoginMgr.h use this for array
// sizing. Penqle uses MAX_LEVEL/STRONG_MAX_LEVEL but not this exact name.
#ifndef DEFAULT_MAX_LEVEL
#define DEFAULT_MAX_LEVEL 60
#endif

// cmangos's Team enum has TEAM_BOTH_ALLOWED for queries that span both factions.
// Penqle's Team enum has TEAM_NONE=0 (used as "no faction filter" sentinel).
// Map TEAM_BOTH_ALLOWED to TEAM_NONE so default-arg conversions work.
#ifndef TEAM_BOTH_ALLOWED
#define TEAM_BOTH_ALLOWED TEAM_NONE
#endif

// Progress reporting is implemented by playerbot/ProgressBar.h, not by a compatibility shim.

// === DBC store aliases ===
// cmangos accesses spell DBC via `sSpellTemplate.LookupEntry<SpellEntry>(id)`.
// Penqle uses `sSpellMgr.GetSpellEntry(id)`. The bot's `sSpellTemplate` is used
// in 600+ call sites; rather than rewrite each, provide a header-only wrapper
// object that exposes a templated LookupEntry() forwarding to Penqle's API.
//
// The forward-decls below need ObjectMgr / SpellMgr access. Because this header
// is included EARLY in botpch.h (before SpellMgr.h), we declare the proxy class
// inline-only — its methods get instantiated at the call sites, after Penqle's
// SpellMgr/ObjectMgr are already in scope via later botpch.h includes.

// Note: this shim is included AFTER Penqle's SpellMgr.h / ObjectMgr.h /
// SpellEntry / ItemPrototype headers in botpch.h, so we can call those APIs
// directly in inline bodies.

// Singleton-like wrapper for cmangos's sSpellTemplate. Inline LookupEntry<>()
// forwards to Penqle's sSpellMgr.GetSpellEntry().
struct CmangosSpellTemplateProxy
{
    template<typename T = SpellEntry>
    T const* LookupEntry(uint32 id) const { return sSpellMgr.GetSpellEntry(id); }
    // cmangos's DBCStorage exposes GetMaxEntry. Bot uses it to iterate spells.
    // Penqle's sSpellMgr exposes GetMaxSpellId() — same purpose.
    uint32 GetMaxEntry() const { return sSpellMgr.GetMaxSpellId(); }
};
inline CmangosSpellTemplateProxy sSpellTemplate;

// Singleton-like wrapper for cmangos's sItemStorage. Direct lookups forward to
// sObjectMgr.GetItemPrototype(); full-store scans use Penqle's native map.
struct CmangosItemStorageProxy
{
    template<typename T = ItemPrototype>
    T const* LookupEntry(uint32 id) const { return sObjectMgr.GetItemPrototype(id); }
};
inline CmangosItemStorageProxy sItemStorage;

// Singleton-like wrapper for cmangos's sMapStore. Penqle uses sMapStorage (SQLStorage).
struct MapEntry;  // defined in Maps/Map.h
struct CmangosMapStoreProxy
{
    template<typename T = MapEntry>
    T const* LookupEntry(uint32 id) const { return sMapStorage.LookupEntry<MapEntry>(id); }
    uint32 GetNumRows() const { return sMapStorage.GetMaxEntry(); }
};
inline CmangosMapStoreProxy sMapStore;

// Singleton-like wrapper for cmangos's sFactionTemplateStore.
struct FactionTemplateEntry;  // defined in Database/DBCStructure.h
struct CmangosFactionTemplateStoreProxy
{
    template<typename T = FactionTemplateEntry>
    T const* LookupEntry(uint32 id) const { return sObjectMgr.GetFactionTemplateEntry(id); }
};
inline CmangosFactionTemplateStoreProxy sFactionTemplateStore;

// === Other defines ===
// cmangos has ITEM_FLAG_HAS_LOOT (lootable item). Penqle uses ITEM_FLAG_HAS_LOOT or ITEM_FLAG_OPENABLE.
#ifndef ITEM_FLAG_HAS_LOOT
#define ITEM_FLAG_HAS_LOOT ITEM_FLAG_LOOTABLE
#endif

// === Type renames (cmangos→Penqle struct name diffs) ===
// cmangos's ItemPrototype has _Spell substruct (older naming);
// Penqle uses _ItemSpell (current naming). They're the same shape.
typedef _ItemSpell _Spell;

// cmangos has TEMPSPAWN_* enum values; Penqle has TEMPSUMMON_*. Map.
#ifndef TEMPSPAWN_TIMED_DESPAWN
#define TEMPSPAWN_TIMED_DESPAWN TEMPSUMMON_TIMED_DESPAWN
#endif
#ifndef TEMPSPAWN_TIMED_OR_DEAD_DESPAWN
#define TEMPSPAWN_TIMED_OR_DEAD_DESPAWN TEMPSUMMON_TIMED_OR_DEAD_DESPAWN
#endif
#ifndef TEMPSPAWN_TIMED_OR_CORPSE_DESPAWN
#define TEMPSPAWN_TIMED_OR_CORPSE_DESPAWN TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN
#endif
#ifndef TEMPSPAWN_DEAD_DESPAWN
#define TEMPSPAWN_DEAD_DESPAWN TEMPSUMMON_DEAD_DESPAWN
#endif
#ifndef TEMPSPAWN_CORPSE_DESPAWN
#define TEMPSPAWN_CORPSE_DESPAWN TEMPSUMMON_CORPSE_DESPAWN
#endif
#ifndef TEMPSPAWN_CORPSE_TIMED_DESPAWN
#define TEMPSPAWN_CORPSE_TIMED_DESPAWN TEMPSUMMON_CORPSE_TIMED_DESPAWN
#endif
#ifndef TEMPSPAWN_MANUAL_DESPAWN
#define TEMPSPAWN_MANUAL_DESPAWN TEMPSUMMON_MANUAL_DESPAWN
#endif

// === Spells namespace functions hoisted to global scope ===
// cmangos's bot calls IsPositiveSpell / GetDispellMask without namespace.
// Penqle wraps these in `namespace Spells`. Bring them into global scope
// for the bot's consumption.
using Spells::IsPositiveSpell;
using Spells::GetDispellMask;
using Spells::IsPassiveSpell;
// SpellEntry* overload: bot passes spellInfo directly.
inline bool IsPositiveSpell(SpellEntry const* spellInfo) { return spellInfo && spellInfo->IsPositiveSpell(); }
inline bool IsPositiveSpell(SpellEntry const* spellInfo, WorldObject const* caster, WorldObject const* victim) { return spellInfo && spellInfo->IsPositiveSpell(caster, victim); }

// === TimePoint (cmangos using; not in Penqle) ===
// Bot uses TimePoint for loot creation timestamps.
#include <chrono>
using TimePoint = std::chrono::system_clock::time_point;

// === Additional cmangos-only DBC store proxies ===
// sFactionStore (faction.dbc) — distinct from sFactionTemplateStore (factiontemplate.dbc).
struct FactionEntry;  // defined in DBCStructure.h
struct CmangosFactionStoreProxy
{
    template<typename T = FactionEntry>
    T const* LookupEntry(uint32 id) const { return sObjectMgr.GetFactionEntry(id); }
};
inline CmangosFactionStoreProxy sFactionStore;

// sCreatureStorage (creature_template SQL).
struct CmangosCreatureStorageProxy
{
    template<typename T = CreatureInfo>
    T const* LookupEntry(uint32 id) const { return sObjectMgr.GetCreatureTemplate(id); }
};
inline CmangosCreatureStorageProxy sCreatureStorage;

// === Helpers ===
// strstri overload: bot's PlayerbotAI.cpp forward-declares strstri(std::string, std::string).
// Penqle's playerbot/Helpers.cpp now provides the implementation (added).
// Re-declare here for visibility at all bot TUs.
char* strstri(std::string const& s1, std::string const& s2);

// Overload of strstr taking std::string haystack — bot calls strstr(proto->Name1, "literal")
// where Name1 is std::string. Forward to libc strstr via .c_str().
inline const char* strstr(std::string const& haystack, const char* needle) {
    return std::strstr(haystack.c_str(), needle);
}

// === BattleGroundMgr alias ===
// Done via forwarder in Penqle's BattleGroundMgr.h (BgTemplateId → BGTemplateId).

// === TEAM_INDEX_ aliases (cmangos) ===
// Penqle uses BG_TEAM_ALLIANCE/BG_TEAM_HORDE. cmangos uses TEAM_INDEX_ALLIANCE/HORDE/NEUTRAL.
#ifndef TEAM_INDEX_ALLIANCE
#define TEAM_INDEX_ALLIANCE BG_TEAM_ALLIANCE
#endif
#ifndef TEAM_INDEX_HORDE
#define TEAM_INDEX_HORDE BG_TEAM_HORDE
#endif
#ifndef TEAM_INDEX_NEUTRAL
#define TEAM_INDEX_NEUTRAL 2
#endif

// === IsAutocastable (cmangos free function) ===
// Penqle's native pet-autocast opcode accepts any known, non-passive pet spell.
// Mirror that host rule instead of disabling autocast through a false stub.
inline bool IsAutocastable(SpellEntry const* spellInfo) { return spellInfo && !spellInfo->IsPassiveSpell(); }
inline bool IsAutocastable(uint32 spellId) { return IsAutocastable(sSpellMgr.GetSpellEntry(spellId)); }

// === IsSpellAppliesAura / IsSpellHaveEffect / IsAreaAuraEffect (cmangos free functions) ===
inline bool IsSpellAppliesAura(SpellEntry const* spellInfo, uint32 effectMask = 0xFFFFFFFF) {
    return spellInfo && spellInfo->IsSpellAppliesAura(effectMask);
}
inline bool IsSpellHaveEffect(SpellEntry const* spellInfo, uint32 effect) {
    if (!spellInfo) return false;
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i) {
        if (spellInfo->Effect[i] == effect) return true;
    }
    return false;
}
inline bool IsAreaAuraEffect(uint32 effect) {
    return effect == SPELL_EFFECT_APPLY_AREA_AURA_PARTY || effect == SPELL_EFFECT_APPLY_AREA_AURA_FRIEND
        || effect == SPELL_EFFECT_APPLY_AREA_AURA_ENEMY || effect == SPELL_EFFECT_APPLY_AREA_AURA_PET
        || effect == SPELL_EFFECT_APPLY_AREA_AURA_OWNER;
}

// === MINIMUM_LOOTING_TIME ===
#ifndef MINIMUM_LOOTING_TIME
#define MINIMUM_LOOTING_TIME 1000
#endif

// === SPELL_RANGE_FLAG_MELEE / RANGED (cmangos defines on SpellRangeEntry::Flags) ===
#ifndef SPELL_RANGE_FLAG_MELEE
#define SPELL_RANGE_FLAG_MELEE 1
#endif
#ifndef SPELL_RANGE_FLAG_RANGED
#define SPELL_RANGE_FLAG_RANGED 2
#endif

// === TAXI_MOTION_TYPE (cmangos) → FLIGHT_MOTION_TYPE (Penqle) ===
#ifndef TAXI_MOTION_TYPE
#define TAXI_MOTION_TYPE FLIGHT_MOTION_TYPE
#endif

// === LfgRoles / LfgRolePriority (cmangos) — bot module's own ClassRoles is similar ===
typedef ClassRoles LfgRoles;
typedef RolesPriority LfgRolePriority;

// === Other small defines ===
#ifndef LOOT_SLOT_NORMAL
#define LOOT_SLOT_NORMAL 0
#endif
#ifndef ROLL_DISENCHANT
#define ROLL_DISENCHANT 4
#endif
#ifndef SPELL_STATE_TARGETING
#define SPELL_STATE_TARGETING 0
#endif

// === SkillLineAbility store proxy ===
// cmangos exposes sSkillLineAbilityStore (DBCStorage<SkillLineAbilityEntry>);
// Penqle exposes sObjectMgr.GetSkillLineAbility(id).
struct SkillLineAbilityEntry;
struct CmangosSkillLineAbilityStoreProxy
{
    template<typename T = SkillLineAbilityEntry>
    T const* LookupEntry(uint32 id) const { return sObjectMgr.GetSkillLineAbility(id); }
    uint32 GetMaxEntry() const { return sObjectMgr.GetMaxSkillLineAbilityId(); }
    uint32 GetNumRows() const { return GetMaxEntry(); }
};
inline CmangosSkillLineAbilityStoreProxy sSkillLineAbilityStore;

// === sGOStorage (cmangos) → sObjectMgr.GetGameObjectInfo ===
struct CmangosGOStorageProxy
{
    template<typename T = GameObjectInfo>
    T const* LookupEntry(uint32 id) const { return sObjectMgr.GetGameObjectInfo(id); }
};
inline CmangosGOStorageProxy sGOStorage;

// === sTaxiNodesStore (cmangos) → sObjectMgr.GetTaxiNodeEntry ===
struct TaxiNodesEntry;
struct CmangosTaxiNodesStoreProxy
{
    template<typename T = TaxiNodesEntry>
    T const* LookupEntry(uint32 id) const { return sObjectMgr.GetTaxiNodeEntry(id); }
    uint32 GetNumRows() const { return sObjectMgr.GetMaxTaxiNodeId(); }
};
inline CmangosTaxiNodesStoreProxy sTaxiNodesStore;

// === sLootMgr adapter (cmangos global; Penqle stores Loot on world objects) ===
// Bot calls sLootMgr.GetLoot(player[, guid]) to fetch the loot the player is currently looking at.
// Penqle embeds Loot directly in Creature/GameObject, so resolve the target on the player's
// current map and return the address of that native Loot object.
struct CmangosLootMgrStub
{
    Loot* GetLoot(Player* player, ObjectGuid guid = ObjectGuid()) const
    {
        if (!player)
            return nullptr;

        if (!guid)
            guid = player->GetLootGuid();

        if (!guid || !player->GetMap())
            return nullptr;

        if (guid.IsCreature())
        {
            Creature* creature = player->GetMap()->GetCreature(guid);
            return creature ? &creature->loot : nullptr;
        }

        if (guid.IsGameObject())
        {
            GameObject* gameObject = player->GetMap()->GetGameObject(guid);
            return gameObject ? &gameObject->loot : nullptr;
        }

        return nullptr;
    }
};
inline CmangosLootMgrStub sLootMgr;

// === Map::GetHitPosition forwarder (cmangos name) ===
// Penqle uses GetLosHitPosition. The bot module's call sites were patched at
// the source level (TravelMgr.cpp / WorldPosition.h).

// === Free-function helpers (cmangos style) wrapping Penqle SpellEntry methods ===
// cmangos exposes these as free functions; Penqle wraps them in SpellEntry::method.
inline uint32 GetSpellCastTime(SpellEntry const* spellInfo, Spell const* spell = nullptr) {
    return spellInfo ? spellInfo->GetCastTime(nullptr, const_cast<Spell*>(spell)) : 0;
}
// 3-arg form: cmangos signature is GetSpellCastTime(SpellEntry, caster, Spell).
inline uint32 GetSpellCastTime(SpellEntry const* spellInfo, WorldObject* caster, Spell const* spell = nullptr) {
    return spellInfo ? spellInfo->GetCastTime(caster, const_cast<Spell*>(spell)) : 0;
}
// IsNextMeleeSwingSpell: cmangos free function checking SPELL_ATTR_ON_NEXT_SWING_1/_2.
inline bool IsNextMeleeSwingSpell(SpellEntry const* spellInfo) {
    return spellInfo && (spellInfo->Attributes & (SPELL_ATTR_ON_NEXT_SWING_1 | SPELL_ATTR_ON_NEXT_SWING_2));
}
inline int32 GetSpellDuration(SpellEntry const* spellInfo) {
    return spellInfo ? spellInfo->GetDuration() : 0;
}
inline bool IsChanneledSpell(SpellEntry const* spellInfo) {
    return spellInfo && spellInfo->IsChanneledSpell();
}
inline SpellSchoolMask GetSpellSchoolMask(SpellEntry const* spellInfo) {
    return spellInfo ? SpellSchoolMask(spellInfo->GetSpellSchoolMask()) : SpellSchoolMask(0);
}
inline bool IsNonCombatSpell(SpellEntry const* spellInfo) {
    return spellInfo && spellInfo->IsNonCombatSpell();
}
inline bool IsPositiveEffect(SpellEntry const* spellInfo, SpellEffectIndex eff) {
    return spellInfo && spellInfo->IsPositiveEffect(eff);
}

// === MeetingStoneInfo / MeetingStoneSet (cmangos LFG) ===
// Definition lives in LFGMgr.h so both host (game.vcxproj) and bot module see the same type.
typedef std::vector<MeetingStoneInfo> MeetingStoneSet;

// === LFGQueue ===
// Penqle has its own LFGQueue in src/game/LFG/LFGMgr.h with stub methods added.
// World::GetLFGQueue() forwards to sLFGMgr. Bot module uses the existing types.

// === GetSpellStore (cmangos) → sSpellMgr (Penqle) ===
// cmangos exposes a global GetSpellStore() returning the DBC store as a POINTER.
inline CmangosSpellTemplateProxy* GetSpellStore() { return &sSpellTemplate; }

// === WORLD_SESSION_STATE_* hoisted into global scope ===
constexpr WorldSession::WorldSessionState WORLD_SESSION_STATE_CREATED = WorldSession::WORLD_SESSION_STATE_CREATED;
constexpr WorldSession::WorldSessionState WORLD_SESSION_STATE_READY = WorldSession::WORLD_SESSION_STATE_READY;
constexpr WorldSession::WorldSessionState WORLD_SESSION_STATE_OFFLINE = WorldSession::WORLD_SESSION_STATE_OFFLINE;
constexpr WorldSession::WorldSessionState WORLD_SESSION_STATE_REMOVING = WorldSession::WORLD_SESSION_STATE_REMOVING;

// === GetApplicationStartTime (cmangos) — free function returning startup timestamp ===
inline std::chrono::system_clock::time_point GetApplicationStartTime() {
    static auto s_start = std::chrono::system_clock::now();
    return s_start;
}

// === GetTeamIndexByTeamId (cmangos) → BattleGround static method ===
// Provide free-function forwarder. (BattleGround.h has it as a static.)
inline BattleGroundTeamIndex GetTeamIndexByTeamId(Team team) {
    return team == ALLIANCE ? BG_TEAM_ALLIANCE : BG_TEAM_HORDE;
}

// === Loot status flags (cmangos LootMgr.h) ===
// Bot's LootValues.cpp returns bitflags describing loot state. Penqle has no equivalent
// (its Loot just exposes items/gold). Define as bitflags so bot computes a value (which
#endif

// === SPELL_ATTR_ON_NEXT_SWING aliases ===
// cmangos has SPELL_ATTR_ON_NEXT_SWING / _NO_DAMAGE; Penqle has SPELL_ATTR_ON_NEXT_SWING_1/_2.
#ifndef SPELL_ATTR_ON_NEXT_SWING
#define SPELL_ATTR_ON_NEXT_SWING SPELL_ATTR_ON_NEXT_SWING_1
#endif
#ifndef SPELL_ATTR_ON_NEXT_SWING_NO_DAMAGE
#define SPELL_ATTR_ON_NEXT_SWING_NO_DAMAGE SPELL_ATTR_ON_NEXT_SWING_2
#endif

// === UNIT_FLAG_UNTARGETABLE / UNIT_FLAG_UNINTERACTIBLE (cmangos names) ===
// Penqle uses UNIT_FLAG_NOT_SELECTABLE for both concepts.
#ifndef UNIT_FLAG_UNTARGETABLE
#define UNIT_FLAG_UNTARGETABLE UNIT_FLAG_NOT_SELECTABLE
#endif
#ifndef UNIT_FLAG_UNINTERACTIBLE
#define UNIT_FLAG_UNINTERACTIBLE UNIT_FLAG_NOT_SELECTABLE
#endif

// === IsAutoRepeatRangedSpell (cmangos free function) ===
// Penqle's SpellEntry has IsAutoRepeatRangedSpell as a method. Wrap as free fn.
inline bool IsAutoRepeatRangedSpell(SpellEntry const* spellInfo) {
    return spellInfo && (spellInfo->AttributesEx2 & SPELL_ATTR_EX2_AUTOREPEAT_FLAG);
}
