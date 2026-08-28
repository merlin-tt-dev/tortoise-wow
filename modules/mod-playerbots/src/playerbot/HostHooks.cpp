// Playerbot host integration for Tortoise's native module/script framework.
//
// The core only exposes two opaque Player-owned pointers and tiny accessors.
// All behavior (startup, ticking, login/logout, chat and outgoing packets)
// is registered through Penqle's WorldScript/PlayerScript/ServerScript hooks.

#include "playerbot/playerbot.h"
#include "Objects/Player.h"
#include "ScriptObjects.h"
#include "playerbot/RandomPlayerbotMgr.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "BotDiagnostics.h"

void Player::CreatePlayerbotAI()
{
    if (!m_playerbotAI)
        m_playerbotAI = new PlayerbotAI(this);
}

void Player::RemovePlayerbotAI()
{
    if (m_playerbotAI)
    {
        delete m_playerbotAI;
        m_playerbotAI = nullptr;
    }
}

void Player::CreatePlayerbotMgr()
{
    if (!m_playerbotMgr)
        m_playerbotMgr = new PlayerbotMgr(this);
}

void Player::RemovePlayerbotMgr()
{
    if (m_playerbotMgr)
    {
        // The manager owns the master's alt-bot membership. Log those bots out
        // before releasing the manager so no bot retains a dangling master path.
        m_playerbotMgr->LogoutAllBots();
        delete m_playerbotMgr;
        m_playerbotMgr = nullptr;
    }
}

bool Player::isRealPlayer() const
{
    WorldSession* session = GetSession();
    if (!session)
        return false;

    std::string const& address = session->GetRemoteAddress();
    return address != "<BOT>" && address != "disconnected/bot";
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

        if (PlayerbotAI* ai = player->GetPlayerbotAI())
        {
            SC_PHASE("PlayerbotPlayerScript/ai.UpdateAI", player->GetName());
            ai->UpdateAI(diff);
        }

        if (PlayerbotMgr* mgr = player->GetPlayerbotMgr())
        {
            SC_PHASE("PlayerbotPlayerScript/mgr.UpdateAI", player->GetName());
            mgr->UpdateAI(diff);
        }
    }

    void OnLogin(Player* player) override
    {
        if (!player || !sPlayerbotAIConfig.enabled)
            return;

        if (!player->GetSession())
            return;

        // Penqle tags every null-socket WorldSession as <BOT>. Synthetic bot
        // sessions must not receive a PlayerbotMgr of their own; OnBotLogin
        // attaches AI after the standard player-login pipeline returns to the
        // bot holder. Keep the historical disconnected/bot sentinel accepted
        // in Player::isRealPlayer() for compatibility with older bot sessions.
        if (!player->isRealPlayer())
        {
            sRandomPlayerbotMgr.OnPlayerLogin(player);
            return;
        }

        player->CreatePlayerbotMgr();
        player->GetPlayerbotMgr()->OnPlayerLogin(player);
        sRandomPlayerbotMgr.OnPlayerLogin(player);
    }

    void OnLogout(Player* player) override
    {
        if (!player)
            return;

        // RandomPlayerbotMgr still needs the AI/master relationship while it
        // detaches the player, so perform that notification before destruction.
        if (sPlayerbotAIConfig.enabled)
            sRandomPlayerbotMgr.OnPlayerLogout(player);

        player->RemovePlayerbotMgr();
        player->RemovePlayerbotAI();
    }

    void OnBeforeSendChatMessage(Player* player, uint32& type, uint32& language, std::string& message) override
    {
        if (!player || !sPlayerbotAIConfig.enabled)
            return;

        // Synthetic bot chat must not recursively become a master command.
        if (PlayerbotAI* ai = player->GetPlayerbotAI())
            if (!ai->IsRealPlayer())
                return;

        if (PlayerbotMgr* mgr = player->GetPlayerbotMgr())
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
        PlayerbotAI* ai = player ? player->GetPlayerbotAI() : nullptr;
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
