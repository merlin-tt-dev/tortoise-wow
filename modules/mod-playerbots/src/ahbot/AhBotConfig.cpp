
#include "AhBotConfig.h"
#include "SystemConfig.h"
#include "Log.h"
#include "playerbot/Helpers.h"

INSTANTIATE_SINGLETON_1(AhBotConfig);

AhBotConfig::AhBotConfig()
{
}

template <class T>
void LoadSet(std::string value, T &res)
{
    std::vector<std::string> ids = split(value, ',');
    for (std::vector<std::string>::iterator i = ids.begin(); i != ids.end(); i++)
    {
        uint32 id = atoi((*i).c_str());
        if (!id)
            continue;

        res.insert(id);
    }
}

bool AhBotConfig::Initialize()
{
    enabled = sConfig.GetBoolDefault("AhBot.Enabled", true);

    if (!enabled)
        sLog.outString("AhBot is Disabled in ahbot.conf");

    guid = (uint64)sConfig.GetIntDefault("AhBot.GUID", 0);
    updateInterval = sConfig.GetIntDefault("AhBot.UpdateIntervalInSeconds", 900);
    historyDays = sConfig.GetIntDefault("AhBot.History.Days", 30);
    itemBuyMinInterval = sConfig.GetIntDefault("AhBot.ItemBuyMinInterval", 600);
    itemBuyMaxInterval = sConfig.GetIntDefault("AhBot.ItemBuyMaxInterval", 7200);
    itemSellMinInterval = sConfig.GetIntDefault("AhBot.ItemSellMinInterval", 600);
    itemSellMaxInterval = sConfig.GetIntDefault("AhBot.ItemSellMaxInterval", 7200);
    maxSellInterval = sConfig.GetIntDefault("AhBot.MaxSellInterval", 3600 * 8);
    alwaysAvailableMoney = sConfig.GetIntDefault("AhBot.AlwaysAvailableMoney", 200000);
    priceMultiplier = sConfig.GetFloatDefault("AhBot.PriceMultiplier", 1.0f);
    defaultMinPrice = sConfig.GetIntDefault("AhBot.DefaultMinPrice", 20);
    maxItemLevel = sConfig.GetIntDefault("AhBot.MaxItemLevel", 199);
    maxRequiredLevel = sConfig.GetIntDefault("AhBot.MaxRequiredLevel", 80);
    stackReducePrice = sConfig.GetIntDefault("AhBot.StackReducePrice", 1000000);
    priceQualityMultiplier = sConfig.GetFloatDefault("AhBot.PriceQualityMultiplier", 1.0f);
    underPriceProbability = sConfig.GetFloatDefault("AhBot.UnderPriceProbability", 0.05f);
    LoadSet<std::set<uint32> >(sConfig.GetStringDefault("AhBot.IgnoreItemIds", "49283,52200,8494,6345,6891,2460,37164,34835"), ignoreItemIds);
    LoadSet<std::set<uint32> >(sConfig.GetStringDefault("AhBot.IgnoreVendorItemIds", "755,858,4592,4593,1710,3827,2455,3385"), ignoreVendorItemIds);
    sendmail = sConfig.GetBoolDefault("AhBot.SendMail", true);


    return enabled;
}
