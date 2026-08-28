#pragma once
#include "playerbot/strategy/Value.h"
#include "playerbot/AiFactory.h"

namespace ai
{
    class LfgProposalValue : public ManualSetValue<uint32>
    {
    public:
        LfgProposalValue(PlayerbotAI* ai) : ManualSetValue<uint32>(ai, 0, "lfg proposal") {}
    };

    class LfgJoinTimeValue : public ManualSetValue<time_t>
    {
    public:
        LfgJoinTimeValue(PlayerbotAI* ai) : ManualSetValue<time_t>(ai, 0, "lfg join time") {}
    };

    class LfgAreaValue : public ManualSetValue<uint32>
    {
    public:
        LfgAreaValue(PlayerbotAI* ai) : ManualSetValue<uint32>(ai, 0, "lfg area") {}
    };

    class BotRolesValue : public Uint8CalculatedValue, public Qualified
    {
    public:
        BotRolesValue(PlayerbotAI* ai, std::string name = "bot roles") : Uint8CalculatedValue(ai, name, 10), Qualified() {}
        virtual uint8 Calculate() override
        {
            return AiFactory::GetPlayerRoles(bot);
        }
    };
}
