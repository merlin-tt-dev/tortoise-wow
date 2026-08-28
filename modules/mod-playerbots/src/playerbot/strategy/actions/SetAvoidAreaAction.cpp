
#include "playerbot/playerbot.h"
#include "SetAvoidAreaAction.h"
#include "playerbot/ServerFacade.h"

#include "playerbot/strategy/values/PositionValue.h"
#include "Maps/PathFinder.h"
using namespace ai;


bool SetAvoidAreaAction::Execute(Event& event)
{
    PathFinder pathfinder(bot->GetMapId());

    SET_AI_VALUE2(PositionEntry, "pos", "last avoid", PositionEntry(bot));

    std::list<ObjectGuid> targets = AI_VALUE_LAZY(std::list<ObjectGuid>, "possible targets");

    if (targets.empty())
        return false;

    FactionTemplateEntry const* humanFaction = sObjectMgr.GetFactionTemplateEntry(1);
    FactionTemplateEntry const* orcFaction = sObjectMgr.GetFactionTemplateEntry(2);

    for (auto& target : targets)
    {
        CreatureInfo const* cInfo = sObjectMgr.GetCreatureTemplate(target.GetEntry());

        if (!cInfo)
            continue;

        if (cInfo->npc_flags > 0) //Ignore npcs.
            continue;

        if (cInfo->level_max < bot->GetLevel() - 3) //Ignore lower level mobs.
            continue;

        FactionTemplateEntry const* factionEntry = sObjectMgr.GetFactionTemplateEntry(cInfo->faction);
        ReputationRank reactionHum = PlayerbotAI::GetFactionReaction(humanFaction, factionEntry);
        ReputationRank reactionOrc = PlayerbotAI::GetFactionReaction(orcFaction, factionEntry);

        if (reactionHum >= REP_NEUTRAL || reactionOrc >= REP_NEUTRAL) //ignore mobs friendly to opposing faction (they share avoid maps)
            continue;

        Unit* targetUnit = ai->GetUnit(target);

        if (!targetUnit)
            continue;

        WorldPosition point(targetUnit);
        pathfinder.MarkNavArea(point.getX(), point.getY(), point.getZ(), PLAYERBOT_MMAP_AREA_AVOID, sServerFacade.GetAggroDistance(targetUnit, bot) * 2.5f);
        pathfinder.MarkNavArea(point.getX(), point.getY(), point.getZ(), PLAYERBOT_MMAP_AREA_DANGER, sServerFacade.GetAggroDistance(targetUnit, bot));
    }

    return true;
}

bool SetAvoidAreaAction::isUseful()
{
    if (bot->GetInstanceId())
        return false;

    PositionEntry p = AI_VALUE2(PositionEntry, "pos", "last avoid");

    if (!p.isSet())
        return true;

    return p.Get().distance(bot) > 20.0f;
}