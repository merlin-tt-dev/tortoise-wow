// Playerbot host integration for Tortoise's native module/script framework.
//
// No Player/core fields are added. Historical PlayerbotAI/PlayerbotMgr ownership
// stays inside this module, while Penqle's native PlayerBots session mechanism is
// used only as a bootstrap for the standard character-login pipeline.

#include "playerbot/playerbot.h"
#include "playerbot/PlayerbotHost.h"
#include "Objects/Player.h"
#include "MapNodes/MasterPlayer.h"
#include "ScriptObjects.h"
#include "WorldSession.h"
#include "AccountMgr.h"
#include "PlayerBots/PlayerBotAI.h"
#include "PlayerBots/PlayerBotMgr.h"
#include "playerbot/RandomPlayerbotMgr.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "playerbot/PerformanceMonitor.h"
#include "ahbot/AhBot.h"
#include "BotDiagnostics.h"
#include "Protocol/Opcodes.h"
#include "Group/Group.h"

#include <array>
#include <deque>
#include <memory>
#include <mutex>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
std::unordered_map<Player*, std::unique_ptr<PlayerbotAI>> s_playerbotAIs;
std::unordered_map<Player*, std::unique_ptr<PlayerbotMgr>> s_playerbotMgrs;
std::unordered_set<PlayerbotHolder*> s_liveHolders;

std::mutex s_observedLfgMutex;
std::map<ObjectGuid, uint32> s_observedRealPlayerLfgAreas;

// PlayerScript::OnBeforeSendChatMessage intentionally exposes only the message
// metadata, not the whisper recipient. SERVERHOOK_CAN_PACKET_RECEIVE runs
// synchronously immediately before the opcode handler, so remember the target
// from the current CMSG_MESSAGECHAT packet and consume it from the validated
// PlayerScript hook below. The next chat packet overwrites any stale value if
// validation rejects the current line before the PlayerScript hook fires.
std::mutex s_observedChatTargetMutex;
std::unordered_map<WorldSession*, std::string> s_observedChatTargets;

void ObserveCurrentChatTarget(WorldSession* session, WorldPacket const& packet)
{
    if (!session || packet.GetOpcode() != CMSG_MESSAGECHAT)
        return;

    std::string target;
    WorldPacket copy(packet);
    copy.rpos(0);

    if (copy.size() >= sizeof(uint32) * 2)
    {
        uint32 type = 0;
        uint32 language = 0;
        copy >> type >> language;
        (void)language;

        if (type == CHAT_MSG_WHISPER)
        {
            copy >> target;
            // Match the canonical Player name stored on the bot. If the raw
            // client spelling is invalid, leave it untouched and let the core
            // reject the whisper as usual.
            std::string normalized = target;
            if (normalizePlayerName(normalized))
                target = normalized;
        }
    }

    std::lock_guard<std::mutex> lock(s_observedChatTargetMutex);
    s_observedChatTargets[session] = std::move(target);
}

std::string ConsumeCurrentChatTarget(Player* player)
{
    WorldSession* session = player ? player->GetSession() : nullptr;
    if (!session)
        return {};

    std::lock_guard<std::mutex> lock(s_observedChatTargetMutex);
    auto it = s_observedChatTargets.find(session);
    if (it == s_observedChatTargets.end())
        return {};

    std::string target = std::move(it->second);
    s_observedChatTargets.erase(it);
    return target;
}

void ClearObservedChatTarget(Player* player)
{
    WorldSession* session = player ? player->GetSession() : nullptr;
    if (!session)
        return;

    std::lock_guard<std::mutex> lock(s_observedChatTargetMutex);
    s_observedChatTargets.erase(session);
}

std::mutex s_targetIconMutex;
using TargetIconCacheKey = std::pair<uint32, uint64>;
std::map<TargetIconCacheKey, std::array<ObjectGuid, TARGET_ICON_COUNT>> s_targetIcons;

TargetIconCacheKey GetTargetIconCacheKey(Group const* group)
{
    return group ? TargetIconCacheKey(group->GetId(), group->GetLeaderGuid().GetRawValue()) : TargetIconCacheKey();
}

void ObserveTargetIconPacket(Player* player, WorldPacket const& packet)
{
    if (!player || packet.GetOpcode() != MSG_RAID_TARGET_UPDATE)
        return;

    Group* group = player->GetGroup();
    if (!group)
        group = player->GetOriginalGroup();
    if (!group)
        return;

    WorldPacket copy(packet);
    copy.rpos(0);
    if (copy.size() < 1)
        return;

    uint8 fullList = 0;
    copy >> fullList;

    std::lock_guard<std::mutex> lock(s_targetIconMutex);
    auto& icons = s_targetIcons[GetTargetIconCacheKey(group)];

    if (fullList == 1)
    {
        icons.fill(ObjectGuid());
        while (copy.size() - copy.rpos() >= 9)
        {
            uint8 index = 0;
            ObjectGuid guid;
            copy >> index >> guid;
            if (index < TARGET_ICON_COUNT)
                icons[index] = guid;
        }
        return;
    }

    if (fullList == 0 && copy.size() - copy.rpos() >= 9)
    {
        uint8 index = 0;
        ObjectGuid guid;
        copy >> index >> guid;
        if (index < TARGET_ICON_COUNT)
            icons[index] = guid;
    }
}

void ObserveRealPlayerLfgArea(Player* player, uint32 areaId)
{
    if (!player || !areaId)
        return;

    std::lock_guard<std::mutex> lock(s_observedLfgMutex);
    s_observedRealPlayerLfgAreas[player->GetObjectGuid()] = areaId;
}

struct QueuedPlayerbotPacket
{
    std::uintptr_t sessionToken = 0;
    std::unique_ptr<WorldPacket> packet;
};

std::mutex s_playerbotPacketMutex;
std::unordered_map<uint32, std::deque<QueuedPlayerbotPacket>> s_playerbotIncomingPackets;

struct LoginBootstrap;
std::unordered_map<uint32, std::unique_ptr<LoginBootstrap>> s_loginBootstraps;
std::vector<WorldSession*> s_orphanSessions;

class NativeLoginBootstrapAI final : public ::PlayerBotAI
{
public:
    NativeLoginBootstrapAI() : ::PlayerBotAI(nullptr) {}

    // Penqle invokes this when Player::AddToWorld fires. The real handoff is
    // intentionally deferred to PlayerScript::OnLogin, which runs at the very
    // end of WorldSession::HandlePlayerLogin. Moving the session earlier would
    // invalidate the remainder of Penqle's login routine.
    void OnPlayerLogin() override {}
    void UpdateAI(uint32 const /*diff*/) override {}
};

struct LoginBootstrap
{
    explicit LoginBootstrap(PlayerbotHolder* owner, uint32 realAccount, uint32 syntheticAccount, uint32 guid)
        : holder(owner), realAccountId(realAccount), syntheticAccountId(syntheticAccount), guidLow(guid),
          entry(guid, syntheticAccount, 100), ai(std::make_unique<NativeLoginBootstrapAI>())
    {
        entry.customBot = true;
        entry.state = PB_STATE_LOADING;
        entry.ai = ai.get();
        ai->botEntry = &entry;
    }

    PlayerbotHolder* holder;
    uint32 realAccountId;
    uint32 syntheticAccountId;
    uint32 guidLow;
    PlayerBotEntry entry;
    std::unique_ptr<NativeLoginBootstrapAI> ai;
    WorldSession* bootstrapSession = nullptr;
    bool loginStarted = false;
};

bool CompleteNativeLoginBootstrap(Player* player)
{
    if (!player || !player->GetSession())
        return false;

    WorldSession* bootstrapSession = player->GetSession();
    auto it = s_loginBootstraps.find(bootstrapSession->GetAccountId());
    if (it == s_loginBootstraps.end() || it->second->bootstrapSession != bootstrapSession)
        return false;

    LoginBootstrap& pending = *it->second;
    uint32 const realAccountId = pending.realAccountId;
    PlayerbotHolder* holder = pending.holder;

    // The bootstrap session has now completed Penqle's entire standard login.
    // Replace it with the historical free bot session before any later tick can
    // save the character under the synthetic account id.
    WorldSession* botSession = new WorldSession(realAccountId, nullptr,
        sAccountMgr.GetSecurity(realAccountId), 0, bootstrapSession->GetSessionDbcLocale(), "<BOT>", 0);

    // MasterPlayer is session-owned and contains the fully loaded social/mail/
    // action state. Transfer it rather than rebuilding a partial session.
    MasterPlayer* masterPlayer = bootstrapSession->GetMasterPlayer();
    if (masterPlayer)
    {
        bootstrapSession->SetMasterPlayer(nullptr);
        masterPlayer->SetSession(botSession);
        botSession->SetMasterPlayer(masterPlayer);
    }

    bootstrapSession->SetPlayer(nullptr);
    bootstrapSession->SetBot(nullptr);

    botSession->SetPlayer(player);
    player->SetSession(botSession);

    // Penqle records the active mover in WorldSession at the end of login.
    // Re-establish it through the public packet handler on the replacement
    // session so movement validation retains identical semantics.
    WorldPacket activeMover(CMSG_SET_ACTIVE_MOVER, 8);
    activeMover << player->GetObjectGuid();
    botSession->HandleSetActiveMoverOpcode(activeMover);

    // Detach Penqle's temporary bootstrap PlayerAI before destroying its entry.
    player->setAI(nullptr);

    // The holder can disappear while the asynchronous login is in flight (for
    // example when a real master logs out). Never dereference a stale holder.
    if (s_liveHolders.find(holder) != s_liveHolders.end())
    {
        holder->OnBotLogin(player);
    }
    else
    {
        sLog.outError("[PlayerBots] login completed for guid %u after its holder disappeared; scheduling clean logout", pending.guidLow);
        s_orphanSessions.push_back(botSession);
    }

    s_loginBootstraps.erase(it);
    return true;
}

void UpdateLoginBootstraps()
{
    for (auto& [syntheticAccountId, pendingPtr] : s_loginBootstraps)
    {
        LoginBootstrap& pending = *pendingPtr;
        if (pending.loginStarted)
            continue;

        WorldSession* session = sWorld.FindSession(syntheticAccountId);
        if (!session || session != pending.bootstrapSession)
            continue;

        pending.loginStarted = true;
        pending.entry.state = PB_STATE_ONLINE;
        session->LoginPlayer(ObjectGuid(HIGHGUID_PLAYER, pending.guidLow));
    }

    if (!s_orphanSessions.empty())
    {
        std::vector<WorldSession*> sessions;
        sessions.swap(s_orphanSessions);
        for (WorldSession* session : sessions)
        {
            if (!session)
                continue;
            if (session->GetPlayer())
                session->LogoutPlayer(true);
            delete session;
        }
    }
}
} // namespace

PlayerbotAI* GetPlayerbotAI(Player* player)
{
    if (!player)
        return nullptr;
    auto it = s_playerbotAIs.find(player);
    return it == s_playerbotAIs.end() ? nullptr : it->second.get();
}

PlayerbotAI* GetPlayerbotAI(Player const* player)
{
    return GetPlayerbotAI(const_cast<Player*>(player));
}

PlayerbotMgr* GetPlayerbotMgr(Player* player)
{
    if (!player)
        return nullptr;
    auto it = s_playerbotMgrs.find(player);
    return it == s_playerbotMgrs.end() ? nullptr : it->second.get();
}

PlayerbotMgr* GetPlayerbotMgr(Player const* player)
{
    return GetPlayerbotMgr(const_cast<Player*>(player));
}

void CreatePlayerbotAI(Player* player)
{
    if (player && !GetPlayerbotAI(player))
        s_playerbotAIs.emplace(player, std::make_unique<PlayerbotAI>(player));
}

void RemovePlayerbotAI(Player* player)
{
    if (player)
    {
        ClearPlayerbotPackets(player);
        s_playerbotAIs.erase(player);
    }
}

void CreatePlayerbotMgr(Player* player)
{
    if (player && !GetPlayerbotMgr(player))
        s_playerbotMgrs.emplace(player, std::make_unique<PlayerbotMgr>(player));
}

void RemovePlayerbotMgr(Player* player)
{
    if (!player)
        return;

    auto it = s_playerbotMgrs.find(player);
    if (it == s_playerbotMgrs.end())
        return;

    it->second->LogoutAllBots();
    s_playerbotMgrs.erase(it);
}

bool IsRealPlayer(Player const* player)
{
    WorldSession* session = player ? player->GetSession() : nullptr;
    if (!session)
        return false;

    std::string const& address = session->GetRemoteAddress();
    return address != "<BOT>" && address != "disconnected/bot";
}

uint32 GetObservedRealPlayerLfgArea(Player const* player)
{
    if (!player)
        return 0;

    std::lock_guard<std::mutex> lock(s_observedLfgMutex);
    auto const it = s_observedRealPlayerLfgAreas.find(player->GetObjectGuid());
    return it == s_observedRealPlayerLfgAreas.end() ? 0 : it->second;
}

void ClearObservedRealPlayerLfgArea(Player const* player)
{
    if (!player)
        return;

    std::lock_guard<std::mutex> lock(s_observedLfgMutex);
    s_observedRealPlayerLfgAreas.erase(player->GetObjectGuid());
}

ObjectGuid GetPlayerbotTargetIcon(Group const* group, uint8 index)
{
    if (!group || index >= TARGET_ICON_COUNT)
        return ObjectGuid();

    std::lock_guard<std::mutex> lock(s_targetIconMutex);
    auto const it = s_targetIcons.find(GetTargetIconCacheKey(group));
    return it == s_targetIcons.end() ? ObjectGuid() : it->second[index];
}

void QueueDelayedPlayerbotPacket(uint32 guidLow, std::uintptr_t sessionToken, std::unique_ptr<WorldPacket> packet)
{
    if (!guidLow || !sessionToken || !packet)
        return;

    std::lock_guard<std::mutex> lock(s_playerbotPacketMutex);
    s_playerbotIncomingPackets[guidLow].push_back({ sessionToken, std::move(packet) });
}

void QueuePlayerbotPacket(Player* player, std::unique_ptr<WorldPacket> packet)
{
    if (!player || !packet || !player->GetSession())
        return;

    // Real client sessions already have Penqle's native packet pump. Only
    // socketless historical bots need the module-owned queue.
    if (IsRealPlayer(player))
    {
        player->GetSession()->QueuePacket(packet.release());
        return;
    }

    QueueDelayedPlayerbotPacket(player->GetGUIDLow(),
        reinterpret_cast<std::uintptr_t>(player->GetSession()), std::move(packet));
}

void QueuePlayerbotPacket(Player* player, WorldPacket const& packet)
{
    QueuePlayerbotPacket(player, std::make_unique<WorldPacket>(packet));
}

void HandlePlayerbotPackets(Player* player)
{
    if (!player || !player->GetSession())
        return;

    uint32 const guidLow = player->GetGUIDLow();
    std::uintptr_t const sessionToken = reinterpret_cast<std::uintptr_t>(player->GetSession());
    std::deque<QueuedPlayerbotPacket> packets;

    {
        std::lock_guard<std::mutex> lock(s_playerbotPacketMutex);
        auto it = s_playerbotIncomingPackets.find(guidLow);
        if (it == s_playerbotIncomingPackets.end())
            return;
        packets.swap(it->second);
        s_playerbotIncomingPackets.erase(it);
    }

    WorldSession* session = player->GetSession();
    for (QueuedPlayerbotPacket& queued : packets)
    {
        // Async LLM replies may finish after a bot logged out/relogged. Never
        // deliver a packet to a replacement session from an older lifetime.
        if (queued.sessionToken != sessionToken || !queued.packet)
            continue;

        OpcodeHandler const* handler = opcodeTable.LookupOpcode(queued.packet->GetOpcode());
        if (!handler || !handler->handler)
        {
            sLog.outError("[PlayerBots] no Penqle opcode handler for queued bot packet %u (guid %u)",
                queued.packet->GetOpcode(), guidLow);
            continue;
        }

        // Penqle's native PlayerBots host invokes public WorldSession handlers
        // directly as well. WorldSession::ExecuteOpcode owns additional private
        // delayed-teleport bookkeeping that is intentionally not a module API;
        // do not expose or duplicate that core-private state just for bots.
        (session->*handler->handler)(*queued.packet);
    }
}

void ClearPlayerbotPackets(Player* player)
{
    if (!player)
        return;

    std::lock_guard<std::mutex> lock(s_playerbotPacketMutex);
    s_playerbotIncomingPackets.erase(player->GetGUIDLow());
}

void LearnPlayerbotClassLevelSpells(Player* player)
{
    if (!player)
        return;

    uint32 const classMask = 1u << (player->GetClass() - 1);
    uint32 const raceMask = 1u << (player->GetRace() - 1);
    uint32 const maxSkillId = sObjectMgr.GetMaxSkillLineAbilityId();

    // This is the same public DBC/SpellMgr algorithm used by Penqle's native
    // PlayerBotAI::AutoLearnSpellsForLevel. Keep the historical module's
    // "learn class level spells" behavior without reaching into native AI
    // ownership or adding a Player/core compatibility method.
    for (uint32 i = 0; i < maxSkillId; ++i)
    {
        SkillLineAbilityEntry const* ability = sObjectMgr.GetSkillLineAbility(i);
        if (!ability || !ability->classmask || !(ability->classmask & classMask))
            continue;
        if (ability->racemask && !(ability->racemask & raceMask))
            continue;
        if (ability->req_skill_value != 0)
            continue;

        SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(ability->spellId);
        if (!spellInfo || (spellInfo->spellLevel && spellInfo->spellLevel > player->GetLevel()))
            continue;
        if (!player->HasSpell(ability->spellId))
            player->LearnSpell(ability->spellId, false);
    }
}

void RegisterPlayerbotHolder(PlayerbotHolder* holder)
{
    if (holder)
        s_liveHolders.insert(holder);
}

void UnregisterPlayerbotHolder(PlayerbotHolder* holder)
{
    if (holder)
        s_liveHolders.erase(holder);
}

bool BeginPlayerbotLogin(PlayerbotHolder* holder, uint32 guidLow, uint32 /*masterAccountId*/)
{
    if (!holder || !sPlayerbotAIConfig.enabled)
        return false;

    ObjectGuid botGuid(HIGHGUID_PLAYER, guidLow);
    uint32 const realAccountId = sObjectMgr.GetPlayerAccountIdByGUID(botGuid);
    if (!realAccountId)
    {
        sLog.outError("[PlayerBots] AddPlayerBot: no account for guid %u", guidLow);
        return false;
    }

    uint32 syntheticAccountId = sPlayerBotMgr.GenBotAccountId();
    while (sWorld.FindSession(syntheticAccountId) || s_loginBootstraps.find(syntheticAccountId) != s_loginBootstraps.end())
        syntheticAccountId = sPlayerBotMgr.GenBotAccountId();

    auto pending = std::make_unique<LoginBootstrap>(holder, realAccountId, syntheticAccountId, guidLow);
    WorldSession* bootstrapSession = new WorldSession(syntheticAccountId, nullptr, SEC_PLAYER, 0, LOCALE_enUS, "<BOT>", 0);
    bootstrapSession->SetBot(&pending->entry);
    pending->bootstrapSession = bootstrapSession;

    s_loginBootstraps.emplace(syntheticAccountId, std::move(pending));
    sWorld.AddSession(bootstrapSession);
    return true;
}

namespace
{
class PlayerbotWorldScript final : public WorldScript
{
public:
    PlayerbotWorldScript()
        : WorldScript("mod_playerbots_world", { WORLDHOOK_ON_STARTUP, WORLDHOOK_ON_UPDATE })
    {
    }

    void OnStartup() override
    {
        sPlayerbotAIConfig.Initialize();
    }

    void OnUpdate(uint32 diff) override
    {
        UpdateLoginBootstraps();
        if (sPlayerbotAIConfig.enabled)
            sRandomPlayerbotMgr.UpdateAI(diff);
    }
};

class PlayerbotPlayerScript final : public PlayerScript
{
public:
    PlayerbotPlayerScript()
        : PlayerScript("mod_playerbots_player",
            { PLAYERHOOK_ON_UPDATE, PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LOGOUT,
              PLAYERHOOK_ON_BEFORE_SEND_CHAT_MESSAGE })
    {
    }

    void OnUpdate(Player* player, uint32 diff) override
    {
        if (!player || !sPlayerbotAIConfig.enabled)
            return;

        if (PlayerbotAI* ai = GetPlayerbotAI(player))
        {
            SC_PHASE("PlayerbotPlayerScript/ai.UpdateAI", player->GetName());
            ai->UpdateAI(diff);
        }

        if (PlayerbotMgr* mgr = GetPlayerbotMgr(player))
        {
            SC_PHASE("PlayerbotPlayerScript/mgr.UpdateAI", player->GetName());
            mgr->UpdateAI(diff);
        }
    }

    void OnLogin(Player* player) override
    {
        if (!player || !sPlayerbotAIConfig.enabled || !player->GetSession())
            return;

        // Complete our synthetic Penqle login only after HandlePlayerLogin has
        // reached its final PlayerScript hook.
        if (CompleteNativeLoginBootstrap(player))
        {
            sRandomPlayerbotMgr.OnPlayerLogin(player);
            return;
        }

        if (!IsRealPlayer(player))
        {
            sRandomPlayerbotMgr.OnPlayerLogin(player);
            return;
        }

        CreatePlayerbotMgr(player);
        if (PlayerbotMgr* mgr = GetPlayerbotMgr(player))
            mgr->OnPlayerLogin(player);
        sRandomPlayerbotMgr.OnPlayerLogin(player);
    }

    void OnLogout(Player* player) override
    {
        if (!player)
            return;

        ClearObservedRealPlayerLfgArea(player);
        ClearObservedChatTarget(player);

        if (sPlayerbotAIConfig.enabled)
            sRandomPlayerbotMgr.OnPlayerLogout(player);

        RemovePlayerbotMgr(player);
        RemovePlayerbotAI(player);
    }

    void OnBeforeSendChatMessage(Player* player, uint32& type, uint32& language, std::string& message) override
    {
        if (!player || !sPlayerbotAIConfig.enabled)
            return;

        if (PlayerbotAI* ai = GetPlayerbotAI(player))
            if (!ai->IsRealPlayer())
                return;

        std::string const target = ConsumeCurrentChatTarget(player);

        // Historical CMaNGOS routes a whisper directly to the addressed bot AI.
        // Do not fan whispers through RandomPlayerbotMgr: that manager has no
        // recipient parameter and would otherwise interpret `/w Botname ...`
        // as a command for every matching random bot.
        if (type == CHAT_MSG_WHISPER)
        {
            if (!target.empty())
            {
                if (Player* targetPlayer = sObjectMgr.GetPlayer(target.c_str()))
                {
                    if (PlayerbotAI* ai = GetPlayerbotAI(targetPlayer))
                        ai->HandleCommand(type, message, *player, language);
                }
            }
            return;
        }

        if (PlayerbotMgr* mgr = GetPlayerbotMgr(player))
            mgr->HandleCommand(type, message, language, target);

        // Preserve the historical free/random population listener for public
        // and group chat. Its own implementation applies distance, group, guild
        // and team filters. `channelName` remains empty here; the whisper target
        // is not a channel name.
        sRandomPlayerbotMgr.HandleCommand(type, message, *player, "", player->GetTeam(), language);
    }
};

class PlayerbotCommandScript final : public AllCommandScript
{
public:
    PlayerbotCommandScript() : AllCommandScript("mod_playerbots_commands") {}

    bool CanExecuteCommand(ChatHandler* handler, char const* command, char const* args) override
    {
        if (!handler || !command)
            return true;

        std::string const name(command);
        char const* commandArgs = args ? args : "";

        if (name == "bot")
        {
            // Historical command table: SEC_PLAYER, console disabled.
            if (!handler->GetSession())
            {
                handler->SendSysMessage("The .bot command requires an active player session.");
                return false;
            }

            PlayerbotMgr::HandlePlayerbotMgrCommand(handler, commandArgs);
            return false;
        }

        if (name == "rndbot" || name == "ahbot" || name == "perfmon" || name == "pmon")
        {
            AccountTypes access = SEC_PLAYER;
            if (WorldSession* session = handler->GetSession())
                access = session->GetSecurity();
            else if (CliHandler* cli = dynamic_cast<CliHandler*>(handler))
                access = cli->GetAccessLevel();

            // Preserve Shyalya's current public command contract:
            //   .rndbot  SEC_PLAYER, console allowed
            //   .ahbot   SEC_MODERATOR, console allowed
            //   .perfmon SEC_MODERATOR, console allowed
            // Keep .pmon as the historical alias so existing operator habits
            // do not regress while .perfmon remains the canonical spelling.
            AccountTypes const required = name == "rndbot" ? SEC_PLAYER : SEC_MODERATOR;
            if (access < required)
            {
                handler->SendSysMessage("You do not have permission to use that playerbot command.");
                return false;
            }

            if (name == "rndbot")
                RandomPlayerbotMgr::HandlePlayerbotConsoleCommand(handler, commandArgs);
            else if (name == "ahbot")
                ahbot::AhBot::HandleAhBotCommand(handler, commandArgs);
            else
                HandlePlayerbotPerfMonCommand(handler, commandArgs);

            return false;
        }

        return true;
    }
};

class PlayerbotServerScript final : public ServerScript
{
public:
    PlayerbotServerScript()
        : ServerScript("mod_playerbots_server", { SERVERHOOK_CAN_PACKET_SEND, SERVERHOOK_CAN_PACKET_RECEIVE })
    {
    }

    bool CanPacketReceive(WorldSession* session, WorldPacket const& packet) override
    {
        Player* player = session ? session->GetPlayer() : nullptr;
        if (!player || !IsRealPlayer(player))
            return true;

        ObserveCurrentChatTarget(session, packet);

        switch (packet.GetOpcode())
        {
            case CMSG_MEETINGSTONE_JOIN:
            {
                WorldPacket copy(packet);
                copy.rpos(0);
                ObjectGuid guid;
                copy >> guid;

                GameObject* stone = player->GetGameObjectIfCanInteractWith(guid, GAMEOBJECT_TYPE_MEETINGSTONE);
                GameObjectInfo const* info = stone ? stone->GetGOInfo() : nullptr;
                if (info)
                    ObserveRealPlayerLfgArea(player, info->meetingstone.areaID);
                break;
            }
            case CMSG_MEETINGSTONE_LEAVE:
                ClearObservedRealPlayerLfgArea(player);
                break;
            default:
                break;
        }

        // Observation only; Penqle's native handler still owns the packet.
        return true;
    }

    bool CanPacketSend(WorldSession* session, WorldPacket const& packet) override
    {
        Player* player = session ? session->GetPlayer() : nullptr;
        ObserveTargetIconPacket(player, packet);

        PlayerbotAI* ai = GetPlayerbotAI(player);
        if (!ai || ai->IsRealPlayer())
            return true;

        ai->HandleBotOutgoingPacket(packet);
        return false;
    }
};
} // namespace

void AddPlayerbotHostScripts()
{
    new PlayerbotWorldScript();
    new PlayerbotPlayerScript();
    new PlayerbotCommandScript();
    new PlayerbotServerScript();
}
