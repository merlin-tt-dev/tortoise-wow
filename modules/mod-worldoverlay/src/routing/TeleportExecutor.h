#ifndef MOD_WORLDOVERLAY_TELEPORT_EXECUTOR_H
#define MOD_WORLDOVERLAY_TELEPORT_EXECUTOR_H

#include "routing/WorldRoutingTypes.h"

#include <string>

class Player;

namespace WorldOverlay
{
    class TeleportExecutor
    {
    public:
        bool Execute(Player* player, ResolvedDestination const& destination, std::string& error) const;
    };
}

#endif
