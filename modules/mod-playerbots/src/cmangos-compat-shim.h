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

// === IsSpellAppliesAura / IsSpellHaveEffect / IsAreaAuraEffect (cmangos free functions) ===
inline bool IsSpellAppliesAura(SpellEntry const* spellInfo, uint32 effectMask = 0xFFFFFFFF) {
    return spellInfo && spellInfo->IsSpellAppliesAura(effectMask);
}
inline bool IsAreaAuraEffect(uint32 effect) {
    return effect == SPELL_EFFECT_APPLY_AREA_AURA_PARTY || effect == SPELL_EFFECT_APPLY_AREA_AURA_FRIEND
        || effect == SPELL_EFFECT_APPLY_AREA_AURA_ENEMY || effect == SPELL_EFFECT_APPLY_AREA_AURA_PET
        || effect == SPELL_EFFECT_APPLY_AREA_AURA_OWNER;
}

// === LfgRoles (MANGOSBOT_TWO compatibility only) ===
typedef ClassRoles LfgRoles;
// === Remaining free-function compatibility helpers ===
// IsNextMeleeSwingSpell: cmangos free function checking SPELL_ATTR_ON_NEXT_SWING_1/_2.
inline bool IsNextMeleeSwingSpell(SpellEntry const* spellInfo) {
    return spellInfo && (spellInfo->Attributes & (SPELL_ATTR_ON_NEXT_SWING_1 | SPELL_ATTR_ON_NEXT_SWING_2));
}
inline SpellSchoolMask GetSpellSchoolMask(SpellEntry const* spellInfo) {
    return spellInfo ? SpellSchoolMask(spellInfo->GetSpellSchoolMask()) : SpellSchoolMask(0);
}
inline bool IsNonCombatSpell(SpellEntry const* spellInfo) {
    return spellInfo && spellInfo->IsNonCombatSpell();
}

// === IsAutoRepeatRangedSpell (cmangos free function) ===
// Penqle's SpellEntry has IsAutoRepeatRangedSpell as a method. Wrap as free fn.
inline bool IsAutoRepeatRangedSpell(SpellEntry const* spellInfo) {
    return spellInfo && (spellInfo->AttributesEx2 & SPELL_ATTR_EX2_AUTOREPEAT_FLAG);
}
