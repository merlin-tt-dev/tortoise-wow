#ifndef MOD_WORLDOVERLAY_DESTINATION_RESOLVER_H
#define MOD_WORLDOVERLAY_DESTINATION_RESOLVER_H

#include "routing/WorldRoutingTypes.h"

#include <string>

namespace WorldOverlay
{
    class DestinationRegistry;

    class DestinationResolver
    {
    public:
        explicit DestinationResolver(DestinationRegistry const& registry) : m_registry(registry) {}

        bool Resolve(std::string const& destinationKey, ResolvedDestination& destination, std::string& error) const;

    private:
        DestinationRegistry const& m_registry;
    };
}

#endif
