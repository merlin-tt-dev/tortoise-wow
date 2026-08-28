#include "Config/Config.h"

#include "playerbot/playerbot.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "playerbot/PlayerbotFactory.h"
#include "AccountMgr.h"
#include "ObjectMgr.h"
#include "Database/DatabaseEnv.h"
#include "PlayerbotAI.h"
#include "Objects/Player.h"
#include "RandomPlayerbotFactory.h"
#include "PlayerbotAppearanceStore.h"
#include "SystemConfig.h"
#include "SocialMgr.h"
#include "Guild/GuildMgr.h"
#include "MapNodes/MasterPlayer.h"
#include "ScriptObjects.h"

#ifndef MANGOSBOT_ZERO
#ifdef CMANGOS
#include "Arena/ArenaTeam.h"
#endif
#ifdef MANGOS
#include "ArenaTeam.h"
#endif
#endif

#include <random>

std::unordered_map<RandomPlayerbotFactory::NameRaceAndGender, std::vector<std::string>> RandomPlayerbotFactory::freeNames;
std::unordered_map<RandomPlayerbotFactory::NameRaceAndGender, std::vector<std::string>> RandomPlayerbotFactory::allNames;
std::mutex RandomPlayerbotFactory::nameMutex;
bool RandomPlayerbotFactory::namesInitialized = false;

RandomPlayerbotFactory::RandomPlayerbotFactory(uint32 accountId) : accountId(accountId)
{
}

bool RandomPlayerbotFactory::isAvailableRace(uint8 cls, uint8 race)
{
    if (!race || race >= MAX_RACES || !cls || cls >= MAX_CLASSES)
        return false;

    ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(cls);
    ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(race);
    if (!classEntry || !raceEntry || raceEntry->HasFlag(CHRRACES_FLAGS_NOT_PLAYABLE))
        return false;

    // playercreateinfo is the authoritative Turtle race/class matrix.
    return sObjectMgr.GetPlayerInfo(race, cls) != nullptr;
}

bool RandomPlayerbotFactory::isAvailableRole(uint8 cls, BotRoles role)
{
    if (role == BotRoles::BOT_ROLE_NONE)
        return true;

    switch (cls)
    {
        case CLASS_WARRIOR:
#ifdef MANGOSBOT_TWO
        case CLASS_DEATH_KNIGHT:
#endif
            return role == BotRoles::BOT_ROLE_TANK || role == BotRoles::BOT_ROLE_DPS;
        case CLASS_PALADIN:
        case CLASS_DRUID:
            return true;
        case CLASS_HUNTER:
        case CLASS_ROGUE:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return role == BotRoles::BOT_ROLE_DPS;
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
            return role == BotRoles::BOT_ROLE_HEALER || role == BotRoles::BOT_ROLE_DPS;
        default:
            return false;
    }
}

uint8 RandomPlayerbotFactory::GetRandomClass(uint8 useRace, BotRoles role)
{
    uint32 classProb[MAX_CLASSES] = { 0 };
    uint32 totalProb = 0;

    for (uint32 race = 1; race < MAX_RACES; ++race)
    {
        if (useRace && useRace != race)
            continue;

        for (uint32 cls = 1; cls < MAX_CLASSES; ++cls)
        {
            if (!isAvailableRole(cls, role) || !isAvailableRace(cls, race))
                continue;

            uint32 weight = sPlayerbotAIConfig.classRaceProbability[cls][race];
            classProb[cls] += weight;
            totalProb += weight;
        }
    }

    if (!totalProb)
        return 0;

    uint32 randomProb = urand(0, totalProb - 1);
    for (uint32 cls = 1; cls < MAX_CLASSES; ++cls)
    {
        if (randomProb < classProb[cls])
            return cls;
        randomProb -= classProb[cls];
    }

    return 0;
}

bool RandomPlayerbotFactory::isRaceForTeam(uint8 race, Team team)
{
    return team == Team::TEAM_NONE || Player::TeamForRace(race) == team;
}

uint8 RandomPlayerbotFactory::GetRandomRace(uint8 cls, Team team)
{
    uint32 totalProb = 0;
    for (uint32 race = 1; race < MAX_RACES; ++race)
    {
        if (!isRaceForTeam(race, team) || !isAvailableRace(cls, race))
            continue;
        totalProb += sPlayerbotAIConfig.classRaceProbability[cls][race];
    }

    if (!totalProb)
        return 0;

    uint32 randomProb = urand(0, totalProb - 1);
    for (uint32 race = 1; race < MAX_RACES; ++race)
    {
        if (!isRaceForTeam(race, team) || !isAvailableRace(cls, race))
            continue;

        uint32 weight = sPlayerbotAIConfig.classRaceProbability[cls][race];
        if (randomProb < weight)
            return race;
        randomProb -= weight;
    }

    return 0;
}

bool RandomPlayerbotFactory::CreateRandomBot(uint8 cls, uint8 inputRace)
{
    sLog.outDebug("Creating new random bot for class %u", cls);

    uint8 gender = urand(0, 1) ? GENDER_MALE : GENDER_FEMALE;
    uint8 race = inputRace ? inputRace : GetRandomRace(cls);
    if (!race || !isAvailableRace(cls, race))
    {
        sLog.outError("No valid playercreateinfo combination for bot race %u class %u", race, cls);
        return false;
    }

    NameRaceAndGender raceAndGender = CombineRaceAndGender(gender, race);
    std::string name = CreateRandomBotName(raceAndGender);
    if (name.empty())
        return false;

    if (!normalizePlayerName(name) || ObjectMgr::CheckPlayerName(name, true) != CHAR_NAME_SUCCESS || sObjectMgr.IsReservedName(name))
    {
        sLog.outError("Generated random bot name '%s' is not valid for normal character creation.", name.c_str());
        return false;
    }

    PlayerbotAppearance appearance;
    sPlayerbotAppearanceStore.GetRandomAppearance(race, gender, appearance);

    std::unique_ptr<WorldSession> session = std::make_unique<WorldSession>(accountId, nullptr, SEC_PLAYER,
#ifdef MANGOSBOT_TWO
        2, 0, LOCALE_enUS, "disconnected/bot", 0, 0, false);
#endif
#ifdef MANGOSBOT_ONE
        2, 0, LOCALE_enUS, "disconnected/bot", 0, 0, false);
#endif
#ifdef MANGOSBOT_ZERO
        0, LOCALE_enUS, "disconnected/bot", 0);
#endif

    std::unique_ptr<Player> player = std::make_unique<Player>(session.get());
    if (!player->Create(sObjectMgr.GeneratePlayerLowGuid(), name, race, cls, gender,
            appearance.skin, appearance.face, appearance.hairStyle, appearance.hairColor, appearance.facialHair))
    {
        sLog.outError("Unable to create random bot for account %u - name: \"%s\"; race: %u; class: %u",
            accountId, name.c_str(), race, cls);
        return false;
    }

    MasterPlayer masterPlayer(session.get());
    masterPlayer.Create(player.get());

    player->SetCinematic(2);
    player->SetAtLoginFlag(AT_LOGIN_FIRST);

    if (sPlayerbotAIConfig.disableRandomLevels && sPlayerbotAIConfig.randombotStartingLevel > 1)
        player->GiveLevel(sPlayerbotAIConfig.randombotStartingLevel);

    if (!player->SaveToDB(false, true, false))
    {
        sLog.outError("Unable to save random bot for account %u - name: \"%s\"; race: %u; class: %u",
            accountId, name.c_str(), race, cls);
        return false;
    }

    masterPlayer.SaveToDB();
    sObjectMgr.InsertPlayerInCache(player.get());
    sObjectMgr.UpdatePlayerCachedPosition(player.get());
    sWorld.UpdateRealmCharCount(accountId);

    ScriptRegistry<PlayerScript>::ForEachEnabledHook(PLAYERHOOK_ON_CREATE, [&](PlayerScript* script)
    {
        script->OnCreate(player.get());
    });
    sObjectMgr.IncreaseActivePlayersCount(Player::TeamForRace(race));

    sLog.outDebug("Random bot created for account %u - name: \"%s\"; race: %u; class: %u",
        accountId, name.c_str(), race, cls);
    return true;
}

std::string RandomPlayerbotFactory::CreateRandomBotName(NameRaceAndGender raceAndGender)
{
    std::lock_guard<std::mutex> lock(nameMutex);
    EnsureNamesInitialized();

    auto it = freeNames.find(raceAndGender);
    if (it == freeNames.end() || it->second.empty())
    {
        sLog.outError("No more names left for random bots for race/gender %u", static_cast<uint8>(raceAndGender));
        return "";
    }

    // Get random index
    uint32 idx = urand(0, it->second.size() - 1);
    std::string name = it->second[idx];
    
    // Swap-remove for O(1)
    std::swap(it->second[idx], it->second.back());
    it->second.pop_back();

    return name;
}

inline std::string GetNamePostFix(int32 nr)
{
    std::string ret;

    std::string str("abcdefghijklmnopqrstuvwxyz");

    while (nr >= 0)
    {
        int32 let = nr % 26;
        ret = str[let] + ret;
        nr /= 26;
        nr--;
    }

    return ret;
}

void RandomPlayerbotFactory::EnsureNamesInitialized()
{
    if (namesInitialized)
        return;

    sLog.outString("Initializing random bot names...");

    auto result = CharacterDatabase.PQuery("SELECT n.gender, n.name, e.guid FROM ai_playerbot_names n LEFT OUTER JOIN characters e ON e.name = n.name");
    if (!result)
    {
        sLog.outError("No names found in ai_playerbot_names table");
        namesInitialized = true; // Avoid re-trying
        return;
    }

    std::unordered_map<std::string, bool> used;

    do
    {
        Field* fields = result->Fetch();
        NameRaceAndGender rg = static_cast<NameRaceAndGender>(fields[0].GetUInt8());
        std::string bname = fields[1].GetString();
        uint32 guidlo = fields[2].GetUInt32();
        if (!guidlo)
            freeNames[rg].push_back(bname);
        allNames[rg].push_back(bname);
        used[bname] = false;
    } while (result->NextRow());

    // Generate extra names for missing race/gender combos
    if (allNames.count(NameRaceAndGender::DwarfMale) == 0)
    {
        sLog.outError("The name database has not been updated. Run ai_playerbot_names.sql to update.");

        auto oldResult = CharacterDatabase.PQuery("SELECT e.name FROM characters e");
        if (oldResult)
        {
            do
            {
                Field* fields = oldResult->Fetch();
                std::string bname = fields[0].GetString();
                used[bname] = false;
            } while (oldResult->NextRow());
        }

        for (uint8 type = 2; type <= static_cast<uint8>(NameRaceAndGender::BloodelfFemale); ++type)
        {
            for (auto name : allNames[static_cast<NameRaceAndGender>(type % 2)])
            {
                name[0] -= 'A' - 'a';
                name = GetNamePostFix(type - 2) + name;
                name[0] += 'A' - 'a';
                allNames[static_cast<NameRaceAndGender>(type)].push_back(name);
                if (used.count(name) == 0)
                {
                    freeNames[static_cast<NameRaceAndGender>(type)].push_back(name);
                }
            }
        }
    }

    // Note: The fallback generation (suffixes) happens on-demand in CreateRandomBots()
    // For single bot creation, we'll generate on-demand here too if needed

    sLog.outString(">> Initialized names for %zu race/gender combinations", freeNames.size());

    namesInitialized = true;
}

void RandomPlayerbotFactory::CreateRandomBots()
{
    EnsureNamesInitialized();

    // check if scheduled for delete
    bool delAccs = false;
    bool delFriends = false;
    auto values = CharacterDatabase.Query(
        "select value from ai_playerbot_random_bots where event = 'bot_delete'");

    if (values)
    {
        delAccs = true;

        Field* fields = values->Fetch();
        uint32 deleteType = fields[0].GetUInt32();

        if (deleteType > 1)
            delFriends = true;

    }

    if (sPlayerbotAIConfig.deleteRandomBotAccounts || delAccs)
    {
        std::list<uint32> botAccounts;
        std::list<uint32> botFriends;

        uint32 maxAccountNum = 0;

        auto accountNrQr = LoginDatabase.PQuery("SELECT max(replace(lower(username), lower('%s'), '') + 1 - 1) maxAccountNr FROM account WHERE replace(lower(username), lower('%s'), '') != 0", sPlayerbotAIConfig.randomBotAccountPrefix.c_str(), sPlayerbotAIConfig.randomBotAccountPrefix.c_str());
        
        if (!accountNrQr)
        {
            sLog.outError("Failed to find last %s account nr.", sPlayerbotAIConfig.randomBotAccountPrefix.c_str());
        }
        else
        {
            Field* fields = accountNrQr->Fetch();
            uint32 accountNumber = sPlayerbotAIConfig.randomBotAccountCount + 1;
            maxAccountNum = fields[0].GetUInt32();
        }

        maxAccountNum = std::max(maxAccountNum, sPlayerbotAIConfig.randomBotAccountCount);

        for (uint32 accountNumber = 0; accountNumber < maxAccountNum; ++accountNumber)
        {
            std::ostringstream out; out << sPlayerbotAIConfig.randomBotAccountPrefix << accountNumber;
            std::string accountName = out.str();

            auto result = LoginDatabase.PQuery("SELECT id FROM account where username = '%s'", accountName.c_str());
            if (!result)
                continue;

            Field* fields = result->Fetch();
            uint32 accountId = fields[0].GetUInt32();

            botAccounts.push_back(accountId);
        }

        if (!delFriends)
            sLog.outString("Deleting random bot characters without friends/guild...");
        else
            sLog.outString("Deleting all random bot characters...");


        // load list of friends
        if (!delFriends)
        {
            auto result = CharacterDatabase.PQuery("SELECT friend FROM character_social WHERE flags='%u'", SOCIAL_FLAG_FRIEND);
            if (result)
            {
                do
                {
                    Field* fields = result->Fetch();
                    uint32 guidlo = fields[0].GetUInt32();
                    botFriends.push_back(guidlo);

                } while (result->NextRow());
            }
        }

        auto results = LoginDatabase.PQuery("SELECT id FROM account where username like '%s%%'", sPlayerbotAIConfig.randomBotAccountPrefix.c_str());
        if (results)
        {
            BarGoLink bar(results->GetRowCount());

            do
            {
                Field* fields = results->Fetch();
                uint32 accId = fields[0].GetUInt32();

                if (!delFriends)
                {
                    // existing characters list
                    auto result = CharacterDatabase.PQuery("SELECT guid FROM characters WHERE account='%u'", accId);
                    if (result)
                    {
                        do
                        {
                            Field* fields = result->Fetch();
                            uint32 guidlo = fields[0].GetUInt32();
                            ObjectGuid guid = ObjectGuid(HIGHGUID_PLAYER, guidlo);

                            // if bot is someone's friend - don't delete it
                            if ((find(botFriends.begin(), botFriends.end(), guidlo) != botFriends.end()) && !delFriends)
                                continue;

                            // if bot is in someone's guild - don't delete it
                            uint32 guildId = Player::GetGuildIdFromDB(guid);
                            if (guildId && !delFriends)
                            {
                                Guild* guild = sGuildMgr.GetGuildById(guildId);
                                uint32 accountId = sObjectMgr.GetPlayerAccountIdByGUID(guild->GetLeaderGuid());

                                if (find(botAccounts.begin(), botAccounts.end(), accountId) == botAccounts.end())
                                    continue;
                            }

                            sRandomPlayerbotMgr.OnPlayerLoginError(guidlo);
                            Player::DeleteFromDB(guid, accId, false, true);       // no need to update realm characters
                            //dels.push_back(std::async([guid, accId] {Player::DeleteFromDB(guid, accId, false, true); }));

                        } while (result->NextRow());
                    }
                    bar.step();
                }
                else
                {
                    bar.step();
                    sAccountMgr.DeleteAccount(accId);
                }

            } while (results->NextRow());
        }

        CharacterDatabase.Execute("DELETE FROM ai_playerbot_random_bots WHERE bot NOT IN (SELECT guid FROM characters)");
        sLog.outString("Random bot characters deleted");
    }

    //Delete temporary bots.

    auto temporarybots = CharacterDatabase.Query("SELECT characters.guid, characters.account FROM ai_playerbot_random_bots JOIN characters ON (characters.guid = ai_playerbot_random_bots.bot AND characters.name = ai_playerbot_random_bots.data) WHERE ai_playerbot_random_bots.event = 'temporary'");

    if (temporarybots)
    {
        sLog.outString("Deleting temporary bots");

        do
        {
            Field* fields = temporarybots->Fetch();
            uint32 guid = fields[0].GetUInt32();
            uint32 accountId = fields[1].GetUInt32();

            CharacterDatabase.PExecute("DELETE FROM ai_playerbot_random_bots WHERE bot = %d", guid);
            Player::DeleteFromDB(ObjectGuid(HIGHGUID_PLAYER, guid), accountId, true, true);

            if (sAccountMgr.GetCharactersCount(accountId) == 0)
            {
                sAccountMgr.DeleteAccount(accountId);
            }
        } while (temporarybots->NextRow());
    }

    CharacterDatabase.PExecute("DELETE FROM ai_playerbot_random_bots WHERE ai_playerbot_random_bots.event = 'temporary'");

    //Loop over randombot accounts that have no characters and delete them as well, to clean up after temporary bots.
    auto temporaryAccounts = LoginDatabase.PQuery("SELECT id FROM account WHERE username like '%s%%' and id >= %u", sPlayerbotAIConfig.randomBotAccountPrefix.c_str(), sPlayerbotAIConfig.randomBotAccountCount);

    if (temporaryAccounts)
    {
        sLog.outString("Deleting temporary empty bot accounts");
        do
        {
            Field* fields = temporaryAccounts->Fetch();
            uint32 accountId = fields[0].GetUInt32();
            sAccountMgr.GetCharactersCount(accountId);

            if (sAccountMgr.GetCharactersCount(accountId) == 0)
            {
                sAccountMgr.DeleteAccount(accountId);
            }
        } while (temporaryAccounts->NextRow());
    }

    if (!sPlayerbotAIConfig.randomBotAutoCreate)
    {
        for (uint32 accountNumber = 0; accountNumber < sPlayerbotAIConfig.randomBotAccountCount; ++accountNumber)
        {
            std::ostringstream out; out << sPlayerbotAIConfig.randomBotAccountPrefix << accountNumber;
            std::string accountName = out.str();

            auto results = LoginDatabase.PQuery("SELECT id FROM account where username = '%s'", accountName.c_str());
            if (!results)
                continue;

            Field* fields = results->Fetch();
            uint32 accountId = fields[0].GetUInt32();

            sPlayerbotAIConfig.randomBotAccounts.push_back(accountId);
        }

        return;
    }

    int totalAccCount = sPlayerbotAIConfig.randomBotAccountCount;
    sLog.outString("Creating random bot accounts...");

    std::vector<std::future<void>> account_creations;

    BarGoLink bar(totalAccCount);
    for (uint32 accountNumber = 0; accountNumber < sPlayerbotAIConfig.randomBotAccountCount; ++accountNumber)
    {
        std::ostringstream out; out << sPlayerbotAIConfig.randomBotAccountPrefix << accountNumber;
        std::string accountName = out.str();
        auto results = LoginDatabase.PQuery("SELECT id FROM account where username = '%s'", accountName.c_str());
        if (results)
        {
            continue;
        }

        std::string password = "";
        if (sPlayerbotAIConfig.randomBotRandomPassword)
        {
            for (int i = 0; i < 10; i++)
            {
                password += (char)urand('!', 'z');
            }
        }
        else
            password = accountName;

#ifndef MANGOSBOT_ZERO
        uint8 max_expansion = MAX_EXPANSION;
        account_creations.push_back(std::async([accountName, password, max_expansion] {sAccountMgr.CreateAccount(accountName, password, max_expansion); }));
#else
        account_creations.push_back(std::async([accountName, password] {sAccountMgr.CreateAccount(accountName, password); }));
#endif

        sLog.outDebug("Account %s created for random bots", accountName.c_str());
        bar.step();
    }

    BarGoLink bar3(account_creations.size());
    for (uint32 i = 0; i < account_creations.size(); i++)
    {
        bar3.step();
        account_creations[i].wait();
    }

    //LoginDatabase.PExecute("UPDATE account SET expansion = '%u' where username like '%s%%'", 2, sPlayerbotAIConfig.randomBotAccountPrefix.c_str());

    int totalRandomBotChars = 0;
    uint32 totalCharCount = sPlayerbotAIConfig.randomBotAccountCount
#ifdef MANGOSBOT_TWO
        * 10;
#else
        * 9;
#endif

    sLog.outString("Using shared freeNames from cache...");

    std::unordered_map<std::string, bool> used;
    for (auto& kv : freeNames)
        for (auto& name : kv.second)
            used[name] = false;

    auto result = CharacterDatabase.PQuery("SELECT n.gender, n.name, e.guid FROM ai_playerbot_names n LEFT OUTER JOIN characters e ON e.name = n.name");
    if (!result)
    {
        sLog.outError("No more names left for random bots");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        NameRaceAndGender raceAndGender = static_cast<NameRaceAndGender>(fields[0].GetUInt8());
        std::string bname = fields[1].GetString();
        uint32 guidlo = fields[2].GetUInt32();
        if (!guidlo)
            freeNames[raceAndGender].push_back(bname);
        allNames[raceAndGender].push_back(bname);
        used[bname] = false;
    } while (result->NextRow());

    // Fallback: Generate extra names if needed based on character count
    auto countResult = CharacterDatabase.Query("SELECT COUNT(guid) FROM characters");
    if (countResult)
        totalCharCount = countResult->Fetch()[0].GetUInt32();

    for (uint8 raceAndGenderIndex = 0; raceAndGenderIndex <= static_cast<uint8>(NameRaceAndGender::BloodelfFemale); ++raceAndGenderIndex)
    {
        const auto raceAndGender = static_cast<NameRaceAndGender>(raceAndGenderIndex);

        if (totalCharCount / 2 < freeNames[raceAndGender].size())
            continue;

        int32 postItt = 0;
        std::vector<std::string> newNames;
        uint32 namesNeeded = totalCharCount / 2 - freeNames[raceAndGender].size();

        while (namesNeeded)
        {
            std::string post = GetNamePostFix(postItt);
            for (auto name : allNames[raceAndGender])
            {
                if (name.size() + post.size() > 12)
                    continue;
                std::string newName = name + post;
                if (used.find(newName) != used.end())
                    continue;
                used[newName] = false;
                newNames.push_back(newName);
                namesNeeded--;
                if (!namesNeeded)
                    break;
            }
            postItt++;
        }

        freeNames[raceAndGender].insert(freeNames[raceAndGender].end(), newNames.begin(), newNames.end());
    }

    sLog.outString(">> Updated names for %zu race/gender combinations.", freeNames.size());

    sLog.outString("Creating random bot characters...");
    uint32 botsCreated = 0;
    BarGoLink bar1(totalCharCount);


    // Shallow copy of the fixed config so we can modify it
    std::map<std::pair<uint8, uint8>, uint32> remaining = sPlayerbotAIConfig.fixedClassRaceCounts;

    for (uint32 accountNumber = 0; accountNumber < sPlayerbotAIConfig.randomBotAccountCount; ++accountNumber)
    {
        std::ostringstream out; out << sPlayerbotAIConfig.randomBotAccountPrefix << accountNumber;
        std::string accountName = out.str();

        auto results = LoginDatabase.PQuery("SELECT id FROM account where username = '%s'", accountName.c_str());
        if (!results)
            continue;

        Field* fields = results->Fetch();
        uint32 accountId = fields[0].GetUInt32();

        sPlayerbotAIConfig.randomBotAccounts.push_back(accountId);

        int count = sAccountMgr.GetCharactersCount(accountId);
#ifdef MANGOSBOT_TWO
        if (count >= 10)
#else
        if (count >= 9)
#endif
        {
            totalRandomBotChars += count;
            continue;
        }

        RandomPlayerbotFactory factory(accountId);
        if (sPlayerbotAIConfig.useFixedClassRaceCounts)
        {
#ifdef MANGOSBOT_TWO
            uint32 maxAllowed = 10 - count;
#else
            uint32 maxAllowed = 9 - count;
#endif
            uint32 created = 0;

            while (!remaining.empty() && created < maxAllowed)
            {
                std::vector<std::pair<uint8, uint8>> shuffledKeys;
                for (const auto& entry : remaining)
                    shuffledKeys.push_back(entry.first);

                std::random_device rnd;
                std::mt19937 rng(rnd());
                std::shuffle(shuffledKeys.begin(), shuffledKeys.end(), rng);

                bool progress = false;
                for (const auto& key : shuffledKeys)
                {
                    if (created >= maxAllowed)
                        break;

                    uint8 cls = key.first;
                    uint8 race = key.second;
                    if (!factory.isAvailableRace(cls, race))
                        continue;

                    if (!factory.CreateRandomBot(cls, race))
                        continue;

                    progress = true;
                    ++created;
                    ++botsCreated;
                    bar1.step();
                    if (--remaining[key] == 0)
                        remaining.erase(key);
                }

                // Invalid/unsaveable fixed combinations must be reported, not spin forever.
                if (!progress)
                    break;
            }
        }
        else
        {
#ifdef MANGOSBOT_TWO
            uint32 maxAllowed = 10 - count;
#else
            uint32 maxAllowed = 9 - count;
#endif
            uint32 created = 0;
            uint32 attempts = 0;
            uint32 maxAttempts = std::max<uint32>(maxAllowed * MAX_CLASSES, maxAllowed);
            while (created < maxAllowed && attempts++ < maxAttempts)
            {
                uint8 randomClass = factory.GetRandomClass();
                if (!randomClass)
                    break;

                if (!factory.CreateRandomBot(randomClass))
                    continue;

                ++created;
                ++botsCreated;
                bar1.step();
            }
        }

        totalRandomBotChars += sAccountMgr.GetCharactersCount(accountId);
    }
    if (sPlayerbotAIConfig.useFixedClassRaceCounts && !remaining.empty())
    {
	sLog.outError("Unable to create all requested fixed class/race bots due to account character limits.");
	sLog.outError("The following class/race combination(s) were left uncreated:");

	uint32 totalCount = 0;
	for(const auto& entry : remaining)
	{
	    uint8 cls = entry.first.first;
	    uint8 race = entry.first.second;
	    uint32 count = entry.second;
	    totalCount += count;

	    sLog.outError(" - Class %u, Race %u: %u bots remaining", cls, race, count);
	}
#ifdef MANGOSBOT_TWO
	uint32 missingAccounts = (totalCount + 9) / 10;
#else
        uint32 missingAccounts = (totalCount + 8) / 9;
#endif
	sLog.outError("You need at least %u additional account(s) to fill the remaining fixed class/race combinations.", missingAccounts);
    }


    if (!botsCreated)
    {
	    sLog.outString("No new random bots needed. Accounts: %zu, bots: %d.", sPlayerbotAIConfig.randomBotAccounts.size(), totalRandomBotChars);

        return;
    }

    sLog.outString("%zu random bot accounts with %d characters available", sPlayerbotAIConfig.randomBotAccounts.size(), totalRandomBotChars);
}


void RandomPlayerbotFactory::CreateRandomGuilds()
{
    std::vector<uint32> randomBots;
    std::map<uint32, std::vector<uint32>> charAccGuids;

    auto charAccounts = CharacterDatabase.PQuery(
        "select `account`, `guid` from `characters`");

    if (charAccounts)
    {
        do
        {
            Field* fields = charAccounts->Fetch();
            uint32 accId = fields[0].GetUInt32();
            uint32 guid = fields[1].GetUInt32();
            charAccGuids[accId].push_back(guid);
        } while (charAccounts->NextRow());
    }

    if (charAccGuids.empty())
        return;

    for (auto charAcc : sPlayerbotAIConfig.randomBotAccounts)
    {
        if (!charAccGuids[charAcc].empty())
            for (auto charGuid : charAccGuids[charAcc])
                randomBots.push_back(charGuid);
    }

    if (randomBots.empty())
        return;

    if (sPlayerbotAIConfig.deleteRandomBotGuilds && !sRandomPlayerbotMgr.guildsDeleted)
    {
        sLog.outString("Deleting random bot guilds...");
        uint32 counter = 0;
        for (std::vector<uint32>::iterator i = randomBots.begin(); i != randomBots.end(); ++i)
        {
            ObjectGuid leader(HIGHGUID_PLAYER, *i);
            Guild* guild = sGuildMgr.GetGuildByLeader(leader);
            if (guild)
            {
                guild->Disband();
                counter++;
            }
        }
        sLog.outString("%d Random bot guilds deleted", counter);

        sRandomPlayerbotMgr.guildsDeleted = true;
    }

    if (!sPlayerbotAIConfig.randomBotGuildCount)
        return;

    uint32 guildNumber = 0;
    std::vector<ObjectGuid> availableLeaders;
    for (std::vector<uint32>::iterator i = randomBots.begin(); i != randomBots.end(); ++i)
    {
        ObjectGuid leader(HIGHGUID_PLAYER, *i);
        Guild* guild = sGuildMgr.GetGuildByLeader(leader);
        if (guild)
        {
            if (find(sPlayerbotAIConfig.randomBotGuilds.begin(), sPlayerbotAIConfig.randomBotGuilds.end(), guild->GetId()) == sPlayerbotAIConfig.randomBotGuilds.end())
            {
                ++guildNumber;
                sPlayerbotAIConfig.randomBotGuilds.push_back(guild->GetId());
            }
        }
        else
        {
            Player* player = sObjectMgr.GetPlayer(leader);
            if (player && !player->GetGuildId() && player->GetLevel() >= 10)
                availableLeaders.push_back(leader);
        }
    }

    if (availableLeaders.empty())
    {
        sLog.outError("No leaders for random guilds available");
        return;
    }

    uint32 attempts = 0;
    uint32 maxNewGuilds = sPlayerbotAIConfig.randomBotGuildCount - sPlayerbotAIConfig.randomBotGuilds.size();
    bool newGuilds = false;
    for (; guildNumber < maxNewGuilds; ++guildNumber)
    {
        attempts++;
        if (attempts > std::min(uint32(5), sPlayerbotAIConfig.randomBotGuildCount))
            break;
        if (sPlayerbotAIConfig.randomBotGuilds.size() >= sPlayerbotAIConfig.randomBotGuildCount)
            break;

        std::string guildName = CreateRandomGuildName();
        if (guildName.empty())
            continue;

        int index = urand(0, availableLeaders.size() - 1);
        ObjectGuid leader = availableLeaders[index];
        Player* player = sObjectMgr.GetPlayer(leader);
        if (!player || player->GetGuildId())
            continue;

        Guild* guild = new Guild();
        if (!guild->Create(player, guildName))
        {
            sLog.outError("Error creating random guild %s", guildName.c_str());
			continue;
        }

        sGuildMgr.AddGuild(guild);

        // create random emblem
        uint32 st, cl, br, bc, bg;
        bg = urand(0, 51);
        bc = urand(0, 17);
        cl = urand(0, 17);
        br = urand(0, 7);
        st = urand(0, 180);
        guild->SetEmblem(st, cl, br, bc, bg);
        guild->SetGINFO(std::to_string(urand(10, 30)));

        sPlayerbotAIConfig.randomBotGuilds.push_back(guild->GetId());
        sLog.outBasic("Random Guild <%s>, GM: %s", guildName.c_str(), player->GetName());
        newGuilds = true;
    }

    if (newGuilds)
        sLog.outString("Total Random Guilds: %d", (uint32)sPlayerbotAIConfig.randomBotGuilds.size());
}

std::string RandomPlayerbotFactory::CreateRandomGuildName()
{
    auto result = CharacterDatabase.Query("SELECT MAX(name_id) FROM ai_playerbot_guild_names");
    if (!result)
    {
        sLog.outError("No more names left for random guilds");
        return "";
    }

    Field *fields = result->Fetch();
    uint32 maxId = fields[0].GetUInt32();

    uint32 id = urand(0, maxId);
    result = CharacterDatabase.PQuery("SELECT n.name FROM ai_playerbot_guild_names n "
            "LEFT OUTER JOIN guild e ON e.name = n.name "
            "WHERE e.guildid IS NULL AND n.name_id >= '%u' LIMIT 1", id);
    if (!result)
    {
        sLog.outError("No more names left for random guilds");
        return "";
    }

    fields = result->Fetch();
    std::string gname = fields[0].GetString();
    return gname;
}

#ifndef MANGOSBOT_ZERO
void RandomPlayerbotFactory::CreateRandomArenaTeams()
{
    std::vector<uint32> randomBots;

    auto results = CharacterDatabase.PQuery(
        "select `bot` from ai_playerbot_random_bots where event = 'add'");

    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            uint32 bot = fields[0].GetUInt32();
            randomBots.push_back(bot);
        } while (results->NextRow());
    }

    if (sPlayerbotAIConfig.deleteRandomBotArenaTeams && !sRandomPlayerbotMgr.arenaTeamsDeleted)
    {
        sLog.outString("Deleting random bot arena teams...");
        for (std::vector<uint32>::iterator i = randomBots.begin(); i != randomBots.end(); ++i)
        {
            ObjectGuid captain(HIGHGUID_PLAYER, *i);
            ArenaTeam* arenateam = sObjectMgr.GetArenaTeamByCaptain(captain);
            if (arenateam)
                //sObjectMgr.RemoveArenaTeam(arenateam->GetId());
                arenateam->Disband(NULL);
        }
        sLog.outString("Random bot arena teams deleted");

        sRandomPlayerbotMgr.arenaTeamsDeleted = true;
    }

    uint32 arenaTeamNumber = 0;
    std::map<uint32, uint32> teamsNumber;
    std::map<uint32, uint32> maxTeamsNumber;
    maxTeamsNumber[ARENA_TYPE_2v2] = (uint32)(sPlayerbotAIConfig.randomBotArenaTeamCount * 0.4f);
    maxTeamsNumber[ARENA_TYPE_3v3] = (uint32)(sPlayerbotAIConfig.randomBotArenaTeamCount * 0.3f);
    maxTeamsNumber[ARENA_TYPE_5v5] = (uint32)(sPlayerbotAIConfig.randomBotArenaTeamCount * 0.3f);
    std::vector<ObjectGuid> availableCaptains;
    for (std::vector<uint32>::iterator i = randomBots.begin(); i != randomBots.end(); ++i)
    {
        ObjectGuid captain(HIGHGUID_PLAYER, *i);
        ArenaTeam* arenateam = sObjectMgr.GetArenaTeamByCaptain(captain);
        if (arenateam)
        {
            teamsNumber[arenateam->GetType()]++;
            sPlayerbotAIConfig.randomBotArenaTeams.push_back(arenateam->GetId());
        }

        Player* player = sObjectMgr.GetPlayer(captain);
        if (player)
        {
            if (player->GetLevel() < PLAYER_MAX_LEVEL)
                continue;

            uint8 slot = ArenaTeam::GetSlotByType(ArenaType(ARENA_TYPE_2v2));
            if (player->GetArenaTeamId(slot))
                continue;

            slot = ArenaTeam::GetSlotByType(ArenaType(ARENA_TYPE_3v3));
            if (player->GetArenaTeamId(slot))
                continue;

            slot = ArenaTeam::GetSlotByType(ArenaType(ARENA_TYPE_5v5));
            if (player->GetArenaTeamId(slot))
                continue;

            availableCaptains.push_back(captain);
        }
    }

    uint32 attempts = 0;
    for (; arenaTeamNumber < sPlayerbotAIConfig.randomBotArenaTeamCount; ++arenaTeamNumber)
    {
        if (attempts > sPlayerbotAIConfig.randomBotArenaTeamCount)
            break;

        ArenaType randomType = ARENA_TYPE_2v2;
        switch (urand(0, 2))
        {
        case 0:
            randomType = ARENA_TYPE_2v2;
            break;
        case 1:
            randomType = ARENA_TYPE_3v3;
            break;
        case 2:
            randomType = ARENA_TYPE_5v5;
            break;
        }

        std::string arenaTeamName = CreateRandomArenaTeamName();
        if (arenaTeamName.empty())
            continue;

        if (availableCaptains.empty())
        {
            sLog.outError("No captains for random arena teams available");
            continue;
        }

        int index = urand(0, availableCaptains.size() - 1);
        ObjectGuid captain = availableCaptains[index];
        Player* player = sObjectMgr.GetPlayer(captain);
        if (!player)
        {
            sLog.outError("Cannot find player for captain %d", player->GetGUIDLow());
            continue;
        }

        if (player->GetLevel() < PLAYER_MAX_LEVEL)
        {
            sLog.outError("Bot %d must be level %d to create an arena team", player->GetGUIDLow(), PLAYER_MAX_LEVEL);
            continue;
        }

        auto results = CharacterDatabase.PQuery("SELECT `type` FROM ai_playerbot_arena_team_names WHERE name = '%s'", arenaTeamName.c_str());
        if (!results)
        {
            sLog.outError("No valid types for arena teams");
            return;
        }

        Field *fields = results->Fetch();
        uint8 slot = fields[0].GetUInt32();

        std::string arenaTypeName;
        ArenaType type = ARENA_TYPE_2v2;
        switch (slot)
        {
        case 2:
            type = ARENA_TYPE_2v2;
            arenaTypeName = "2v2";
            break;
        case 3:
            type = ARENA_TYPE_3v3;
            arenaTypeName = "3v3";
            break;
        case 5:
            type = ARENA_TYPE_5v5;
            arenaTypeName = "5v5";
            break;
        }

        attempts++;

        if (type != randomType)
            continue;

        if (teamsNumber[type] >= maxTeamsNumber[type])
            continue;

        if (player->GetArenaTeamId(ArenaTeam::GetSlotByType(type)))
            continue;

        ArenaTeam* arenateam = new ArenaTeam();
        if (!arenateam->Create(player->GetObjectGuid(), type, arenaTeamName))
        {
            sLog.outError("Error creating arena team %s", arenaTeamName.c_str());
            continue;
        }
        arenateam->SetCaptain(player->GetObjectGuid());
        sLog.outBasic("Bot #%d %s:%d <%s>: captain of random Arena %s team - %s", player->GetGUIDLow(), player->GetTeam() == ALLIANCE ? "A" : "H", player->GetLevel(), player->GetName(), arenaTypeName.c_str(), arenateam->GetName().c_str());
        // set random emblem
        uint32 backgroundColor = urand(0xFF000000, 0xFFFFFFFF), emblemStyle = urand(0, 101), emblemColor = urand(0xFF000000, 0xFFFFFFFF), borderStyle = urand(0, 5), borderColor = urand(0xFF000000, 0xFFFFFFFF);
        arenateam->SetEmblem(backgroundColor, emblemStyle, emblemColor, borderStyle, borderColor);
        // set random kills (wip)
        //arenateam->SetStats(STAT_TYPE_GAMES_WEEK, urand(0, 30));
        //arenateam->SetStats(STAT_TYPE_WINS_WEEK, urand(0, arenateam->GetStats().games_week));
        //arenateam->SetStats(STAT_TYPE_GAMES_SEASON, urand(arenateam->GetStats().games_week, arenateam->GetStats().games_week * 5));
        //arenateam->SetStats(STAT_TYPE_WINS_SEASON, urand(arenateam->GetStats().wins_week, arenateam->GetStats().games_season));
        sObjectMgr.AddArenaTeam(arenateam);
        sPlayerbotAIConfig.randomBotArenaTeams.push_back(arenateam->GetId());

        for (uint32 i = 0; i < 10; i++)
        {
            if (arenateam->GetMembersSize() >= type)
                break;

            int index = urand(0, availableCaptains.size() - 1);
            ObjectGuid possibleMember = availableCaptains[index];
            if (possibleMember == captain)
                continue;

            Player* member = sObjectMgr.GetPlayer(possibleMember);
            if (!member)
                continue;
            if (member->GetArenaTeamId(arenateam->GetSlot()))
                continue;
            if (member->GetTeam() != player->GetTeam())
                continue;

            arenateam->AddMember(member->GetObjectGuid());
            sLog.outBasic("Bot #%d %s:%d <%s>: added to random Arena %s team - %s", member->GetGUIDLow(), member->GetTeam() == ALLIANCE ? "A" : "H", member->GetLevel(), member->GetName(), arenaTypeName.c_str(), arenateam->GetName().c_str());

            /*if (player->GetArenaTeamIdFromDB(possibleMember, type))
                continue;*/

        }

        if (arenateam->GetMembersSize() < type)
        {
            sLog.outBasic("Random Arena team %s %s: failed to get enough members, deleting...", arenaTypeName.c_str(), arenateam->GetName().c_str());
            arenateam->Disband(nullptr);
            return;
        }

        // set random rating
        arenateam->SetRatingForAll(urand(1500, 2700));
        arenateam->SaveToDB();

        sLog.outBasic("Random Arena team %s %s: created", arenaTypeName.c_str(), arenateam->GetName().c_str());
    }

    sLog.outString("%d random bot arena teams available", arenaTeamNumber);
}

std::string RandomPlayerbotFactory::CreateRandomArenaTeamName()
{
    auto result = CharacterDatabase.Query("SELECT MAX(name_id) FROM ai_playerbot_arena_team_names");
    if (!result)
    {
        sLog.outError("No more names left for random arena teams");
        return "";
    }

    Field *fields = result->Fetch();
    uint32 maxId = fields[0].GetUInt32();

    uint32 id = urand(0, maxId);
    result = CharacterDatabase.PQuery("SELECT n.name FROM ai_playerbot_arena_team_names n "
        "LEFT OUTER JOIN arena_team e ON e.name = n.name "
        "WHERE e.arenateamid IS NULL AND n.name_id >= '%u' LIMIT 1", id);
    if (!result)
    {
        sLog.outError("No more names left for random arena teams");
        return "";
    }

    fields = result->Fetch();
    std::string aname = fields[0].GetString();
    return aname;
}
#endif

