#include "playerbot/PlayerbotAppearanceStore.h"

#include "World.h"

#include <algorithm>

namespace
{
char const PlayerbotCharSectionsEntryfmt[] = "diiiiixxxi";
}

PlayerbotAppearanceStore sPlayerbotAppearanceStore;

PlayerbotAppearanceStore::PlayerbotAppearanceStore() : m_store(PlayerbotCharSectionsEntryfmt)
{
    static_assert(sizeof(CharSectionsEntry) == 6 * sizeof(uint32),
        "Playerbot CharSectionsEntry must match the native DBCStorage format");
}

bool PlayerbotAppearanceStore::Load()
{
    std::call_once(m_loadOnce, [this]()
    {
        if (DBCFileLoader::GetFormatRecordSize(PlayerbotCharSectionsEntryfmt) != sizeof(CharSectionsEntry))
        {
            sLog.outError("Playerbot CharSections DBC format does not match its entry structure; using Tortoise appearance fallback.");
            return;
        }

        std::string path = sWorld.GetDataPath() + "dbc/CharSections.dbc";
        if (!m_store.Load(path.c_str()))
        {
            sLog.outError("Playerbot could not load '%s' with Tortoise DBCStorage; using Tortoise appearance fallback.", path.c_str());
            return;
        }

        for (uint32 id = 0; id < m_store.GetNumRows(); ++id)
        {
            CharSectionsEntry const* entry = m_store.LookupEntry(id);
            if (!entry || !entry->Race || entry->Race >= MAX_RACES || entry->Gender > GENDER_FEMALE ||
                entry->BaseSection > SECTION_HAIR || entry->VariationIndex > 0xFF || entry->ColorIndex > 0xFF)
                continue;
            if (entry->Flags & SECTION_FLAG_UNAVAILABLE)
                continue;

            m_sections[MakeKey(uint8(entry->Race), uint8(entry->Gender), uint8(entry->BaseSection))].push_back(entry);
        }

        if (m_sections.empty())
        {
            sLog.outError("Playerbot loaded CharSections.dbc but found no usable appearance sections; using Tortoise appearance fallback.");
            return;
        }

        m_loaded = true;
        sLog.outString("Playerbot loaded CharSections.dbc through native Tortoise DBCStorage (%zu indexed section groups).", m_sections.size());
    });

    return m_loaded;
}

PlayerbotAppearanceStore::SectionList const* PlayerbotAppearanceStore::GetSections(
    uint8 race, uint8 gender, CharSectionType section) const
{
    auto itr = m_sections.find(MakeKey(race, gender, uint8(section)));
    return itr == m_sections.end() ? nullptr : &itr->second;
}


void PlayerbotAppearanceStore::WarnMissingSectionsOnce(uint8 race, uint8 gender)
{
    uint32 key = uint32(race) | (uint32(gender) << 8);
    std::lock_guard<std::mutex> lock(m_warningMutex);
    if (!m_warnedRaceGenders.insert(key).second)
        return;

    sLog.outError("Playerbot CharSections.dbc has no complete usable appearance for race %u gender %u; using native Tortoise 0..5 fallback.",
        race, gender);
}

void PlayerbotAppearanceStore::FillFallback(PlayerbotAppearance& out)
{
    // This intentionally matches Tortoise's native PlayerBotAI::SpawnNewPlayer
    // fallback. Player::Create remains the final character-creation authority.
    out.skin = uint8(urand(0, 5));
    out.face = uint8(urand(0, 5));
    out.hairStyle = uint8(urand(0, 5));
    out.hairColor = uint8(urand(0, 5));
    out.facialHair = uint8(urand(0, 5));
}

bool PlayerbotAppearanceStore::GetRandomAppearance(uint8 race, uint8 gender, PlayerbotAppearance& out)
{
    if (!Load())
    {
        FillFallback(out);
        return false;
    }

    SectionList const* skins = GetSections(race, gender, SECTION_SKIN);
    SectionList const* faces = GetSections(race, gender, SECTION_FACE);
    SectionList const* hairs = GetSections(race, gender, SECTION_HAIR);
    SectionList const* facialHairs = GetSections(race, gender, SECTION_FACIAL_HAIR);

    if (!skins || skins->empty() || !faces || faces->empty() || !hairs || hairs->empty())
    {
        WarnMissingSectionsOnce(race, gender);
        FillFallback(out);
        return false;
    }

    // CMaNGOS' proven Classic semantics use a FACE entry's ColorIndex as the
    // selected skin colour. Filter against actual skin sections so custom
    // Turtle races cannot select a face backed by an unavailable skin.
    std::vector<CharSectionsEntry const*> compatibleFaces;
    compatibleFaces.reserve(faces->size());
    for (CharSectionsEntry const* face : *faces)
    {
        bool hasSkin = std::any_of(skins->begin(), skins->end(), [face](CharSectionsEntry const* skin)
        {
            return skin->ColorIndex == face->ColorIndex;
        });
        if (hasSkin)
            compatibleFaces.push_back(face);
    }

    if (compatibleFaces.empty())
    {
        WarnMissingSectionsOnce(race, gender);
        FillFallback(out);
        return false;
    }

    CharSectionsEntry const* face = compatibleFaces[urand(0, compatibleFaces.size() - 1)];
    CharSectionsEntry const* hair = (*hairs)[urand(0, hairs->size() - 1)];

    out.skin = uint8(face->ColorIndex);
    out.face = uint8(face->VariationIndex);
    out.hairStyle = uint8(hair->VariationIndex);
    out.hairColor = uint8(hair->ColorIndex);
    out.facialHair = 0;

    // Keep the historical CMaNGOS field interpretation: ColorIndex is the
    // customization byte passed as facialHair. Unlike the old factory, do not
    // hardcode race/gender exclusions; the DBC itself decides whether choices
    // exist for a race/gender.
    if (facialHairs && !facialHairs->empty())
        out.facialHair = uint8((*facialHairs)[urand(0, facialHairs->size() - 1)]->ColorIndex);

    return true;
}
