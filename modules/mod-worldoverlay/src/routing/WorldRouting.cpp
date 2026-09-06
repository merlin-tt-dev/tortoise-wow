#include "routing/WorldRouting.h"

namespace WorldOverlay
{
    WorldRouting::WorldRouting() : m_resolver(m_destinations)
    {
    }

    WorldRouting& WorldRouting::Instance()
    {
        static WorldRouting instance;
        return instance;
    }

    void WorldRouting::Load()
    {
        m_destinations.Load();
        m_bindings.Load();
    }

    bool WorldRouting::ResolveDestination(std::string const& destinationKey, ResolvedDestination& destination, std::string& error) const
    {
        return m_resolver.Resolve(destinationKey, destination, error);
    }

    bool WorldRouting::TeleportToDestination(Player* player, std::string const& destinationKey, std::string& error) const
    {
        ResolvedDestination destination;
        if (!m_resolver.Resolve(destinationKey, destination, error))
            return false;

        return m_executor.Execute(player, destination, error);
    }

    bool WorldRouting::TeleportToOverlayDestination(Player* player, std::string const& overlayKey,
        std::string const& requestedDestinationKey, std::string& resolvedDestinationKey, std::string& error) const
    {
        DestinationDefinition definition;
        if (requestedDestinationKey.empty())
        {
            if (!m_destinations.FindDefaultOverlayDestination(overlayKey, definition))
            {
                error = "overlay '" + overlayKey + "' has no enabled OVERLAY destination";
                return false;
            }
        }
        else
        {
            if (!m_destinations.Find(requestedDestinationKey, definition))
            {
                error = "unknown or disabled destination '" + requestedDestinationKey + "'";
                return false;
            }

            if (definition.instancePolicy != DestinationInstancePolicy::Overlay || definition.overlayKey != overlayKey)
            {
                error = "destination '" + requestedDestinationKey + "' does not belong to overlay '" + overlayKey + "'";
                return false;
            }
        }

        resolvedDestinationKey = definition.key;
        return TeleportToDestination(player, definition.key, error);
    }

    bool WorldRouting::FindBinding(TeleportBindingSourceType sourceType, uint32 sourceEntry, TeleportBinding& binding) const
    {
        return m_bindings.Find(sourceType, sourceEntry, binding);
    }
}
