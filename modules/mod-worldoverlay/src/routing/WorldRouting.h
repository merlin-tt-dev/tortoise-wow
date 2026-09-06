#ifndef MOD_WORLDOVERLAY_WORLD_ROUTING_H
#define MOD_WORLDOVERLAY_WORLD_ROUTING_H

#include "routing/DestinationRegistry.h"
#include "routing/DestinationResolver.h"
#include "routing/TeleportBindingManager.h"
#include "routing/TeleportExecutor.h"

#include <string>

class Player;

namespace WorldOverlay
{
    class WorldRouting
    {
    public:
        static WorldRouting& Instance();

        void Load();
        bool ResolveDestination(std::string const& destinationKey, ResolvedDestination& destination, std::string& error) const;
        bool TeleportToDestination(Player* player, std::string const& destinationKey, std::string& error) const;
        bool TeleportToOverlayDestination(Player* player, std::string const& overlayKey,
            std::string const& requestedDestinationKey, std::string& resolvedDestinationKey, std::string& error) const;
        bool FindBinding(TeleportBindingSourceType sourceType, uint32 sourceEntry, TeleportBinding& binding) const;

        uint32 DestinationCount() const { return m_destinations.Size(); }
        uint32 BindingCount() const { return m_bindings.Size(); }

    private:
        WorldRouting();

        DestinationRegistry m_destinations;
        DestinationResolver m_resolver;
        TeleportBindingManager m_bindings;
        TeleportExecutor m_executor;
    };
}

#endif
