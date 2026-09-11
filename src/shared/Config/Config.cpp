/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2011 MaNGOSZero <https://github.com/mangos/zero>
 * Copyright (C) 2011-2016 Nostalrius <https://nostalrius.org>
 * Copyright (C) 2016-2017 Elysium Project <https://github.com/elysium-project>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Config.h"

#include "Log.h"
#include "Policies/SingletonImp.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

INSTANTIATE_SINGLETON_2(Config, Config::Lock);
INSTANTIATE_CLASS_MUTEX(Config, std::shared_mutex);

#ifndef TW_MODULE_CONFIG_LIST
#define TW_MODULE_CONFIG_LIST ""
#endif

static char const* GetConfigImportErrorText(int importResult)
{
    switch (importResult)
    {
        case -1:
            return "file could not be opened";
        case -3:
            return "config has invalid INI syntax";
        case -4:
            return "config is missing a section header";
        default:
            return "unknown import error";
    }
}

enum class ConfigIncludeType
{
    Directory,
    File
};

struct ConfigIncludeDirective
{
    ConfigIncludeType type;
    std::string key;
    std::string value;
    uint32 line;
};

struct ConfigKeyDefinition
{
    std::string section;
    std::string key;
    std::string file;
    uint32 line;
};

struct ConfigFileMetadata
{
    bool active = true;
    std::vector<ConfigIncludeDirective> includes;
    std::vector<ConfigKeyDefinition> keys;
};

static std::string TrimConfigText(std::string value)
{
    std::string::size_type const first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";

    std::string::size_type const last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

static std::string StripInlineConfigComment(std::string value)
{
    char quote = 0;
    for (std::string::size_type index = 0; index < value.size(); ++index)
    {
        char const character = value[index];

        if (quote)
        {
            if (character == quote)
                quote = 0;
            continue;
        }

        if (character == '"' || character == '\'')
        {
            quote = character;
            continue;
        }

        if (character == '#' || character == ';')
            return value.substr(0, index);
    }

    return value;
}

static std::string NormalizeConfigValue(std::string value)
{
    value = TrimConfigText(StripInlineConfigComment(value));
    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\'')))
    {
        value = value.substr(1, value.size() - 2);
    }

    return value;
}

static bool IsDisabledConfigValue(std::string value)
{
    value = NormalizeConfigValue(value);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character)
    {
        return static_cast<char>(std::tolower(character));
    });

    return value == "0" || value == "false" || value == "no" || value == "off";
}

static bool IsConfigControlKey(std::string const& key)
{
    return key == "ConfigFileActive" ||
        key == "IncludeDir" || key.rfind("IncludeDir.", 0) == 0 ||
        key == "IncludeFile" || key.rfind("IncludeFile.", 0) == 0;
}

static bool ReadConfigFileMetadata(
    std::string const& fileName,
    bool collectIncludes,
    ConfigFileMetadata& metadata)
{
    std::ifstream input(fileName);
    if (!input.is_open())
    {
        sLog.outError("Could not inspect configuration file %s.", fileName.c_str());
        return false;
    }

    std::string section;
    std::string line;
    uint32 lineNumber = 0;

    while (std::getline(input, line))
    {
        ++lineNumber;
        std::string const trimmed = TrimConfigText(line);

        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';')
            continue;

        if (trimmed[0] == '[')
        {
            std::string::size_type const sectionEnd = trimmed.find(']');
            if (sectionEnd != std::string::npos)
                section = TrimConfigText(trimmed.substr(1, sectionEnd - 1));
            continue;
        }

        std::string::size_type const separator = trimmed.find('=');
        if (separator == std::string::npos)
            continue;

        std::string const key = TrimConfigText(trimmed.substr(0, separator));
        std::string const value = NormalizeConfigValue(trimmed.substr(separator + 1));
        if (key.empty())
            continue;

        if (key == "ConfigFileActive")
        {
            metadata.active = !IsDisabledConfigValue(value);
            continue;
        }

        if (collectIncludes)
        {
            if (key == "IncludeDir" || key.rfind("IncludeDir.", 0) == 0)
            {
                if (!value.empty())
                    metadata.includes.push_back({ConfigIncludeType::Directory, key, value, lineNumber});
                continue;
            }

            if (key == "IncludeFile" || key.rfind("IncludeFile.", 0) == 0)
            {
                if (!value.empty())
                    metadata.includes.push_back({ConfigIncludeType::File, key, value, lineNumber});
                continue;
            }
        }

        if (!IsConfigControlKey(key))
            metadata.keys.push_back({section, key, fileName, lineNumber});
    }

    return true;
}

static void WarnAndRegisterDuplicateKeys(
    ConfigFileMetadata const& metadata,
    std::unordered_map<std::string, ConfigKeyDefinition>& knownKeys)
{
    for (ConfigKeyDefinition const& definition : metadata.keys)
    {
        std::string const identity = definition.section + "\x1f" + definition.key;
        auto const previous = knownKeys.find(identity);

        if (previous != knownKeys.end())
        {
            sLog.outString(
                "WARNING: config key [%s] %s is parsed more than once; %s:%u overrides %s:%u.",
                definition.section.c_str(), definition.key.c_str(),
                definition.file.c_str(), definition.line,
                previous->second.file.c_str(), previous->second.line);
        }

        knownKeys[identity] = definition;
    }
}

std::vector<std::string> Config::GetModuleConfigFiles() const
{
    std::vector<std::string> files;
    std::string const configuredFiles = TW_MODULE_CONFIG_LIST;
    std::string::size_type start = 0;

    while (start < configuredFiles.size())
    {
        std::string::size_type end = configuredFiles.find(',', start);
        if (end == std::string::npos)
            end = configuredFiles.size();

        std::string file = configuredFiles.substr(start, end - start);
        if (!file.empty())
            files.push_back(file);

        start = end + 1;
    }

    return files;
}

std::string Config::GetConfigDirectory() const
{
    std::string::size_type separator = mFilename.find_last_of("/\\");
    if (separator == std::string::npos)
        return "";

    return mFilename.substr(0, separator + 1);
}

bool Config::LoadIncludes()
{
    ConfigFileMetadata rootMetadata;
    if (!ReadConfigFileMetadata(mFilename, true, rootMetadata))
        return false;

    std::unordered_map<std::string, ConfigKeyDefinition> knownKeys;
    WarnAndRegisterDuplicateKeys(rootMetadata, knownKeys);

    std::filesystem::path const configDirectory(GetConfigDirectory());
    std::vector<std::filesystem::path> includeFiles;

    for (ConfigIncludeDirective const& include : rootMetadata.includes)
    {
        std::filesystem::path includePath(include.value);
        if (includePath.is_relative())
            includePath = configDirectory / includePath;

        if (include.type == ConfigIncludeType::File)
        {
            includeFiles.push_back(includePath);
            continue;
        }

        std::error_code error;
        if (!std::filesystem::exists(includePath, error))
        {
            if (error)
            {
                sLog.outError("Could not inspect config %s=%s from %s:%u: %s.",
                    include.key.c_str(), includePath.string().c_str(), mFilename.c_str(), include.line,
                    error.message().c_str());
                return false;
            }

            sLog.outDetail("Config %s=%s from %s:%u does not exist; continuing without this include directory.",
                include.key.c_str(), includePath.string().c_str(), mFilename.c_str(), include.line);
            continue;
        }

        if (!std::filesystem::is_directory(includePath, error) || error)
        {
            sLog.outError("Config %s=%s from %s:%u is not a readable directory.",
                include.key.c_str(), includePath.string().c_str(), mFilename.c_str(), include.line);
            return false;
        }

        std::vector<std::filesystem::path> directoryFiles;
        std::filesystem::directory_iterator end;
        for (std::filesystem::directory_iterator itr(includePath, error); !error && itr != end; itr.increment(error))
        {
            std::error_code fileError;
            if (itr->is_regular_file(fileError) && !fileError && itr->path().extension() == ".conf")
                directoryFiles.push_back(itr->path());
        }

        if (error)
        {
            sLog.outError("Could not enumerate config %s=%s from %s:%u: %s.",
                include.key.c_str(), includePath.string().c_str(), mFilename.c_str(), include.line,
                error.message().c_str());
            return false;
        }

        std::sort(directoryFiles.begin(), directoryFiles.end(), [](std::filesystem::path const& left, std::filesystem::path const& right)
        {
            return left.filename().string() < right.filename().string();
        });

        includeFiles.insert(includeFiles.end(), directoryFiles.begin(), directoryFiles.end());
    }

    std::unordered_set<std::string> loadedIncludeFiles;

    // Includes are intentionally single-level. IncludeDir/IncludeFile directives
    // inside included files are ignored, which keeps ordering deterministic and
    // makes include cycles impossible.
    for (std::filesystem::path includeFile : includeFiles)
    {
        includeFile = includeFile.lexically_normal();
        std::string const includeFileName = includeFile.string();

        if (!loadedIncludeFiles.insert(includeFileName).second)
        {
            sLog.outString("WARNING: configuration file %s was requested more than once; duplicate include skipped.",
                includeFileName.c_str());
            continue;
        }

        ConfigFileMetadata metadata;
        if (!ReadConfigFileMetadata(includeFileName, false, metadata))
            return false;

        if (!metadata.active)
        {
            sLog.outDetail("Skipping included configuration file %s because ConfigFileActive is disabled.",
                includeFileName.c_str());
            continue;
        }

        WarnAndRegisterDuplicateKeys(metadata, knownKeys);

        ACE_Ini_ImpExp includeImporter(*mConf);
        int const importResult = includeImporter.import_config(includeFileName.c_str());
        if (importResult != 0)
        {
            sLog.outError("Could not load included configuration file %s: %s.",
                includeFileName.c_str(), GetConfigImportErrorText(importResult));
            return false;
        }

        sLog.outDetail("Loaded included configuration file %s.", includeFileName.c_str());
    }

    return true;
}

// Defined here as it must not be exposed to end-users.
bool Config::GetValueHelper(const char* name, ACE_TString &result)
{
    GuardType guard(m_configLock);

    if (!mConf)
        return false;

    ACE_TString section_name;
    ACE_Configuration_Section_Key section_key;
    const ACE_Configuration_Section_Key &root_key = mConf->root_section();

    int i = 0;
    while (mConf->enumerate_sections(root_key, i, section_name) == 0)
    {
        mConf->open_section(root_key, section_name.c_str(), 0, section_key);
        if (mConf->get_string_value(section_key, name, result) == 0)
            return true;
        ++i;
    }

    return false;
}

std::string Config::GetStringDefaultInSection(const char* name, const char* section, const char* def)
{
    GuardType guard(m_configLock);

    if (!mConf)
        return def;

    const ACE_Configuration_Section_Key &root_key = mConf->root_section();
    ACE_Configuration_Section_Key primary_section_key;
    int openErrCode = mConf->open_section(root_key, section, 0, primary_section_key);
    if (openErrCode != 0)
    {
        return def;
    }

    ACE_TString section_value;
    openErrCode = mConf->get_string_value(primary_section_key, name, section_value);
    if (openErrCode != 0) return def;

    return section_value.c_str();
}

void Config::GetRootSections(std::vector<std::string>& OutSectionList)
{
    const ACE_Configuration_Section_Key &RootKey = mConf->root_section();

    ACE_TString section_name;
    int i = 0;
    while (mConf->enumerate_sections(RootKey, i, section_name) == 0)
    {
        OutSectionList.emplace_back(section_name.c_str());
        ++i;
    }
}

void Config::GetSections(const char* SectionName, std::vector<std::string>& OutSectionList)
{
    const ACE_Configuration_Section_Key &RootKey = mConf->root_section();
    ACE_Configuration_Section_Key BaseSectionKey;
    if (mConf->open_section(RootKey, SectionName, 0, BaseSectionKey) == 0)
    {
        ACE_TString section_name;
        int i = 0;
        while (mConf->enumerate_sections(BaseSectionKey, i, section_name) == 0)
        {
            OutSectionList.emplace_back(section_name.c_str());
            ++i;
        }
    }
}

void Config::GetKeys(const char* SectionName, std::vector<std::string>& OutKeysList)
{
    const ACE_Configuration_Section_Key &RootKey = mConf->root_section();
    ACE_Configuration_Section_Key BaseSectionKey;
    if (mConf->open_section(RootKey, SectionName, 0, BaseSectionKey) == 0)
    {
        ACE_TString key_name;
        ACE_Configuration::VALUETYPE valueType;
        int i = 0;
        while (mConf->enumerate_values(BaseSectionKey, i, key_name, valueType) == 0)
        {
            OutKeysList.emplace_back(key_name.c_str());
            ++i;
        }
    }
}

Config::Config()
    : mConf(nullptr)
{
}

Config::~Config()
{
    delete mConf;
}

bool Config::SetSource(const char *file)
{
    mFilename = file;

    return Reload();
}

bool Config::Reload()
{
    delete mConf;
    mConf = new ACE_Configuration_Heap;

    if (mConf->open() != -1)
    {
        ACE_Ini_ImpExp config_importer(*mConf);
        int const importResult = config_importer.import_config(mFilename.c_str());
        if (importResult == 0)
        {
            if (LoadIncludes())
                return true;
        }
        else
        {
            sLog.outError("Could not load configuration file %s: %s.",
                mFilename.c_str(), GetConfigImportErrorText(importResult));
        }
    }

    delete mConf;
    mConf = nullptr;
    return false;
}

bool Config::LoadModulesConfigs()
{
    if (!mConf)
        return false;

    std::vector<std::string> const moduleConfigFiles = GetModuleConfigFiles();
    if (moduleConfigFiles.empty())
        return true;

    std::string const moduleConfigDirectory = GetConfigDirectory() + "modules/";

    for (std::string const& moduleConfigFile : moduleConfigFiles)
    {
        std::string const moduleConfigPath = moduleConfigDirectory + moduleConfigFile;
        ACE_Ini_ImpExp moduleConfigImporter(*mConf);

        int const importResult = moduleConfigImporter.import_config(moduleConfigPath.c_str());
        if (importResult != 0)
        {
            sLog.outError("Could not load module configuration file %s: %s.",
                moduleConfigPath.c_str(), GetConfigImportErrorText(importResult));
            return false;
        }
    }

    return true;
}

std::string Config::GetStringDefault(const char* name, const char* def)
{
    ACE_TString val;
    return GetValueHelper(name, val) ? val.c_str() : def;
}

bool Config::GetBoolDefault(const char* name, bool def)
{
    ACE_TString val;
    if (!GetValueHelper(name, val))
        return def;

    const char* str = val.c_str();
    return strcmp(str, "true") == 0 || strcmp(str, "TRUE") == 0 ||
           strcmp(str, "yes") == 0 || strcmp(str, "YES") == 0 ||
           strcmp(str, "1") == 0;
}


int32 Config::GetIntDefault(const char* name, int32 def)
{
    ACE_TString val;
    return GetValueHelper(name, val) ? atoi(val.c_str()) : def;
}


float Config::GetFloatDefault(const char* name, float def)
{
    ACE_TString val;
    return GetValueHelper(name, val) ? (float)atof(val.c_str()) : def;
}

float Config::GetFloatDefault(const char* name, const char* section, const float def)
{
    std::string rawValue = GetStringDefaultInSection(name, section, "invalid");
    if (rawValue == "invalid") return def;
    return atof(rawValue.c_str());
}