#include "routing/TeleportBindingManager.h"

#include "Database/DatabaseEnv.h"
#include "Log.h"

namespace WorldOverlay
{
    void TeleportBindingManager::Load()
    {
        std::map<BindingKey, TeleportBinding> loaded;

        QueryResult* result = WorldDatabase.Query(
            "SELECT source_type, source_entry, destination_key, cooldown_ms, flags "
            "FROM worldoverlay_teleport_binding WHERE enabled = 1 ORDER BY source_type, source_entry");

        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                uint8 const sourceType = fields[0].GetUInt8();
                TeleportBinding binding;
                binding.sourceType = static_cast<TeleportBindingSourceType>(sourceType);
                binding.sourceEntry = fields[1].GetUInt32();
                binding.destinationKey = fields[2].GetCppString();
                binding.cooldownMs = fields[3].GetUInt32();
                binding.flags = fields[4].GetUInt32();

                if (sourceType < static_cast<uint8>(TeleportBindingSourceType::Item) ||
                    sourceType > static_cast<uint8>(TeleportBindingSourceType::Script) ||
                    !binding.sourceEntry || binding.destinationKey.empty())
                {
                    sLog.outError("[WorldRouting] Ignoring invalid teleport binding source_type=%u source_entry=%u.",
                        sourceType, binding.sourceEntry);
                    continue;
                }

                loaded[{ sourceType, binding.sourceEntry }] = binding;
            }
            while (result->NextRow());

            delete result;
        }

        {
            std::lock_guard<std::mutex> guard(m_mutex);
            m_bindings.swap(loaded);
        }

        sLog.outString("[WorldRouting] Loaded %u teleport binding(s).", Size());
    }

    bool TeleportBindingManager::Find(TeleportBindingSourceType sourceType, uint32 sourceEntry, TeleportBinding& binding) const
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        auto const itr = m_bindings.find({ static_cast<uint8>(sourceType), sourceEntry });
        if (itr == m_bindings.end())
            return false;

        binding = itr->second;
        return true;
    }

    uint32 TeleportBindingManager::Size() const
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        return uint32(m_bindings.size());
    }
}
