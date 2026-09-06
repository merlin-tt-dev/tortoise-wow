#ifndef MOD_WORLDOVERLAY_TELEPORT_BINDING_MANAGER_H
#define MOD_WORLDOVERLAY_TELEPORT_BINDING_MANAGER_H

#include "routing/WorldRoutingTypes.h"

#include <map>
#include <mutex>
#include <utility>

namespace WorldOverlay
{
    class TeleportBindingManager
    {
    public:
        void Load();
        bool Find(TeleportBindingSourceType sourceType, uint32 sourceEntry, TeleportBinding& binding) const;
        uint32 Size() const;

    private:
        using BindingKey = std::pair<uint8, uint32>;

        mutable std::mutex m_mutex;
        std::map<BindingKey, TeleportBinding> m_bindings;
    };
}

#endif
