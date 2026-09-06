#include "builder/WorldOverlayCommandScript.h"

#include "Chat.h"
#include "Player.h"
#include "ScriptObjects.h"
#include "WorldSession.h"
#include "overlay/WorldOverlayManager.h"
#include "routing/WorldRouting.h"

#include <cstring>
#include <sstream>
#include <string>

namespace WorldOverlay
{
    namespace
    {
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
                    auto overlays = WorldOverlayManager::Instance().GetOverlays();
                    if (overlays.empty())
                    {
                        handler->SendSysMessage("WorldOverlay: no enabled Phase-0 overlays loaded.");
                        return false;
                    }

                    handler->SendSysMessage("WorldOverlay overlays:");
                    for (OverlayDefinition const& overlay : overlays)
                    {
                        RuntimeDefinition runtime;
                        if (WorldOverlayManager::Instance().GetRuntime(overlay.key, runtime))
                        {
                            handler->PSendSysMessage("  %s: map %u, runtime %u, GO %u", overlay.key.c_str(),
                                overlay.mapId, runtime.instanceId, runtime.gameObjectCount);
                        }
                        else
                            handler->PSendSysMessage("  %s: map %u, runtime inactive", overlay.key.c_str(), overlay.mapId);
                    }
                    return false;
                }

                if (subcommand == "info")
                {
                    Player* player = handler->GetPlayer();
                    std::string overlayKey;
                    RuntimeDefinition runtime;
                    if (!WorldOverlayManager::Instance().FindRuntimeForLocation(
                            player->GetMapId(), player->GetInstanceId(), overlayKey, runtime))
                    {
                        handler->PSendSysMessage("WorldOverlay: none (current map %u, instance %u).",
                            player->GetMapId(), player->GetInstanceId());
                        return false;
                    }

                    OverlayDefinition overlay;
                    WorldOverlayManager::Instance().GetOverlay(overlayKey, overlay);
                    handler->PSendSysMessage("WorldOverlay: %s", overlayKey.c_str());
                    handler->PSendSysMessage("Map: %u", runtime.mapId);
                    handler->PSendSysMessage("Runtime instance: %u", runtime.instanceId);
                    handler->PSendSysMessage("Base spawns: %s",
                        overlay.baseSpawnPolicy == OverlayBaseSpawnPolicy::Inherit ? "INHERIT" : "NONE");
                    handler->PSendSysMessage("Overlay GO spawns: %u", runtime.gameObjectCount);
                    return false;
                }

                if (subcommand == "where")
                {
                    Player* player = handler->GetPlayer();
                    std::string overlayKey;
                    RuntimeDefinition runtime;
                    if (WorldOverlayManager::Instance().FindRuntimeForLocation(
                            player->GetMapId(), player->GetInstanceId(), overlayKey, runtime))
                    {
                        handler->PSendSysMessage("WorldOverlay: current map %u, instance %u, overlay %s.",
                            player->GetMapId(), player->GetInstanceId(), overlayKey.c_str());
                    }
                    else
                    {
                        handler->PSendSysMessage("WorldOverlay: current map %u, instance %u, no overlay runtime.",
                            player->GetMapId(), player->GetInstanceId());
                    }
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

                    OverlayDefinition overlay;
                    if (!WorldOverlayManager::Instance().GetOverlay(overlayKey, overlay))
                    {
                        handler->PSendSysMessage("WorldOverlay: unknown overlay '%s'.", overlayKey.c_str());
                        return false;
                    }

                    RuntimeDefinition runtime;
                    if (!WorldOverlayManager::Instance().GetRuntime(overlayKey, runtime))
                    {
                        handler->PSendSysMessage("WorldOverlay: %s has no active runtime instance.", overlayKey.c_str());
                        return false;
                    }

                    handler->PSendSysMessage("WorldOverlay: %s -> map %u, runtime instance %u, overlay GO count %u.",
                        overlayKey.c_str(), runtime.mapId, runtime.instanceId, runtime.gameObjectCount);
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

                    OverlayDefinition overlay;
                    if (!WorldOverlayManager::Instance().GetOverlay(overlayKey, overlay))
                    {
                        handler->PSendSysMessage("WorldOverlay: unknown or disabled overlay '%s'.", overlayKey.c_str());
                        return false;
                    }

                    std::string resolvedDestinationKey;
                    std::string error;
                    if (!WorldRouting::Instance().TeleportToOverlayDestination(handler->GetPlayer(), overlayKey,
                            destinationKey, resolvedDestinationKey, error))
                    {
                        handler->PSendSysMessage("WorldOverlay: %s.", error.c_str());
                        return false;
                    }

                    RuntimeDefinition runtime;
                    if (WorldOverlayManager::Instance().GetRuntime(overlayKey, runtime))
                    {
                        handler->PSendSysMessage("WorldOverlay: entering %s via %s -> map %u, runtime instance %u, overlay GO count %u.",
                            overlayKey.c_str(), resolvedDestinationKey.c_str(), runtime.mapId, runtime.instanceId, runtime.gameObjectCount);
                    }
                    return false;
                }

                if (subcommand == "reload")
                {
                    if (WorldOverlayManager::Instance().HasActiveRuntimes())
                    {
                        handler->SendSysMessage("WorldOverlay: reload refused while an overlay runtime is active.");
                        return false;
                    }

                    WorldOverlayManager::Instance().ClearRuntimes();
                    WorldOverlayManager::Instance().LoadDefinitions();
                    WorldRouting::Instance().Load();
                    handler->PSendSysMessage("WorldOverlay: definitions reloaded (%u destinations, %u bindings).",
                        WorldRouting::Instance().DestinationCount(), WorldRouting::Instance().BindingCount());
                    return false;
                }

                handler->PSendSysMessage("WorldOverlay: unknown subcommand '%s'.", subcommand.c_str());
                return false;
            }
        };
    }

    void RegisterWorldOverlayCommandScript()
    {
        new WorldOverlayCommandScript();
    }
}
