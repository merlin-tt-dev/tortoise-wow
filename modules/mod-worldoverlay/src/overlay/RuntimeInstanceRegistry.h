#ifndef MOD_WORLDOVERLAY_RUNTIME_INSTANCE_REGISTRY_H
#define MOD_WORLDOVERLAY_RUNTIME_INSTANCE_REGISTRY_H

#include "overlay/WorldOverlayTypes.h"

#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

class Map;

namespace WorldOverlay
{
    class RuntimeInstanceRegistry
    {
    public:
        bool GetActive(std::string const& overlayKey, RuntimeDefinition& runtime);
        bool FindByLocation(uint32 mapId, uint32 instanceId, std::string& overlayKey, RuntimeDefinition& runtime);
        std::vector<std::pair<std::string, RuntimeDefinition>> Snapshot() const;

        void Store(std::string const& overlayKey, RuntimeDefinition const& runtime);
        void Erase(std::string const& overlayKey);
        void OnMapDestroyed(Map const* map);
        void Clear();
        bool HasActiveRuntimes();

    private:
        mutable std::mutex m_mutex;
        std::map<std::string, RuntimeDefinition> m_runtimes;
    };
}

#endif
