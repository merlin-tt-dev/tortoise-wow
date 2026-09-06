#ifndef MOD_WORLDOVERLAY_DESTINATION_REGISTRY_H
#define MOD_WORLDOVERLAY_DESTINATION_REGISTRY_H

#include "routing/WorldRoutingTypes.h"

#include <map>
#include <mutex>
#include <string>

namespace WorldOverlay
{
    class DestinationRegistry
    {
    public:
        void Load();
        bool Find(std::string const& destinationKey, DestinationDefinition& destination) const;
        bool FindDefaultOverlayDestination(std::string const& overlayKey, DestinationDefinition& destination) const;
        uint32 Size() const;

    private:
        mutable std::mutex m_mutex;
        std::map<std::string, DestinationDefinition> m_destinations;
    };
}

#endif
