/*
 * Copyright (C) 2017-2018 namreeb (legal@namreeb.org)
 *
 * This is private software and may not be shared under any circumstances,
 * absent permission of namreeb.
 */

#include "WardenModuleMgr.hpp"
#include "WardenModule.hpp"
#include "../Config.hpp"

#include "Platform/Define.h"
#include "Policies/SingletonImp.h"
#include "Util.h"

#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace
{
std::vector<std::string> GetModuleNames(const std::string& moduleDir)
{
    std::vector<std::string> results;
    std::error_code error;

    for (std::filesystem::directory_iterator itr(moduleDir, error), end; !error && itr != end; itr.increment(error))
    {
        const std::string modulePath = itr->path().generic_string();

        // Look only for .bin files, and assume (for now) that the corresponding .key and .cr files exist.
        if (modulePath.size() >= 4 && modulePath.compare(modulePath.size() - 4, 4, ".bin") == 0)
            results.emplace_back(modulePath);
    }

    return results;
}
}

WardenModuleMgr sWardenModuleMgr;

WardenModuleMgr::WardenModuleMgr()
{
    
}

void WardenModuleMgr::LoadWardenModules()
{
    auto const moduleDir = sAnticheatConfig.GetWardenModuleDirectory();
    auto const modules = GetModuleNames(moduleDir);

    for (auto const& mod : modules)
    {
        auto const key = mod.substr(0, mod.length() - 3) + "key";
        auto const cr = mod.substr(0, mod.length() - 3) + "cr";

        try
        {
            auto newMod = WardenModule(mod, key, cr);

            if (newMod.Windows())
                _winModules.emplace_back(std::move(newMod));
            else
                _macModules.emplace_back(std::move(newMod));
        }
        catch (const std::runtime_error&)
        {
            continue;
        }
    }
}


const WardenModule *WardenModuleMgr::GetWindowsModule() const
{
    if (_winModules.empty())
        return nullptr;
    return &_winModules[urand(0, _winModules.size() - 1)];
}

const WardenModule *WardenModuleMgr::GetMacModule() const
{
    MANGOS_ASSERT(!_macModules.empty());

    return &_macModules[urand(0, _macModules.size() - 1)];
}
