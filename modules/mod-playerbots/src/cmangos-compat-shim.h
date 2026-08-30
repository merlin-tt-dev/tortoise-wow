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

// Temporary compatibility for remaining unqualified passive-spell call sites.
using Spells::IsPassiveSpell;

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

// === LfgRoles (MANGOSBOT_TWO compatibility only) ===
typedef ClassRoles LfgRoles;
