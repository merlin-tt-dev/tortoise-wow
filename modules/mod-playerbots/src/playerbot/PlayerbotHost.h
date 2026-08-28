#pragma once

#include "Common.h"

#include <cstdint>
#include <memory>

class Player;
class PlayerbotAI;
class PlayerbotMgr;
class PlayerbotHolder;
class WorldPacket;

// Module-owned host state for the historical playerbot implementation.
// Penqle's Player stays untouched; bot AI/manager ownership lives in mod-playerbots.
PlayerbotAI* GetPlayerbotAI(Player* player);
PlayerbotAI* GetPlayerbotAI(Player const* player);
PlayerbotMgr* GetPlayerbotMgr(Player* player);
PlayerbotMgr* GetPlayerbotMgr(Player const* player);

void CreatePlayerbotAI(Player* player);
void RemovePlayerbotAI(Player* player);
void CreatePlayerbotMgr(Player* player);
void RemovePlayerbotMgr(Player* player);
bool IsRealPlayer(Player const* player);

// Penqle keeps solo LFG queue area ids private to LFGQueue. Observe the native
// meeting-stone client packets instead of adding a core getter solely for bots.
uint32 GetObservedRealPlayerLfgArea(Player const* player);
void ClearObservedRealPlayerLfgArea(Player const* player);

// Historical playerbots enqueue simulated client packets. Penqle intentionally
// does not process the receive queue of socketless module bots, so keep that
// queue module-owned and dispatch it through Penqle's real opcode handlers.
void QueuePlayerbotPacket(Player* player, std::unique_ptr<WorldPacket> packet);
void QueuePlayerbotPacket(Player* player, WorldPacket const& packet);
void QueueDelayedPlayerbotPacket(uint32 guidLow, std::uintptr_t sessionToken, std::unique_ptr<WorldPacket> packet);
void HandlePlayerbotPackets(Player* player);
void ClearPlayerbotPackets(Player* player);
void LearnPlayerbotClassLevelSpells(Player* player);

void RegisterPlayerbotHolder(PlayerbotHolder* holder);
void UnregisterPlayerbotHolder(PlayerbotHolder* holder);

// Start a bot login through Penqle's native PlayerBots/WorldSession login pipeline.
// The synthetic bootstrap session is replaced with a free session using the bot's
// real account id only after Penqle's full login callback has completed.
bool BeginPlayerbotLogin(PlayerbotHolder* holder, uint32 guidLow, uint32 masterAccountId);
