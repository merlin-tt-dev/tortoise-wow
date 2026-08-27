#ifndef _PLAYERBOT_APPEARANCE_STORE_H
#define _PLAYERBOT_APPEARANCE_STORE_H

#include "Common.h"
#include "Database/DBCStore.h"

#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct PlayerbotAppearance
{
    uint8 skin = 0;
    uint8 face = 0;
    uint8 hairStyle = 0;
    uint8 hairColor = 0;
    uint8 facialHair = 0;
};

// Module-owned CharSections.dbc reader. It deliberately uses Tortoise's native
// DBCStorage/DBCFileLoader instead of duplicating the WDBC parser in the
// compatibility shim or extending the core DBC store API solely for bots.
class PlayerbotAppearanceStore
{
public:
    PlayerbotAppearanceStore();

    // Loads DataDir/dbc/CharSections.dbc once. Failure is non-fatal because
    // Tortoise itself can create players using its historic 0..5 appearance
    // fallback; callers can distinguish that through the return value.
    bool Load();

    // Returns true when an appearance was selected from CharSections.dbc.
    // Returns false when the DBC is unavailable or has no usable entries for
    // this race/gender; out is then filled with Tortoise's native 0..5 fallback.
    bool GetRandomAppearance(uint8 race, uint8 gender, PlayerbotAppearance& out);

    bool IsLoaded() const { return m_loaded; }

private:
    struct CharSectionsEntry
    {
        uint32 Race;
        uint32 Gender;
        uint32 BaseSection;
        uint32 VariationIndex;
        uint32 ColorIndex;
        uint32 Flags;
    };

    enum CharSectionType : uint8
    {
        SECTION_SKIN = 0,
        SECTION_FACE = 1,
        SECTION_FACIAL_HAIR = 2,
        SECTION_HAIR = 3,
    };

    static constexpr uint32 SECTION_FLAG_UNAVAILABLE = 0x01;

    using SectionList = std::vector<CharSectionsEntry const*>;

    static uint32 MakeKey(uint8 race, uint8 gender, uint8 section)
    {
        return uint32(section) | (uint32(gender) << 8) | (uint32(race) << 16);
    }

    SectionList const* GetSections(uint8 race, uint8 gender, CharSectionType section) const;
    static void FillFallback(PlayerbotAppearance& out);
    void WarnMissingSectionsOnce(uint8 race, uint8 gender);

    DBCStorage<CharSectionsEntry> m_store;
    std::unordered_map<uint32, SectionList> m_sections;
    std::once_flag m_loadOnce;
    std::mutex m_warningMutex;
    std::unordered_set<uint32> m_warnedRaceGenders;
    bool m_loaded = false;
};

extern PlayerbotAppearanceStore sPlayerbotAppearanceStore;

#endif
