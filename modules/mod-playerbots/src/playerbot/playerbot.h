#pragma once

#include "Spells/Spell.h"
#include "WorldPacket.h"
#include "LootMgr.h"
#include "GossipDef.h"
#include "Chat/Chat.h"
#include "Common.h"
#include "World.h"
#include "Spells/SpellMgr.h"
#include "ObjectMgr.h"
#include "Objects/Unit.h"
#include "SharedDefines.h"
#include "Movement/MotionMaster.h"
#include "Spells/SpellAuras.h"
#include "Guild/Guild.h"

#include "playerbotDefs.h"
#include "playerbot/Helpers.h"
#include "playerbot/PlayerbotHost.h"
#include "playerbot/ProgressBar.h"
#include "playerbot/PlayerbotAIAware.h"
#include "PlayerbotMgr.h"
#include "playerbot/RandomPlayerbotMgr.h"
#include "ChatHelper.h"
#include "BroadcastHelper.h"
#include "PlayerbotAI.h"
#include "PlayerbotDbStore.h"

#define MANGOSBOT_VERSION 2

#ifndef WIN32
int strcmpi(std::string s1, std::string s2);
#endif
