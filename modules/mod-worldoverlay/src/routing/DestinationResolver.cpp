#include "routing/DestinationResolver.h"

#include "routing/DestinationRegistry.h"

namespace WorldOverlay
{
    bool DestinationResolver::Resolve(std::string const& destinationKey, ResolvedDestination& destination, std::string& error) const
    {
        DestinationDefinition definition;
        if (!m_registry.Find(destinationKey, definition))
        {
            error = "unknown or disabled destination '" + destinationKey + "'";
            return false;
        }

        switch (definition.resolverType)
        {
            case DestinationResolverType::Own:
                destination.key = definition.key;
                destination.mapId = definition.mapId;
                destination.x = definition.x;
                destination.y = definition.y;
                destination.z = definition.z;
                destination.o = definition.o;
                destination.instancePolicy = definition.instancePolicy;
                destination.overlayKey = definition.overlayKey;
                return true;
            case DestinationResolverType::SpellTargetPosition:
                error = "destination '" + destinationKey + "' uses reserved SPELL_TARGET_POSITION resolution, not implemented in Phase 0";
                return false;
            case DestinationResolverType::AreaTriggerTeleport:
                error = "destination '" + destinationKey + "' uses reserved AREATRIGGER_TELEPORT resolution, not implemented in Phase 0";
                return false;
        }

        error = "destination '" + destinationKey + "' has an unsupported resolver type";
        return false;
    }
}
