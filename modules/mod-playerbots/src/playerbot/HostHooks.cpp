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
#include "BotDiagnostics.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
std::unordered_map<Player*, std::unique_ptr<PlayerbotAI>> s_playerbotAIs;
std::unordered_map<Player*, std::unique_ptr<PlayerbotMgr>> s_playerbotMgrs;
std::unordered_set<PlayerbotHolder*> s_liveHolders;

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
        s_playerbotAIs.erase(player);
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

        if (PlayerbotMgr* mgr = GetPlayerbotMgr(player))
            mgr->HandleCommand(type, message, language);

        sRandomPlayerbotMgr.HandleCommand(type, message, *player, "", player->GetTeam(), language);
    }
};

class PlayerbotServerScript final : public ServerScript
{
public:
    PlayerbotServerScript()
        : ServerScript("mod_playerbots_server", { SERVERHOOK_CAN_PACKET_SEND })
    {
    }

    bool CanPacketSend(WorldSession* session, WorldPacket const& packet) override
    {
        Player* player = session ? session->GetPlayer() : nullptr;
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
    new PlayerbotServerScript();
}
