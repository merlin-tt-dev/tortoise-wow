#include "spawn/OverlayGameObjectSpawner.h"

#include "Database/DatabaseEnv.h"
#include "GameObject.h"
#include "Log.h"
#include "Map.h"
#include "MapManager.h"
#include "ObjectMgr.h"

namespace WorldOverlay
{
    uint32 OverlayGameObjectSpawner::Materialize(std::string const& overlayKey, uint32 mapId, DungeonMap* map) const
    {
        if (!map)
            return 0;

        std::string escapedKey = overlayKey;
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
                    spawnId, overlayKey.c_str(), entry);
                continue;
            }

            if (!MapManager::IsValidMapCoord(mapId, x, y, z, o))
            {
                sLog.outError("[WorldOverlay] GO spawn " UI64FMTD " in '%s' has invalid coordinates.",
                    spawnId, overlayKey.c_str());
                continue;
            }

            // Phase 0 keeps overlay objects map-local and alive for the runtime map lifetime.
            // No row is inserted into the core gameobject table and no runtime InstanceID is persisted.
            GameObject* object = map->SummonGameObject(entry, x, y, z, o, r0, r1, r2, r3, 0, WORLD_DEFAULT_OBJECT);
            if (!object)
            {
                sLog.outError("[WorldOverlay] Failed to materialize GO spawn " UI64FMTD " (entry %u) in '%s'.",
                    spawnId, entry, overlayKey.c_str());
                continue;
            }

            ++count;
        }
        while (result->NextRow());

        delete result;
        return count;
    }
}
