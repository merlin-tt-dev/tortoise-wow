#ifndef MOD_WORLDOVERLAY_WORLD_OVERLAY_TYPES_H
#define MOD_WORLDOVERLAY_WORLD_OVERLAY_TYPES_H

#include "Common.h"

#include <string>

namespace WorldOverlay
{
    enum class OverlayBaseSpawnPolicy : uint8
    {
        None = 0,
        Inherit = 1
    };

    struct OverlayDefinition
    {
        uint32 overlayId = 0;
        std::string key;
        uint32 mapId = 0;
        OverlayBaseSpawnPolicy baseSpawnPolicy = OverlayBaseSpawnPolicy::None;
        uint8 lifecyclePolicy = 0;
    };

    struct RuntimeDefinition
    {
        uint32 mapId = 0;
        uint32 instanceId = 0;
        uint32 gameObjectCount = 0;
    };
}

#endif
