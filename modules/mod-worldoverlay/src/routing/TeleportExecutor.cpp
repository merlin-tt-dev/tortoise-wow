#include "routing/TeleportExecutor.h"

#include "MapManager.h"
#include "Player.h"
#include "overlay/WorldOverlayManager.h"

namespace WorldOverlay
{
    bool TeleportExecutor::Execute(Player* player, ResolvedDestination const& destination, std::string& error) const
    {
        if (!player)
        {
            error = "no player available for teleport";
            return false;
        }

        switch (destination.instancePolicy)
        {
            case DestinationInstancePolicy::Auto:
                if (!player->TeleportTo(destination.mapId, destination.x, destination.y, destination.z, destination.o))
                {
                    error = "normal Tortoise teleport failed for destination '" + destination.key + "'";
                    return false;
                }
                return true;

            case DestinationInstancePolicy::Current:
                if (player->GetMapId() == destination.mapId)
                {
                    if (!player->NearTeleportTo(destination.x, destination.y, destination.z, destination.o))
                    {
                        error = "same-instance teleport failed for destination '" + destination.key + "'";
                        return false;
                    }
                    return true;
                }

                if (!player->TeleportTo(destination.mapId, destination.x, destination.y, destination.z, destination.o))
                {
                    error = "CURRENT destination '" + destination.key + "' could not fall back to normal map transfer";
                    return false;
                }
                return true;

            case DestinationInstancePolicy::Overlay:
            {
                if (destination.overlayKey.empty())
                {
                    error = "OVERLAY destination '" + destination.key + "' has no overlay_key";
                    return false;
                }

                OverlayDefinition overlay;
                if (!WorldOverlayManager::Instance().GetOverlay(destination.overlayKey, overlay))
                {
                    error = "OVERLAY destination '" + destination.key + "' references unknown overlay '" + destination.overlayKey + "'";
                    return false;
                }

                if (overlay.mapId != destination.mapId)
                {
                    error = "OVERLAY destination '" + destination.key + "' map does not match overlay '" + destination.overlayKey + "'";
                    return false;
                }

                RuntimeDefinition runtime;
                DungeonMap* map = WorldOverlayManager::Instance().ResolveRuntime(destination.overlayKey, runtime, error);
                if (!map)
                    return false;

                if (!sMapMgr.TeleportPlayerToUnboundDungeon(player, map,
                        destination.x, destination.y, destination.z, destination.o))
                {
                    error = "instance-aware transfer failed for destination '" + destination.key + "'";
                    return false;
                }

                return true;
            }

            case DestinationInstancePolicy::ExplicitInstance:
                error = "EXPLICIT_INSTANCE is reserved for admin/debug routing and is not implemented in Phase 0";
                return false;
        }

        error = "destination '" + destination.key + "' has an unsupported instance policy";
        return false;
    }
}
