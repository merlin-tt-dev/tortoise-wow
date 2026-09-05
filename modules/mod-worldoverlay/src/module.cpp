#include "ScriptObjects.h"

#include "Database/DatabaseEnv.h"
#include "GameObject.h"
#include "Log.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "WorldSession.h"

#include <cstring>
#include <map>
#include <mutex>
#include <sstream>
#include <string>

namespace
{
    struct OverlayDefinition
    {
        uint32 overlayId = 0;
        std::string key;
        uint32 mapId = 0;
        uint8 baseSpawnPolicy = 0;
        uint8 lifecyclePolicy = 0;
    };

    struct DestinationDefinition
    {
        uint32 destinationId = 0;
        std::string key;
        uint32 mapId = 0;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float o = 0.0f;
        uint8 instancePolicy = 0;
        std::string overlayKey;
    };

    struct RuntimeDefinition
    {
        uint32 mapId = 0;
        uint32 instanceId = 0;
        uint32 gameObjectCount = 0;
    };

    class WorldOverlayManager
    {
    public:
        static WorldOverlayManager& Instance()
        {
            static WorldOverlayManager instance;
            return instance;
        }

        void LoadDefinitions()
        {
            std::lock_guard<std::recursive_mutex> guard(m_mutex);

            m_overlays.clear();
            m_destinations.clear();
            m_defaultDestinations.clear();

            QueryResult* overlayResult = WorldDatabase.Query(
                "SELECT overlay_id, overlay_key, map_id, base_spawn_policy, lifecycle_policy "
                "FROM worldoverlay_overlay WHERE enabled = 1 ORDER BY overlay_id");

            if (overlayResult)
            {
                do
                {
                    Field* fields = overlayResult->Fetch();
                    OverlayDefinition definition;
                    definition.overlayId = fields[0].GetUInt32();
                    definition.key = fields[1].GetCppString();
                    definition.mapId = fields[2].GetUInt32();
                    definition.baseSpawnPolicy = fields[3].GetUInt8();
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

                    m_overlays[definition.key] = definition;
                }
                while (overlayResult->NextRow());

                delete overlayResult;
            }

            QueryResult* destinationResult = WorldDatabase.Query(
                "SELECT destination_id, destination_key, map_id, position_x, position_y, position_z, orientation, "
                "instance_policy, overlay_key FROM worldoverlay_destination "
                "WHERE enabled = 1 ORDER BY destination_id");

            if (destinationResult)
            {
                do
                {
                    Field* fields = destinationResult->Fetch();
                    DestinationDefinition destination;
                    destination.destinationId = fields[0].GetUInt32();
                    destination.key = fields[1].GetCppString();
                    destination.mapId = fields[2].GetUInt32();
                    destination.x = fields[3].GetFloat();
                    destination.y = fields[4].GetFloat();
                    destination.z = fields[5].GetFloat();
                    destination.o = fields[6].GetFloat();
                    destination.instancePolicy = fields[7].GetUInt8();
                    destination.overlayKey = fields[8].GetCppString();

                    if (destination.key.empty() || destination.overlayKey.empty())
                        continue;

                    if (destination.instancePolicy != 2)
                        continue;

                    auto const overlayItr = m_overlays.find(destination.overlayKey);
                    if (overlayItr == m_overlays.end())
                        continue;

                    if (destination.mapId != overlayItr->second.mapId)
                    {
                        sLog.outError("[WorldOverlay] Destination '%s' map %u does not match overlay '%s' map %u.",
                            destination.key.c_str(), destination.mapId, destination.overlayKey.c_str(), overlayItr->second.mapId);
                        continue;
                    }

                    if (!MapManager::IsValidMapCoord(destination.mapId, destination.x, destination.y, destination.z, destination.o))
                    {
                        sLog.outError("[WorldOverlay] Destination '%s' has invalid coordinates.", destination.key.c_str());
                        continue;
                    }

                    m_destinations[destination.key] = destination;
                    if (!m_defaultDestinations.count(destination.overlayKey))
                        m_defaultDestinations[destination.overlayKey] = destination.key;
                }
                while (destinationResult->NextRow());

                delete destinationResult;
            }

            sLog.outString("[WorldOverlay] Loaded %u overlay definition(s) and %u overlay destination(s).",
                uint32(m_overlays.size()), uint32(m_destinations.size()));
        }

        void OnMapDestroyed(Map* map)
        {
            if (!map)
                return;

            std::lock_guard<std::recursive_mutex> guard(m_mutex);
            for (auto itr = m_runtimes.begin(); itr != m_runtimes.end();)
            {
                if (itr->second.mapId == map->GetId() && itr->second.instanceId == map->GetInstanceId())
                {
                    sLog.outString("[WorldOverlay] Runtime '%s' released map %u instance %u.",
                        itr->first.c_str(), itr->second.mapId, itr->second.instanceId);
                    itr = m_runtimes.erase(itr);
                }
                else
                    ++itr;
            }
        }

        void ClearRuntimes()
        {
            std::lock_guard<std::recursive_mutex> guard(m_mutex);
            m_runtimes.clear();
        }

        bool Enter(ChatHandler* handler, Player* player, std::string const& overlayKey, std::string const& destinationKey)
        {
            if (!handler || !player)
                return false;

            std::lock_guard<std::recursive_mutex> guard(m_mutex);

            auto const overlayItr = m_overlays.find(overlayKey);
            if (overlayItr == m_overlays.end())
            {
                handler->PSendSysMessage("WorldOverlay: unknown or disabled overlay '%s'.", overlayKey.c_str());
                return false;
            }

            DestinationDefinition const* destination = ResolveDestination(overlayItr->second, destinationKey);
            if (!destination)
            {
                if (destinationKey.empty())
                    handler->PSendSysMessage("WorldOverlay: overlay '%s' has no enabled OVERLAY destination.", overlayKey.c_str());
                else
                    handler->PSendSysMessage("WorldOverlay: destination '%s' does not belong to overlay '%s'.",
                        destinationKey.c_str(), overlayKey.c_str());
                return false;
            }

            RuntimeDefinition* runtime = EnsureRuntime(overlayItr->second, handler);
            if (!runtime)
                return false;

            DungeonMap* map = static_cast<DungeonMap*>(sMapMgr.FindMap(runtime->mapId, runtime->instanceId));
            if (!map)
            {
                handler->PSendSysMessage("WorldOverlay: runtime '%s' disappeared before transfer.", overlayKey.c_str());
                m_runtimes.erase(overlayKey);
                return false;
            }

            if (!sMapMgr.TeleportPlayerToUnboundDungeon(player, map,
                    destination->x, destination->y, destination->z, destination->o))
            {
                handler->PSendSysMessage("WorldOverlay: transfer to '%s' failed.", overlayKey.c_str());
                return false;
            }

            handler->PSendSysMessage("WorldOverlay: entering %s -> map %u, runtime instance %u, overlay GO count %u.",
                overlayKey.c_str(), runtime->mapId, runtime->instanceId, runtime->gameObjectCount);
            return true;
        }

        void List(ChatHandler* handler)
        {
            if (!handler)
                return;

            std::lock_guard<std::recursive_mutex> guard(m_mutex);
            if (m_overlays.empty())
            {
                handler->SendSysMessage("WorldOverlay: no enabled Phase-0 overlays loaded.");
                return;
            }

            handler->SendSysMessage("WorldOverlay overlays:");
            for (auto const& pair : m_overlays)
            {
                auto const runtimeItr = m_runtimes.find(pair.first);
                if (runtimeItr == m_runtimes.end() ||
                    !sMapMgr.FindMap(runtimeItr->second.mapId, runtimeItr->second.instanceId))
                {
                    handler->PSendSysMessage("  %s: map %u, runtime inactive", pair.first.c_str(), pair.second.mapId);
                }
                else
                {
                    handler->PSendSysMessage("  %s: map %u, runtime %u, GO %u", pair.first.c_str(),
                        pair.second.mapId, runtimeItr->second.instanceId, runtimeItr->second.gameObjectCount);
                }
            }
        }

        void Info(ChatHandler* handler, Player* player)
        {
            if (!handler || !player)
                return;

            std::lock_guard<std::recursive_mutex> guard(m_mutex);
            for (auto const& pair : m_runtimes)
            {
                RuntimeDefinition const& runtime = pair.second;
                if (runtime.mapId == player->GetMapId() && runtime.instanceId == player->GetInstanceId())
                {
                    auto const definitionItr = m_overlays.find(pair.first);
                    handler->PSendSysMessage("WorldOverlay: %s", pair.first.c_str());
                    handler->PSendSysMessage("Map: %u", runtime.mapId);
                    handler->PSendSysMessage("Runtime instance: %u", runtime.instanceId);
                    if (definitionItr != m_overlays.end())
                        handler->PSendSysMessage("Base spawns: %s", definitionItr->second.baseSpawnPolicy == 1 ? "INHERIT" : "NONE");
                    handler->PSendSysMessage("Overlay GO spawns: %u", runtime.gameObjectCount);
                    return;
                }
            }

            handler->PSendSysMessage("WorldOverlay: none (current map %u, instance %u).",
                player->GetMapId(), player->GetInstanceId());
        }

        void RuntimeInfo(ChatHandler* handler, std::string const& overlayKey)
        {
            if (!handler)
                return;

            std::lock_guard<std::recursive_mutex> guard(m_mutex);
            auto const definitionItr = m_overlays.find(overlayKey);
            if (definitionItr == m_overlays.end())
            {
                handler->PSendSysMessage("WorldOverlay: unknown overlay '%s'.", overlayKey.c_str());
                return;
            }

            auto runtimeItr = m_runtimes.find(overlayKey);
            if (runtimeItr != m_runtimes.end() &&
                !sMapMgr.FindMap(runtimeItr->second.mapId, runtimeItr->second.instanceId))
            {
                m_runtimes.erase(runtimeItr);
                runtimeItr = m_runtimes.end();
            }

            if (runtimeItr == m_runtimes.end())
            {
                handler->PSendSysMessage("WorldOverlay: %s has no active runtime instance.", overlayKey.c_str());
                return;
            }

            handler->PSendSysMessage("WorldOverlay: %s -> map %u, runtime instance %u, overlay GO count %u.",
                overlayKey.c_str(), runtimeItr->second.mapId, runtimeItr->second.instanceId,
                runtimeItr->second.gameObjectCount);
        }

        bool Reload(ChatHandler* handler)
        {
            std::lock_guard<std::recursive_mutex> guard(m_mutex);
            for (auto const& pair : m_runtimes)
            {
                if (sMapMgr.FindMap(pair.second.mapId, pair.second.instanceId))
                {
                    if (handler)
                        handler->SendSysMessage("WorldOverlay: reload refused while an overlay runtime is active.");
                    return false;
                }
            }

            m_runtimes.clear();
            LoadDefinitions();
            if (handler)
                handler->SendSysMessage("WorldOverlay: definitions reloaded.");
            return true;
        }

    private:
        DestinationDefinition const* ResolveDestination(OverlayDefinition const& overlay, std::string const& requestedKey) const
        {
            std::string key = requestedKey;
            if (key.empty())
            {
                auto const defaultItr = m_defaultDestinations.find(overlay.key);
                if (defaultItr == m_defaultDestinations.end())
                    return nullptr;
                key = defaultItr->second;
            }

            auto const itr = m_destinations.find(key);
            if (itr == m_destinations.end())
                return nullptr;

            if (itr->second.overlayKey != overlay.key || itr->second.mapId != overlay.mapId || itr->second.instancePolicy != 2)
                return nullptr;

            return &itr->second;
        }

        RuntimeDefinition* EnsureRuntime(OverlayDefinition const& overlay, ChatHandler* handler)
        {
            auto runtimeItr = m_runtimes.find(overlay.key);
            if (runtimeItr != m_runtimes.end())
            {
                Map* existing = sMapMgr.FindMap(runtimeItr->second.mapId, runtimeItr->second.instanceId);
                if (existing && existing->IsDungeon() && existing->GetId() == overlay.mapId)
                    return &runtimeItr->second;

                m_runtimes.erase(runtimeItr);
            }

            DungeonMap* map = sMapMgr.CreateUnboundDungeonMap(overlay.mapId);
            if (!map)
            {
                if (handler)
                    handler->PSendSysMessage("WorldOverlay: failed to allocate dungeon runtime for '%s'.", overlay.key.c_str());
                return nullptr;
            }

            RuntimeDefinition runtime;
            runtime.mapId = overlay.mapId;
            runtime.instanceId = map->GetInstanceId();
            runtime.gameObjectCount = MaterializeGameObjects(overlay, map);

            auto inserted = m_runtimes.emplace(overlay.key, runtime);
            sLog.outString("[WorldOverlay] Runtime '%s' allocated map %u instance %u with %u overlay GO(s).",
                overlay.key.c_str(), runtime.mapId, runtime.instanceId, runtime.gameObjectCount);
            return &inserted.first->second;
        }

        uint32 MaterializeGameObjects(OverlayDefinition const& overlay, DungeonMap* map)
        {
            std::string escapedKey = overlay.key;
            WorldDatabase.escape_string(escapedKey);

            QueryResult* result = WorldDatabase.PQuery(
                "SELECT spawn_id, entry, position_x, position_y, position_z, orientation, "
                "rotation0, rotation1, rotation2, rotation3 "
                "FROM worldoverlay_gameobject WHERE overlay_key = '%s' AND enabled = 1 ORDER BY spawn_id",
                escapedKey.c_str());

            if (!result)
                return 0;

            uint32 count = 0;
            do
            {
                Field* fields = result->Fetch();
                uint64 const spawnId = fields[0].GetUInt64();
                uint32 const entry = fields[1].GetUInt32();
                float const x = fields[2].GetFloat();
                float const y = fields[3].GetFloat();
                float const z = fields[4].GetFloat();
                float const o = fields[5].GetFloat();
                float const r0 = fields[6].GetFloat();
                float const r1 = fields[7].GetFloat();
                float const r2 = fields[8].GetFloat();
                float const r3 = fields[9].GetFloat();

                if (!sObjectMgr.GetGameObjectInfo(entry))
                {
                    sLog.outError("[WorldOverlay] GO spawn " UI64FMTD " in '%s' references missing gameobject_template %u.",
                        spawnId, overlay.key.c_str(), entry);
                    continue;
                }

                if (!MapManager::IsValidMapCoord(overlay.mapId, x, y, z, o))
                {
                    sLog.outError("[WorldOverlay] GO spawn " UI64FMTD " in '%s' has invalid coordinates.",
                        spawnId, overlay.key.c_str());
                    continue;
                }

                // Phase 0 keeps materialized overlay objects alive for the lifetime of the runtime map.
                // Persistent respawn semantics remain module policy for a later phase.
                GameObject* object = map->SummonGameObject(entry, x, y, z, o, r0, r1, r2, r3, 0, WORLD_DEFAULT_OBJECT);
                if (!object)
                {
                    sLog.outError("[WorldOverlay] Failed to materialize GO spawn " UI64FMTD " (entry %u) in '%s'.",
                        spawnId, entry, overlay.key.c_str());
                    continue;
                }

                ++count;
            }
            while (result->NextRow());

            delete result;
            return count;
        }

        std::recursive_mutex m_mutex;
        std::map<std::string, OverlayDefinition> m_overlays;
        std::map<std::string, DestinationDefinition> m_destinations;
        std::map<std::string, std::string> m_defaultDestinations;
        std::map<std::string, RuntimeDefinition> m_runtimes;
    };

    class WorldOverlayWorldScript final : public WorldScript
    {
    public:
        WorldOverlayWorldScript()
            : WorldScript("mod_worldoverlay_world", { WORLDHOOK_ON_STARTUP, WORLDHOOK_ON_AFTER_UNLOAD_ALL_MAPS })
        {
        }

        void OnStartup() override
        {
            WorldOverlayManager::Instance().LoadDefinitions();
            sLog.outString("[WorldOverlay] Phase-0 runtime loaded.");
        }

        void OnAfterUnloadAllMaps() override
        {
            WorldOverlayManager::Instance().ClearRuntimes();
        }
    };

    class WorldOverlayMapScript final : public AllMapScript
    {
    public:
        WorldOverlayMapScript() : AllMapScript("mod_worldoverlay_maps") {}

        void OnDestroyMap(Map* map) override
        {
            WorldOverlayManager::Instance().OnMapDestroyed(map);
        }
    };

    class WorldOverlayCommandScript final : public AllCommandScript
    {
    public:
        WorldOverlayCommandScript() : AllCommandScript("mod_worldoverlay_commands") {}

        bool CanExecuteCommand(ChatHandler* handler, char const* command, char const* args) override
        {
            if (!command || (std::strcmp(command, "wo") != 0 && std::strcmp(command, "woverlay") != 0))
                return true;

            if (!handler || !handler->GetSession() || !handler->GetPlayer())
                return false;

            if (handler->GetSession()->GetSecurity() < SEC_DEVELOPER)
            {
                handler->SendSysMessage("WorldOverlay commands require developer security.");
                return false;
            }

            std::istringstream input(args ? args : "");
            std::string subcommand;
            input >> subcommand;

            if (subcommand.empty())
            {
                handler->SendSysMessage("WorldOverlay Phase 0: .wo list | .wo info | .wo where | .wo enter <overlay> [destination] | .wo runtime <overlay> | .wo reload");
                return false;
            }

            if (subcommand == "list")
            {
                WorldOverlayManager::Instance().List(handler);
                return false;
            }

            if (subcommand == "info" || subcommand == "where")
            {
                WorldOverlayManager::Instance().Info(handler, handler->GetPlayer());
                return false;
            }

            if (subcommand == "enter")
            {
                std::string overlayKey;
                std::string destinationKey;
                input >> overlayKey >> destinationKey;
                if (overlayKey.empty())
                {
                    handler->SendSysMessage("Usage: .wo enter <overlay> [destination]");
                    return false;
                }

                WorldOverlayManager::Instance().Enter(handler, handler->GetPlayer(), overlayKey, destinationKey);
                return false;
            }

            if (subcommand == "runtime")
            {
                std::string overlayKey;
                input >> overlayKey;
                if (overlayKey.empty())
                {
                    handler->SendSysMessage("Usage: .wo runtime <overlay>");
                    return false;
                }

                WorldOverlayManager::Instance().RuntimeInfo(handler, overlayKey);
                return false;
            }

            if (subcommand == "reload")
            {
                WorldOverlayManager::Instance().Reload(handler);
                return false;
            }

            handler->PSendSysMessage("WorldOverlay Phase 0: unknown subcommand '%s'.", subcommand.c_str());
            return false;
        }
    };
}

void Addmod_worldoverlayScripts()
{
    new WorldOverlayWorldScript();
    new WorldOverlayMapScript();
    new WorldOverlayCommandScript();
}
