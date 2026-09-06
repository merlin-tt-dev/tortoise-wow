#ifndef MOD_WORLDOVERLAY_WORLD_ROUTING_TYPES_H
#define MOD_WORLDOVERLAY_WORLD_ROUTING_TYPES_H

#include "Common.h"

#include <string>

namespace WorldOverlay
{
    enum class DestinationResolverType : uint8
    {
        Own = 0,
        SpellTargetPosition = 1,
        AreaTriggerTeleport = 2
    };

    enum class DestinationInstancePolicy : uint8
    {
        Auto = 0,
        Current = 1,
        Overlay = 2,
        ExplicitInstance = 3 // Reserved for admin/debug use; not implemented in Phase 0.
    };

    struct DestinationDefinition
    {
        uint32 destinationId = 0;
        std::string key;
        DestinationResolverType resolverType = DestinationResolverType::Own;
        uint32 resolverRef = 0;
        uint32 mapId = 0;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float o = 0.0f;
        DestinationInstancePolicy instancePolicy = DestinationInstancePolicy::Auto;
        std::string overlayKey;
    };

    struct ResolvedDestination
    {
        std::string key;
        uint32 mapId = 0;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float o = 0.0f;
        DestinationInstancePolicy instancePolicy = DestinationInstancePolicy::Auto;
        std::string overlayKey;
        uint32 explicitInstanceId = 0;
    };

    enum class TeleportBindingSourceType : uint8
    {
        Item = 1,
        GameObject = 2,
        Creature = 3,
        Trigger = 4,
        Command = 5,
        GossipAction = 6,
        Script = 7
    };

    struct TeleportBinding
    {
        TeleportBindingSourceType sourceType = TeleportBindingSourceType::Item;
        uint32 sourceEntry = 0;
        std::string destinationKey;
        uint32 cooldownMs = 0;
        uint32 flags = 0;
    };
}

#endif
