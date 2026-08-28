#pragma once

#include "Common.h"

class Player;
class PlayerbotAI;
class PlayerbotMgr;
class PlayerbotHolder;

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

void RegisterPlayerbotHolder(PlayerbotHolder* holder);
void UnregisterPlayerbotHolder(PlayerbotHolder* holder);

// Start a bot login through Penqle's native PlayerBots/WorldSession login pipeline.
// The synthetic bootstrap session is replaced with a free session using the bot's
// real account id only after Penqle's full login callback has completed.
bool BeginPlayerbotLogin(PlayerbotHolder* holder, uint32 guidLow, uint32 masterAccountId);
