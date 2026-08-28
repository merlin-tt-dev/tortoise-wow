#include "playerbot/playerbot.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "PlayerbotDbStore.h"
#include "playerbot/PlayerbotFactory.h"
#include "playerbot/RandomPlayerbotFactory.h"
#include "playerbot/RandomPlayerbotMgr.h"
#include "playerbot/ServerFacade.h"
#include "playerbot/TravelMgr.h"
#include "playerbot/PlayerbotLoginMgr.h"
#include "BotDiagnostics.h"
#include "BotActionLog.h"
#include "Chat/ChannelMgr.h"
#include "SocialMgr.h"
#include "AccountMgr.h"
#include "strategy/actions/ChangeTalentsAction.h"
#include "strategy/actions/InviteToGroupAction.h"
#include "AiFactory.h"
#include "Guild/GuildMgr.h"
#include "World.h"
#include "Database/DatabaseImpl.h"
#include "ObjectMgr.h"

#ifdef GenerateBotTests
#include "strategy/tests/TestAction.h"
#include "strategy/tests/TestRegistry.h"
#endif

class CharacterHandler;

// real bot login flow.
//
// AddPlayerBot creates a synthetic WorldSession for the bot if needed, queues the bot's
// character data load via the standard Penqle CharacterDatabase pipeline, and routes the
// callback to HandlePlayerBotLoginCallback below. After the holder fires, we hand it off
// to WorldSession::HandlePlayerLogin (the same path real players use), then attach the
// PlayerbotAI via OnBotLogin.
//
// PlayerbotLoginQueryHolder is defined in PlayerbotLoginMgr.cpp (file-local class). Since
// it's not exported, we use Penqle's plain LoginQueryHolder here — the bot doesn't need
// the cmangos extra fields (masterAccountId stored on the holder is convenient but
// non-essential; we already have it via the m_pendingBotLogins map below).

// Pending logins: maps holder pointer to (botGuid, masterAccountId) so the callback can
// resolve which bot is being added. Cleared on callback completion.
namespace {
    struct PendingBotLogin {
        ObjectGuid botGuid;
        uint32 masterAccountId;
    };
    std::map<SqlQueryHolder*, PendingBotLogin> m_pendingBotLogins;
}

void PlayerbotHolder::AddPlayerBot(uint32 guidLow, uint32 masterAccountId)
{
    if (!sPlayerbotAIConfig.enabled)
        return;

    ObjectGuid botGuid(HIGHGUID_PLAYER, guidLow);

    // Already loaded? Attach idempotently.
    if (Player* existing = ObjectAccessor::FindPlayerNotInWorld(botGuid))
    {
        if (existing->IsInWorld())
        {
            OnBotLogin(existing);
            return;
        }

        // Preserve the existing ghost recovery behavior rather than launching a
        // second login for a Player that is still registered by the core.
        Player* ghost = HashMapHolder<Player>::Find(botGuid);
        if (ghost && ghost->IsBeingTeleportedFar())
            ghost->GetSession()->HandleMoveWorldportAckOpcode();
        else if (ghost && ghost->IsBeingTeleportedNear())
        {
            WorldPacket packet(MSG_MOVE_TELEPORT_ACK, 16);
            packet << ghost->GetObjectGuid() << uint32(0) << uint32(time(nullptr));
            ghost->GetSession()->HandleMoveTeleportAckOpcode(packet);
        }

        if (ghost && ghost->IsInWorld())
        {
            OnBotLogin(ghost);
            return;
        }

        sLog.outError("[PlayerBots] AddPlayerBot: guid %u is already registered but not in world; refusing duplicate login", guidLow);
        return;
    }

    if (!BeginPlayerbotLogin(this, guidLow, masterAccountId))
        sLog.outError("[PlayerBots] AddPlayerBot: native Penqle login bootstrap failed for guid %u", guidLow);
}

PlayerbotHolder::PlayerbotHolder() : PlayerbotAIBase()
{
    RegisterPlayerbotHolder(this);
    m_holderHandlers["list"] = &PlayerbotHolder::HandleList;
    m_holderHandlers["help"] = &PlayerbotHolder::HandleHelp;
    m_holderHandlers["reload"] = &PlayerbotHolder::HandleReload;
    m_holderHandlers["tweak"] = &PlayerbotHolder::HandleTweak;
    m_holderHandlers["self"] = &PlayerbotHolder::HandleSelf;
    m_holderHandlers["spoof"] = &PlayerbotHolder::HandleSpoof;
    m_holderHandlers["p"] = &PlayerbotHolder::HandleParty;
    m_holderHandlers["g"] = &PlayerbotHolder::HandleGuild;
    m_holderHandlers["r"] = &PlayerbotHolder::HandleRaid;
    m_holderHandlers["rl"] = &PlayerbotHolder::HandleRaidLeader;
    m_holderHandlers["create"] = &PlayerbotHolder::HandleCreate;
    m_holderHandlers["group"] = &PlayerbotHolder::HandleGroup;
#ifdef GenerateBotTests
    m_holderHandlers["runtest"] = &PlayerbotHolder::HandleRunTest;
#endif

    m_botCommandHandlers["add"] = &PlayerbotHolder::HandleBotAddLogin;
    m_botCommandHandlers["login"] = &PlayerbotHolder::HandleBotAddLogin;
    m_botCommandHandlers["remove"] = &PlayerbotHolder::HandleBotRemoveLogout;
    m_botCommandHandlers["logout"] = &PlayerbotHolder::HandleBotRemoveLogout;
    m_botCommandHandlers["rm"] = &PlayerbotHolder::HandleBotRemoveLogout;
    m_botCommandHandlers["delete"] = &PlayerbotHolder::HandleBotDelete;
    m_botCommandHandlers["gear"] = &PlayerbotHolder::HandleBotGear;
    m_botCommandHandlers["equip"] = &PlayerbotHolder::HandleBotGear;
    m_botCommandHandlers["train"] = &PlayerbotHolder::HandleBotTrainLearn;
    m_botCommandHandlers["learn"] = &PlayerbotHolder::HandleBotTrainLearn;
    m_botCommandHandlers["food"] = &PlayerbotHolder::HandleBotFoodDrink;
    m_botCommandHandlers["drink"] = &PlayerbotHolder::HandleBotFoodDrink;
    m_botCommandHandlers["potions"] = &PlayerbotHolder::HandleBotPotions;
    m_botCommandHandlers["pots"] = &PlayerbotHolder::HandleBotPotions;
    m_botCommandHandlers["consumes"] = &PlayerbotHolder::HandleBotConsumes;
    m_botCommandHandlers["consumables"] = &PlayerbotHolder::HandleBotConsumes;
    m_botCommandHandlers["consums"] = &PlayerbotHolder::HandleBotConsumes;
    m_botCommandHandlers["regs"] = &PlayerbotHolder::HandleBotReagents;
    m_botCommandHandlers["reg"] = &PlayerbotHolder::HandleBotReagents;
    m_botCommandHandlers["reagents"] = &PlayerbotHolder::HandleBotReagents;
    m_botCommandHandlers["prepare"] = &PlayerbotHolder::HandleBotPrepare;
    m_botCommandHandlers["prep"] = &PlayerbotHolder::HandleBotPrepare;
    m_botCommandHandlers["init"] = &PlayerbotHolder::HandleBotInit;
    m_botCommandHandlers["enchants"] = &PlayerbotHolder::HandleBotEnchants;
    m_botCommandHandlers["ammo"] = &PlayerbotHolder::HandleBotAmmo;
    m_botCommandHandlers["pet"] = &PlayerbotHolder::HandleBotPet;
    m_botCommandHandlers["levelup"] = &PlayerbotHolder::HandleBotLevelUp;
    m_botCommandHandlers["level"] = &PlayerbotHolder::HandleBotLevelUp;
    m_botCommandHandlers["random"] = &PlayerbotHolder::HandleBotRandom;
    m_botCommandHandlers["summon"] = &PlayerbotHolder::HandleBotSummon;
    m_botCommandHandlers["recall"] = &PlayerbotHolder::HandleBotSummon;
    m_botCommandHandlers["come"]   = &PlayerbotHolder::HandleBotSummon;

    m_botCommandHandlers["always"] = &PlayerbotHolder::HandleBotAlways;
    m_botCommandHandlers["debug"] = &PlayerbotHolder::HandleBotDebug;
    m_botCommandHandlers["c"] = &PlayerbotHolder::HandleBotC;
    m_botCommandHandlers["w"] = &PlayerbotHolder::HandleConsoleWhisper;
    m_botCommandHandlers["cmd"] = &PlayerbotHolder::HandleConsoleCmd;
    m_botCommandHandlers["test"] = &PlayerbotHolder::HandleBotTest;
    m_botCommandHandlers["do"] = &PlayerbotHolder::HandleBotDo;
    m_botCommandHandlers["record"] = &PlayerbotHolder::HandleBotRecord;
    m_botCommandHandlers["read"] = &PlayerbotHolder::HandleBotRead;
    m_botCommandHandlers["clear"] = &PlayerbotHolder::HandleBotClear;

    for (uint32 spellId = 0; spellId < sServerFacade.GetSpellInfoRows(); spellId++)
    {
        sServerFacade.LookupSpellInfo(spellId);
    }
}

PlayerbotHolder::~PlayerbotHolder()
{
    UnregisterPlayerbotHolder(this);
}

void PlayerbotHolder::ForEachPlayerbot(std::function<void(Player*)> callback) const
{
    for (auto& itr : playerBots)
    {
        Player* bot = itr.second;
        if (bot)
        {
            callback(bot);
        }
    }
}

void PlayerbotHolder::MovePlayerBot(uint32 guid, PlayerbotHolder* newHolder)
{
    if (newHolder)
    {
        auto it = playerBots.find(guid); 
        if (it != playerBots.end() && it->second != nullptr)
        {
            newHolder->OnBotLogin(it->second);
            playerBots[guid] = nullptr;
        }
    }
}

void PlayerbotHolder::UpdateAIInternal(uint32 elapsed, bool minimal)
{
#ifdef GenerateBotTests
    UpdatePendingTests(elapsed);
#endif
}

void PlayerbotHolder::UpdateSessions(uint32 elapsed)
{
    ForEachPlayerbot([&](Player* bot)
    {
        // Per-iteration diagnostic snapshot. We only emit it for "interesting"
        // states (mid-teleport, ghost, logout-pending) to keep log volume sane —
        // a healthy in-world bot looks identical every tick. If a bot is stuck
        // mid-teleport, this line will fire every master tick and we'll be able
        // to see exactly how long it's been stuck.
        const bool isMidTeleport = bot->IsBeingTeleported();
        const bool isInWorld     = bot->IsInWorld();
        const bool isLoading     = bot->GetSession() && bot->GetSession()->PlayerLoading();
        // True ghost: not in world, not mid-teleport, not mid-login. The login
        // path can briefly land in (!IsInWorld && !IsBeingTeleported) before
        // SetMap finishes; we must not yank such a bot to homebind.
        const bool isGhost       = !isInWorld && !isMidTeleport && !isLoading;
        const bool wantsLogout   = GetPlayerbotAI(bot) && GetPlayerbotAI(bot)->GetShouldLogOut();
        if (isMidTeleport || isGhost || wantsLogout)
        {
            SC_LOG("UpdateSessions iter bot=%s guid=%u midTeleport=%d inWorld=%d "
                   "ghost=%d wantsLogout=%d isBeingTeleportedFar=%d isBeingTeleportedNear=%d",
                   bot->GetName(), bot->GetGUIDLow(),
                   (int)isMidTeleport, (int)isInWorld, (int)isGhost, (int)wantsLogout,
                   (int)bot->IsBeingTeleportedFar(),
                   (int)bot->IsBeingTeleportedNear());
        }

        if (GetPlayerbotAI(bot) && bot->IsBeingTeleported())
        {
            GetPlayerbotAI(bot)->HandleTeleportAck();
        }
        else if (bot->IsInWorld())
        {
            HandlePlayerbotPackets(bot);
        }
        else
        {
            // Underlying root-cause fix candidate (ghost branch). Bot has no
            // teleport flag but is also not in world. This is the limbo state
            // that produces "Trying to login already ingame" on the next
            // .bot add. Most likely cause: HandleMoveWorldportAckOpcode's
            // map->Add returned false and HandleReturnOnTeleportFail's
            // chained TeleportTo also failed — leaving the Player removed
            // from old map but never added to any new map. SemaphoreTeleportFar
            // was reset by HandleReturnOnTeleportFail so we can no longer
            // re-drive the ACK.
            //
            // Recovery: kick the bot to homebind synchronously so it lands
            // *somewhere* in-world. Better than ghost-state forever.
            if (GetPlayerbotAI(bot))
            {
                SC_LOG("UpdateSessions GHOST RECOVERY bot=%s guid=%u — driving "
                       "TeleportToHomebind to break out of limbo",
                       bot->GetName(), bot->GetGUIDLow());
                bot->TeleportToHomebind();
            }
        }

        if (GetPlayerbotAI(bot) && GetPlayerbotAI(bot)->GetShouldLogOut() && !bot->GetSession()->isLogingOut())
        {
            LogoutPlayerBot(bot->GetObjectGuid().GetRawValue());
        }
    });

    Cleanup();
}

void PlayerbotHolder::Cleanup()
{
    auto it = playerBots.begin();
    while (it != playerBots.end())
    {
        if (it->second == nullptr)
        {
            it = playerBots.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void PlayerbotHolder::LogoutAllBots()
{
    int total = 0, kept = 0, skippedRealPlayer = 0, skippedNoAI = 0, loggedOut = 0;
    ForEachPlayerbot([&](Player* bot)
    {
        ++total;
        const bool hasAI    = GetPlayerbotAI(bot) != nullptr;
        const bool realPlay = hasAI && GetPlayerbotAI(bot)->IsRealPlayer();
        if (!hasAI)        { ++skippedNoAI; return; }
        if (realPlay)      { ++skippedRealPlayer; return; }
        SC_LOG("LogoutAllBots: logging out bot=%s guid=%u (remote=%s)",
               bot->GetName(), bot->GetGUIDLow(),
               bot->GetSession() ? bot->GetSession()->GetRemoteAddress().c_str() : "(no-session)");
        LogoutPlayerBot(bot->GetGUIDLow());
        ++loggedOut;
    });
    SC_LOG("LogoutAllBots summary: total=%d loggedOut=%d skippedRealPlayer=%d skippedNoAI=%d",
           total, loggedOut, skippedRealPlayer, skippedNoAI);

    Cleanup();
}

void PlayerbotMgr::CancelLogout()
{
    Player* master = GetMaster();
    if (!master)
        return;

    ForEachPlayerbot([&](Player* bot)
    {
        PlayerbotAI* ai = GetPlayerbotAI(bot);
        if (ai && !ai->IsRealPlayer())
        {
            if (bot->GetSession()->isLogingOut())
            {
                WorldPacket p;
                bot->GetSession()->HandleLogoutCancelOpcode(p);
                ai->TellPlayer(GetMaster(), BOT_TEXT("logout_cancel"));
            }
        }
    });

    sRandomPlayerbotMgr.ForEachPlayerbot([&](Player* bot)
    {
        PlayerbotAI* ai = GetPlayerbotAI(bot);
        if (ai && !ai->IsRealPlayer() && ai->GetMaster() == master)
        {
            if (bot->GetSession()->isLogingOut())
            {
                WorldPacket p;
                bot->GetSession()->HandleLogoutCancelOpcode(p);
            }
        }
    });
}

void PlayerbotHolder::LogoutPlayerBot(uint32 guid, bool allowInstant, bool forDelete)
{
    SC_LOG("LogoutPlayerBot entry guid=%u allowInstant=%d forDelete=%d",
           guid, (int)allowInstant, (int)forDelete);
    Player* bot = GetPlayerBot(guid);
    if (bot)
    {
        SC_LOG("LogoutPlayerBot bot=%s found in playerBots map", bot->GetName());
        PlayerbotAI* ai = GetPlayerbotAI(bot);
        if (!ai)
        {
            SC_LOG("LogoutPlayerBot bot=%s has no AI — early return", bot->GetName());
            return;
        }

        // BotActionLog: write LIFECYCLE LOGOUT and close the per-bot log
        // file. Done early in the logout sequence so the file flushes
        // before any potentially-crashing teardown work runs.
        ai::botdiag::BotActionLog::Write(ai, "LIFECYCLE",
            "event=LOGOUT bot=%s guid=%u allowInstant=%d forDelete=%d",
            bot->GetName(), guid, (int)allowInstant, (int)forDelete);
        ai::botdiag::BotActionLog::LogState(ai, "pre-logout");
        ai::botdiag::BotActionLog::Close(ai);

        if (!sPlayerbotAIConfig.bExplicitDbStoreSave)
        {
           Group* group = bot->GetGroup();
           if (group && !bot->InBattleGround() && !bot->InBattleGroundQueue() && ai->HasActivePlayerMaster())
           {
              sPlayerbotDbStore.Save(ai);
           }
        }
        SC_LOG("LogoutPlayerBot bot=%s — about to SaveToDB", bot->GetName());
        sLog.outDebug("Bot %s logging out", bot->GetName());
        if (!forDelete)
            bot->SaveToDB();
        SC_LOG("LogoutPlayerBot bot=%s — SaveToDB done", bot->GetName());

        WorldSession* botWorldSessionPtr = bot->GetSession();
        WorldSession* masterWorldSessionPtr = nullptr;

        Player* master = ai->GetMaster();
        if (master)
            masterWorldSessionPtr = master->GetSession();

        // check for instant logout
        bool logout = botWorldSessionPtr->ShouldLogOut(time(nullptr));

        // if no instant logout, request normal logout
        if (!allowInstant)
        {
            if (bot && (bot->GetSession()->isLogingOut()))
            {
                return;
            }
            else if (bot)
            {
                SC_LOG("LogoutPlayerBot bot=%s — queueing CMSG_LOGOUT_REQUEST", bot->GetName());
                ai->TellPlayer(ai->GetMaster(), BOT_TEXT("logout_start"));

                WorldPacket p(CMSG_LOGOUT_REQUEST);
                std::unique_ptr<WorldPacket> packet(new WorldPacket(p));
                QueuePlayerbotPacket(bot, std::move(packet));
                SC_LOG("LogoutPlayerBot bot=%s — CMSG_LOGOUT_REQUEST queued, returning", bot->GetName());

                //WorldPacket p;
                //botWorldSessionPtr->HandleLogoutRequestOpcode(p);
                if (!bot)
                {
                    playerBots[guid] = nullptr;
                    delete botWorldSessionPtr;    
                }
                
                return;
            }
            else
            {
                playerBots[guid] = nullptr;  // deletes bot player ptr inside this WorldSession PlayerBotMap
                delete botWorldSessionPtr;  // finally delete the bot's WorldSession
            }
            
            return;
        } 
        // if instant logout possible, do it
        else if (bot && (logout || !botWorldSessionPtr->isLogingOut()))
        {
            ai->TellPlayer(ai->GetMaster(), BOT_TEXT("goodbye"));
            playerBots[guid] = nullptr;    // deletes bot player ptr inside this WorldSession PlayerBotMap
            botWorldSessionPtr->LogoutPlayer(true); // save and delete the bot Player object
            //botWorldSessionPtr->LogoutPlayer(true); // this will delete the bot Player object and PlayerbotAI object
            if(!sWorld.FindSession(botWorldSessionPtr->GetAccountId())) //Real player sessions will get removed later.
                delete botWorldSessionPtr;  // finally delete the bot's WorldSession
        }
    }
}

void PlayerbotHolder::DisablePlayerBot(uint32 guid, bool logOutPlayer)
{
    Player* bot = GetPlayerBot(guid);
    if (bot)
    {
        // CMaNGOS SetOffline changed the socket/session state before removing bot AI.
        // Penqle has no equivalent public state transition, and this function does
        // not log the live real-player session out; leaving that session untouched
        // preserves its group membership while only detaching module AI.
        (void)logOutPlayer;
        GetPlayerbotAI(bot)->TellPlayer(GetPlayerbotAI(bot)->GetMaster(), BOT_TEXT("goodbye"));
        GetPlayerbotAI(bot)->StopMoving();
        MotionMaster& mm = *bot->GetMotionMaster();
        mm.Clear();

        if (!sPlayerbotAIConfig.bExplicitDbStoreSave)
        {
           Group* group = bot->GetGroup();
           if (group && !bot->InBattleGround() && !bot->InBattleGroundQueue() && GetPlayerbotAI(bot)->HasActivePlayerMaster())
           {
              sPlayerbotDbStore.Save(GetPlayerbotAI(bot));
           }
        }

        sLog.outDebug("Bot %s logged out", bot->GetName());
        bot->SaveToDB();

        WorldSession* botWorldSessionPtr = bot->GetSession();
        playerBots[guid] = nullptr;    // deletes bot player ptr inside this WorldSession PlayerBotMap

        if (GetPlayerbotAI(bot))
        {
            RemovePlayerbotAI(bot);
        }
    }
}

Player* PlayerbotHolder::GetPlayerBot(uint32 playerGuid) const
{
    PlayerBotMap::const_iterator it = playerBots.find(playerGuid);
    return (it == playerBots.end()) ? nullptr : it->second ? it->second : nullptr;
}

void PlayerbotHolder::JoinChatChannels(Player* bot)
{
    // bots join World chat if not solo oriented
    if (bot->GetLevel() >= 10 && sRandomPlayerbotMgr.IsFreeBot(bot) && GetPlayerbotAI(bot) && GetPlayerbotAI(bot)->GetGrouperType() != GrouperType::SOLO)
    {
        // TODO make action/config
        // Make the bot join the world channel for chat
        WorldPacket pkt(CMSG_JOIN_CHANNEL);
#ifndef MANGOSBOT_ZERO
        pkt << uint32(0) << uint8(0) << uint8(0);
#endif
        pkt << std::string("World");
        pkt << ""; // Pass
        bot->GetSession()->HandleJoinChannelOpcode(pkt);
    }
    // join standard channels
    uint8 locale = BroadcastHelper::GetLocale();

    AreaEntry const* current_zone = GetPlayerbotAI(bot)->GetCurrentZone();
    ChannelMgr* cMgr = channelMgr(bot->GetTeam());
    std::string current_zone_name = current_zone ? GetPlayerbotAI(bot)->GetLocalizedAreaName(current_zone) : "";

    if (current_zone && cMgr)
    {
        static uint32 const channelIds[] =
        {
            ChatChannelId::GENERAL,
            ChatChannelId::TRADE,
            ChatChannelId::LOCAL_DEFENSE,
            ChatChannelId::WORLD_DEFENSE,
            ChatChannelId::LOOKING_FOR_GROUP,
            ChatChannelId::GUILD_RECRUITMENT
        };

        for (uint32 channelId : channelIds)
        {
            ChatChannelsEntry const* channel = sObjectMgr.GetChannelEntryFor(channelId);
            if (!channel || locale >= MAX_DBC_LOCALE || channel->name[locale].empty())
                continue;

            std::string const& channelPattern = channel->name[locale];
            Channel* new_channel = nullptr;
            switch (channelId)
            {
                case ChatChannelId::GENERAL:
                case ChatChannelId::LOCAL_DEFENSE:
                {
                    char new_channel_name_buf[100];
                    snprintf(new_channel_name_buf, 100, channelPattern.c_str(), current_zone_name.c_str());
                    new_channel = cMgr->GetOrCreateChannel(new_channel_name_buf);
                    break;
                }
                case ChatChannelId::TRADE:
                case ChatChannelId::GUILD_RECRUITMENT:
                {
                    char new_channel_name_buf[100];
                    //3459 is ID for a zone named "City" (only exists for the sake of using its name)
                    //Currently in magons TBC, if you switch zones, then you join "Trade - <zone>" and "GuildRecruitment - <zone>"
                    //which is a core bug, should be "Trade - City" and "GuildRecruitment - City" in both 1.12 and TBC
                    //but if you (actual player) logout in a city and log back in - you join "City" versions
                    snprintf(
                        new_channel_name_buf,
                        100,
                        channelPattern.c_str(),
                        GetPlayerbotAI(bot)->GetLocalizedAreaName(AreaEntry::GetById(ImportantAreaId::CITY)).c_str()
                    );

                    new_channel = cMgr->GetOrCreateChannel(new_channel_name_buf);
                    break;
                }
                case ChatChannelId::LOOKING_FOR_GROUP:
                case ChatChannelId::WORLD_DEFENSE:
                {
                    new_channel = cMgr->GetOrCreateChannel(channelPattern);
                    break;
                }
                default:
                    break;
            }
            if (new_channel)
                new_channel->Join(bot->GetObjectGuid(), "");
        }
    }
}

void PlayerbotHolder::OnBotLogin(Player * const bot)
{
    if (!sPlayerbotAIConfig.enabled)
        return;

    PlayerbotAI* ai = GetPlayerbotAI(bot);
    if (!ai)
    {
        CreatePlayerbotAI(bot);
        ai = GetPlayerbotAI(bot);
    }

    if(!ai->HasRealPlayerMaster())
	    OnBotLoginInternal(bot);

    playerBots[bot->GetGUIDLow()] = bot;

    Player* master = ai->GetMaster();
    if (!master && sPlayerbotAIConfig.IsFreeAltBot(bot))
    {
        ai->SetMaster(bot);
        master = bot;
    }

    if (master)
    {
        ObjectGuid masterGuid = master->GetObjectGuid();
        if (master->GetGroup() && !master->GetGroup()->IsLeader(masterGuid) && !sPlayerbotAIConfig.IsFreeAltBot(bot))
            master->GetGroup()->ChangeLeader(masterGuid);
    }

    Group* group = bot->GetGroup();
    if (group)
    {
        bool groupValid = false;
        Group::MemberSlotList const& slots = group->GetMemberSlots();
        for (Group::MemberSlotList::const_iterator i = slots.begin(); i != slots.end(); ++i)
        {
            ObjectGuid member = i->guid;
            if (master)
            {
                if (master->GetObjectGuid() == member)
                {
                    groupValid = true;
                    break;
                }
            }

            // Don't disband alt groups when master goes away
            // (will need to manually disband with leave command)
            uint32 account = sObjectMgr.GetPlayerAccountIdByGUID(member);
            if (!sPlayerbotAIConfig.IsInRandomAccountList(account))
            {
                groupValid = true;
                break;
            }
        }

        if (!groupValid)
        {
            WorldPacket p;
            std::string member = bot->GetName();
            p << uint32(PARTY_OP_LEAVE) << member << uint32(0);
            bot->GetSession()->HandleGroupDisbandOpcode(p);
        }
    }

    ai->ResetStrategies();

    if (master && !master->IsTaxiFlying())
    {
        bot->GetMotionMaster()->MovementExpired();
    }

    // check activity
    ai->AllowActivity(ALL_ACTIVITY, true);
    // set delay on login
    ai->SetActionDuration(urand(2000, 4000));

    ai->TellPlayer(ai->GetMaster(), BOT_TEXT("hello"));

    JoinChatChannels(bot);

    if (sRandomPlayerbotMgr.IsRandomBot(bot))
    {
        uint32 lowguid = bot->GetObjectGuid().GetCounter();
        auto result = CharacterDatabase.PQuery("SELECT 1 FROM character_social WHERE flags='%u' and friend='%d'", SOCIAL_FLAG_FRIEND, lowguid);
        if (result)
            GetPlayerbotAI(bot)->SetPlayerFriend(true);
        else
            GetPlayerbotAI(bot)->SetPlayerFriend(false);

        if (sPlayerbotAIConfig.instantRandomize && !sPlayerbotAIConfig.disableRandomLevels && !bot->GetTotalPlayedTime())
        {
            sRandomPlayerbotMgr.InstaRandomize(bot);
        }
    }

    if (!bot->HasItemCount(6948, 1)
#ifdef MANGOSBOT_TWO
        && !bot->HasItemCount(40582, 1)
#endif
        )
    {
#ifdef MANGOSBOT_TWO
        if (bot->GetClass() == CLASS_DEATH_KNIGHT && bot->GetMapId() == 609)
            bot->StoreNewItemInBestSlots(40582, 1);
        else
#endif
            bot->StoreNewItemInBestSlots(6948, 1);
    }
}

std::string PlayerbotHolder::ProcessBotCommand(std::string cmd, ObjectGuid guid, ObjectGuid masterguid, bool admin, uint32 masterAccountId, uint32 masterGuildId, const std::string param)
{
    Player* bot = sObjectMgr.GetPlayer(guid);
    Player* master = masterguid ? sObjectMgr.GetPlayer(masterguid) : nullptr;

    if (!sPlayerbotAIConfig.enabled || guid.IsEmpty())
        return "Bot system is disabled";

    uint32 botAccount = sObjectMgr.GetPlayerAccountIdByGUID(guid);
    bool isRandomAccount = sPlayerbotAIConfig.IsInRandomAccountList(botAccount);
    bool isMasterAccount = (masterAccountId == botAccount);

    if (!isRandomAccount && (!isMasterAccount && !admin && masterguid))
    {
        if (master && (!sPlayerbotAIConfig.allowGuildBots || !masterGuildId || (masterGuildId && master->GetGuildIdFromDB(guid) != masterGuildId)))
            return "Not in your guild or account";
    }

    if (!isRandomAccount && this == &sRandomPlayerbotMgr && !admin)
    {
        return "Can not control alt-bots with this command.";
    }

    std::string subType;
    size_t eqPos = cmd.find('=');
    if (eqPos != std::string::npos)
    {
        subType = cmd.substr(eqPos + 1);
        cmd = cmd.substr(0, eqPos);
    }

    auto it = m_botCommandHandlers.find(cmd);
    if (it != m_botCommandHandlers.end())
    {
        std::string realParam;
        
        if (!subType.empty())
            realParam = subType;
        else if (it->second == &PlayerbotHolder::HandleBotAddLogin || it->second == &PlayerbotHolder::HandleBotAlways || it->second == &PlayerbotHolder::HandleBotDelete)
            realParam = std::to_string(guid.GetRawValue());        
        else
            realParam = param;            

        return (this->*it->second)(bot, master, realParam);
    }

    return "unknown command";
}

bool PlayerbotMgr::HandlePlayerbotMgrCommand(ChatHandler* handler, char const* args)
{
	if (!sPlayerbotAIConfig.enabled)
	{
		handler->PSendSysMessage("|cffff0000Playerbot system is currently disabled!");
        return false;
	}

    WorldSession *m_session = handler->GetSession();

    if (!m_session)
    {
        handler->PSendSysMessage("You may only add bots from an active session");
        return false;
    }

    Player* player = m_session->GetPlayer();
    PlayerbotMgr* mgr = GetPlayerbotMgr(player);
    if (!mgr)
    {
        handler->PSendSysMessage("you cannot control bots yet");
        return false;
    }

    std::list<std::string> messages = mgr->HandlePlayerbotCommand(args, player);
    if (messages.empty())
        return true;

    for (std::list<std::string>::iterator i = messages.begin(); i != messages.end(); ++i)
    {
        handler->PSendSysMessage("%s",i->c_str());
    }

    return true;
}

std::list<std::string> PlayerbotHolder::HandlePlayerbotCommand(const std::string args, Player* master, AccountTypes security)
{
    AccountTypes useSecurity = master ? master->GetSession()->GetSecurity() : security;

    if (!master && m_spoofGuid)
        master = sObjectMgr.GetPlayer(m_spoofGuid);

    std::vector<std::string> params = Qualified::getMultiQualifiers(args, " ");

    std::list<std::string> messages;

    if (params.empty())
    {
        std::string helpText = GetCommandTexts("");
        messages.push_back(helpText);
        return messages;
    }

    std::string command = params[0];
    std::string param, charname;

    if (params.size() > 1)
    {
        param = args.substr(params[0].size() + 1);
        charname = params[1];
    }
    
    for (auto& [prefix, handler] : m_holderHandlers)
    {
        if (command != prefix)
            continue;

        messages = (this->*handler)(master, param, useSecurity);
        return messages;
    }

    std::string securityCommand = command.substr(0, command.find('='));
    if (securityCommand == "debug" && useSecurity < SEC_DEVELOPER)
    {
        messages.push_back("You do not have permission to use this command.");
        return messages;
    }
    if (securityCommand == "c" && useSecurity < SEC_MODERATOR)
    {
        messages.push_back("You do not have permission to use this command.");
        return messages;
    }

    std::set<std::string> bots;

    if (charname.empty())
    {
        if (master && master->GetSelectedUnit() && master->GetSelectedUnit()->IsPlayer() && !IsRealPlayer((Player*)master->GetSelectedUnit()))
        {
            bots.insert(master->GetSelectedUnit()->GetName());
        }
        else
        {
            std::string helpText = GetCommandTexts("");
            messages.push_back(helpText);
            return messages;
        }
    }    

    if (charname == "*" && master)
    {
        Group* group = master->GetGroup();
        if (!group)
        {
            messages.push_back("you must be in group");
            return messages;
        }

        Group::MemberSlotList slots = group->GetMemberSlots();
        for (Group::member_citerator i = slots.begin(); i != slots.end(); i++)
        {
            ObjectGuid member = i->guid;

            if (member.GetRawValue() == master->GetObjectGuid().GetRawValue())
                continue;

            std::string bot;
            if (sObjectMgr.GetPlayerNameByGUID(member, bot))
                bots.insert(bot);
        }
    }

    if (charname == "guild" && master)
    {
        if (!master->GetGuildId())
        {
            messages.push_back("you must be in a guild");
            return messages;
        }

        auto result = CharacterDatabase.PQuery("SELECT m.guid, (select name from characters c where c.guid = m.guid) FROM guild_member m WHERE guildid = '%u'", master->GetGuildId());

        if (!result)
        {
            messages.push_back("No guild members");
            return messages;
        }

        do
        {
            Field* fields = result->Fetch();
            uint32 guid = fields[0].GetUInt32();
            std::string bot = fields[1].GetString();

            if (guid == master->GetGUIDLow())
                continue;

            bots.insert(bot);
        } while (result->NextRow());
    }

    if (charname == "!" && useSecurity > SEC_DEVELOPER)
    {
        for (auto& itr : playerBots)
        {
            Player* bot = itr.second;
            if (bot && (bot->IsInWorld() || param.find("add") == 0 || param.find("login") == 0 || param.find("delete") == 0))
                bots.insert(bot->GetName());
        }
    }

    if (bots.empty())
    {
        std::vector<std::string> chars = split(charname, ',');
        for (auto name : chars)
        {
            uint32 accountId = GetAccountId(name);
            if (!accountId)
            {
                bots.insert(name);
                continue;
            }

            auto results = CharacterDatabase.PQuery(
                "SELECT name FROM characters WHERE account = '%u'",
                accountId);
            if (results)
            {
                do
                {
                    Field* fields = results->Fetch();
                    std::string charName = fields[0].GetString();
                    bots.insert(charName);
                } while (results->NextRow());
            }
        }
    }

    if (bots.size())
    {
        if (params.size() > 2)
            param = args.substr(params[0].size() + params[1].size() + 2);
        else
            param = "";
    }

    for (auto bot :  bots)
    {
        std::ostringstream out;
        out << command << ": " << bot << " - ";

        ObjectGuid member = sObjectMgr.GetPlayerGuidByName(bot);
        if (!member)
        {
            out << "character not found";
        }
        else if (master)
        {
            out << ProcessBotCommand(command, member, master->GetObjectGuid(), useSecurity >= SEC_DEVELOPER, master->GetSession()->GetAccountId(), master->GetGuildId(), param);
        }
        else
        {
            out << ProcessBotCommand(command, member, ObjectGuid(), useSecurity >= SEC_DEVELOPER, -1, -1, param);
        }

        messages.push_back(out.str());
    }

    if (messages.empty())
        messages.push_back("Unknown command. Use 'help' for more information.");

    return messages;
}

uint32 PlayerbotHolder::GetAccountId(std::string name)
{
    uint32 accountId = 0;

    auto results = LoginDatabase.PQuery("SELECT id FROM account WHERE username = '%s'", name.c_str());
    if(results)
    {
        Field* fields = results->Fetch();
        accountId = fields[0].GetUInt32();
    }

    return accountId;
}

std::string PlayerbotHolder::ListBots(Player* master, const std::string param)
{
    std::set<std::string> bots;
    std::map<uint8, std::string> classNames;
    classNames[CLASS_DRUID] = "Druid";
    classNames[CLASS_HUNTER] = "Hunter";
    classNames[CLASS_MAGE] = "Mage";
    classNames[CLASS_PALADIN] = "Paladin";
    classNames[CLASS_PRIEST] = "Priest";
    classNames[CLASS_ROGUE] = "Rogue";
    classNames[CLASS_SHAMAN] = "Shaman";
    classNames[CLASS_WARLOCK] = "Warlock";
    classNames[CLASS_WARRIOR] = "Warrior";
#ifdef MANGOSBOT_TWO
    classNames[CLASS_DEATH_KNIGHT] = "DeathKnight";
#endif

    std::map<std::string, std::string> online;
    std::list<std::string> names;
    std::map<std::string, std::string> classes;

    for (auto& itr : playerBots)
    {
        Player* bot = itr.second;

        if (!bot)
            continue;

        std::string name = bot->GetName();

        if (!param.empty() && name.find(param) != 0)
            continue;

        bots.insert(name);
        names.push_back(name);
        online[name] = "+";
        classes[name] = classNames[bot->GetClass()];
    }

    if (master)
    {
        auto results = CharacterDatabase.PQuery("SELECT class,name FROM characters where account = '%u'",
            master->GetSession()->GetAccountId());
        if (results != NULL)
        {
            do
            {
                Field* fields = results->Fetch();
                uint8 cls = fields[0].GetUInt8();
                std::string name = fields[1].GetString();

                if (!param.empty() && name.find(param) != 0)
                    continue;

                if (bots.find(name) == bots.end() && name != master->GetSession()->GetPlayerName())
                {
                    names.push_back(name);
                    online[name] = "-";
                    classes[name] = classNames[cls];
                }
            } while (results->NextRow());
        }
    }

    names.sort();

    if (master)
    {
        Group* group = master->GetGroup();
        if (group)
        {
            Group::MemberSlotList const& groupSlot = group->GetMemberSlots();
            for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
            {
                Player* member = sObjectMgr.GetPlayer(itr->guid);

                if (member && sRandomPlayerbotMgr.IsFreeBot(member))
                {
                    std::string name = member->GetName();

                    if (!param.empty() && name.find(param) != 0)
                        continue;

                    names.push_back(name);
                    online[name] = "+";
                    classes[name] = classNames[member->GetClass()];
                }
            }
        }
    }

    std::ostringstream out;
    bool first = true;
    out << "Bot roster: ";
    for (std::list<std::string>::iterator i = names.begin(); i != names.end(); ++i)
    {
        if (first)
            first = false;
        else
            out << ", ";
        std::string name = *i;
        out << online[name] << name << " " << classes[name];
    }

    return out.str();
}


uint32 PlayerbotHolder::GetPlayerbotsAmount() const
{
    uint32 amount = 0;
    for (const auto& pair : playerBots)
    {
        if (pair.second)
        {
            amount++;
        }
    }

    return amount;
}

PlayerbotMgr::PlayerbotMgr(Player* const master) : PlayerbotHolder(),  master(master), lastErrorTell(0)
{
}

PlayerbotMgr::~PlayerbotMgr()
{
}

void PlayerbotMgr::UpdateAIInternal(uint32 elapsed, bool minimal)
{
    SetAIInternalUpdateDelay(sPlayerbotAIConfig.reactDelay);
    CheckTellErrors(elapsed);

    // Tick our bots' sessions so any queued bot-only handling fires per
    // master frame. Most importantly:
    // HandleTeleportAck for cross-map teleports — without this call, a
    // bot scheduled for a far-teleport via TeleportTo() stays in
    // IsBeingTeleported() forever (no client to ACK), its UpdateAI
    // early-exits, and the bot is effectively frozen on the source map.
    // This call was defined in PlayerbotHolder but never wired in the
    // cmangos→Penqle port. Symptom: `.bot add a bot` (alt on different
    // continent) auto-teleport to master never completes, bot doesn't
    // appear in /who, can't be invited.
    UpdateSessions(elapsed);
}

void PlayerbotMgr::HandleCommand(uint32 type, const std::string& text, uint32 lang)
{
    Player *master = GetMaster();
    if (!master)
        return;

    if (!sPlayerbotAIConfig.enabled)
        return;

    if (text.find(sPlayerbotAIConfig.commandSeparator) != std::string::npos)
    {
        std::vector<std::string> commands;
        split(commands, text, sPlayerbotAIConfig.commandSeparator.c_str());
        for (std::vector<std::string>::iterator i = commands.begin(); i != commands.end(); ++i)
        {
            HandleCommand(type, *i,lang);
        }
        return;
    }

    ForEachPlayerbot([&](Player *bot)
    {
        if (type == CHAT_MSG_SAY)
            if (bot->GetMapId() != master->GetMapId() || sServerFacade.GetDistance2d(bot, master) > 25)
                return;

        if (type == CHAT_MSG_YELL)
            if (bot->GetMapId() != master->GetMapId() || sServerFacade.GetDistance2d(bot, master) > 300)
               return;

        GetPlayerbotAI(bot)->HandleCommand(type, text, *master, lang);
    });

    sRandomPlayerbotMgr.ForEachPlayerbot([&](Player* bot)
    {
        if (type == CHAT_MSG_SAY)
            if (bot->GetMapId() != master->GetMapId() || sServerFacade.GetDistance2d(bot, master) > 25)
               return;

        if (type == CHAT_MSG_YELL)
            if (bot->GetMapId() != master->GetMapId() || sServerFacade.GetDistance2d(bot, master) > 300)
               return;

        if (GetPlayerbotAI(bot)->GetMaster() == master)
            GetPlayerbotAI(bot)->HandleCommand(type, text, *master, lang);
    });
}

void PlayerbotMgr::HandleMasterIncomingPacket(const WorldPacket& packet)
{
    ForEachPlayerbot([&](Player* bot)
    {
        GetPlayerbotAI(bot)->HandleMasterIncomingPacket(packet);
    });

    sRandomPlayerbotMgr.ForEachPlayerbot([&](Player* bot)
    {
        if (GetPlayerbotAI(bot)->GetMaster() == GetMaster())
            GetPlayerbotAI(bot)->HandleMasterIncomingPacket(packet);
    });

    switch (packet.GetOpcode())
    {
        // if master is logging out, log out all bots
        case CMSG_LOGOUT_REQUEST:
        {
            LogoutAllBots();
            return;
        }
        // if master cancelled logout, cancel too
        case CMSG_LOGOUT_CANCEL:
        {
            CancelLogout();
            return;
        }
    }
}
void PlayerbotMgr::HandleMasterOutgoingPacket(const WorldPacket& packet)
{
   ForEachPlayerbot([&](Player* bot)
   {
        if (!GetPlayerbotAI(bot))
            return;

        GetPlayerbotAI(bot)->HandleMasterOutgoingPacket(packet);
    });

    sRandomPlayerbotMgr.ForEachPlayerbot([&](Player* bot)
    {
        if (GetPlayerbotAI(bot)->GetMaster() == GetMaster())
            GetPlayerbotAI(bot)->HandleMasterOutgoingPacket(packet);
    });
}

void PlayerbotMgr::SaveToDB()
{
    ForEachPlayerbot([&](Player* bot)
    {
        bot->SaveToDB();
    });

    sRandomPlayerbotMgr.ForEachPlayerbot([&](Player* bot)
    {
        if (GetPlayerbotAI(bot)->GetMaster() == GetMaster())
            bot->SaveToDB();
    });
}

void PlayerbotMgr::OnBotLoginInternal(Player * const bot)
{
    GetPlayerbotAI(bot)->SetMaster(master);
    GetPlayerbotAI(bot)->ResetStrategies();
    sLog.outDebug("Bot %s logged in", bot->GetName());
}

void PlayerbotMgr::OnPlayerLogin(Player* player)
{
    if (player->GetSession() != player->PlayerTalkClass->GetGossipMenu().GetMenuSession())
    {
        player->PlayerTalkClass->GetGossipMenu() = GossipMenu(player->GetSession());
    }

    if (!sPlayerbotAIConfig.enabled)
        return;

    // set locale priority for bot texts
    sPlayerbotTextMgr.AddLocalePriority(player->GetSession()->GetSessionDbLocaleIndex());
    sLog.outDetail("Player %s logged in, localeDbc %i, localeDb %i", player->GetName(), (uint32)(player->GetSession()->GetSessionDbcLocale()), player->GetSession()->GetSessionDbLocaleIndex());

    if (sPlayerbotAIConfig.IsFreeAltBot(player))
    {
        sLog.outDetail("Enabling selfbot on login for %s", player->GetName());
        HandlePlayerbotCommand("self", player);
    }

    if (sPlayerbotAIConfig.botAutologin == BotAutoLogin::DISABLED)
        return;

    uint32 accountId = player->GetSession()->GetAccountId();
    auto results = CharacterDatabase.PQuery(
        "SELECT guid, name FROM characters WHERE account = '%u'",
        accountId);
    if (results)
    {
        std::ostringstream out; out << "add ";
        bool first = true;
        do
        {
            Field* fields = results->Fetch();
            if (first) first = false; else out << ",";
            if(sPlayerbotAIConfig.botAutologin == BotAutoLogin::LOGIN_ONLY_ALWAYS_ACTIVE && !sPlayerbotAIConfig.IsFreeAltBot(fields[0].GetUInt32())) continue;
            out << fields[1].GetString();
        } while (results->NextRow());

        HandlePlayerbotCommand(out.str().c_str(), player);
    }
}

void PlayerbotMgr::TellError(std::string botName, std::string text)
{
    std::set<std::string> names = errors[text];
    if (names.find(botName) == names.end())
    {
        names.insert(botName);
    }
    errors[text] = names;
}

std::vector<std::string> PlayerbotMgr::GetBotErrors(std::string botName)
{
    std::vector<std::string> botErrors;
    for (auto& [error, names] : errors)
    {
        if (names.find(botName) != names.end())
            botErrors.push_back(error);
    }

    return botErrors;
}

void PlayerbotMgr::CheckTellErrors(uint32 elapsed)
{
    time_t now = time(0);
    if ((now - lastErrorTell) < sPlayerbotAIConfig.errorDelay / 1000)
        return;

    lastErrorTell = now;

    for (PlayerBotErrorMap::iterator i = errors.begin(); i != errors.end(); ++i)
    {
        std::string text = i->first;
        std::set<std::string> names = i->second;

        std::ostringstream out;
        bool first = true;
        for (std::set<std::string>::iterator j = names.begin(); j != names.end(); ++j)
        {
            if (!first) out << ", "; else first = false;
            out << *j;
        }
        out << "|cfff00000: " << text;
        
        ChatHandler(master->GetSession()).PSendSysMessage("%s", out.str().c_str());
    }
    errors.clear();
}

std::list<std::string> PlayerbotHolder::HandleList(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;
    messages.push_back(ListBots(master, param));
    return messages;
}

std::list<std::string> PlayerbotHolder::HandleHelp(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;
    
    if (param.empty())
    {
        messages.push_back("Available commands: list, reload, tweak, always, self, debug, c, do, record, read, clear");
        messages.push_back("Type 'help <command>' for more information on a specific command.");
        return messages;
    }

    if (param == "commands")
    {
        std::string commands = "Commands: ";
        for (auto& [command, help] : GetCommandTexts())
        {
            commands += command + ", ";
        }

        commands = commands.substr(0, commands.size() - 2);
        messages.push_back(commands);
        return messages;
    }
    
    std::string helpText = GetCommandTexts(param);
    if (helpText.empty())
    {
        messages.push_back("No help available for '" + param + "'");
    }
    else
    {
        messages.push_back(helpText);
    }
    
    return messages;
}

std::list<std::string> PlayerbotHolder::HandleReload(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;
    if (security < SEC_DEVELOPER)
    {
        messages.push_back("You do not have permission to use this command.");
        return messages;
    }
    messages.push_back("Reloading config");
    sPlayerbotAIConfig.Initialize();
    return messages;
}

std::list<std::string> PlayerbotHolder::HandleTweak(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;
    if (security < SEC_DEVELOPER)
    {
        messages.push_back("You do not have permission to use this command.");
        return messages;
    }
    sPlayerbotAIConfig.tweakValue = sPlayerbotAIConfig.tweakValue++;
    if (sPlayerbotAIConfig.tweakValue > 2)
        sPlayerbotAIConfig.tweakValue = 0;
    messages.push_back("Set tweakvalue to " + std::to_string(sPlayerbotAIConfig.tweakValue));
    return messages;
}

std::string PlayerbotHolder::HandleBotAlways(Player* bot, Player* master, const std::string param)
{
    if (sPlayerbotAIConfig.selfBotLevel == BotSelfBotLevel::DISABLED)
    {
        return "Self-bot is disabled";
    }

    ObjectGuid guid = ObjectGuid(uint64(std::stoull(param)));
    uint32 accountId = sObjectMgr.GetPlayerAccountIdByGUID(guid);
    std::string alwaysName;    

    if (!sObjectMgr.GetPlayerNameByGUID(guid, alwaysName))
        return "Unable to find player.";


    BotAlwaysOnline always = BotAlwaysOnline(sRandomPlayerbotMgr.GetValue(guid.GetCounter(), "always"));

    if (always == BotAlwaysOnline::DISABLED || always == BotAlwaysOnline::DISABLED_BY_COMMAND)
    {
        sRandomPlayerbotMgr.SetValue(guid.GetCounter(), "always", (uint32)BotAlwaysOnline::ACTIVE);
        sPlayerbotAIConfig.freeAltBots.push_back(std::make_pair(accountId, guid.GetCounter()));

        Player* existingBot = sRandomPlayerbotMgr.GetPlayerBot(guid);
        if (existingBot)
        {
            if (master)
            {
                ProcessBotCommand("add", guid, master->GetObjectGuid(), false, master->GetSession()->GetAccountId(), master->GetGuildId());
            }
        }
        else
        {
            Player* player = sObjectMgr.GetPlayer(guid);
            if (player)
                OnBotLogin(player);
        }

        return "Enabled offline player ai for " + alwaysName;
    }
    else
    {
        sRandomPlayerbotMgr.SetValue(guid.GetCounter(), "always", (uint32)BotAlwaysOnline::DISABLED_BY_COMMAND);

        Player* onlineBot = sObjectMgr.GetPlayer(guid);
        if (onlineBot && GetPlayerbotAI(onlineBot))
        {
            if (!master || guid != master->GetObjectGuid())
            {
                if (sPlayerbotAIConfig.IsFreeAltBot(onlineBot))
                    sRandomPlayerbotMgr.LogoutPlayerBot(guid);
                else
                    DisablePlayerBot(guid, false);
            }
            else if (master)
            {
                DisablePlayerBot(guid, false);
            }
        }

        auto it = std::remove_if(sPlayerbotAIConfig.freeAltBots.begin(), sPlayerbotAIConfig.freeAltBots.end(), [guid](std::pair<uint32, uint32> i) { return i.second == guid.GetCounter(); });
        sPlayerbotAIConfig.freeAltBots.erase(it, sPlayerbotAIConfig.freeAltBots.end());

        return "Disabled offline player ai for " + alwaysName;
    }
}

std::list<std::string> PlayerbotHolder::HandleSelf(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;
    if (!master)
    {
        messages.push_back("self requires a master (in-game)");
        return messages;
    }

    if (GetPlayerbotAI(master))
    {
        DisablePlayerBot(master->GetGUIDLow(), false);
       
        if (sRandomPlayerbotMgr.GetValue(master->GetObjectGuid().GetCounter(), "selfbot"))
        {
            messages.push_back("Disable player ai (on login)");
            sRandomPlayerbotMgr.SetValue(master->GetObjectGuid().GetCounter(), "selfbot", (uint32)BotAlwaysOnline::DISABLED);
        }
        else
            messages.push_back("Disable player ai");
    }
    else if (sPlayerbotAIConfig.selfBotLevel == BotSelfBotLevel::DISABLED)
        messages.push_back("Self-bot is disabled");
    else if (sPlayerbotAIConfig.selfBotLevel == BotSelfBotLevel::GM_ONLY && security < SEC_DEVELOPER)
        messages.push_back("You do not have permission to enable player ai");
    else
    {
        OnBotLogin(master);

        if (!param.empty() && param == "login")
        {
            messages.push_back("Enable player ai (on login)");
            sRandomPlayerbotMgr.SetValue(master->GetObjectGuid().GetCounter(), "selfbot", 1);
        }
        else
            messages.push_back("Enable player ai");
    }
   return messages;
}

std::string PlayerbotHolder::HandleBotDebug(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "debug requires a bot";

    PlayerbotAI* ai = GetPlayerbotAI(bot);
    if (!ai)
        return "Bot has no AI";

    ai->RecordMessages(true, true);

    std::string command = param;

    if(!ai->DoSpecificAction("cdebug", Event(".bot privileged", command, master ? master : bot), true))
    {
        return "debug failed";
    }

    std::vector<std::string> output = ai->GetRecordedMessages();
    if (output.empty())
        return "(no output)";

    std::string result;
    for (const auto& line : output)
    {
        result += line + "\n";
    }
    return result;
}

std::string PlayerbotHolder::HandleBotC(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "c requires a bot";

    PlayerbotAI* ai = GetPlayerbotAI(bot);
    if (!ai)
        return "Bot has no AI";

    ai->DoSpecificAction("cdebug", Event(".bot privileged", "monstertalk " + param, master ? master : bot), true);
    return "ok";
}

std::string PlayerbotHolder::HandleConsoleWhisper(Player* bot, Player* master, const std::string param)
{
    Player* sender = master;
    Player* reciever = bot;


    if (!reciever)
        return "d requires a bot";

    PlayerbotAI* ai = GetPlayerbotAI(bot);
    if (!ai)
        return "Bot has no AI";

    std::string message = param;

    if (!sender)
    {
        //Try format .(rnd)bot w <sender> <reciever> <message>

        std::string botName = param.substr(0, param.find(" "));

        master = sObjectAccessor.FindPlayerByName(botName.c_str());

        if (master)
        {
            if (message.size() > param.find(" ") + 1)
                message = param.substr(param.find(" ") + 1);
            else
                message = "";

            sender = bot; //Switch sender reciever
            reciever = master; 
        }
    }

    if (!sender)
        sender = bot;

    if (message.empty())
    {
        std::ostringstream out;
        if (!GetPlayerbotAI(sender))
            out << "Player ";
        if (!GetPlayerbotAI(sender)->IsRealPlayer())
            out << "Player bot ";
        else if (sRandomPlayerbotMgr.IsRandomBot(sender))
            out << "Random bot ";
        else if (sPlayerbotAIConfig.IsFreeAltBot(sender))
            out << "Free alt bot ";
        else
            out << "Bot ";

        out << reciever->GetName();
        out << " level " << std::to_string(reciever->GetLevel());
        out << " " << ChatHelper::formatRace(reciever->GetRace());
        out << " " << ChatHelper::formatClass(reciever->GetClass());

        if (GetPlayerbotAI(sender) && GetPlayerbotAI(sender)->GetMaster())
            out << " (master " << GetPlayerbotAI(sender)->GetMaster()->GetName() << ")";

        return out.str(); 
    }

    WorldPacket packet_template(CMSG_MESSAGECHAT);

    packet_template << CHAT_MSG_WHISPER;
    packet_template << LANG_UNIVERSAL;
    packet_template << reciever->GetName();
    packet_template << message;

    std::unique_ptr<WorldPacket> packetPtr(new WorldPacket(packet_template));

    QueuePlayerbotPacket(sender, std::move(packetPtr));

    std::string msg = "Sending whisper " + message + " to player " + reciever->GetName() + " from " + sender->GetName();

    return msg;
}

std::string PlayerbotHolder::HandleConsoleCmd(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "do requires a bot";

    PlayerbotAI* ai = GetPlayerbotAI(bot);
    if (!ai)
        return "Bot has no AI";

    ExternalEventHelper helper(ai->GetAiObjectContext());

    std::string msg = "Sending command " + param + " to player " + bot->GetName();

    if (!helper.ParseChatCommand(param, master ? master : bot))
    {
        return "command failed";
    }    

    return msg;
}

std::string PlayerbotHolder::HandleBotTest(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "test requires a bot";

    PlayerbotAI* ai = GetPlayerbotAI(bot);
    if (!ai)
        return "Bot has no AI";

    if (param.empty())
    {
        return "Usage: test <testName>. Available tests: walk_to_ironforge, flight_ratchet_to_booty_bay";
    }

    // Activate test strategy which will run the test over multiple ticks
    std::string strategyName = "test::" + param;
    ai->ChangeStrategy("+" + strategyName, BotState::BOT_STATE_NON_COMBAT);
    
    return "Test '" + param + "' started for bot " + bot->GetName();
}

std::string PlayerbotHolder::HandleBotDo(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "do requires a bot";

    PlayerbotAI* ai = GetPlayerbotAI(bot);
    if (!ai)
        return "Bot has no AI";

    std::string actionName = param;
    std::string subparam = "";

    Action* action = nullptr;

    size_t i = std::string::npos;
    while (true)
    {
        action = ai->GetAiObjectContext()->GetAction(param);

        if (action)
            break;

        size_t found = param.rfind(" ", i);
        if (found == std::string::npos || !found)
            break;

        actionName = param.substr(0, found);
        subparam = param.substr(found + 1);

        i = found - 1;
    }

    if (!action)
        return "action not found";

    ai->RecordMessages(true, true);

    std::vector<std::string> output;

    if (!ai->DoSpecificAction(actionName, Event(".bot", subparam, master ? master : bot), true))
    {
        output = GetBotErrors(bot->GetName());

        if (output.empty())
            return "action failed";

        std::string result;
        for (const auto& line : output)
        {
            result += line + "\n";
        }
        return result;
    }

    output = ai->GetRecordedMessages();
    if (output.empty())
        return "(no output)";

    std::string result;
    for (const auto& line : output)
    {
        result += line + "\n";
    }
    return result;
}

std::string PlayerbotHolder::HandleBotRecord(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "record requires a bot";

    PlayerbotAI* ai = GetPlayerbotAI(bot);
    if (!ai)
        return "Bot has no AI";

    ai->RecordMessages(true, !param.empty());
    return "Recording enabled on " + std::string(bot->GetName());
}

std::string PlayerbotHolder::HandleBotRead(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "read requires a bot";

    PlayerbotAI* ai = GetPlayerbotAI(bot);
    if (!ai)
        return "Bot has no AI";

    std::vector<std::string> output = ai->GetRecordedMessages();

    if (output.empty())
        return "(no messages)";

    std::string result;
    for (const auto& line : output)
    {
        result += line + "\n";
    }
    return result;
}

std::string PlayerbotHolder::HandleBotClear(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "clear requires a bot";

    PlayerbotAI* ai = GetPlayerbotAI(bot);
    if (!ai)
        return "Bot has no AI";

    ai->ClearRecordedMessages();
    return "Messages cleared";
}

std::list<std::string> PlayerbotHolder::HandleParty(Player* master, const std::string param, AccountTypes security)
{
    std::string message;
    std::string botName;

    if (!master)
    {
        botName = param.substr(0, param.find(" "));
        master = sObjectAccessor.FindPlayerByName(botName.c_str());
    }

    if (!master)
        return {"No sender found"};

    if (param.find(" ") == std::string::npos)
        message = param;
    else if (param.size() > param.find(" ") + 1)
        message = param.substr(param.find(" ") + 1);

    if (!master->GetGroup())
        return {"Sender is not in a group"};

    if (message.empty())
    {
        Group* group = master->GetGroup();
        Group::MemberSlotList const& members = group->GetMemberSlots();
        Player* leader = sObjectMgr.GetPlayer(group->GetLeaderGuid());

        std::string leaderName = leader ? leader->GetName() : "Unknown";
        std::string otherMembers;

        for (auto const& slot : members)
        {
            if (slot.guid == master->GetObjectGuid())
                continue;

            Player* member = sObjectMgr.GetPlayer(slot.guid);
            if (member)
            {
                if (!otherMembers.empty())
                    otherMembers += ", ";
                otherMembers += member->GetName();
            }
        }

        return {"Party with " + leaderName + " as leader" + (otherMembers.empty() ? "" : " and " + otherMembers)};
    }

    WorldPacket packet_template(CMSG_MESSAGECHAT);
    packet_template << CHAT_MSG_PARTY;
    packet_template << LANG_UNIVERSAL;
    packet_template << message;

    std::unique_ptr<WorldPacket> packetPtr(new WorldPacket(packet_template));
    QueuePlayerbotPacket(master, std::move(packetPtr));
    return {"Sent party message \"" + message + "\" as " + master->GetName()};
}

std::list<std::string> PlayerbotHolder::HandleGuild(Player* master, const std::string param, AccountTypes security)
{
    std::string message;
    std::string botName;

    if (!master)
    {
        botName = param.substr(0, param.find(" "));
        master = sObjectAccessor.FindPlayerByName(botName.c_str());
    }

    if (!master)
        return {"No sender found"};

    if (param.find(" ") == std::string::npos)
        message = param;
    else if (param.size() > param.find(" ") + 1)
        message = param.substr(param.find(" ") + 1);

    if (!master->GetGuildId())
        return {"Sender is not in a guild"};

    if (message.empty())
    {
        Guild* guild = sGuildMgr.GetGuildById(master->GetGuildId());
        if (!guild)
            return {"Guild info not found"};

        std::string guildName = guild->GetName();
        std::string guildLeader;
        sObjectMgr.GetPlayerNameByGUID(guild->GetLeaderGuid(), guildLeader);
        uint32 memberCount = guild->GetMemberSize();

        return {"Guild: " + guildName + ", Leader: " + guildLeader + ", Members: " + std::to_string(memberCount)};
    }

    WorldPacket packet_template(CMSG_MESSAGECHAT);
    packet_template << CHAT_MSG_GUILD;
    packet_template << LANG_UNIVERSAL;
    packet_template << message;

    std::unique_ptr<WorldPacket> packetPtr(new WorldPacket(packet_template));
    QueuePlayerbotPacket(master, std::move(packetPtr));
    return {"Sent guild message \"" + message + "\" as " + master->GetName()};
}

std::list<std::string> PlayerbotHolder::HandleRaid(Player* master, const std::string param, AccountTypes security)
{
    std::string message = param;

    if (!master)
    {
        std::string botName = param.substr(0, param.find(" "));

        master = sObjectAccessor.FindPlayerByName(botName.c_str());
        if (message.size() > param.find(" ") + 1)
            message = param.substr(param.find(" ") + 1);
    }

    if (!master)
        return {"No sender found"};

    if (!master->GetGroup() || !master->GetGroup()->isRaidGroup())
        return {"Sender is not in a raid group"};

    if (message.empty())
    {
        Group* group = master->GetGroup();
        Group::MemberSlotList const& members = group->GetMemberSlots();
        Player* leader = sObjectMgr.GetPlayer(group->GetLeaderGuid());

        std::string leaderName = leader ? leader->GetName() : "Unknown";
        std::string otherMembers;

        for (auto const& slot : members)
        {
            if (slot.guid == master->GetObjectGuid())
                continue;

            Player* member = sObjectMgr.GetPlayer(slot.guid);
            if (member)
            {
                if (!otherMembers.empty())
                    otherMembers += ", ";
                otherMembers += member->GetName();
            }
        }

        return {"Raid with " + leaderName + " as leader" + (otherMembers.empty() ? "" : " and " + otherMembers)};
    }

    WorldPacket packet_template(CMSG_MESSAGECHAT);
    packet_template << CHAT_MSG_RAID;
    packet_template << LANG_UNIVERSAL;
    packet_template << message;

    std::unique_ptr<WorldPacket> packetPtr(new WorldPacket(packet_template));
    QueuePlayerbotPacket(master, std::move(packetPtr));
    return {"Sent raid message \"" + message + "\" as " + master->GetName()};
}

std::list<std::string> PlayerbotHolder::HandleRaidLeader(Player* master, const std::string param, AccountTypes security)
{
    std::string message = param;

    if (!master)
    {
        std::string botName = param.substr(0, param.find(" "));

        master = sObjectAccessor.FindPlayerByName(botName.c_str());
        if (message.size() > param.find(" ") + 1)
            message = param.substr(param.find(" ") + 1);
    }

    if (!master)
        return {"No sender found"};

    if (!master->GetGroup() || !master->GetGroup()->isRaidGroup())
        return {"Sender is not in a raid group"};

    WorldPacket packet_template(CMSG_MESSAGECHAT);
    packet_template << CHAT_MSG_RAID;
    packet_template << LANG_UNIVERSAL;
    packet_template << message;

    std::unique_ptr<WorldPacket> packetPtr(new WorldPacket(packet_template));
    QueuePlayerbotPacket(master, std::move(packetPtr));
    std::string result = "Sent raid leader transfer request as " + std::string(master->GetName());
    return {result};
}

std::string PlayerbotHolder::HandleBotAddLogin(Player* bot, Player* master, const std::string param)
{
    SC_LOG("HandleBotAddLogin entry bot=%s master=%s param=%s",
           bot ? bot->GetName() : "(null)",
           master ? master->GetName() : "(null)",
           param.c_str());

    if (bot)
    {
        SC_LOG("HandleBotAddLogin bot already online — returning early");
        return "Player already logged in";
    }

    if (!Qualified::isValidNumberString(param))
        return "Add: Error parsing " + param;

    ObjectGuid guid = ObjectGuid(uint64(std::stoull(param)));

    uint32 guildId = Player::GetGuildIdFromDB(guid);
    uint32 masterAccountId = master ? master->GetSession()->GetAccountId() : 0;
    uint32 masterGuildId = master ? master->GetGuildId() : 0;
    uint32 botAccount = sObjectMgr.GetPlayerAccountIdByGUID(guid);
    bool isMasterAccount = (masterAccountId == botAccount);
    bool isRandomAccount = sPlayerbotAIConfig.IsInRandomAccountList(botAccount);

    SC_LOG("HandleBotAddLogin guid=%u botAccount=%u masterAccount=%u isMaster=%d isRandom=%d",
           guid.GetCounter(), botAccount, masterAccountId,
           (int)isMasterAccount, (int)isRandomAccount);

    if (isRandomAccount)
    {
        SC_LOG("HandleBotAddLogin -> RandomPlayerbotMgr.AddRandomBot");
        sRandomPlayerbotMgr.AddRandomBot(guid);
    }
    else if (isMasterAccount || sPlayerbotAIConfig.allowMultiAccountAltBots)
    {
        SC_LOG("HandleBotAddLogin -> AddPlayerBot (master-account path)");
        AddPlayerBot(guid, masterAccountId);
    }
    else
    {
        SC_LOG("HandleBotAddLogin -> rejected: not in account");
        return "Not in your account";
    }

    SC_LOG("HandleBotAddLogin returning ok bot=%u", guid.GetCounter());
    return "ok";
}

// `.bot summon <name>` (also aliased as
// `recall` / `come`). Teleports an already-online bot to the master's
// current position. Distinct from `.bot add` which brings the bot online
// at its last logout location. Useful when:
//   - The auto-teleport-on-summon flag failed (race / master null at tick)
//   - Bot wandered off via grind/free strategy and you want it back
//   - Bot disconnected from master and is stuck on a different map
std::string PlayerbotHolder::HandleBotSummon(Player* bot, Player* master, const std::string param)
{
    SC_LOG("HandleBotSummon entry bot=%s master=%s",
           bot ? bot->GetName() : "(null)",
           master ? master->GetName() : "(null)");

    if (!bot)
        return "Bot is offline (use `.bot add <name>` first)";
    if (!master)
        return "No master session — can only be called by an in-world player";

    uint32 masterAccountId = master->GetSession()->GetAccountId();
    uint32 botAccount = sObjectMgr.GetPlayerAccountIdByGUID(bot->GetObjectGuid());
    const bool isMasterAccount = (masterAccountId == botAccount);
    const bool isRandomAccount = sPlayerbotAIConfig.IsInRandomAccountList(botAccount);

    // Security: only allow summoning bots on the master's account, random
    // bots, or bots that have explicitly accepted this master via group.
    bool allowed = isMasterAccount || isRandomAccount;
    if (!allowed)
    {
        PlayerbotAI* ai = GetPlayerbotAI(bot);
        if (ai && ai->GetMaster() == master)
            allowed = true;
    }
    if (!allowed)
        return "This bot isn't yours to summon.";

    // Block when in combat, BG, or instance — these usually mean the bot
    // is mid-fight or in restricted content. The master can manually
    // address those (e.g. wait for combat to end, leave the BG queue).
    if (bot->IsInCombat())
        return "Bot is in combat — wait for it to settle.";
    if (bot->InBattleGround() || bot->InBattleGroundQueue())
        return "Bot is in BG / BG queue.";
    if (bot->GetMap() && bot->GetMap()->Instanceable() && bot->GetMap() != master->GetMap())
        return "Bot is in an instance — can't pull cross-instance.";

    SC_LOG("HandleBotSummon teleport bot=%s -> master=%s map=%u pos=%.1f,%.1f,%.1f",
           bot->GetName(), master->GetName(),
           master->GetMapId(),
           master->GetPositionX(), master->GetPositionY(), master->GetPositionZ());

    bot->TeleportTo(master->GetMapId(),
                    master->GetPositionX(),
                    master->GetPositionY(),
                    master->GetPositionZ(),
                    master->GetOrientation());

    return "ok — teleporting to you";
}

std::string PlayerbotHolder::HandleBotRemoveLogout(Player* bot, Player* master, const std::string param)
{
    SC_LOG("HandleBotRemoveLogout entry bot=%s master=%s",
           bot ? bot->GetName() : "(null)",
           master ? master->GetName() : "(null)");

    if (!bot)
        return "Player is offline";

    uint32 guildId = Player::GetGuildIdFromDB(bot->GetObjectGuid());
    uint32 masterAccountId = master ? master->GetSession()->GetAccountId() : 0;
    uint32 masterGuildId = master ? master->GetGuildId() : 0;
    uint32 botAccount = sObjectMgr.GetPlayerAccountIdByGUID(bot->GetObjectGuid());
    bool isMasterAccount = (masterAccountId == botAccount);
    bool isRandomAccount = sPlayerbotAIConfig.IsInRandomAccountList(botAccount);

    SC_LOG("HandleBotRemoveLogout botAccount=%u masterAccount=%u isMaster=%d isRandom=%d",
           botAccount, masterAccountId, (int)isMasterAccount, (int)isRandomAccount);

    //:
    // Refuse `.rndbot remove` for random-account bots that aren't currently
    // master-linked to the player issuing the command. Symptom we're guarding
    // against: WorldSession::LogoutPlayer crashed during the group-cleanup
    // chain (between CleanupChannels and SMSG_LOGOUT_COMPLETE) for a bot
    // whose group/master pointers may have been left dangling by the
    // previous mangosd session. By requiring an explicit master-link, we
    // ensure the bot's state is "fresh" enough to logout safely. The user
    // can still acquire-then-remove via /invite ... /uninvite ... or via
    // the standard `.bot rm` path for master-account bots.
    if (isRandomAccount && master)
    {
        PlayerbotAI* botAi = GetPlayerbotAI(bot);
        if (!botAi || botAi->GetMaster() != master)
            return "This bot isn't bound to you. /invite first to acquire, then /uninvite + .rndbot remove.";
    }

    if (isRandomAccount)
        sRandomPlayerbotMgr.Remove(bot);
    else if (GetPlayerBot(bot->GetGUIDLow()))
        LogoutPlayerBot(bot->GetGUIDLow());
    else
        return "Not your bot";

    return "ok";
}

void PlayerbotHolder::CreateBot(Player* master, const std::string param, std::list<std::string>& messages, ObjectGuid& guid)
{    
    // Allow null master for RA/console usage
    // Player* master can be null when called via .rndbot commands

    std::string name;
    std::string testName;
    uint8 race = 0;
    uint8 cls = 0;
    uint32 level = 0;
    bool autoAdd = master;
    bool temporary = false;
    uint8 gender = GENDER_NONE;
    Team team = Team::TEAM_NONE;
    BotRoles role = BotRoles::BOT_ROLE_NONE;
    std::string groupWith = master ? master->GetName() : "";
    std::string gear = "default";

    std::vector<std::string> args = Qualified::getMultiQualifiers(param, " ");
    for (const auto& arg : args)
    {
        size_t eqPos = arg.find('=');
        if (eqPos == std::string::npos)
            continue;

        std::string key = arg.substr(0, eqPos);
        std::string value = arg.substr(eqPos + 1);

        if (key == "name")
            name = value;
        else if (key == "faction")
            team = ChatHelper::parseTeam(value);
        else if (key == "race")
            race = ChatHelper::parseRace(value);
        else if (key == "class")
            cls = ChatHelper::parseClass(value);
        else if (key == "gender")
            gender = ChatHelper::parseGender(value);
        else if (key == "level")
            level = std::stoul(value);
        else if (key == "role")
            role = ChatHelper::parseRole(value);
        else if (key == "login")
            autoAdd = (value == "1" || value == "true" || value == "yes");
        else if (key == "group")
            groupWith = value;
        else if (key == "gear")
            gear = value;
        else if (key == "test")
        {
            testName = value;
            autoAdd = true;
        }
        else if (key == "temporary")
            temporary = (value == "1" || value == "true" || value == "yes");
    }

    std::string error;
    uint32 accountId = GetOrCreateAccount(master, error);
    if (accountId == 0)
    {
        messages.push_back(error);
        return;
    }

    uint32 maxCharsPerAccount = 9;
#ifdef MANGOSBOT_TWO
    maxCharsPerAccount = 10;
#endif

    if (sAccountMgr.GetCharactersCount(accountId) >= maxCharsPerAccount)
    {
        messages.push_back("Account has max characters");
        return;
    }

    uint8 skin = 0, face = 0, hairStyle = 0, hairColor = 0, facialHair = 0;

    if (!name.empty())
    {
        auto result = CharacterDatabase.PQuery("SELECT guid FROM characters WHERE name = '%s'", name.c_str());
        if (result)
        {
            messages.push_back("Name already exists");
            return;
        }
    }

    if (team == TEAM_NONE && master)
        team = master->GetTeam();

    if (gender == GENDER_NONE)
        gender = urand(GENDER_MALE, GENDER_FEMALE);

    RandomPlayerbotFactory factory(0);

    if (cls == 0)
        cls = factory.GetRandomClass(race);

    if (race == 0)
    {
        race = factory.GetRandomRace(cls, team);
    }

    if (name.empty())
    {
        RandomPlayerbotFactory::NameRaceAndGender raceAndGender = RandomPlayerbotFactory::CombineRaceAndGender(gender, race);
        name = RandomPlayerbotFactory::CreateRandomBotName(raceAndGender);
    }

    // remote_ip MUST be "disconnected/bot" — see comment in HandlePlayerBotLoginCallback above.
    // Empty string makes PlayerbotAI::IsRealPlayer() return TRUE, breaking HandleTeleportAck.
    WorldSession* botSession = new WorldSession(accountId, NULL, SEC_PLAYER,
#ifdef MANGOSBOT_TWO
        2,
        0,
        LOCALE_enUS,
        "disconnected/bot",
        0,
        0,
        false);
#endif
#ifdef MANGOSBOT_ONE
        2, 0, LOCALE_enUS, "disconnected/bot", 0, 0, false);
#endif
#ifdef MANGOSBOT_ZERO
        0, LOCALE_enUS, "disconnected/bot", 0);
#endif

        // Null-socket Penqle sessions are constructed with NullSessionAnticheat.
        // Do not add the removed CMaNGOS SetNoAnticheat compatibility API.
        Player* newBot = new Player(botSession);
        if (!newBot->Create(sObjectMgr.GeneratePlayerLowGuid(), name, race, cls, gender, skin, face, hairStyle, hairColor, facialHair))
        {
            delete botSession;
            delete newBot;
            messages.push_back("Failed to create character");
            return;
        }

        newBot->SetCinematic(2);
        newBot->SetAtLoginFlag(AT_LOGIN_NONE);
        sObjectAccessor.AddObject(newBot);

        uint32 botGuid = newBot->GetGUIDLow();
        guid = newBot->GetObjectGuid();

        if (level > 1)
        {
            newBot->SetLevel(level);
            newBot->SetUInt32Value(PLAYER_XP, 0);
            newBot->InitStatsForLevel(true);
#ifdef MANGOSBOT_ZERO
            newBot->InitTaxiNodes();
#else
        newBot->InitTaxiNodesForLevel();
#endif
            newBot->InitTalentForLevel();
            // Player::Create already initialized the primary-profession allowance.
            newBot->LearnDefaultSpells();
            LearnPlayerbotClassLevelSpells(newBot);

            std::ostringstream out;
            ChangeTalentsAction::AutoSelectTalents(newBot, &out, role);

            sRandomPlayerbotMgr.SetValue(botGuid, "create levelup", 1);
            sRandomPlayerbotMgr.SetValue(botGuid, "create group", 1, groupWith);
            sRandomPlayerbotMgr.SetValue(botGuid, "create gear", 1, gear);
        }
        else
            newBot->SetLevel(1);

        if (!testName.empty())
        {
            CharacterDatabase.escape_string(testName);

            sRandomPlayerbotMgr.SetValue(botGuid, "test", 1, testName);
        }
        if (temporary)
        {
            sRandomPlayerbotMgr.SetValue(botGuid, "temporary", 1, name);
        }

        if (master)
        {
            newBot->SetMap(master->GetMap());
            newBot->SetPosition(master->GetPositionX(), master->GetPositionY(), master->GetPositionZ(), master->GetOrientation());
        }

        newBot->SaveToDB();

        messages.push_back("Bot created: " + name);

        botSession->LogoutPlayer(true);
        sObjectAccessor.RemoveObject(newBot);
        delete newBot;
        delete botSession;

        if (autoAdd)
        {
            sPlayerbotAIConfig.freeAltBots.push_back(std::make_pair(accountId, botGuid));
            messages.push_back("Bot is now online");
        }
        else
        {
            messages.push_back("Use '.rndbot add " + name + "' to bring this bot online");
        }

        return;
}

std::list<std::string> PlayerbotHolder::HandleCreate(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;
    ObjectGuid guid;

    CreateBot(master, param, messages, guid);

    return messages;
}

std::list<std::string> PlayerbotHolder::HandleGroup(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;

    if (!master)
    {
        messages.push_back("group command requires a master (in-game)");
        return messages;
    }

    uint32 masterLevel = master->GetLevel();
    uint8 masterClass = master->GetClass();
    Team team = master->GetTeam();
    BotRoles masterRole = AiFactory::GetPlayerRoles(master);
    uint8 groupSize = 5;
    uint8 currentGroupSize = 1;
    Group* group = master->GetGroup();
    if (group)
        currentGroupSize = group->GetMembersCount();

    std::string passThroughParam = "";

    std::vector<std::string> args = Qualified::getMultiQualifiers(param, " ");
    for (const auto& arg : args)
    {
        size_t eqPos = arg.find('=');
        if (eqPos == std::string::npos)
            continue;

        std::string key = arg.substr(0, eqPos);
        std::string value = arg.substr(eqPos + 1);

        if (key == "size" && Qualified::isValidNumberString(value))
            groupSize = stoi(value);
        else
            passThroughParam += key + "=" + value + " ";
    }
    
    std::unordered_map<uint8, std::unordered_map<BotRoles, uint32>> allowedClassNr = LfgAction::AllowedClassRoleNr(master, groupSize);

    RandomPlayerbotFactory factory(0);

    uint32 maxTries = 10*groupSize;

    uint32 botsCreated = 0;
    uint32 continue_role = 0, continue_race = 0, continue_class = 0;
    std::map<uint8, uint32> classesCreated;

    while (currentGroupSize < groupSize)
    {
        maxTries--;
        if (!maxTries)
            break;

        BotRoles role = BotRoles(urand(BotRoles::BOT_ROLE_TANK, BotRoles::BOT_ROLE_DPS));

        if (allowedClassNr[0][role] == 0)
        {
            continue_role++;
            continue;
        }

        uint8 cls = factory.GetRandomClass(0, role);

#ifdef MANGOSBOT_ZERO
        if (cls == CLASS_PALADIN && team == HORDE)
        {
            continue_race++;
            continue;
        }
        if (cls == CLASS_SHAMAN && team == ALLIANCE)
        {
            continue_race++;
            continue;
        }
#endif

        if (allowedClassNr[cls].find(role) != allowedClassNr[cls].end() && allowedClassNr[cls][role] == 0)
        {
            continue_class++;
            continue;
        }

        std::ostringstream paramStr;
        paramStr << "level=" << masterLevel << " class=" << ChatHelper::formatClass(cls) << " group=" << master->GetName() << " " << passThroughParam; //Passthrough will override.

        auto result = HandleCreate(master, paramStr.str(), security);
        messages.splice(messages.end(), result);

        if (!messages.empty())
        {
            auto lastMsg = messages.front();
            if (lastMsg.find("Bot created:") != std::string::npos)
            {
                classesCreated[cls]++;
                botsCreated++;
                currentGroupSize++;
            }
        }
    
        allowedClassNr[0][role]--; 
        
        if (allowedClassNr[cls].find(role) != allowedClassNr[cls].end())
            allowedClassNr[cls][role]--;
    }

    std::ostringstream debugInfo;
    debugInfo << "DEBUG group: target=" << (int)groupSize << ", created=" << botsCreated;
    if (maxTries == 0)
        debugInfo << " (maxTries exhausted)";
    debugInfo << ", continues: role=" << continue_role << ", race=" << continue_race << ", class=" << continue_class;
    debugInfo << ", classes: ";
    for (auto& kv : classesCreated)
        debugInfo << ChatHelper::formatClass(kv.first) << "=" << kv.second << ",";
    sLog.outString("%s", debugInfo.str().c_str());

    return messages;
}

#ifdef GenerateBotTests
std::list<std::string> PlayerbotHolder::HandleRunTest(Player* master, const std::string param, AccountTypes security)
{    
    std::list<std::string> messages;

    if (param.empty())
    {
        messages.push_back("Usage: .rndbot runtest <testnamepart> [count]");
        messages.push_back("Available tests:");
        std::vector<std::string> availableTests = TestRegistry::GetAvailableTests();
        for (const auto& test : availableTests)
            messages.push_back("  " + test);
        return messages;
    }

    std::string testNamePart = param;
    size_t maxTests = 0;

    size_t lastSpace = param.find_last_of(" \t");
    if (lastSpace != std::string::npos)
    {
        std::string maybeLimit = param.substr(lastSpace + 1);
        if (!maybeLimit.empty())
        {
            bool numericLimit = std::all_of(maybeLimit.begin(), maybeLimit.end(), ::isdigit);
            if (numericLimit)
            {
                try
                {
                    maxTests = std::stoul(maybeLimit);
                }
                catch (...)
                {
                    messages.push_back("Invalid count '" + maybeLimit + "'");
                    return messages;
                }

                if (!maxTests)
                {
                    messages.push_back("Count must be greater than 0");
                    return messages;
                }

                testNamePart = param.substr(0, lastSpace);
                size_t nameEnd = testNamePart.find_last_not_of(" \t");
                testNamePart = (nameEnd == std::string::npos) ? "" : testNamePart.substr(0, nameEnd + 1);
            }
        }
    }

    if (testNamePart.empty())
    {
        messages.push_back("Usage: .rndbot runtest <testnamepart> [count]");
        return messages;
    }

    bool listTests = false;

    if (testNamePart[0] == '?')
    {
        listTests = true;
        testNamePart = testNamePart.substr(2);
    }

    std::transform(testNamePart.begin(), testNamePart.end(), testNamePart.begin(), ::tolower);

    std::vector<std::string> matchingTests;
    std::vector<std::string> allTests = TestRegistry::GetAvailableTests();
    for (const auto& test : allTests)
    {
        std::string lowerTest = test;
        std::transform(lowerTest.begin(), lowerTest.end(), lowerTest.begin(), ::tolower);
        if (lowerTest.find(testNamePart) != std::string::npos || testNamePart == "*")
        {
            matchingTests.push_back(test);
            if (maxTests && matchingTests.size() >= maxTests)
                break;
        }
    }

    if (matchingTests.empty())
    {
        messages.push_back("No tests matching '" + param + "' found");
        return messages;
    }

    if (listTests)
    {
        messages.push_back("Tests matching '" + param + "':");
        for (const auto& test : matchingTests)
            messages.push_back("  " + test);
        return messages;
    }

    {
        std::lock_guard<std::mutex> lock(testResultsMutex);
        for (const auto& test : matchingTests)
        {
            PendingTest pt;
            pt.testName = test;
            pt.result = "";
            pt.pending = false;
            pt.completed = false;
            pt.expectedBotSpawnCount = TestRegistry::ExpectedBotSpawnCount(test);
            pt.retry = 0;
            pendingTests.push_back(pt);
        }
    }

    std::ostringstream out;
    out << "Queued " << matchingTests.size() << " test(s): ";
    for (size_t i = 0; i < matchingTests.size() && i < 3; i++)
        out << matchingTests[i] << (i < matchingTests.size() - 1 && i < 2 ? ", " : "");
    if (matchingTests.size() > 3)
        out << "...";
    messages.push_back(out.str());

    return messages;
}

void PlayerbotHolder::UpdatePendingTests(uint32 elapsed)
{
    std::lock_guard<std::mutex> lock(testResultsMutex);

    for (auto& pt : pendingTests)
    {
        if (pt.pending)
            continue;

        if (pt.completed)
            continue;

        if (!TestRegistry::HasTest(pt.testName))
        {
            pt.result = "Test not found";
            pt.completed = true;
            continue;
        }

        if (dynamic_cast<PlayerbotMgr*>(this))
        {
            uint32 maxCharsPerAccount = 9;
#ifdef MANGOSBOT_TWO
            maxCharsPerAccount = 10;
#endif
            uint32 accountId = sObjectMgr.GetPlayerAccountIdByGUID((dynamic_cast<PlayerbotMgr*>(this))->GetMaster()->GetObjectGuid());
                if (accountId == 0) continue;

            uint32 currentChars = sAccountMgr.GetCharactersCount(accountId);
            if (currentChars >= maxCharsPerAccount)
                continue;
        }

        static constexpr uint32 maxActiveTestBots = 50;
        uint32 activeTestBots = 0;
        for (const auto& test : pendingTests)
        {
            if (test.pending && !test.completed)
                activeTestBots += std::max<uint32>(1, test.expectedBotSpawnCount);
        }

        uint32 newTestBotCount = std::max<uint32>(1, pt.expectedBotSpawnCount);
        if (activeTestBots + newTestBotCount >= maxActiveTestBots)
            continue;

        std::string createParams = TestRegistry::GetBotCreationRequirement(pt.testName);

        createParams += " login=0 temporary=1 test=" + pt.testName;       

        std::list<std::string> createMsgs = HandleCreate(nullptr, createParams, SEC_PLAYER);

        pt.pending = true;
    }
}

void PlayerbotHolder::DepositTestResult(const std::string& testName, const std::string& result)
{
    std::lock_guard<std::mutex> lock(testResultsMutex);

    for (auto& pt : pendingTests)
    {
        if (!pt.pending)
            continue;

        if (pt.testName == testName && !pt.completed)
        {
            pt.pending = false;
            if (result == "ABORT") //Failed this time but might work next time.
            {
                pt.result = result;
                pt.retry++;
                break;
            }

            pt.result = result;
            pt.completed = true;
            testResults.push_back(pt);
            break;
        }
    }
}
#endif

uint32 PlayerbotHolder::GetOrCreateAccount(Player* master, std::string& error)
{
    if (!master)
    {
        // For console/RA usage without master - delegate to derived class
        error = "GetOrCreateAccount requires master or override in derived class";
        return 0;
    }
    
    uint32 masterAccountId = master->GetSession()->GetAccountId();
    return masterAccountId;
}

void PlayerbotHolder::OnBotDeleted(uint32 botGuid, uint32 accountId)
{
}

bool PlayerbotHolder::DeleteBot(ObjectGuid guid, bool allowInstant)
{
    uint32 botAccount = sObjectMgr.GetPlayerAccountIdByGUID(guid);

    if (Player* player = sObjectMgr.GetPlayer(guid))
    {
        //Attempt instant logout.
        player->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING); 
        LogoutPlayerBot(guid, allowInstant, true);
    }

    Player::DeleteFromDB(guid, botAccount, true, true);

    OnBotDeleted(guid, botAccount);

    return true;
}

std::string PlayerbotHolder::HandleBotDelete(Player* bot, Player* master, const std::string param)
{
    ObjectGuid guid;
    if (!bot)
    {
        if (!Qualified::isValidNumberString(param))
            return "Add: Error parsing " + param;

        guid = ObjectGuid(uint64(std::stoull(param)));
    }
    else
    {
        guid = bot->GetObjectGuid();
    }

    uint32 masterAccountId = master ? master->GetSession()->GetAccountId() : 0;
    PlayerbotMgr* mgr = master ? GetPlayerbotMgr(master) : nullptr;
    
    uint32 botAccount = sObjectMgr.GetPlayerAccountIdByGUID(guid);
    bool isRandomAccount = sPlayerbotAIConfig.IsInRandomAccountList(botAccount);

    if (!isRandomAccount && masterAccountId != botAccount)
        return "Not your bot";

    if (isRandomAccount && mgr == this)
        return "Not your bot";

    DeleteBot(guid);

    return "ok";
}

std::string PlayerbotHolder::HandleBotGear(Player* bot, Player* master, const std::string param)
{
    if (param.empty())
    {
        PlayerbotFactory factory(bot, bot->GetLevel());
        factory.EquipGear();
        return "random gear equipped";
    }
    if (param == "green" || param == "uncommon")
    {
        PlayerbotFactory factory(bot, bot->GetLevel(), ITEM_QUALITY_UNCOMMON);
        factory.EquipGear();
        return "random green gear equipped";
    }
    if (param == "blue" || param == "rare")
    {
        PlayerbotFactory factory(bot, bot->GetLevel(), ITEM_QUALITY_RARE);
        factory.EquipGear();
        return "random blue gear equipped";
    }
    if (param == "purple" || param == "epic")
    {
        PlayerbotFactory factory(bot, bot->GetLevel(), ITEM_QUALITY_EPIC);
        factory.EquipGear();
        return "random epic gear equipped";
    }
    if (param == "upgrade")
    {
        PlayerbotFactory factory(bot, master ? master->GetLevel() : bot->GetLevel(), ITEM_QUALITY_NORMAL);
        factory.UpgradeGear(false);
        return "gear upgraded";
    }
    if (param == "sync")
    {
        PlayerbotFactory factory(bot, master ? master->GetLevel() : bot->GetLevel(), ITEM_QUALITY_NORMAL);
        factory.UpgradeGear(true);
        return "gear upgraded";
    }
    if (param == "best")
    {
        PlayerbotFactory factory(bot, bot->GetLevel());
        factory.EquipGearBest();
        return "random best gear equipped";
    }
    if (param == "partial")
    {
        PlayerbotFactory factory(bot, bot->GetLevel());
        factory.EquipGearPartialUpgrade();
        return "random gear upgraded to some slots";
    }

    return "unknown gear command";
}

std::string PlayerbotHolder::HandleBotTrainLearn(Player* bot, Player* master, const std::string param)
{
#ifndef MANGOSBOT_ONE
    LearnPlayerbotClassLevelSpells(bot);
#endif
    return "class level spells learned";
}

std::string PlayerbotHolder::HandleBotFoodDrink(Player* bot, Player* master, const std::string param)
{
    uint32 level = master ? master->GetLevel() : bot->GetLevel();
    PlayerbotFactory factory(bot, level, ITEM_QUALITY_NORMAL);
    factory.AddFood();
    return "food added";
}

std::string PlayerbotHolder::HandleBotPotions(Player* bot, Player* master, const std::string param)
{
    uint32 level = master ? master->GetLevel() : bot->GetLevel();
    PlayerbotFactory factory(bot, level, ITEM_QUALITY_NORMAL);
    factory.AddPotions();
    return "potions added";
}

std::string PlayerbotHolder::HandleBotConsumes(Player* bot, Player* master, const std::string param)
{
    uint32 level = master ? master->GetLevel() : bot->GetLevel();
    PlayerbotFactory factory(bot, level, ITEM_QUALITY_NORMAL);
    factory.AddConsumes();
    return "consumables added";
}

std::string PlayerbotHolder::HandleBotReagents(Player* bot, Player* master, const std::string param)
{
    uint32 level = master ? master->GetLevel() : bot->GetLevel();
    PlayerbotFactory factory(bot, level, ITEM_QUALITY_NORMAL);
    factory.AddReagents();
    return "reagents added";
}

std::string PlayerbotHolder::HandleBotPrepare(Player* bot, Player* master, const std::string param)
{
    uint32 level = master ? master->GetLevel() : bot->GetLevel();
    PlayerbotFactory factory(bot, level, ITEM_QUALITY_NORMAL);
    factory.Refresh();
    return "consumes/regs added";
}

std::string PlayerbotHolder::HandleBotInit(Player* bot, Player* master, const std::string param)
{
    uint32 level = master ? master->GetLevel() : bot->GetLevel();

    if (param.empty())
    {
        PlayerbotFactory factory(bot, level, ITEM_QUALITY_NORMAL);
        factory.Randomize(true, false);
    }
    else if (param == "white" || param == "common")
    {
        PlayerbotFactory factory(bot, level, ITEM_QUALITY_NORMAL);
        factory.Randomize(false, false);
    }
    else if (param == "green" || param == "uncommon")
    {
        PlayerbotFactory factory(bot, level, ITEM_QUALITY_UNCOMMON);
        factory.Randomize(false, false);
    }
    else if (param == "blue" || param == "rare")
    {
        PlayerbotFactory factory(bot, level, ITEM_QUALITY_RARE);
        factory.Randomize(false, false);
    }
    else if (param == "epic" || param == "purple")
    {
        PlayerbotFactory factory(bot, level, ITEM_QUALITY_EPIC);
        factory.Randomize(false, false);
    }
    else if (param == "legendary" || param == "yellow")
    {
        PlayerbotFactory factory(bot, level, ITEM_QUALITY_LEGENDARY);
        factory.Randomize(false, false);
    }
    else if (param == "sync")
    {
        PlayerbotFactory factory(bot, level, ITEM_QUALITY_LEGENDARY);
        factory.Randomize(false, true);
    }

    return "ok";
}

std::string PlayerbotHolder::HandleBotEnchants(Player* bot, Player* master, const std::string param)
{
    PlayerbotFactory factory(bot, bot->GetLevel(), ITEM_QUALITY_LEGENDARY);
    factory.EnchantEquipment();
    return "ok";
}

std::string PlayerbotHolder::HandleBotAmmo(Player* bot, Player* master, const std::string param)
{
    PlayerbotFactory factory(bot, bot->GetLevel(), ITEM_QUALITY_LEGENDARY);
    factory.InitAmmo();
    return "ok";
}

std::string PlayerbotHolder::HandleBotPet(Player* bot, Player* master, const std::string param)
{
    PlayerbotFactory factory(bot, bot->GetLevel(), ITEM_QUALITY_LEGENDARY);
    factory.InitPet();
    factory.InitPetSpells();
    return "ok";
}

std::string PlayerbotHolder::HandleBotLevelUp(Player* bot, Player* master, const std::string param)
{
    PlayerbotFactory factory(bot, bot->GetLevel());
    factory.Randomize(true, false);
    return "ok";
}

std::string PlayerbotHolder::HandleBotRefresh(Player* bot, Player* master, const std::string param)
{
    PlayerbotFactory factory(bot, bot->GetLevel());
    factory.Refresh();
    return "ok";
}

std::string PlayerbotHolder::HandleBotRandom(Player* bot, Player* master, const std::string param)
{
    sRandomPlayerbotMgr.Randomize(bot);
    return "ok";
}

std::string PlayerbotHolder::GetCommandTexts(const std::string& command)
{
    auto texts = GetCommandTexts();
    auto it = texts.find(command);
    if (it != texts.end())
        return it->second;
    return "";
}

std::unordered_map<std::string, std::string> PlayerbotHolder::GetCommandTexts()
{
    return std::unordered_map<std::string, std::string>
    {
        // Holder commands (used with .(rnd)bot)
        {"list", "List all active player bots.\nUsage: .(rnd)bot list"},
        {"help", "Show help for commands.\nUsage: .(rnd)bot help <command>"},
        {"reload", "Reload the playerbot config (developer rank and above).\nUsage: .(rnd)bot reload"},
        {"tweak", "Adjust the tweak value for testing (developer rank and above).\nUsage: .(rnd)bot tweak"},
        {"self", "Enable self-bot mode for a player.\nUsage: .(rnd)bot self <playername>"},
        {"group", "Create 4 bots with complementary classes at master's level.\nUsage: .(rnd)bot group"},
        {"create", "Create a new bot character.\nUsage: .(rnd)bot create level=<n> class=<class> race=<race>"},
        {"spoof", "Spoof as another bot for command routing.\nUsage: .(rnd)bot spoof <botname>"},
        {"runtest", "Run bot tests.\nUsage: .rndbot runtest <testnamepart> [count]"},
        
        // Bot commands (used with .(rnd)bot <bot> ...)
        {"add", "Add a bot to the player's group.\nUsage: .(rnd)bot add <playername>"},
        {"login", "Add a bot to the player's group.\nUsage: .(rnd)bot login <playername>"},
        {"remove", "Remove a bot from the player's group.\nUsage: .(rnd)bot remove <botname>"},
        {"logout", "Remove a bot from the player's group.\nUsage: .(rnd)bot logout <botname>"},
        {"rm", "Remove a bot from the player's group.\nUsage: .(rnd)bot rm <botname>"},
        {"delete", "Delete a bot character.\nUsage: .(rnd)bot delete <botname>"},
        
        {"gear", "Equip best gear on bot.\nUsage: .(rnd)bot gear <bot> "},
        {"equip", "Equip best gear on bot.\nUsage: .(rnd)bot equip  <bot> "},
        
        {"train", "Train bot spells at trainer.\nUsage: .(rnd)bot train <bot> "},
        {"learn", "Train bot spells at trainer.\nUsage: .(rnd)bot learn <bot> "},
        
        {"food", "Buy food/drink for bot.\nUsage: .(rnd)bot food <bot> "},
        {"drink", "Buy food/drink for bot.\nUsage: .(rnd)bot drink <bot> "},
        
        {"potions", "Buy potions for bot.\nUsage: .(rnd)bot potions <bot> "},
        {"pots", "Buy potions for bot.\nUsage: .(rnd)bot pots <bot> "},
        
        {"consumes", "Buy all consumables for bot.\nUsage: .(rnd)bot consumes <bot> "},
        {"consumables", "Buy all consumables for bot.\nUsage: .(rnd)bot consumables <bot> "},
        
        {"regs", "Buy reagents for bot.\nUsage: .(rnd)bot regs <bot> "},
        {"reg", "Buy reagents for bot.\nUsage: .(rnd)bot reg <bot> "},
        {"reagents", "Buy reagents for bot.\nUsage: .(rnd)bot reagents  <bot> "},
        
        {"prepare", "Prepare bot (gear, food, pots, etc).\nUsage: .(rnd)bot prepare <bot> "},
        {"prep", "Prepare bot (gear, food, pots, etc).\nUsage: .(rnd)bot prep <bot>"},
        {"refresh", "Refresh bot gear and items.\nUsage: .(rnd)bot refresh <bot> "},
        
        {"init", "Initialize bot with default actions.\nUsage: .(rnd)bot init <bot> "},
        
        {"enchants", "Apply enchants to bot's gear.\nUsage: .(rnd)bot enchants <bot> "},
        
        {"ammo", "Buy ammo for bot.\nUsage: .(rnd)bot ammo <bot> "},
        
        {"pet", "Summon/dismiss pet for bot.\nUsage: .(rnd)bot pet <bot> "},
        
        {"levelup", "Level up bot.\nUsage: .(rnd)bot levelup <bot>"},
        {"level", "Level up bot.\nUsage: .(rnd)bot level <bot>"},
        
        {"random", "Randomize bot appearance and gear.\nUsage: .(rnd)bot random <bot>"},
        
        {"always", "Enable offline AI for a player.\nUsage: .(rnd)bot always <playername>"},
        
        {"debug", "Run debug commands on the bot (developer rank and above).\nUsage: .(rnd)bot debug <bot> <command>"},
        
        {"c", "Execute a privileged chat command on the bot (moderator rank and above).\nUsage: .(rnd)bot c <bot> <command>"},
        
        {"w", "Send a whisper.\nUsage: .(rnd)bot w <bot> <message> (while spoofing as sender)\nUsage: .(rnd)bot <sender> <reciever> "},
        
        {"p", "Send a party message as the bot.\nUsage: .(rnd)bot p <message> (while spoofing as sender)\n .(rnd)bot p <botname> <message>\nNote: No message = party info.\nExample: .rndbot p Dunpriest (shows party info)"},
        
        {"g", "Send a guild message as the bot.\nUsage: .(rnd)bot g <message> (while spoofing as sender)\n .(rnd)bot g <botname> <message>\nNote: No message = guild info.\nExample: .rndbot g Dunpriest (shows guild info)"},
        
        {"r", "Send a raid message as the bot.\nUsage: .(rnd)bot r <message> (while spoofing as sender)\n .(rnd)bot r <botname> <message>"},
        
        {"rl", "Transfer raid leadership.\nUsage: .(rnd)bot rl <message> (while spoofing as sender)\n .(rnd)bot rl <botname>"},
        
        {"do", "Execute a bot action (sync, immediate response).\nUsage: .(rnd)bot do <bot> <action>\nExample: .(rnd)bot do <bot> stats, where, quests, who"},
        
        {"cmd", "Execute a bot action (async, queued).\nUsage: .(rnd)bot cmd <bot> do <action>\nNote: Use with record to capture output."},
        
        {"record", "Enable message recording for async commands.\nUsage: .(rnd)bot record <bot> enable\nUsage: .(rnd)bot record <bot> disable"},
        
        {"read", "Get recorded async command output.\nUsage: .(rnd)bot read <bot>"},
        
        {"clear", "Clear recorded messages without retrieving.\nUsage: .(rnd)bot clear <bot>"},
        
        {"spoof", "Spoof as another bot for command routing.\nUsage: .(rnd)bot spoof <botname>\nUsage: .(rnd)bot spoof (to clear)"}
    };
}

std::list<std::string> PlayerbotHolder::HandleSpoof(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;
    
    if (param.empty())
    {
        // Clear the spoof
        if (m_spoofGuid)
        {
            std::string playerName;
            if (sObjectMgr.GetPlayerNameByGUID(m_spoofGuid, playerName))
            {
                messages.push_back("Spoof cleared. Was spoofing: " + playerName);
            }
            else
            {
                messages.push_back("Spoof cleared.");
            }
            m_spoofGuid = ObjectGuid();
        }
        else
        {
            messages.push_back("Spoof is not set.");
        }
        return messages;
    }
    
    // Look up player by name
    ObjectGuid guid = sObjectMgr.GetPlayerGuidByName(param);
    if (!guid)
    {
        messages.push_back("Player '" + param + "' not found.");
        return messages;
    }
    
    // Get the player to verify they exist
    Player* player = sObjectMgr.GetPlayer(guid);
    if (!player)
    {
        messages.push_back("Player '" + param + "' found but is not online.");
        return messages;
    }
    
    std::string playerName;
    sObjectMgr.GetPlayerNameByGUID(guid, playerName);
    m_spoofGuid = guid;
    
    messages.push_back("Spoof set to: " + playerName + " (" + std::to_string(guid.GetCounter()) + ")");
    return messages;
}
