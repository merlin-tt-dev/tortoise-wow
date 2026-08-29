
#include "playerbot/playerbot.h"
#include "ShareQuestAction.h"

using namespace ai;

bool ShareQuestAction::Execute(Event& event)
{
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
    std::string link = event.getParam();

    if (!requester)
        return false;

    PlayerbotChatHandler handler(requester);
    uint32 entry = handler.extractQuestId(link);
    if (!entry)
        return false;

    Quest const* quest = sObjectMgr.GetQuestTemplate(entry);
    if (!quest)
        return false;

    if (!bot->IsCurrentQuest(entry))
        return false;

    WorldPacket p;
    p << entry;
    bot->GetSession()->HandlePushQuestToParty(p);
    ai->TellPlayer(requester, "Quest shared", PlayerbotSecurityLevel::PLAYERBOT_SECURITY_ALLOW_ALL, false);
    return true;
}

bool AutoShareQuestAction::Execute(Event& event)
{
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
    bool shared = false;

    for (uint32 logQuest : ai->GetAllCurrentQuestIds())
    {
        Quest const* quest = sObjectMgr.GetQuestTemplate(logQuest);

        if (!quest)
            continue;

        bool partyNeedsQuest = false;

        for (GroupReference* itr = bot->GetGroup()->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* player = itr->getSource();

            if (!player || player == bot || !player->IsInWorld() || !ai->IsSafe(player))         // skip self
                continue;

            if (bot->GetDistance(player) > 10)
                continue;

            if (!player->SatisfyQuestStatus(quest, false))
                continue;

            if (player->GetQuestStatus(logQuest) == QUEST_STATUS_COMPLETE)
                continue;

            if (!player->CanTakeQuest(quest, false))
                continue;

            if (!player->SatisfyQuestLog(false))
                continue;

            if (player->GetDividerGuid())
                continue;

            if (GetPlayerbotAI(player))
            {
                if (PAI_VALUE(uint8, "free quest log slots") < 15 || !urand(0,5))
                {
                    WorldPacket packet(CMSG_PUSHQUESTTOPARTY, 20);
                    packet << logQuest;
                    GetPlayerbotAI(player)->HandleMasterIncomingPacket(packet);
                }
            }
            else
                partyNeedsQuest = true;
        }

        if (!partyNeedsQuest)
            continue;

        WorldPacket p;
        p << logQuest;
        bot->GetSession()->HandlePushQuestToParty(p);
        ai->TellPlayer(requester, "Quest shared", PlayerbotSecurityLevel::PLAYERBOT_SECURITY_ALLOW_ALL, false);
        shared = true;
    }

    return shared;
}
