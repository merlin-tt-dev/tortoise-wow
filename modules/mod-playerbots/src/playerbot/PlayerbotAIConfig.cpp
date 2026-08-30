
#include "playerbot/PlayerbotAIConfig.h"
#include "playerbot/playerbot.h"
#include "RandomPlayerbotFactory.h"
#include "AccountMgr.h"
#include "playerbot/PlayerbotFactory.h"
#include "RandomItemMgr.h"
#include "playerbot/PlayerbotHelpMgr.h"
#include "playerbot/strategy/actions/CheatAction.h"

#include "playerbot/TravelMgr.h"

#include <iostream>
#include <numeric>
#include <iomanip>
#include <boost/algorithm/string.hpp>
#include <regex>
#include <fstream>
#include <sstream>
#include "PlayerbotLoginMgr.h"

namespace
{
std::vector<std::string> GetConfigKeysContaining(Config& config, const std::string& name)
{
    std::vector<std::string> values;
    std::vector<std::string> sections;
    const std::string nameLower = boost::algorithm::to_lower_copy(name);

    config.GetRootSections(sections);
    for (const std::string& section : sections)
    {
        std::vector<std::string> keys;
        config.GetKeys(section.c_str(), keys);

        for (const std::string& key : keys)
        {
            if (boost::algorithm::to_lower_copy(key).find(nameLower) != std::string::npos)
                values.push_back(key);
        }
    }

    return values;
}
} // namespace

INSTANTIATE_SINGLETON_1(PlayerbotAIConfig);

PlayerbotAIConfig::PlayerbotAIConfig()
    : enabled(false)
{
}

template <class T>
void LoadList(std::string value, T& list)
{
    list.clear();
    std::vector<std::string> ids = split(value, ',');
    for (std::vector<std::string>::iterator i = ids.begin(); i != ids.end(); i++)
    {
        std::string string = *i;
        if (string.empty())
            continue;

        uint32 id = atoi(string.c_str());

        list.push_back(id);
    }
}

template <class T>
void LoadListString(std::string value, T& list)
{
    list.clear();
    std::vector<std::string> strings = split(value, ',');
    for (std::vector<std::string>::iterator i = strings.begin(); i != strings.end(); i++)
    {
        std::string string = *i;
        if (string.empty())
            continue;

        list.push_back(string);
    }
}

inline ParsedUrl parseUrl(const std::string& url)
{
    std::regex urlRegex(R"((http|https)://([^:/]+)(:([0-9]+))?(/.*)?)");
    std::smatch match;
    if (!std::regex_match(url, match, urlRegex))
    {
        throw std::invalid_argument("Invalid URL format");
    }

    ParsedUrl parsed;
    parsed.hostname = match[2];
    parsed.https = match[1] == "https";
    parsed.port = parsed.https ? 443 : (match[4].length() ? std::stoi(match[4]) : 80);
    parsed.path = match[5].length() ? match[5] : std::string("/");
    return parsed;
}

bool PlayerbotAIConfig::Initialize()
{
    sLog.outString("Initializing AI Playerbot by ike3, based on the original Playerbot by blueboy");

    enabled = sConfig.GetBoolDefault("AiPlayerbot.Enabled", false);
    if (!enabled)
    {
        sLog.outString("AI Playerbot is Disabled in aiplayerbot.conf");
        return false;
    }

    BarGoLink::SetOutputState(sConfig.GetBoolDefault("AiPlayerbot.ShowProgressBars", false));
    globalCoolDown = (uint32)sConfig.GetIntDefault("AiPlayerbot.GlobalCooldown", 500);
    maxWaitForMove = sConfig.GetIntDefault("AiPlayerbot.MaxWaitForMove", 3000);
    expireActionTime = sConfig.GetIntDefault("AiPlayerbot.ExpireActionTime", 5000);
    dispelAuraDuration = sConfig.GetIntDefault("AiPlayerbot.DispelAuraDuration", 2000);
    reactDelay = (uint32)sConfig.GetIntDefault("AiPlayerbot.ReactDelay", 100);
    passiveDelay = (uint32)sConfig.GetIntDefault("AiPlayerbot.PassiveDelay", 4000);
    repeatDelay = (uint32)sConfig.GetIntDefault("AiPlayerbot.RepeatDelay", 5000);
    errorDelay = (uint32)sConfig.GetIntDefault("AiPlayerbot.ErrorDelay", 5000);
    rpgDelay = (uint32)sConfig.GetIntDefault("AiPlayerbot.RpgDelay", 3000);
    sitDelay = (uint32)sConfig.GetIntDefault("AiPlayerbot.SitDelay", 30000);
    returnDelay = (uint32)sConfig.GetIntDefault("AiPlayerbot.ReturnDelay", 7000);
    lootDelay = (uint32)sConfig.GetIntDefault("AiPlayerbot.LootDelayDelay", 750);

    farDistance = sConfig.GetFloatDefault("AiPlayerbot.FarDistance", 20.0f);
    sightDistance = sConfig.GetFloatDefault("AiPlayerbot.SightDistance", 75.0f);
    spellDistance = sConfig.GetFloatDefault("AiPlayerbot.SpellDistance", 25.0f);
    shootDistance = sConfig.GetFloatDefault("AiPlayerbot.ShootDistance", 25.0f);
    healDistance = sConfig.GetFloatDefault("AiPlayerbot.HealDistance", 125.0f);
    reactDistance = sConfig.GetFloatDefault("AiPlayerbot.ReactDistance", 150.0f);
    maxFreeMoveDistance = sConfig.GetFloatDefault("AiPlayerbot.MaxFreeMoveDistance", 150.0f);
    freeMoveDelay = sConfig.GetFloatDefault("AiPlayerbot.FreeMoveDelay", 30.0f);
    grindDistance = sConfig.GetFloatDefault("AiPlayerbot.GrindDistance", 75.0f);
    aggroDistance = sConfig.GetFloatDefault("AiPlayerbot.AggroDistance", 22.0f);
    lootDistance = sConfig.GetFloatDefault("AiPlayerbot.LootDistance", 25.0f);
    groupMemberLootDistance = sConfig.GetFloatDefault("AiPlayerbot.GroupMemberLootDistance", 15.0f);
    groupMemberLootDistanceWithActiveMaster = sConfig.GetFloatDefault("AiPlayerbot.GroupMemberLootDistanceWithActiveMaster", 10.0f);
    gatheringDistance = sConfig.GetFloatDefault("AiPlayerbot.GatheringDistance", 15.0f);
    groupMemberGatheringDistance = sConfig.GetFloatDefault("AiPlayerbot.GroupMemberGatheringDistance", 10.0f);
    groupMemberGatheringDistanceWithActiveMaster = sConfig.GetFloatDefault("AiPlayerbot.GroupMemberGatheringDistanceWithActiveMaster", 5.0f);
    fleeDistance = sConfig.GetFloatDefault("AiPlayerbot.FleeDistance", 8.0f);
    tooCloseDistance = sConfig.GetFloatDefault("AiPlayerbot.TooCloseDistance", 5.0f);
    meleeDistance = sConfig.GetFloatDefault("AiPlayerbot.MeleeDistance", 1.5f);
    followDistance = sConfig.GetFloatDefault("AiPlayerbot.FollowDistance", 1.5f);
    raidFollowDistance = sConfig.GetFloatDefault("AiPlayerbot.RaidFollowDistance", 5.0f);
    wanderMinDistance = sConfig.GetFloatDefault("AiPlayerbot.WanderMinDistance", 5.0f);
    wanderMaxDistance = sConfig.GetFloatDefault("AiPlayerbot.WanderMaxDistance", 50.0f);
    whisperDistance = sConfig.GetFloatDefault("AiPlayerbot.WhisperDistance", 6000.0f);
    contactDistance = sConfig.GetFloatDefault("AiPlayerbot.ContactDistance", 0.5f);
    aoeRadius = sConfig.GetFloatDefault("AiPlayerbot.AoeRadius", 5.0f);
    rpgDistance = sConfig.GetFloatDefault("AiPlayerbot.RpgDistance", 80.0f);
    proximityDistance = sConfig.GetFloatDefault("AiPlayerbot.ProximityDistance", 20.0f);
    walkDistance = sConfig.GetFloatDefault("AiPlayerbot.WalkDistance", 5.0f);

    criticalHealth = sConfig.GetIntDefault("AiPlayerbot.CriticalHealth", 20);
    lowHealth = sConfig.GetIntDefault("AiPlayerbot.LowHealth", 50);
    mediumHealth = sConfig.GetIntDefault("AiPlayerbot.MediumHealth", 70);
    almostFullHealth = sConfig.GetIntDefault("AiPlayerbot.AlmostFullHealth", 90);
    lowMana = sConfig.GetIntDefault("AiPlayerbot.LowMana", 15);
    mediumMana = sConfig.GetIntDefault("AiPlayerbot.MediumMana", 40);

    randomGearMaxLevel = sConfig.GetIntDefault("AiPlayerbot.RandomGearMaxLevel", 500);
    randomGearMaxDiff = sConfig.GetIntDefault("AiPlayerbot.RandomGearMaxDiff", 9);
    randomGearUpgradeEnabled = sConfig.GetBoolDefault("AiPlayerbot.RandomGearUpgradeEnabled", false);
    randomGearTabards = sConfig.GetBoolDefault("AiPlayerbot.RandomGearTabards", false);
    randomGearTabardsChance = sConfig.GetFloatDefault("AiPlayerbot.RandomGearTabardsChance", 0.1f);
    randomGearTabardsReplaceGuild = sConfig.GetBoolDefault("AiPlayerbot.RandomGearTabardsReplaceGuild", false);
    randomGearTabardsUnobtainable = sConfig.GetBoolDefault("AiPlayerbot.RandomGearTabardsUnobtainable", false);
    LoadList<std::list<uint32>>(sConfig.GetStringDefault("AiPlayerbot.RandomGearBlacklist", ""), randomGearBlacklist);
    LoadList<std::list<uint32>>(sConfig.GetStringDefault("AiPlayerbot.RandomGearWhitelist", ""), randomGearWhitelist);
    randomGearProgression = sConfig.GetBoolDefault("AiPlayerbot.RandomGearProgression", true);
    randomGearLoweringChance = sConfig.GetFloatDefault("AiPlayerbot.RandomGearLoweringChance", 0.15f);
    randomBotMaxLevelChance = sConfig.GetFloatDefault("AiPlayerbot.RandomBotMaxLevelChance", 0.15f);
    rollBadItemsWithPlayer = sConfig.GetBoolDefault("AiPlayerbot.RollBadItemsWithPlayer", false);
    randomBotRpgChance = sConfig.GetFloatDefault("AiPlayerbot.RandomBotRpgChance", 0.35f);
    usePotionChance = sConfig.GetFloatDefault("AiPlayerbot.UsePotionChance", 1.0f);
    attackEmoteChance = sConfig.GetFloatDefault("AiPlayerbot.AttackEmoteChance", 0.0f);

    jumpNoCombatChance = sConfig.GetFloatDefault("AiPlayerbot.JumpNoCombatChance", 0.5f);
    jumpMeleeInCombatChance = sConfig.GetFloatDefault("AiPlayerbot.JumpMeleeInCombatChance", 0.5f);
    jumpRandomChance = sConfig.GetFloatDefault("AiPlayerbot.JumpRandomChance", 0.20f);
    jumpInPlaceChance = sConfig.GetFloatDefault("AiPlayerbot.JumpInPlaceChance", 0.50f);
    jumpBackwardChance = sConfig.GetFloatDefault("AiPlayerbot.JumpBackwardChance", 0.10f);
    jumpHeightLimit = sConfig.GetFloatDefault("AiPlayerbot.JumpHeightLimit", 60.f);
    jumpVSpeed = sConfig.GetFloatDefault("AiPlayerbot.JumpVSpeed", 7.96f);
    jumpHSpeed = sConfig.GetFloatDefault("AiPlayerbot.JumpHSpeed", 7.0f);
    jumpInBg = sConfig.GetBoolDefault("AiPlayerbot.JumpInBg", false);
    jumpWithPlayer = sConfig.GetBoolDefault("AiPlayerbot.JumpWithPlayer", false);
    jumpFollow = sConfig.GetBoolDefault("AiPlayerbot.JumpFollow", true);
    jumpChase = sConfig.GetBoolDefault("AiPlayerbot.JumpChase", true);
    useKnockback = sConfig.GetBoolDefault("AiPlayerbot.UseKnockback", true);

    iterationsPerTick = sConfig.GetIntDefault("AiPlayerbot.IterationsPerTick", 100);

    allowGuildBots = sConfig.GetBoolDefault("AiPlayerbot.AllowGuildBots", true);
    allowMultiAccountAltBots = sConfig.GetBoolDefault("AiPlayerbot.AllowMultiAccountAltBots", true);

    randomBotMapsAsString = sConfig.GetStringDefault("AiPlayerbot.RandomBotMaps", "0,1,530,571");
    LoadList<std::vector<uint32>>(randomBotMapsAsString, randomBotMaps);
    LoadList<std::list<uint32>>(sConfig.GetStringDefault("AiPlayerbot.RandomBotQuestItems", "6948,5175,5176,5177,5178,16309,12382,13704,11000,22754"), randomBotQuestItems);
    LoadList<std::list<uint32>>(sConfig.GetStringDefault("AiPlayerbot.RandomBotSpellIds", "54197"), randomBotSpellIds);
    LoadList<std::list<uint32>>(sConfig.GetStringDefault("AiPlayerbot.PvpProhibitedZoneIds", "2255,656,2361,2362,2363,976,35,2268,3425,392,541,1446,3828,3712,3738,3565,3539,3623,4152,3988,4658,4284,4418,4436,4275,4323"), pvpProhibitedZoneIds);

#ifndef MANGOSBOT_ZERO
    // disable pvp near dark portal if event is active
    if (sWorldState.GetExpansion() == EXPANSION_NONE)
        pvpProhibitedZoneIds.insert(pvpProhibitedZoneIds.begin(), 72);
#endif

    LoadList<std::list<uint32>>(sConfig.GetStringDefault("AiPlayerbot.RandomBotQuestIds", "7848,3802,5505,6502,7761,9378"), randomBotQuestIds);
    LoadList<std::list<uint32>>(sConfig.GetStringDefault("AiPlayerbot.ImmuneSpellIds", ""), immuneSpellIds);

    botAutologin = BotAutoLogin(sConfig.GetIntDefault("AiPlayerbot.BotAutologin", 0));
    randomBotAutologin = sConfig.GetBoolDefault("AiPlayerbot.RandomBotAutologin", true);
    randomBotAutoCreate = sConfig.GetBoolDefault("AiPlayerbot.RandomBotAutoCreate", true);
    minRandomBots = sConfig.GetIntDefault("AiPlayerbot.MinRandomBots", 50);
    maxRandomBots = sConfig.GetIntDefault("AiPlayerbot.MaxRandomBots", 200);
    randomBotUpdateInterval = sConfig.GetIntDefault("AiPlayerbot.RandomBotUpdateInterval", 1 * 1000);
    randomBotCountChangeMinInterval = sConfig.GetIntDefault("AiPlayerbot.RandomBotCountChangeMinInterval", 1 * 1800);
    randomBotCountChangeMaxInterval = sConfig.GetIntDefault("AiPlayerbot.RandomBotCountChangeMaxInterval", 2 * 3600);
    randomBotTimedLogout = sConfig.GetBoolDefault("AiPlayerbot.RandomBotTimedLogout", true);
    randomBotTimedOffline = sConfig.GetBoolDefault("AiPlayerbot.RandomBotTimedOffline", false);
    minRandomBotInWorldTime = sConfig.GetIntDefault("AiPlayerbot.MinRandomBotInWorldTime", 1 * 1800);
    maxRandomBotInWorldTime = sConfig.GetIntDefault("AiPlayerbot.MaxRandomBotInWorldTime", 6 * 3600);
    minRandomBotRandomizeTime = sConfig.GetIntDefault("AiPlayerbot.MinRandomBotRandomizeTime", 6 * 3600);
    maxRandomBotRandomizeTime = sConfig.GetIntDefault("AiPlayerbot.MaxRandomRandomizeTime", 24 * 3600);
    minRandomBotChangeStrategyTime = sConfig.GetIntDefault("AiPlayerbot.MinRandomBotChangeStrategyTime", 1800);
    maxRandomBotChangeStrategyTime = sConfig.GetIntDefault("AiPlayerbot.MaxRandomBotChangeStrategyTime", 2 * 3600);
    minRandomBotReviveTime = sConfig.GetIntDefault("AiPlayerbot.MinRandomBotReviveTime", 60);
    maxRandomBotReviveTime = sConfig.GetIntDefault("AiPlayerbot.MaxRandomReviveTime", 300);
    enableRandomTeleports = sConfig.GetBoolDefault("AiPlayerbot.EnableRandomTeleports", true);
    enableMinimalMove = sConfig.GetBoolDefault("AiPlayerbot.EnableMinimalMove", true);

    randomBotTeleportDistance = sConfig.GetIntDefault("AiPlayerbot.RandomBotTeleportDistance", 1000);
    transportTeleportType = sConfig.GetIntDefault("AiPlayerbot.TransportTeleportType", 2);
    randomBotTeleportNearPlayer = sConfig.GetBoolDefault("AiPlayerbot.RandomBotTeleportNearPlayer", false);
    randomBotTeleportNearPlayerMaxAmount = sConfig.GetIntDefault("AiPlayerbot.RandomBotTeleportNearPlayerMaxAmount", 0);
    randomBotTeleportNearPlayerMaxAmountRadius = sConfig.GetFloatDefault("AiPlayerbot.RandomBotTeleportNearPlayerMaxAmountRadius", 0.0f);
    randomBotTeleportMinInterval = sConfig.GetIntDefault("AiPlayerbot.RandomBotTeleportTeleportMinInterval", 2 * 3600);
    randomBotTeleportMaxInterval = sConfig.GetIntDefault("AiPlayerbot.RandomBotTeleportTeleportMaxInterval", 48 * 3600);
    randomBotsMaxLoginsPerInterval = sConfig.GetIntDefault("AiPlayerbot.RandomBotsMaxLoginsPerInterval", 10);
    randomBotsPerInterval = sConfig.GetIntDefault("AiPlayerbot.RandomBotsPerInterval", 0);
    minRandomBotsPriceChangeInterval = sConfig.GetIntDefault("AiPlayerbot.MinRandomBotsPriceChangeInterval", 2 * 3600);
    maxRandomBotsPriceChangeInterval = sConfig.GetIntDefault("AiPlayerbot.MaxRandomBotsPriceChangeInterval", 48 * 3600);
    //Auction house settings
    shouldQueryAHListingsOutsideOfAH = sConfig.GetBoolDefault("AiPlayerbot.ShouldQueryAHListingsOutsideOfAH", true);
    LoadList<std::list<uint32>>(sConfig.GetStringDefault("AiPlayerbot.AhOverVendorItemIds", ""), ahOverVendorItemIds);
    LoadList<std::list<uint32>>(sConfig.GetStringDefault("AiPlayerbot.VendorOverAHItemIds", ""), vendorOverAHItemIds);
    botCheckAllAuctionListings = sConfig.GetBoolDefault("AiPlayerbot.BotCheckAllAuctionListings", false);
    botsSaveEpics = sConfig.GetBoolDefault("AiPlayerbot.BotsSaveEpics", true);
    //
    randomBotJoinLfg = sConfig.GetBoolDefault("AiPlayerbot.RandomBotJoinLfg", true);
    logRandomBotJoinLfg = sConfig.GetBoolDefault("AiPlayerbot.LogRandomBotJoinLfg", false);
    randomBotJoinBG = sConfig.GetBoolDefault("AiPlayerbot.RandomBotJoinBG", true);
    randomBotAutoJoinBG = sConfig.GetBoolDefault("AiPlayerbot.RandomBotAutoJoinBG", false);
    randomBotBracketCount = sConfig.GetIntDefault("AiPlayerbot.RandomBotBracketCount", 3);
    logInGroupOnly = sConfig.GetBoolDefault("AiPlayerbot.LogInGroupOnly", true);
    logValuesPerTick = sConfig.GetBoolDefault("AiPlayerbot.LogValuesPerTick", false);
    fleeingEnabled = sConfig.GetBoolDefault("AiPlayerbot.FleeingEnabled", true);
    summonAtInnkeepersEnabled = sConfig.GetBoolDefault("AiPlayerbot.SummonAtInnkeepersEnabled", true);
    randomBotMinLevel = sConfig.GetIntDefault("AiPlayerbot.RandomBotMinLevel", 1);
    randomBotMaxLevel = sConfig.GetIntDefault("AiPlayerbot.RandomBotMaxLevel", 255);
    randomBotLoginAtStartup = sConfig.GetBoolDefault("AiPlayerbot.RandomBotLoginAtStartup", true);
    randomBotTeleLevel = sConfig.GetIntDefault("AiPlayerbot.RandomBotTeleLevel", 5);
    openGoSpell = sConfig.GetIntDefault("AiPlayerbot.OpenGoSpell", 6477);

    randomChangeMultiplier = sConfig.GetFloatDefault("AiPlayerbot.RandomChangeMultiplier", 1.0);

    randomBotCombatStrategies = sConfig.GetStringDefault("AiPlayerbot.RandomBotCombatStrategies", "-threat,+custom::say");
    randomBotNonCombatStrategies = sConfig.GetStringDefault("AiPlayerbot.RandomBotNonCombatStrategies", "+custom::say");
    randomBotReactStrategies = sConfig.GetStringDefault("AiPlayerbot.RandomBotReactStrategies", "");
    randomBotDeadStrategies = sConfig.GetStringDefault("AiPlayerbot.RandomBotDeadStrategies", "");
    combatStrategies = sConfig.GetStringDefault("AiPlayerbot.CombatStrategies", "");
    nonCombatStrategies = sConfig.GetStringDefault("AiPlayerbot.NonCombatStrategies", "+return,+delayed roll");
    reactStrategies = sConfig.GetStringDefault("AiPlayerbot.ReactStrategies", "");
    deadStrategies = sConfig.GetStringDefault("AiPlayerbot.DeadStrategies", "");

    commandPrefix = sConfig.GetStringDefault("AiPlayerbot.CommandPrefix", "");
    commandSeparator = sConfig.GetStringDefault("AiPlayerbot.CommandSeparator", "\\\\");

    commandServerPort = sConfig.GetIntDefault("AiPlayerbot.CommandServerPort", 0);
    perfMonEnabled = sConfig.GetBoolDefault("AiPlayerbot.PerfMonEnabled", false);
    bExplicitDbStoreSave = sConfig.GetBoolDefault("AiPlayerbot.ExplicitDbStoreSave", false);

    randomBotLoginWithPlayer = sConfig.GetBoolDefault("AiPlayerbot.RandomBotLoginWithPlayer", false);
    asyncBotLogin = sConfig.GetBoolDefault("AiPlayerbot.AsyncBotLogin", false);
    preloadHolders = sConfig.GetBoolDefault("AiPlayerbot.PreloadHolders", false);

    freeRoomForNonSpareBots = sConfig.GetIntDefault("AiPlayerbot.FreeRoomForNonSpareBots", 1);

    loginBotsNearPlayerRange = sConfig.GetIntDefault("AiPlayerbot.LoginBotsNearPlayerRange", 1000);

    LoadListString<std::vector<std::string>>(sConfig.GetStringDefault("AiPlayerbot.DefaultLoginCriteria", "maxbots,spareroom,offline"), defaultLoginCriteria);

    std::vector<std::string> criteriaValues = GetConfigKeysContaining(sConfig, "AiPlayerbot.LoginCriteria");
    std::sort(criteriaValues.begin(), criteriaValues.end());
    loginCriteria.clear();
    for (auto& value : criteriaValues)
    {
        loginCriteria.push_back({});
        LoadListString<std::vector<std::string>>(sConfig.GetStringDefault(value.c_str(), ""), loginCriteria.back());
    }

    if (criteriaValues.empty())
    {
        loginCriteria.push_back({"group"});
        loginCriteria.push_back({"arena"});
        loginCriteria.push_back({"bg"});
        loginCriteria.push_back({"guild"});
        loginCriteria.push_back({"logoff,classrace,level,online"});
        loginCriteria.push_back({"logoff,classrace,level"});
        loginCriteria.push_back({"logoff,classrace"});
    }

    for (uint32 level = 1; level <= PLAYER_MAX_LEVEL; ++level)
    {
        std::string key = "AiPlayerbot.LevelProbability." + std::to_string(level);
        levelProbability[level] = sConfig.GetIntDefault(key.c_str(), 100);
    }

    sLog.outString("Loading Race/Class probabilities");

    classRaceProbabilityTotal = 0;

    useFixedClassRaceCounts = sConfig.GetBoolDefault("AiPlayerbot.ClassRace.UseFixedClassRaceCounts", false);
    fixedClassRaceCounts.clear();
    RandomPlayerbotFactory factory(0);

    for (uint32 race = 1; race < MAX_RACES; ++race)
    {
        //Set race defaults
        if (race > 0)
        {
            std::string key = "AiPlayerbot.ClassRaceProb.0." + std::to_string(race);
            int rProb = sConfig.GetIntDefault(key.c_str(), 100);

            for (uint32 cls = 1; cls < MAX_CLASSES; ++cls)
            {
                classRaceProbability[cls][race] = rProb;
            }
        }
    }

    //Class overrides
    for (uint32 cls = 1; cls < MAX_CLASSES; ++cls)
    {
        std::string key = "AiPlayerbot.ClassRaceProb." + std::to_string(cls);
        int cProb = sConfig.GetIntDefault(key.c_str(), -1);

        if (cProb >= 0)
        {
            for (uint32 race = 1; race < MAX_RACES; ++race)
            {
                classRaceProbability[cls][race] = cProb;
            }
        }
    }

    //Race Class overrides
    for (uint32 race = 1; race < MAX_RACES; ++race)
    {
        for (uint32 cls = 1; cls < MAX_CLASSES; ++cls)
        {
            std::string key = "AiPlayerbot.ClassRaceProb." + std::to_string(cls) + "." + std::to_string(race);
            int rcProb = sConfig.GetIntDefault(key.c_str(), -1);
            if (rcProb >= 0)
                classRaceProbability[cls][race] = rcProb;

            if (!factory.isAvailableRace(cls, race))
                classRaceProbability[cls][race] = 0;
            else
                classRaceProbabilityTotal += classRaceProbability[cls][race];
        }
    }

    if (useFixedClassRaceCounts)
    {

        // Warn about unsupported config keys
        for (uint32 race = 1; race < MAX_RACES; ++race)
        {
            std::string raceKey = "AiPlayerbot.ClassRaceProb.0." + std::to_string(race);
            int val = sConfig.GetIntDefault(raceKey.c_str(), -1);
            if (val >= 0)
                sLog.outError("Fixed class/race counts does not yet support '%s' (race-only). This config entry will be ignored.", raceKey.c_str());
        }

        for (uint32 cls = 1; cls < MAX_CLASSES; ++cls)
        {
            std::string classKey = "AiPlayerbot.ClassRaceProb." + std::to_string(cls);
            int val = sConfig.GetIntDefault(classKey.c_str(), -1);
            if (val >= 0)
                sLog.outError("Fixed class/race counts does not yet support '%s' (class-only). This config entry will be ignored.", classKey.c_str());
        }

        //Parse and build fixedClassRacesCounts
        {
            for (uint32 cls = 1; cls < MAX_CLASSES; ++cls)
            {
                for (uint32 race = 1; race < MAX_RACES; ++race)
                {
                    std::string key = "AiPlayerbot.ClassRaceProb." + std::to_string(cls) + "." + std::to_string(race);
                    int count = sConfig.GetIntDefault(key.c_str(), -1);

                    if (count >= 0 && factory.isAvailableRace(cls, race))
                    {
                        fixedClassRaceCounts[{cls, race}] = count;
                    }
                }
            }
        }
    }

    botCheatMask = uint32(CheatAction::GetCheatMask(sConfig.GetStringDefault("AiPlayerbot.BotCheats", "taxi,item,breath")));

    rndBotCheatMask = uint32(CheatAction::GetCheatMask(sConfig.GetStringDefault("AiPlayerbot.RndBotCheats", "taxi,item,breath")));

    LoadListString<std::list<std::string>>(sConfig.GetStringDefault("AiPlayerbot.AllowedLogFiles", ""), allowedLogFiles);
    LoadListString<std::list<std::string>>(sConfig.GetStringDefault("AiPlayerbot.DebugFilter", "add gathering loot,check values,emote,check mount state,jump"), debugFilter);

    worldBuffs.clear();

    //Get all config values starting with AiPlayerbot.WorldBuff
    std::vector<std::string> values = GetConfigKeysContaining(sConfig, "AiPlayerbot.WorldBuff");

    if (values.size())
    {
        sLog.outString("Loading WorldBuffs");
        BarGoLink wbuffBar(values.size());

        for (auto value : values)
        {
            std::vector<std::string> ids = split(value, '.');
            std::vector<uint32> params = {0, 0, 0, 0, 0, 0};

            //Extract faction, class, spec, minlevel, maxlevel
            for (uint8 i = 0; i < 6; i++)
                if (ids.size() > i + 2)
                    params[i] = stoi(ids[i + 2]);

            //Get list of buffs for this combination.
            std::list<uint32> buffs;
            LoadList<std::list<uint32>>(sConfig.GetStringDefault(value.c_str(), ""), buffs);

            //Store buffs for later application.
            for (auto buff : buffs)
            {
                worldBuff wb = {buff, params[0], params[1], params[2], params[3], params[4], params[5]};
                worldBuffs.push_back(wb);
            }

            wbuffBar.step();
        }
    }

    randomBotAccountPrefix = sConfig.GetStringDefault("AiPlayerbot.RandomBotAccountPrefix", "rndbot");
    randomBotAccountCount = sConfig.GetIntDefault("AiPlayerbot.RandomBotAccountCount", 50);
    deleteRandomBotAccounts = sConfig.GetBoolDefault("AiPlayerbot.DeleteRandomBotAccounts", false);
    randomBotGuildCount = sConfig.GetIntDefault("AiPlayerbot.RandomBotGuildCount", 20);
    deleteRandomBotGuilds = sConfig.GetBoolDefault("AiPlayerbot.DeleteRandomBotGuilds", false);

    //arena
    randomBotArenaTeamCount = sConfig.GetIntDefault("AiPlayerbot.RandomBotArenaTeamCount", 20);
    deleteRandomBotArenaTeams = sConfig.GetBoolDefault("AiPlayerbot.DeleteRandomBotArenaTeams", false);

    //cosmetics (by lidocain)
    randomBotShowCloak = sConfig.GetBoolDefault("AiPlayerbot.RandomBotShowCloak", false);
    randomBotShowHelmet = sConfig.GetBoolDefault("AiPlayerbot.RandomBotShowHelmet", false);

    //SPP switches
    enableGreet = sConfig.GetBoolDefault("AiPlayerbot.EnableGreet", false);
    disableRandomLevels = sConfig.GetBoolDefault("AiPlayerbot.DisableRandomLevels", false);
    instantRandomize = sConfig.GetBoolDefault("AiPlayerbot.InstantRandomize", true);
    randomBotRandomPassword = sConfig.GetBoolDefault("AiPlayerbot.RandomBotRandomPassword", true);
    playerbotsXPrate = sConfig.GetFloatDefault("AiPlayerbot.XPRate", 1.0f);
    // Native-map, opt-in bot AI load controller. Core Player/Map/session updates are never skipped.
    // Legacy negative switches are retained as hard kill-switches for existing configurations.
    disableBotOptimizations = sConfig.GetBoolDefault("AiPlayerbot.DisableBotOptimizations", false);
    disableActivityPriorities = sConfig.GetBoolDefault("AiPlayerbot.DisableActivityPriorities", false);
    forceActiveWhenNearPlayer = sConfig.GetBoolDefault("AiPlayerbot.ForceActiveWhenNearPlayer", false);
    limitCombatActivity = sConfig.GetBoolDefault("AiPlayerbot.LimitCombatActivity", false);
    guildOrderAlwaysActive = sConfig.GetBoolDefault("AiPlayerbot.GuildOrderAlwaysActive", true);
    diffWithPlayer = sConfig.GetIntDefault("AiPlayerbot.DiffWithPlayer", 100);
    diffEmpty = sConfig.GetIntDefault("AiPlayerbot.DiffEmpty", 200);

    auto loadUInt = [&](const char* key, int32 defaultValue, int32 minValue, int32 maxValue) -> uint32
    {
        int32 value = sConfig.GetIntDefault(key, defaultValue);
        value = std::max(minValue, std::min(maxValue, value));
        return uint32(value);
    };

    loadOptimizationEnabled = sConfig.GetBoolDefault("AiPlayerbot.LoadOptimization.Enabled", false);
    loadOptimizationSampleIntervalMs = loadUInt("AiPlayerbot.LoadOptimization.SampleIntervalMs", 1000, 100, 60000);
    loadOptimizationDecisionCacheMs = loadUInt("AiPlayerbot.LoadOptimization.DecisionCacheMs", 5000, 0, 600000);
    loadOptimizationRotationIntervalSec = loadUInt("AiPlayerbot.LoadOptimization.RotationIntervalSec", 60, 1, 86400);
    loadOptimizationBaseActivity = sConfig.GetFloatDefault("AiPlayerbot.LoadOptimization.BaseActivity", 100.0f);
    loadOptimizationMinActivity = sConfig.GetFloatDefault("AiPlayerbot.LoadOptimization.MinActivity", 10.0f);
    loadOptimizationMaxActivity = sConfig.GetFloatDefault("AiPlayerbot.LoadOptimization.MaxActivity", 100.0f);
    loadOptimizationTargetDiffWithPlayers = loadUInt("AiPlayerbot.LoadOptimization.TargetDiffWithPlayers", int32(diffWithPlayer), 1, 10000);
    loadOptimizationTargetDiffEmpty = loadUInt("AiPlayerbot.LoadOptimization.TargetDiffEmpty", int32(diffEmpty), 1, 10000);
    loadOptimizationPidKp = std::max(0.0f, sConfig.GetFloatDefault("AiPlayerbot.LoadOptimization.PID.Kp", 0.05f));
    loadOptimizationPidKi = std::max(0.0f, sConfig.GetFloatDefault("AiPlayerbot.LoadOptimization.PID.Ki", 0.001f));
    loadOptimizationPidKd = std::max(0.0f, sConfig.GetFloatDefault("AiPlayerbot.LoadOptimization.PID.Kd", 0.05f));
    loadOptimizationMinimalReactionMultiplier = loadUInt("AiPlayerbot.LoadOptimization.MinimalReactionMultiplier", 10, 1, 1000);
    loadOptimizationInactiveActionDelayMs = loadUInt("AiPlayerbot.LoadOptimization.InactiveActionDelayMs", 0, 0, 600000);
    loadOptimizationProtectPlayerInteraction = sConfig.GetBoolDefault("AiPlayerbot.LoadOptimization.Protect.PlayerInteraction", true);
    loadOptimizationProtectCombat = sConfig.GetBoolDefault("AiPlayerbot.LoadOptimization.Protect.Combat", true);
    loadOptimizationProtectBattleground = sConfig.GetBoolDefault("AiPlayerbot.LoadOptimization.Protect.Battleground", true);
    loadOptimizationProtectInstances = sConfig.GetBoolDefault("AiPlayerbot.LoadOptimization.Protect.Instance", true);

    auto loadBracket = [&](const char* name, uint32 defaultMin, uint32 defaultFull)
    {
        LoadOptimizationBracket bracket;
        std::string prefix = std::string("AiPlayerbot.LoadOptimization.Bracket.") + name;
        bracket.minActivity = loadUInt((prefix + ".Min").c_str(), int32(defaultMin), 0, 100);
        bracket.fullActivity = loadUInt((prefix + ".Full").c_str(), int32(defaultFull), 0, 100);
        if (bracket.fullActivity < bracket.minActivity)
            bracket.fullActivity = bracket.minActivity;
        return bracket;
    };

    loadOptimizationPlayerInteractionBracket = loadBracket("PlayerInteraction", 0, 10);
    loadOptimizationBattlegroundBracket = loadBracket("Battleground", 0, 10);
    loadOptimizationInstanceBracket = loadBracket("Instance", 0, 5);
    loadOptimizationCombatBracket = loadBracket("Combat", 0, 10);
    loadOptimizationBgQueueBracket = loadBracket("BGQueue", 0, 20);
    loadOptimizationLfgBracket = loadBracket("LFG", 0, 30);
    loadOptimizationNearbyPlayerBracket = loadBracket("NearbyPlayer", 0, 40);
    loadOptimizationSocialBracket = loadBracket("Social", 0, 50);
    loadOptimizationNoPathBracket = loadBracket("NoPath", 50, 99);
    loadOptimizationActiveAreaBracket = loadBracket("ActiveArea", 50, 100);
    loadOptimizationEmptyServerBracket = loadBracket("EmptyServer", 50, 100);
    loadOptimizationActiveMapBracket = loadBracket("ActiveMap", 70, 100);
    loadOptimizationInactiveMapBracket = loadBracket("InactiveMap", 80, 100);

    loadOptimizationMinActivity = std::max(0.0f, std::min(100.0f, loadOptimizationMinActivity));
    loadOptimizationMaxActivity = std::max(loadOptimizationMinActivity, std::min(100.0f, loadOptimizationMaxActivity));
    loadOptimizationBaseActivity = std::max(loadOptimizationMinActivity, std::min(loadOptimizationMaxActivity, loadOptimizationBaseActivity));
    RandombotsWalkingRPG = sConfig.GetBoolDefault("AiPlayerbot.RandombotsWalkingRPG", false);
    RandombotsWalkingRPGInDoors = sConfig.GetBoolDefault("AiPlayerbot.RandombotsWalkingRPG.InDoors", false);
    minEnchantingBotLevel = sConfig.GetIntDefault("AiPlayerbot.minEnchantingBotLevel", 60);
    randombotStartingLevel = sConfig.GetIntDefault("AiPlayerbot.randombotStartingLevel", 5);
    gearscorecheck = sConfig.GetBoolDefault("AiPlayerbot.GearScoreCheck", false);
    levelCheck = sConfig.GetIntDefault("AiPlayerbot.LevelCheck", 30);
    randomBotPreQuests = sConfig.GetBoolDefault("AiPlayerbot.PreQuests", true);
    randomBotSayWithoutMaster = sConfig.GetBoolDefault("AiPlayerbot.RandomBotSayWithoutMaster", false);
    randomBotInvitePlayer = sConfig.GetBoolDefault("AiPlayerbot.RandomBotInvitePlayer", true);
    randomBotGroupNearby = sConfig.GetBoolDefault("AiPlayerbot.RandomBotGroupNearby", true);
    randomBotRaidNearby = sConfig.GetBoolDefault("AiPlayerbot.RandomBotRaidNearby", true);
    randomBotGuildNearby = sConfig.GetBoolDefault("AiPlayerbot.RandomBotGuildNearby", true);
    inviteChat = sConfig.GetBoolDefault("AiPlayerbot.InviteChat", true);
    botsSilent = sConfig.GetBoolDefault("AiPlayerbot.BotsSilent", false);
    enableActionLog = sConfig.GetBoolDefault("AiPlayerbot.EnableActionLog", false);
    enableOffSpecStrategies = sConfig.GetBoolDefault("AiPlayerbot.EnableOffSpecStrategies", true);
    useWanderAsDefaultFollowStrategy = sConfig.GetBoolDefault("AiPlayerbot.UseWanderAsDefaultFollowStrategy", true);
    defaultFormation = sConfig.GetStringDefault("AiPlayerbot.DefaultFormation", "near");

    guildMaxBotLimit = sConfig.GetIntDefault("AiPlayerbot.GuildMaxBotLimit", 1000);

    ////////////////////////////
    enableBroadcasts = sConfig.GetBoolDefault("AiPlayerbot.EnableBroadcasts", true);

    //broadcastChanceMaxValue is used in urand(1, broadcastChanceMaxValue) for broadcasts,
    //lowering it will increase the chance, setting it to 0 will disable broadcasts
    //for internal use, not intended to be change by the user
    broadcastChanceMaxValue = enableBroadcasts ? 30000 : 0;

    //all broadcast chances should be in range 1-broadcastChanceMaxValue, value of 0 will disable this particular broadcast
    //setting value to max does not guarantee the broadcast, as there are some internal randoms as well
    broadcastToGuildGlobalChance = sConfig.GetIntDefault("AiPlayerbot.BroadcastToGuildGlobalChance", 30000);
    broadcastToWorldGlobalChance = sConfig.GetIntDefault("AiPlayerbot.BroadcastToWorldGlobalChance", 30000);
    broadcastToGeneralGlobalChance = sConfig.GetIntDefault("AiPlayerbot.BroadcastToGeneralGlobalChance", 30000);
    broadcastToTradeGlobalChance = sConfig.GetIntDefault("AiPlayerbot.BroadcastToTradeGlobalChance", 30000);
    broadcastToLFGGlobalChance = sConfig.GetIntDefault("AiPlayerbot.BroadcastToLFGGlobalChance", 30000);
    broadcastToLocalDefenseGlobalChance = sConfig.GetIntDefault("AiPlayerbot.BroadcastToLocalDefenseGlobalChance", 30000);
    broadcastToWorldDefenseGlobalChance = sConfig.GetIntDefault("AiPlayerbot.BroadcastToWorldDefenseGlobalChance", 30000);
    broadcastToGuildRecruitmentGlobalChance = sConfig.GetIntDefault("AiPlayerbot.BroadcastToGuildRecruitmentGlobalChance", 30000);
    broadcastToSayGlobalChance = sConfig.GetIntDefault("AiPlayerbot.BroadcastToSayGlobalChance", 30000);
    broadcastToYellGlobalChance = sConfig.GetIntDefault("AiPlayerbot.BroadcastToYellGlobalChance", 30000);

    broadcastChanceLootingItemPoor = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceLootingItemPoor", 30);
    broadcastChanceLootingItemNormal = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceLootingItemNormal", 300);
    broadcastChanceLootingItemUncommon = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceLootingItemUncommon", 10000);
    broadcastChanceLootingItemRare = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceLootingItemRare", 20000);
    broadcastChanceLootingItemEpic = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceLootingItemEpic", 30000);
    broadcastChanceLootingItemLegendary = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceLootingItemLegendary", 30000);
    broadcastChanceLootingItemArtifact = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceLootingItemArtifact", 30000);

    broadcastChanceQuestAccepted = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceQuestAccepted", 6000);
    broadcastChanceQuestUpdateObjectiveCompleted = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceQuestUpdateObjectiveCompleted", 300);
    broadcastChanceQuestUpdateObjectiveProgress = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceQuestUpdateObjectiveProgress", 300);
    broadcastChanceQuestUpdateFailedTimer = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceQuestUpdateFailedTimer", 300);
    broadcastChanceQuestUpdateComplete = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceQuestUpdateComplete", 1000);
    broadcastChanceQuestTurnedIn = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceQuestTurnedIn", 10000);

    broadcastChanceKillNormal = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceKillNormal", 30);
    broadcastChanceKillElite = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceKillElite", 300);
    broadcastChanceKillRareelite = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceKillRareelite", 3000);
    broadcastChanceKillWorldboss = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceKillWorldboss", 20000);
    broadcastChanceKillRare = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceKillRare", 10000);
    broadcastChanceKillUnknown = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceKillUnknown", 100);
    broadcastChanceKillPet = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceKillPet", 10);
    broadcastChanceKillPlayer = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceKillPlayer", 30);

    broadcastChanceLevelupGeneric = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceLevelupGeneric", 20000);
    broadcastChanceLevelupTenX = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceLevelupTenX", 30000);
    broadcastChanceLevelupMaxLevel = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceLevelupMaxLevel", 30000);

    broadcastChanceSuggestInstance = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceSuggestInstance", 5000);
    broadcastChanceSuggestQuest = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceSuggestQuest", 10000);
    broadcastChanceSuggestGrindMaterials = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceSuggestGrindMaterials", 5000);
    broadcastChanceSuggestGrindReputation = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceSuggestGrindReputation", 5000);
    broadcastChanceSuggestSell = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceSuggestSell", 300);
    broadcastChanceSuggestSomething = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceSuggestSomething", 30000);

    broadcastChanceSuggestSomethingToxic = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceSuggestSomethingToxic", 0);

    broadcastChanceSuggestToxicLinks = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceSuggestToxicLinks", 0);
    toxicLinksPrefix = sConfig.GetStringDefault("AiPlayerbot.ToxicLinksPrefix", "gnomes");

    broadcastChanceSuggestThunderfury = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceSuggestThunderfury", 1);

    //does not depend on global chance
    broadcastChanceGuildManagement = sConfig.GetIntDefault("AiPlayerbot.BroadcastChanceGuildManagement", 30000);
    ////////////////////////////

    toxicLinksRepliesChance = sConfig.GetIntDefault("AiPlayerbot.ToxicLinksRepliesChance", 30);   //0-100
    thunderfuryRepliesChance = sConfig.GetIntDefault("AiPlayerbot.ThunderfuryRepliesChance", 40); //0-100
    guildRepliesRate = sConfig.GetIntDefault("AiPlayerbot.GuildRepliesRate", 100);                //0-100

    botAcceptDuelMinimumLevel = sConfig.GetIntDefault("AiPlayerbot.BotAcceptDuelMinimumLevel", 10);

    randomBotFormGuild = sConfig.GetBoolDefault("AiPlayerbot.RandomBotFormGuild", true);

    boostFollow = sConfig.GetBoolDefault("AiPlayerbot.BoostFollow", false);
    turnInRpg = sConfig.GetBoolDefault("AiPlayerbot.TurnInRpg", false);
    shareTargets = sConfig.GetBoolDefault("AiPlayerbot.ShareTargets", true);
    globalSoundEffects = sConfig.GetBoolDefault("AiPlayerbot.GlobalSoundEffects", false);
    nonGmFreeSummon = sConfig.GetBoolDefault("AiPlayerbot.NonGmFreeSummon", false);

    //SPP automation
    autoPickReward = sConfig.GetStringDefault("AiPlayerbot.AutoPickReward", "no");
    autoEquipUpgradeLoot = sConfig.GetBoolDefault("AiPlayerbot.AutoEquipUpgradeLoot", false);
    syncQuestWithPlayer = sConfig.GetBoolDefault("AiPlayerbot.SyncQuestWithPlayer", false);
    syncQuestForPlayer = sConfig.GetBoolDefault("AiPlayerbot.SyncQuestForPlayer", false);
    autoTrainSpells = sConfig.GetStringDefault("AiPlayerbot.AutoTrainSpells", "no");
    autoPickTalents = sConfig.GetStringDefault("AiPlayerbot.AutoPickTalents", "no");
    autoLearnTrainerSpells = sConfig.GetBoolDefault("AiPlayerbot.AutoLearnTrainerSpells", false);
    autoLearnQuestSpells = sConfig.GetBoolDefault("AiPlayerbot.AutoLearnQuestSpells", false);
    autoLearnDroppedSpells = sConfig.GetBoolDefault("AiPlayerbot.AutoLearnDroppedSpells", false);
    autoDoQuests = sConfig.GetBoolDefault("AiPlayerbot.AutoDoQuests", true);
    syncLevelWithPlayers = sConfig.GetBoolDefault("AiPlayerbot.SyncLevelWithPlayers", false);
    syncLevelMaxAbove = sConfig.GetIntDefault("AiPlayerbot.SyncLevelMaxAbove", 5);
    syncLevelNoPlayer = sConfig.GetIntDefault("AiPlayerbot.SyncLevelNoPlayer", randombotStartingLevel);
    syncAltLevelToMaster = sConfig.GetBoolDefault("AiPlayerbot.SyncAltLevelToMaster", false);
    tweakValue = sConfig.GetIntDefault("AiPlayerbot.TweakValue", 0);
    talentsInPublicNote = sConfig.GetBoolDefault("AiPlayerbot.TalentsInPublicNote", false);
    respawnModNeutral = sConfig.GetFloatDefault("AiPlayerbot.RespawnModNeutral", 10.0f);
    respawnModHostile = sConfig.GetFloatDefault("AiPlayerbot.RespawnModHostile", 5.0f);
    respawnModThreshold = sConfig.GetIntDefault("AiPlayerbot.RespawnModThreshold", 10);
    respawnModMax = sConfig.GetIntDefault("AiPlayerbot.RespawnModMax", 18);
    respawnModForPlayerBots = sConfig.GetBoolDefault("AiPlayerbot.RespawnModForPlayerBots", false);
    respawnModForInstances = sConfig.GetBoolDefault("AiPlayerbot.RespawnModForInstances", false);

    //LLM START
    llmEnabled = sConfig.GetIntDefault("AiPlayerbot.LLMEnabled", 1);
    llmApiEndpoint = sConfig.GetStringDefault("AiPlayerbot.LLMApiEndpoint", "http://127.0.0.1:5001/api/v1/generate");
    try
    {
        llmEndPointUrl = parseUrl(llmApiEndpoint);
    }
    catch (const std::invalid_argument& e)
    {
        sLog.outError("Unable to parse LLMApiEndpoint url: %s", e.what());
    }
    llmApiKey = sConfig.GetStringDefault("AiPlayerbot.LLMApiKey", "");
    llmApiJson = sConfig.GetStringDefault("AiPlayerbot.LLMApiJson", "{ \"max_length\": 100, \"prompt\": \"[<pre prompt>]<context> <prompt> <post prompt>\"}");
    llmContextLength = sConfig.GetIntDefault("AiPlayerbot.LLMContextLength", 4096);
    llmGenerationTimeout = sConfig.GetIntDefault("AiPlayerbot.LLMGenerationTimeout", 600);
    llmMaxSimultaniousGenerations = sConfig.GetIntDefault("AiPlayerbot.LLMMaxSimultaniousGenerations", 100);

    llmPrePrompt = sConfig.GetStringDefault("AiPlayerbot.LLMPrePrompt", "You are a roleplaying character in World of Warcraft: <expansion name>. Your name is <bot name>. The <other type> <other name> is speaking to you <channel name> and is an <other gender> <other race> <other class> of level <other level>. You are level <bot level> and play as a <bot gender> <bot race> <bot class> that is currently in <bot subzone> <bot zone>. Answer as a roleplaying character. Limit responses to 100 characters.");

    llmPreRpgPrompt = sConfig.GetStringDefault("AiPlayerbot.LLMRpgPrompt", "In World of Warcraft: <expansion name> in <bot zone> <bot subzone> stands <bot type> <bot name> a level <bot level> <bot gender> <bot race> <bot class>."
                                                                          " Standing nearby is <unit type> <unit name> <unit subname> a level <unit level> <unit gender> <unit race> <unit faction> <unit class>. Answer as a roleplaying character. Limit responses to 100 characters.");

    llmPrompt = sConfig.GetStringDefault("AiPlayerbot.LLMPrompt", "<receiver name>:<initial message>");
    llmPostPrompt = sConfig.GetStringDefault("AiPlayerbot.LLMPostPrompt", "<sender name>:");

    llmResponseStartPattern = sConfig.GetStringDefault("AiPlayerbot.LLMResponseStartPattern", R"(("text":\s*"))");
    llmResponseEndPattern = sConfig.GetStringDefault("AiPlayerbot.LLMResponseEndPattern", R"(("|\b(?!<sender name>\b)(\w+):))");
    llmResponseDeletePattern = sConfig.GetStringDefault("AiPlayerbot.LLMResponseDeletePattern", R"((\\n|<sender name>:|\\[^ ]+))");
    llmResponseSplitPattern = sConfig.GetStringDefault("AiPlayerbot.LLMResponseSplitPattern", R"((\*.*?\*)|(\[.*?\])|(\'.*\')|([^\*\[\] ][^\*\[\]]+?[.?!]))");

    if (false) //Disable for release
    {
        sLog.outError("# AiPlayerbot.LLMResponseStartPattern = %s", llmResponseStartPattern.c_str());
        sLog.outError("# AiPlayerbot.LLMResponseEndPattern = %s", llmResponseEndPattern.c_str());
        sLog.outError("# AiPlayerbot.LLMResponseDeletePattern = %s", llmResponseDeletePattern.c_str());
        sLog.outError("# AiPlayerbot.LLMResponseSplitPattern = %s", llmResponseSplitPattern.c_str());
    }

    try
    {
        std::regex pattern(llmResponseStartPattern);
    }
    catch (const std::regex_error& e)
    {
        sLog.outError("Regex error in %s: %s", llmResponseStartPattern.c_str(), e.what());
    }

    try
    {
        std::regex pattern(llmResponseEndPattern);
    }
    catch (const std::regex_error& e)
    {
        sLog.outError("Regex error in %s: %s", llmResponseEndPattern.c_str(), e.what());
    }

    try
    {
        std::regex pattern(llmResponseDeletePattern);
    }
    catch (const std::regex_error& e)
    {
        sLog.outError("Regex error in %s: %s", llmResponseDeletePattern.c_str(), e.what());
    }

    try
    {
        std::regex pattern(llmResponseSplitPattern);
    }
    catch (const std::regex_error& e)
    {
        sLog.outError("Regex error in %s: %s", llmResponseSplitPattern.c_str(), e.what());
    }

    llmGlobalContext = sConfig.GetBoolDefault("AiPlayerbot.LLMGlobalContext", false);
    llmBotToBotChatChance = sConfig.GetIntDefault("AiPlayerbot.LLMBotToBotChatChance", 0);
    llmRpgAIChatChance = sConfig.GetIntDefault("AiPlayerbot.LLMRpgAIChatChance", 100);

    std::list<std::string> blockedChannels;
    LoadListString<std::list<std::string>>(sConfig.GetStringDefault("AiPlayerbot.LLMBlockedReplyChannels", ""), blockedChannels);
    std::map<std::string, ChatChannelSource> sourceName;
    sourceName["guild"] = ChatChannelSource::SRC_GUILD;
    sourceName["world"] = ChatChannelSource::SRC_WORLD;
    sourceName["general"] = ChatChannelSource::SRC_GENERAL;
    sourceName["trade"] = ChatChannelSource::SRC_TRADE;
    sourceName["lfg"] = ChatChannelSource::SRC_LOOKING_FOR_GROUP;
    sourceName["ldefence"] = ChatChannelSource::SRC_LOCAL_DEFENSE;
    sourceName["wdefence"] = ChatChannelSource::SRC_WORLD_DEFENSE;
    sourceName["grecruitement"] = ChatChannelSource::SRC_GUILD_RECRUITMENT;
    sourceName["say"] = ChatChannelSource::SRC_SAY;
    sourceName["whisper"] = ChatChannelSource::SRC_WHISPER;
    sourceName["emote"] = ChatChannelSource::SRC_EMOTE;
    sourceName["temote"] = ChatChannelSource::SRC_TEXT_EMOTE;
    sourceName["yell"] = ChatChannelSource::SRC_YELL;
    sourceName["party"] = ChatChannelSource::SRC_PARTY;
    sourceName["raid"] = ChatChannelSource::SRC_RAID;

    for (auto& channelName : blockedChannels)
        llmBlockedReplyChannels.insert(sourceName[channelName]);

    {
        std::string promptsFile = sConfig.GetStringDefault("AiPlayerbot.LLMDefaultPromptsFile", "llm_character_card");
        LoadLLMDefaultPrompts(promptsFile);
    }

    // Gear progression system
    gearProgressionSystemEnabled = sConfig.GetBoolDefault("AiPlayerbot.GearProgressionSystem.Enable", false);

    // Gear progression phase
    for (uint8 phase = 0; phase < MAX_GEAR_PROGRESSION_LEVEL; phase++)
    {
        std::ostringstream os;
        os << "AiPlayerbot.GearProgressionSystem." << std::to_string(phase) << ".MinItemLevel";
        gearProgressionSystemItemLevels[phase][0] = sConfig.GetIntDefault(os.str().c_str(), 9999999);
        os.str("");
        os << "AiPlayerbot.GearProgressionSystem." << std::to_string(phase) << ".MaxItemLevel";
        gearProgressionSystemItemLevels[phase][1] = sConfig.GetIntDefault(os.str().c_str(), 9999999);

        // Gear progression class
        for (uint8 cls = 1; cls < MAX_CLASSES; cls++)
        {
            // Gear progression spec
            for (uint8 spec = 0; spec < 4; spec++)
            {
                // Gear progression slot
                for (uint8 slot = 0; slot < SLOT_EMPTY; slot++)
                {
                    std::ostringstream os;
                    os << "AiPlayerbot.GearProgressionSystem." << std::to_string(phase) << "." << std::to_string(cls) << "." << std::to_string(spec) << "." << std::to_string(slot);
                    gearProgressionSystemItems[phase][cls][spec][slot] = sConfig.GetIntDefault(os.str().c_str(), -1);
                }
            }
        }
    }

    sLog.outString("Loading free bots.");
    selfBotLevel = BotSelfBotLevel(sConfig.GetIntDefault("AiPlayerbot.SelfBotLevel", uint32(BotSelfBotLevel::GM_ONLY)));
    LoadListString<std::list<std::string>>(sConfig.GetStringDefault("AiPlayerbot.ToggleAlwaysOnlineAccounts", ""), toggleAlwaysOnlineAccounts);
    LoadListString<std::list<std::string>>(sConfig.GetStringDefault("AiPlayerbot.ToggleAlwaysOnlineChars", ""), toggleAlwaysOnlineChars);

    for (std::string& nm : toggleAlwaysOnlineAccounts)
        std::transform(nm.begin(), nm.end(), nm.begin(), toupper);

    for (std::string& nm : toggleAlwaysOnlineChars)
    {
        std::transform(nm.begin(), nm.end(), nm.begin(), tolower);
        nm[0] = toupper(nm[0]);
    }

    loadFreeAltBotAccounts();

    targetPosRecalcDistance = sConfig.GetFloatDefault("AiPlayerbot.TargetPosRecalcDistance", 0.1f),

    sLog.outString("Loading area levels.");
    sTravelMgr.LoadAreaLevels();
    sLog.outString("Loading spellIds.");
    ChatHelper::PopulateSpellNameList();
    ItemUsageValue::PopulateProfessionReagentIds();
    ItemUsageValue::PopulateSoldByVendorItemIds();
    ItemUsageValue::PopulateReagentItemIdsForCraftableItemIds();

    RandomPlayerbotFactory::CreateRandomBots();
    PlayerbotFactory::Init();
    sRandomItemMgr.Init();
    sPlayerbotTextMgr.LoadBotTexts();
    sPlayerbotTextMgr.LoadBotTextChance();
    sPlayerbotHelpMgr.LoadBotHelpTexts();

    LoadTalentSpecs();

    if (sPlayerbotAIConfig.autoDoQuests)
    {
        sLog.outString("Loading Quest Detail Data...");
        sTravelMgr.LoadQuestTravelTable();
    }

    sLog.outString("Loading named locations...");
    sRandomPlayerbotMgr.LoadNamedLocations();

    if (sPlayerbotAIConfig.randomBotJoinBG)
        sRandomPlayerbotMgr.LoadBattleMastersCache();

    sLog.outString("---------------------------------------");
    sLog.outString("        AI Playerbot initialized       ");
    sLog.outString("---------------------------------------");
    sLog.outString();

    return true;
}

bool PlayerbotAIConfig::IsInRandomAccountList(uint32 id)
{
    // Fast path: already in the loaded/discovered list.
    if (find(randomBotAccounts.begin(), randomBotAccounts.end(), id) != randomBotAccounts.end())
        return true;

    // Slow path: the account may have been created at runtime AFTER the
    // startup loader ran (e.g. via `.rndbot create` with a number outside
    // the configured pool, or any manual account creation that legitimately
    // belongs to the bot pool). Look up the username and accept any account
    // whose name starts with the configured RNDBOT prefix. Once recognized,
    // add to the cached list so subsequent calls take the fast path.
    auto qr = LoginDatabase.PQuery("SELECT username FROM account WHERE id = %u", id);
    if (!qr)
        return false;
    Field* fields = qr->Fetch();
    std::string username = fields[0].GetCppString();
    std::string prefix = randomBotAccountPrefix;
    if (username.size() < prefix.size())
        return false;
    // Case-insensitive prefix compare (account usernames are typically
    // upper-cased in the DB but the config string may not be).
    for (size_t i = 0; i < prefix.size(); ++i)
    {
        if (std::tolower((unsigned char)username[i]) != std::tolower((unsigned char)prefix[i]))
            return false;
    }
    randomBotAccounts.push_back(id);
    return true;
}

bool PlayerbotAIConfig::IsFreeAltBot(uint32 guid)
{
    for (auto bot : freeAltBots)
        if (bot.second == guid)
            return true;

    return false;
}

bool PlayerbotAIConfig::IsInRandomQuestItemList(uint32 id)
{
    return find(randomBotQuestItems.begin(), randomBotQuestItems.end(), id) != randomBotQuestItems.end();
}

bool PlayerbotAIConfig::IsInPvpProhibitedZone(uint32 id)
{
    return find(pvpProhibitedZoneIds.begin(), pvpProhibitedZoneIds.end(), id) != pvpProhibitedZoneIds.end();
}

std::string PlayerbotAIConfig::GetValue(std::string name)
{
    std::ostringstream out;

    if (name == "GlobalCooldown")
        out << globalCoolDown;
    else if (name == "ReactDelay")
        out << reactDelay;

    else if (name == "SightDistance")
        out << sightDistance;
    else if (name == "SpellDistance")
        out << spellDistance;
    else if (name == "ReactDistance")
        out << reactDistance;
    else if (name == "GrindDistance")
        out << grindDistance;
    else if (name == "LootDistance")
        out << lootDistance;
    else if (name == "FleeDistance")
        out << fleeDistance;

    else if (name == "CriticalHealth")
        out << criticalHealth;
    else if (name == "LowHealth")
        out << lowHealth;
    else if (name == "MediumHealth")
        out << mediumHealth;
    else if (name == "AlmostFullHealth")
        out << almostFullHealth;
    else if (name == "LowMana")
        out << lowMana;

    else if (name == "IterationsPerTick")
        out << iterationsPerTick;

    return out.str();
}

void PlayerbotAIConfig::SetValue(std::string name, std::string value)
{
    std::istringstream out(value, std::istringstream::in);

    if (name == "GlobalCooldown")
        out >> globalCoolDown;
    else if (name == "ReactDelay")
        out >> reactDelay;

    else if (name == "SightDistance")
        out >> sightDistance;
    else if (name == "SpellDistance")
        out >> spellDistance;
    else if (name == "ReactDistance")
        out >> reactDistance;
    else if (name == "GrindDistance")
        out >> grindDistance;
    else if (name == "LootDistance")
        out >> lootDistance;
    else if (name == "FleeDistance")
        out >> fleeDistance;

    else if (name == "CriticalHealth")
        out >> criticalHealth;
    else if (name == "LowHealth")
        out >> lowHealth;
    else if (name == "MediumHealth")
        out >> mediumHealth;
    else if (name == "AlmostFullHealth")
        out >> almostFullHealth;
    else if (name == "LowMana")
        out >> lowMana;

    else if (name == "IterationsPerTick")
        out >> iterationsPerTick;
}

void PlayerbotAIConfig::loadFreeAltBotAccounts()
{
    bool allCharsOnline = (selfBotLevel == BotSelfBotLevel::ALWAYS_ACTIVE);

    freeAltBots.clear();

    auto results = LoginDatabase.PQuery("SELECT username, id FROM account where username not like '%s%%'", randomBotAccountPrefix.c_str());
    if (results)
    {
        do
        {
            bool accountToggle = false;

            Field* fields = results->Fetch();
            std::string accountName = fields[0].GetString();
            uint32 accountId = fields[1].GetUInt32();

            if (std::find(toggleAlwaysOnlineAccounts.begin(), toggleAlwaysOnlineAccounts.end(), accountName) != toggleAlwaysOnlineAccounts.end())
                accountToggle = true;

            auto result = CharacterDatabase.PQuery("SELECT name, guid FROM characters WHERE account = '%u'", accountId);
            if (!result)
                continue;

            do
            {
                bool charToggle = false;

                Field* fields = result->Fetch();
                std::string charName = fields[0].GetString();
                uint32 guid = fields[1].GetUInt32();

                BotAlwaysOnline always = BotAlwaysOnline(sRandomPlayerbotMgr.GetValue(guid, "always"));

                if (always == BotAlwaysOnline::DISABLED_BY_COMMAND)
                    continue;

                if (std::find(toggleAlwaysOnlineChars.begin(), toggleAlwaysOnlineChars.end(), charName) != toggleAlwaysOnlineChars.end())
                    charToggle = true;

                bool thisCharAlwaysOnline = allCharsOnline;

                if (accountToggle || charToggle)
                    thisCharAlwaysOnline = !thisCharAlwaysOnline;

                if ((thisCharAlwaysOnline && always != BotAlwaysOnline::DISABLED_BY_COMMAND) || always == BotAlwaysOnline::ACTIVE)
                {
                    sLog.outString("Enabling always online for %s", charName.c_str());
                    freeAltBots.push_back(std::make_pair(accountId, guid));
                }

            } while (result->NextRow());

        } while (results->NextRow());
    }
}

std::string PlayerbotAIConfig::GetTimestampStr()
{
    time_t t = time(nullptr);
    tm* aTm = localtime(&t);
    //       YYYY   year
    //       MM     month (2 digits 01-12)
    //       DD     day (2 digits 01-31)
    //       HH     hour (2 digits 00-23)
    //       MM     minutes (2 digits 00-59)
    //       SS     seconds (2 digits 00-59)
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", aTm);

    return std::string(buf);
}

bool PlayerbotAIConfig::openLog(std::string fileName, char const* mode, bool haslog)
{
    if (!haslog && !hasLog(fileName))
        return false;

    auto logFileIt = logFiles.find(fileName);
    if (logFileIt == logFiles.end())
    {
        logFiles.insert(make_pair(fileName, std::make_pair(nullptr, false)));
        logFileIt = logFiles.find(fileName);
    }

    FILE* file = logFileIt->second.first;
    bool fileOpen = logFileIt->second.second;

    if (fileOpen) //close log file
        fclose(file);

    std::string m_logsDir = sConfig.GetStringDefault("LogsDir", "");
    if (!m_logsDir.empty())
    {
        if ((m_logsDir.at(m_logsDir.length() - 1) != '/') && (m_logsDir.at(m_logsDir.length() - 1) != '\\'))
            m_logsDir.append("/");
    }

    file = fopen((m_logsDir + fileName).c_str(), mode);
    fileOpen = true;

    logFileIt->second.first = file;
    logFileIt->second.second = fileOpen;

    return true;
}

void PlayerbotAIConfig::log(std::string fileName, const char* str, ...)
{
    if (!str)
        return;

    std::lock_guard<std::mutex> guard(m_logMtx);

    if (!isLogOpen(fileName))
        if (!openLog(fileName, "a"))
            return;

    FILE* file = logFiles.find(fileName)->second.first;

    va_list ap;
    va_start(ap, str);
    vfprintf(file, str, ap);
    fprintf(file, "\n");
    va_end(ap);
    fflush(file);

    fflush(stdout);
}

void PlayerbotAIConfig::logEvent(PlayerbotAI* ai, std::string eventName, std::string info1, std::string info2)
{
    if (hasLog("bot_events.csv"))
    {
        Player* bot = ai->GetBot();

        std::ostringstream out;
        out << sPlayerbotAIConfig.GetTimestampStr() << "+00,";
        out << bot->GetName() << ",";
        out << eventName << ",";
        out << std::fixed << std::setprecision(2);
        WorldPosition(bot).printWKT(out);

        out << std::to_string(bot->GetRace()) << ",";
        out << std::to_string(bot->GetClass()) << ",";
        float subLevel = ai->GetLevelFloat();

        out << subLevel << ",";

        out << "\"" << info1 << "\",";
        out << "\"" << info2 << "\"";

        log("bot_events.csv", out.str().c_str());
    }
};

void PlayerbotAIConfig::logEvent(PlayerbotAI* ai, std::string eventName, ObjectGuid guid, std::string info2)
{
    std::string info1 = "";

    Unit* victim;
    if (guid)
    {
        victim = ai->GetUnit(guid);
        if (victim)
            info1 = victim->GetName();
    }

    logEvent(ai, eventName, info1, info2);
};

bool PlayerbotAIConfig::CanLogAction(PlayerbotAI* ai, std::string actionName, bool isExecute, std::string lastActionName)
{
    bool forRpg = (actionName.find("rpg") == 0) && ai->HasStrategy("debug rpg", BotState::BOT_STATE_NON_COMBAT);

    if (!forRpg)
    {
        if (isExecute && !ai->HasStrategy("debug", BotState::BOT_STATE_NON_COMBAT))
            return false;

        if (!isExecute && !ai->HasStrategy("debug action", BotState::BOT_STATE_NON_COMBAT))
            return false;

        if ((lastActionName == actionName) && (actionName == "melee"))
        {
            return false;
        }
    }

    return std::find(debugFilter.begin(), debugFilter.end(), actionName) == debugFilter.end();
}

void PlayerbotAIConfig::LoadTalentSpecs()
{
    sLog.outString("Loading TalentSpecs");

    uint32 maxSpecLevel = 0;

    for (uint32 cls = 1; cls < MAX_CLASSES; ++cls)
    {
        classSpecs[cls] = ClassSpecs(1 << (cls - 1));
        for (uint32 spec = 0; spec < MAX_LEVEL; ++spec)
        {
            std::ostringstream os;
            os << "AiPlayerbot.PremadeSpecName." << cls << "." << spec;
            std::string specName = sConfig.GetStringDefault(os.str().c_str(), "");
            if (!specName.empty())
            {
                std::ostringstream os;
                os << "AiPlayerbot.PremadeSpecProb." << cls << "." << spec;
                int probability = sConfig.GetIntDefault(os.str().c_str(), 100);

                TalentPath talentPath(spec, specName, probability);

                for (uint32 level = 10; level <= 100; level++)
                {
                    std::ostringstream os;
                    os << "AiPlayerbot.PremadeSpecLink." << cls << "." << spec << "." << level;
                    std::string specLink = sConfig.GetStringDefault(os.str().c_str(), "");
                    specLink = specLink.substr(0, specLink.find("#", 0));
                    specLink = specLink.substr(0, specLink.find(" ", 0));

                    if (!specLink.empty())
                    {
                        if (maxSpecLevel < level)
                            maxSpecLevel = level;

                        std::ostringstream out;

                        //Ignore bad specs.
                        if (!classSpecs[cls].baseSpec.CheckTalentLink(specLink, &out))
                        {
                            sLog.outErrorDb("Error with premade spec link: %s", specLink.c_str());
                            sLog.outErrorDb("%s", out.str().c_str());
                            continue;
                        }

                        TalentSpec linkSpec(&classSpecs[cls].baseSpec, specLink);

                        if (!linkSpec.CheckTalents(TalentSpec::LeveltoPoints(level), &out))
                        {
                            sLog.outErrorDb("Error with premade spec: %s", specLink.c_str());
                            sLog.outErrorDb("%s", out.str().c_str());
                            continue;
                        }

                        talentPath.talentSpec.push_back(linkSpec);
                    }

                    {
                        //Glyphs

                        using GlyphPriority = std::pair<std::string, uint32>;
                        using GlyphPriorityList = std::vector<GlyphPriority>;
                        using GlyphPriorityLevelMap = std::unordered_map<uint32, GlyphPriorityList>;
                        using GlyphPrioritySpecMap = std::unordered_map<uint32, GlyphPriorityLevelMap>;

                        std::ostringstream os;
                        os << "AiPlayerbot.PremadeSpecGlyp." << cls << "." << spec << "." << level;

                        std::string glyphList = sConfig.GetStringDefault(os.str().c_str(), "");
                        glyphList = glyphList.substr(0, glyphList.find("#", 0));
                        boost::trim_right(glyphList);

                        if (!glyphList.empty())
                        {
                            Tokens premadeSpecGlyphs = Qualified::getMultiQualifiers(glyphList, ",");

                            for (auto& glyph : premadeSpecGlyphs)
                            {
                                Tokens tokens = Qualified::getMultiQualifiers(glyph, "|");
                                std::string glyphName = "Glyph of " + tokens[0];
                                uint32 talentId = tokens.size() > 1 ? stoi(tokens[1]) : 0;

                                bool glyphFound = false;
                                for (auto& itemId : sRandomItemMgr.GetGlyphs(1 << (cls - 1)))
                                {
                                    ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);

                                    if (!proto)
                                        continue;

                                    if (proto->Name1 == glyphName)
                                    {
                                        glyphPriorityMap[cls][spec][level].push_back(std::make_pair(itemId, talentId));
                                        glyphFound = true;
                                        break;
                                    }
                                }

                                if (!glyphFound)
                                {
                                    sLog.outError("%s is not found for class %d (spec %d level %d)", glyphName.c_str(), cls, spec, level);
                                }
                            }
                        }
                    }
                }

                //Only add paths that have atleast 1 spec.
                if (talentPath.talentSpec.size() > 0)
                    classSpecs[cls].talentPath.push_back(talentPath);
            }
        }
    }

    if (classSpecs[1].talentPath.empty())
        sLog.outErrorDb("No premade specs found!!");
    else
    {
        if (maxSpecLevel < PLAYER_MAX_LEVEL && randomBotMaxLevel < PLAYER_MAX_LEVEL)
            sLog.outErrorDb("!!!!!!!!!!! randomBotMaxLevel and the talentspec levels are below this expansions max level. Please check if you have the correct config file!!!!!!");
    }
}

void PlayerbotAIConfig::LoadLLMDefaultPrompts(const std::string& fileName)
{
    std::ifstream file(fileName);
    if (!file.is_open())
    {
        sLog.outString("LLM default prompts file '%s' not found or unreadable.", fileName.c_str());
        return;
    }

    std::string line;
    uint32 loaded = 0;

    std::string likePattern = std::string("manual saved string::llmdefaultprompt>%");
    CharacterDatabase.escape_string(likePattern);

    while (std::getline(file, line))
    {
        boost::trim(line);
        if (line.empty() || line.front() == '#')
            continue;

        size_t delim = line.find("::");
        if (delim == std::string::npos)
        {
            sLog.outError("LLM prompts file '%s' contains invalid line (missing '::'): %s", fileName.c_str(), line.c_str());
            continue;
        }

        std::string name = line.substr(0, delim);
        std::string text = line.substr(delim + 2);
        boost::trim(name);
        boost::trim(text);

        if (name.empty())
        {
            sLog.outError("LLM prompts file '%s' contains empty name: %s", fileName.c_str(), line.c_str());
            continue;
        }

        auto result = CharacterDatabase.PQuery("SELECT guid FROM characters WHERE name = '%s' LIMIT 1", name.c_str());
        if (!result)
        {
            sLog.outError("Character '%s' not found in characters DB while loading '%s'.", name.c_str(), fileName.c_str());
            continue;
        }

        Field* fields = result->Fetch();
        uint32 guid = fields[0].GetUInt32();

        CharacterDatabase.PExecute(
            "DELETE FROM `ai_playerbot_db_store` WHERE `guid` = '%u' AND `key` = '%s' AND `value` LIKE '%s'",
            guid, "value", likePattern.c_str());

        std::string dbValue = std::string("manual saved string::llmdefaultprompt>") + text;
        CharacterDatabase.escape_string(dbValue);

        CharacterDatabase.PExecute(
            "INSERT INTO `ai_playerbot_db_store` (`guid`, `preset`, `key`, `value`) VALUES ('%u', '%s', '%s', '%s')",
            guid, "", "value", dbValue.c_str());

        ++loaded;
    }

    sLog.outString("Loaded %u LLM character personalities from %s", loaded, fileName.c_str());
}
