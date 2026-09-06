#ifndef MOD_WORLDOVERLAY_WORLD_OVERLAY_MANAGER_H
#define MOD_WORLDOVERLAY_WORLD_OVERLAY_MANAGER_H

#include "overlay/RuntimeInstanceRegistry.h"
#include "overlay/WorldOverlayTypes.h"
#include "spawn/OverlayGameObjectSpawner.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

class DungeonMap;
class Map;

namespace WorldOverlay
{
    class WorldOverlayManager
    {
    public:
        static WorldOverlayManager& Instance();

        void LoadDefinitions();
        bool GetOverlay(std::string const& overlayKey, OverlayDefinition& definition) const;
        std::vector<OverlayDefinition> GetOverlays() const;

        DungeonMap* ResolveRuntime(std::string const& overlayKey, RuntimeDefinition& runtime, std::string& error);
        bool GetRuntime(std::string const& overlayKey, RuntimeDefinition& runtime);
        bool FindRuntimeForLocation(uint32 mapId, uint32 instanceId, std::string& overlayKey, RuntimeDefinition& runtime);
        bool HasActiveRuntimes();

        void OnMapDestroyed(Map const* map);
        void ClearRuntimes();

    private:
        WorldOverlayManager() = default;

        mutable std::mutex m_mutex;
        std::map<std::string, OverlayDefinition> m_overlays;
        std::mutex m_runtimeAllocationMutex;
        RuntimeInstanceRegistry m_runtimeRegistry;
        OverlayGameObjectSpawner m_gameObjectSpawner;
    };
}

#endif
