#include "routing/DestinationRegistry.h"

#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "MapManager.h"

namespace WorldOverlay
{
    void DestinationRegistry::Load()
    {
        std::map<std::string, DestinationDefinition> loaded;

        QueryResult* result = WorldDatabase.Query(
            "SELECT destination_id, destination_key, resolver_type, resolver_ref, map_id, "
            "position_x, position_y, position_z, orientation, instance_policy, overlay_key "
            "FROM worldoverlay_destination WHERE enabled = 1 ORDER BY destination_id");

        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                DestinationDefinition destination;
                destination.destinationId = fields[0].GetUInt32();
                destination.key = fields[1].GetCppString();
                destination.resolverType = static_cast<DestinationResolverType>(fields[2].GetUInt8());
                destination.resolverRef = fields[3].GetUInt32();
                destination.mapId = fields[4].GetUInt32();
                destination.x = fields[5].GetFloat();
                destination.y = fields[6].GetFloat();
                destination.z = fields[7].GetFloat();
                destination.o = fields[8].GetFloat();
                destination.instancePolicy = static_cast<DestinationInstancePolicy>(fields[9].GetUInt8());
                destination.overlayKey = fields[10].GetCppString();

                if (destination.key.empty())
                {
                    sLog.outError("[WorldRouting] Ignoring destination %u with empty destination_key.", destination.destinationId);
                    continue;
                }

                uint8 const resolverType = static_cast<uint8>(destination.resolverType);
                if (resolverType > static_cast<uint8>(DestinationResolverType::AreaTriggerTeleport))
                {
                    sLog.outError("[WorldRouting] Destination '%s' has unknown resolver_type %u.",
                        destination.key.c_str(), resolverType);
                    continue;
                }

                uint8 const instancePolicy = static_cast<uint8>(destination.instancePolicy);
                if (instancePolicy > static_cast<uint8>(DestinationInstancePolicy::ExplicitInstance))
                {
                    sLog.outError("[WorldRouting] Destination '%s' has unknown instance_policy %u.",
                        destination.key.c_str(), instancePolicy);
                    continue;
                }

                if (destination.resolverType == DestinationResolverType::Own &&
                    !MapManager::IsValidMapCoord(destination.mapId, destination.x, destination.y, destination.z, destination.o))
                {
                    sLog.outError("[WorldRouting] Destination '%s' has invalid OWN coordinates.", destination.key.c_str());
                    continue;
                }

                if (destination.instancePolicy == DestinationInstancePolicy::Overlay && destination.overlayKey.empty())
                {
                    sLog.outError("[WorldRouting] OVERLAY destination '%s' has no overlay_key.", destination.key.c_str());
                    continue;
                }

                loaded[destination.key] = destination;
            }
            while (result->NextRow());

            delete result;
        }

        {
            std::lock_guard<std::mutex> guard(m_mutex);
            m_destinations.swap(loaded);
        }

        sLog.outString("[WorldRouting] Loaded %u destination(s).", Size());
    }

    bool DestinationRegistry::Find(std::string const& destinationKey, DestinationDefinition& destination) const
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        auto const itr = m_destinations.find(destinationKey);
        if (itr == m_destinations.end())
            return false;

        destination = itr->second;
        return true;
    }

    bool DestinationRegistry::FindDefaultOverlayDestination(std::string const& overlayKey, DestinationDefinition& destination) const
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        bool found = false;
        for (auto const& pair : m_destinations)
        {
            DestinationDefinition const& candidate = pair.second;
            if (candidate.instancePolicy != DestinationInstancePolicy::Overlay || candidate.overlayKey != overlayKey)
                continue;

            if (!found || candidate.destinationId < destination.destinationId)
            {
                destination = candidate;
                found = true;
            }
        }

        return found;
    }

    uint32 DestinationRegistry::Size() const
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        return uint32(m_destinations.size());
    }
}
