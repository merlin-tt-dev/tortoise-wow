#include "playerbot/PlayerbotSupplementalDBCStore.h"

#include "World.h"

namespace
{
// Classic TransportAnimation.dbc:
// id (sort/index only), transport entry, time segment, x, y, z, movement id.
char const PlayerbotTransportAnimationEntryfmt[] = "diifffx";

// Classic EmotesTextSound.dbc:
// id, text emote id, race, gender, sound id.
char const PlayerbotEmotesTextSoundEntryfmt[] = "niiii";
}

PlayerbotSupplementalDBCStore sPlayerbotSupplementalDBCStore;

PlayerbotSupplementalDBCStore::PlayerbotSupplementalDBCStore()
    : m_transportAnimationStore(PlayerbotTransportAnimationEntryfmt),
      m_emotesTextSoundStore(PlayerbotEmotesTextSoundEntryfmt)
{
    static_assert(sizeof(TransportAnimationEntry) == 5 * sizeof(uint32),
        "Playerbot TransportAnimation entry must match the native DBCStorage format");
    static_assert(sizeof(EmotesTextSoundEntry) == 5 * sizeof(uint32),
        "Playerbot EmotesTextSound entry must match the native DBCStorage format");
}

bool PlayerbotSupplementalDBCStore::LoadTransportAnimations()
{
    std::call_once(m_transportLoadOnce, [this]()
    {
        if (DBCFileLoader::GetFormatRecordSize(PlayerbotTransportAnimationEntryfmt) != sizeof(TransportAnimationEntry))
        {
            sLog.outError("Playerbot TransportAnimation DBC format does not match its entry structure; elevator/tram travel nodes will be skipped.");
            return;
        }

        std::string path = sWorld.GetDataPath() + "dbc/TransportAnimation.dbc";
        if (!m_transportAnimationStore.Load(path.c_str()))
        {
            sLog.outError("Playerbot could not load '%s' with Tortoise DBCStorage; elevator/tram travel nodes will be skipped.", path.c_str());
            return;
        }

        for (uint32 id = 0; id < m_transportAnimationStore.GetNumRows(); ++id)
        {
            TransportAnimationEntry const* entry = m_transportAnimationStore.LookupEntry(id);
            if (!entry || !entry->transportEntry)
                continue;

            m_transportAnimations[entry->transportEntry][entry->timeSegment] =
                { entry->timeSegment, entry->x, entry->y, entry->z };
        }

        if (m_transportAnimations.empty())
        {
            sLog.outError("Playerbot loaded TransportAnimation.dbc but found no usable transport paths; elevator/tram travel nodes will be skipped.");
            return;
        }

        m_transportLoaded = true;
        sLog.outString("Playerbot loaded TransportAnimation.dbc through native Tortoise DBCStorage (%zu transport paths).",
            m_transportAnimations.size());
    });

    return m_transportLoaded;
}

PlayerbotSupplementalDBCStore::TransportAnimationPath const*
PlayerbotSupplementalDBCStore::GetTransportAnimationPath(uint32 transportEntry)
{
    if (!LoadTransportAnimations())
        return nullptr;

    auto itr = m_transportAnimations.find(transportEntry);
    return itr == m_transportAnimations.end() ? nullptr : &itr->second;
}

bool PlayerbotSupplementalDBCStore::LoadTextEmoteSounds()
{
    std::call_once(m_emoteSoundLoadOnce, [this]()
    {
        if (DBCFileLoader::GetFormatRecordSize(PlayerbotEmotesTextSoundEntryfmt) != sizeof(EmotesTextSoundEntry))
        {
            sLog.outError("Playerbot EmotesTextSound DBC format does not match its entry structure; text-emote sounds will be disabled.");
            return;
        }

        std::string path = sWorld.GetDataPath() + "dbc/EmotesTextSound.dbc";
        if (!m_emotesTextSoundStore.Load(path.c_str()))
        {
            sLog.outError("Playerbot could not load '%s' with Tortoise DBCStorage; text-emote sounds will be disabled.", path.c_str());
            return;
        }

        for (uint32 id = 0; id < m_emotesTextSoundStore.GetNumRows(); ++id)
        {
            EmotesTextSoundEntry const* entry = m_emotesTextSoundStore.LookupEntry(id);
            if (!entry || !entry->soundId)
                continue;

            m_emoteSounds[EmoteSoundKey(entry->textEmoteId, entry->race, entry->gender)] = entry->soundId;
        }

        if (m_emoteSounds.empty())
        {
            sLog.outError("Playerbot loaded EmotesTextSound.dbc but found no usable sound mappings; text-emote sounds will be disabled.");
            return;
        }

        m_emoteSoundsLoaded = true;
        sLog.outString("Playerbot loaded EmotesTextSound.dbc through native Tortoise DBCStorage (%zu emote sound mappings).",
            m_emoteSounds.size());
    });

    return m_emoteSoundsLoaded;
}

bool PlayerbotSupplementalDBCStore::GetTextEmoteSound(uint32 textEmoteId, uint32 race, uint32 gender, uint32& soundId)
{
    if (!LoadTextEmoteSounds())
        return false;

    auto itr = m_emoteSounds.find(EmoteSoundKey(textEmoteId, race, gender));
    if (itr == m_emoteSounds.end())
        return false;

    soundId = itr->second;
    return true;
}
