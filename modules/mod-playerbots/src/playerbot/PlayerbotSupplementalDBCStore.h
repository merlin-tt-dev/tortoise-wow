#ifndef _PLAYERBOT_SUPPLEMENTAL_DBC_STORE_H
#define _PLAYERBOT_SUPPLEMENTAL_DBC_STORE_H

#include "Common.h"
#include "Database/DBCStore.h"

#include <map>
#include <mutex>
#include <tuple>
#include <unordered_map>

// Playerbot-only access to genuine client DBCs that Tortoise does not need to
// load for normal core gameplay. The files are parsed through Tortoise's native
// DBCStorage/DBCFileLoader; this class only builds module-local lookup indexes.
class PlayerbotSupplementalDBCStore
{
public:
    PlayerbotSupplementalDBCStore();

    struct TransportAnimationNode
    {
        uint32 timeSegment = 0;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    using TransportAnimationPath = std::map<uint32, TransportAnimationNode>;

    // Returns the ordered TransportAnimation.dbc path for a GO entry, or
    // nullptr when the optional DBC is unavailable or the entry has no path.
    TransportAnimationPath const* GetTransportAnimationPath(uint32 transportEntry);

    // Resolves the exact historical EmotesTextSound key
    // (text-emote, race, gender) to its client sound id.
    bool GetTextEmoteSound(uint32 textEmoteId, uint32 race, uint32 gender, uint32& soundId);

private:
    struct TransportAnimationEntry
    {
        uint32 transportEntry;
        uint32 timeSegment;
        float x;
        float y;
        float z;
    };

    struct EmotesTextSoundEntry
    {
        uint32 id;
        uint32 textEmoteId;
        uint32 race;
        uint32 gender;
        uint32 soundId;
    };

    using EmoteSoundKey = std::tuple<uint32, uint32, uint32>;

    bool LoadTransportAnimations();
    bool LoadTextEmoteSounds();

    DBCStorage<TransportAnimationEntry> m_transportAnimationStore;
    DBCStorage<EmotesTextSoundEntry> m_emotesTextSoundStore;

    std::unordered_map<uint32, TransportAnimationPath> m_transportAnimations;
    std::map<EmoteSoundKey, uint32> m_emoteSounds;

    std::once_flag m_transportLoadOnce;
    std::once_flag m_emoteSoundLoadOnce;
    bool m_transportLoaded = false;
    bool m_emoteSoundsLoaded = false;
};

extern PlayerbotSupplementalDBCStore sPlayerbotSupplementalDBCStore;

#endif
