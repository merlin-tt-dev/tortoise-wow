#include "overlay/RuntimeInstanceRegistry.h"

#include "Log.h"
#include "Map.h"
#include "MapManager.h"

namespace WorldOverlay
{
    bool RuntimeInstanceRegistry::GetActive(std::string const& overlayKey, RuntimeDefinition& runtime)
    {
        RuntimeDefinition candidate;
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            auto const itr = m_runtimes.find(overlayKey);
            if (itr == m_runtimes.end())
                return false;
            candidate = itr->second;
        }

        Map* map = sMapMgr.FindMap(candidate.mapId, candidate.instanceId);
        if (map && map->IsDungeon())
        {
            runtime = candidate;
            return true;
        }

        std::lock_guard<std::mutex> guard(m_mutex);
        auto const itr = m_runtimes.find(overlayKey);
        if (itr != m_runtimes.end() && itr->second.mapId == candidate.mapId && itr->second.instanceId == candidate.instanceId)
            m_runtimes.erase(itr);
        return false;
    }

    bool RuntimeInstanceRegistry::FindByLocation(uint32 mapId, uint32 instanceId, std::string& overlayKey, RuntimeDefinition& runtime)
    {
        std::string candidateKey;
        RuntimeDefinition candidate;
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            for (auto const& pair : m_runtimes)
            {
                if (pair.second.mapId == mapId && pair.second.instanceId == instanceId)
                {
                    candidateKey = pair.first;
                    candidate = pair.second;
                    break;
                }
            }
        }

        if (candidateKey.empty())
            return false;

        Map* map = sMapMgr.FindMap(candidate.mapId, candidate.instanceId);
        if (map && map->IsDungeon())
        {
            overlayKey = candidateKey;
            runtime = candidate;
            return true;
        }

        std::lock_guard<std::mutex> guard(m_mutex);
        auto const itr = m_runtimes.find(candidateKey);
        if (itr != m_runtimes.end() &&
            itr->second.mapId == candidate.mapId && itr->second.instanceId == candidate.instanceId)
            m_runtimes.erase(itr);

        return false;
    }

    std::vector<std::pair<std::string, RuntimeDefinition>> RuntimeInstanceRegistry::Snapshot() const
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        return { m_runtimes.begin(), m_runtimes.end() };
    }

    void RuntimeInstanceRegistry::Store(std::string const& overlayKey, RuntimeDefinition const& runtime)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_runtimes[overlayKey] = runtime;
    }

    void RuntimeInstanceRegistry::Erase(std::string const& overlayKey)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_runtimes.erase(overlayKey);
    }

    void RuntimeInstanceRegistry::OnMapDestroyed(Map const* map)
    {
        if (!map)
            return;

        std::lock_guard<std::mutex> guard(m_mutex);
        for (auto itr = m_runtimes.begin(); itr != m_runtimes.end();)
        {
            if (itr->second.mapId == map->GetId() && itr->second.instanceId == map->GetInstanceId())
            {
                sLog.outString("[WorldOverlay] Runtime '%s' released map %u instance %u.",
                    itr->first.c_str(), itr->second.mapId, itr->second.instanceId);
                itr = m_runtimes.erase(itr);
            }
            else
                ++itr;
        }
    }

    void RuntimeInstanceRegistry::Clear()
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_runtimes.clear();
    }

    bool RuntimeInstanceRegistry::HasActiveRuntimes()
    {
        for (auto const& pair : Snapshot())
        {
            if (sMapMgr.FindMap(pair.second.mapId, pair.second.instanceId))
                return true;

            std::lock_guard<std::mutex> guard(m_mutex);
            auto const itr = m_runtimes.find(pair.first);
            if (itr != m_runtimes.end() && itr->second.mapId == pair.second.mapId && itr->second.instanceId == pair.second.instanceId)
                m_runtimes.erase(itr);
        }

        return false;
    }
}
