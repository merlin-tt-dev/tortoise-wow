#include "ScriptObjects.h"

#include "Log.h"
#include "MapManager.h"
#include "builder/WorldOverlayCommandScript.h"
#include "overlay/WorldOverlayManager.h"
#include "routing/WorldRouting.h"

namespace WorldOverlay
{
    namespace
    {
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
                WorldRouting::Instance().Load();
                sLog.outString("[WorldOverlay] Phase-0 runtime loaded with internal WorldRouting.");
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

    }

    void RegisterWorldOverlayRuntimeScripts()
    {
        new WorldOverlayWorldScript();
        new WorldOverlayMapScript();
    }
}

void Addmod_worldoverlayScripts()
{
    WorldOverlay::RegisterWorldOverlayRuntimeScripts();
    WorldOverlay::RegisterWorldOverlayCommandScript();
}
