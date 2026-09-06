#ifndef MOD_WORLDOVERLAY_OVERLAY_GAMEOBJECT_SPAWNER_H
#define MOD_WORLDOVERLAY_OVERLAY_GAMEOBJECT_SPAWNER_H

#include "Common.h"

#include <string>

class DungeonMap;

namespace WorldOverlay
{
    class OverlayGameObjectSpawner
    {
    public:
        uint32 Materialize(std::string const& overlayKey, uint32 mapId, DungeonMap* map) const;
    };
}

#endif
