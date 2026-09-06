#include "overlay/WorldOverlayManager.h"

#include "Database/DatabaseEnv.h"
#include "Database/DBCStores.h"
#include "Log.h"
#include "Map.h"
#include "MapManager.h"

namespace WorldOverlay
{
    WorldOverlayManager& WorldOverlayManager::Instance()
    {
        static WorldOverlayManager instance;
        return instance;
    }

    void WorldOverlayManager::LoadDefinitions()
    {
        std::map<std::string, OverlayDefinition> loaded;

        QueryResult* result = WorldDatabase.Query(
            "SELECT overlay_id, overlay_key, map_id, base_spawn_policy, lifecycle_policy "
            "FROM worldoverlay_overlay WHERE enabled = 1 ORDER BY overlay_id");

        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                OverlayDefinition definition;
                definition.overlayId = fields[0].GetUInt32();
                definition.key = fields[1].GetCppString();
                definition.mapId = fields[2].GetUInt32();
                definition.baseSpawnPolicy = static_cast<OverlayBaseSpawnPolicy>(fields[3].GetUInt8());
                definition.lifecyclePolicy = fields[4].GetUInt8();

                if (definition.key.empty())
                {
                    sLog.outError("[WorldOverlay] Ignoring overlay %u with empty overlay_key.", definition.overlayId);
                    continue;
                }

                MapEntry const* mapEntry = sMapStorage.LookupEntry<MapEntry>(definition.mapId);
                if (!mapEntry || !mapEntry->IsDungeon() || mapEntry->IsBattleGround())
                {
                    sLog.outError("[WorldOverlay] Overlay '%s' uses unsupported Phase-0 map %u.",
                        definition.key.c_str(), definition.mapId);
                    continue;
                }

                if (definition.baseSpawnPolicy != OverlayBaseSpawnPolicy::Inherit)
                {
                    sLog.outError("[WorldOverlay] Overlay '%s' uses base_spawn_policy %u; Phase 0 supports INHERIT only.",
                        definition.key.c_str(), fields[3].GetUInt8());
                    continue;
                }

                loaded[definition.key] = definition;
            }
            while (result->NextRow());

            delete result;
        }

        uint32 const loadedCount = uint32(loaded.size());
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            m_overlays.swap(loaded);
        }

        sLog.outString("[WorldOverlay] Loaded %u overlay definition(s).", loadedCount);
    }

    bool WorldOverlayManager::GetOverlay(std::string const& overlayKey, OverlayDefinition& definition) const
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        auto const itr = m_overlays.find(overlayKey);
        if (itr == m_overlays.end())
            return false;

        definition = itr->second;
        return true;
    }

    std::vector<OverlayDefinition> WorldOverlayManager::GetOverlays() const
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        std::vector<OverlayDefinition> overlays;
        overlays.reserve(m_overlays.size());
        for (auto const& pair : m_overlays)
            overlays.push_back(pair.second);
        return overlays;
    }

    DungeonMap* WorldOverlayManager::ResolveRuntime(std::string const& overlayKey, RuntimeDefinition& runtime, std::string& error)
    {
        OverlayDefinition overlay;
        if (!GetOverlay(overlayKey, overlay))
        {
            error = "unknown or disabled overlay '" + overlayKey + "'";
            return nullptr;
        }

        // Serialize singleton allocation without holding the overlay-definition lock across MapManager calls.
        std::lock_guard<std::mutex> allocationGuard(m_runtimeAllocationMutex);

        if (m_runtimeRegistry.GetActive(overlayKey, runtime))
        {
            Map* existing = sMapMgr.FindMap(runtime.mapId, runtime.instanceId);
            if (existing && existing->IsDungeon() && existing->GetId() == overlay.mapId)
                return static_cast<DungeonMap*>(existing);

            m_runtimeRegistry.Erase(overlayKey);
        }

        DungeonMap* map = sMapMgr.CreateUnboundDungeonMap(overlay.mapId);
        if (!map)
        {
            error = "failed to allocate dungeon runtime for overlay '" + overlayKey + "'";
            return nullptr;
        }

        runtime.mapId = overlay.mapId;
        runtime.instanceId = map->GetInstanceId();
        runtime.gameObjectCount = m_gameObjectSpawner.Materialize(overlay.key, overlay.mapId, map);
        m_runtimeRegistry.Store(overlay.key, runtime);

        sLog.outString("[WorldOverlay] Runtime '%s' allocated map %u instance %u with %u overlay GO(s).",
            overlay.key.c_str(), runtime.mapId, runtime.instanceId, runtime.gameObjectCount);
        return map;
    }

    bool WorldOverlayManager::GetRuntime(std::string const& overlayKey, RuntimeDefinition& runtime)
    {
        return m_runtimeRegistry.GetActive(overlayKey, runtime);
    }

    bool WorldOverlayManager::FindRuntimeForLocation(uint32 mapId, uint32 instanceId, std::string& overlayKey, RuntimeDefinition& runtime)
    {
        return m_runtimeRegistry.FindByLocation(mapId, instanceId, overlayKey, runtime);
    }

    bool WorldOverlayManager::HasActiveRuntimes()
    {
        return m_runtimeRegistry.HasActiveRuntimes();
    }

    void WorldOverlayManager::OnMapDestroyed(Map const* map)
    {
        m_runtimeRegistry.OnMapDestroyed(map);
    }

    void WorldOverlayManager::ClearRuntimes()
    {
        m_runtimeRegistry.Clear();
    }
}
