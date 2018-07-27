/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include "ParseConfig.h"
#include "ISystem.h"
#include <AzCore/base.h>
#include <AzCore/IO/SystemFile.h>
#include <CryPath.h>

#define BOOTSTRAP_CFG_FILE                      "bootstrap.cfg"

// any of the following tags can be present in the bootstrap.cfg
// you can also prefix it with a platform.
// so for example, you can specify remote_ip alone to specify it for all platforms
// or you could specify android_remote_ip to change it for android only.
// the instructions are executed in the order that they appear, so you can set the default
// by using the non-platform-specific version, and then later on in the file you
// can override specific platforms.

#define CONFIG_KEY_FOR_REMOTEIP                 "remote_ip"
#define CONFIG_KEY_FOR_REMOTEPORT               "remote_port"
#define CONFIG_KEY_FOR_GAMEDLL                  "sys_dll_game"
#define CONFIG_KEY_FOR_GAMEFOLDER               "sys_game_folder"
#define CONFIG_KEY_FOR_REMOTEFILEIO             "remote_filesystem"
#define CONFIG_KEY_FOR_CONNECTTOREMOTE          "connect_to_remote"
#define CONFIG_KEY_WAIT_FOR_CONNECT             "wait_for_connect"
#define DEFAULT_GAMEDLL                         "EmptyTemplate"
#define DEFAULT_GAMEFOLDER                      "EmptyTemplate"
#define DEFAULT_REMOTEIP                        "127.0.0.1"
#define DEFAULT_REMOTEPORT                      45643
#define CONFIG_KEY_FOR_ASSETS                   "assets"
#define CONFIG_KEY_FOR_BRANCHTOKEN              "assetProcessor_branch_token"

//////////////////////////////////////////////////////////////////////////
class CEngineConfig
    : public CParseConfig
{
public:
    string  m_gameFolder; // folder only ("MyGame")
    string m_assetPlatform; // what platform folder assets are from if more than one is available or using VFS ("pc" / "es3")
    string  m_gameDLL; // dll name only ("something"), without extension
    bool    m_connectToRemote;
    bool    m_remoteFileIO;
    bool    m_waitForConnect;
    string  m_remoteIP;
    int     m_remotePort;

    string m_rootFolder; // where we found either system.cfg or bootstrap.cfg
    string m_branchToken;

    void LoadEngineConfig(
        const char** searchPaths,
        size_t numSearchPaths,
        size_t numLevelsUp)
    {
        // Always do at least one pass on the current folder.
        numLevelsUp++;

        static const char* blankPath[] =
        {
            "."
        };

        if (!searchPaths)
        {
            searchPaths = blankPath;
            numSearchPaths = 1;
        }

        string path;
        for (size_t searchPathIndex = 0; searchPathIndex < numSearchPaths; ++searchPathIndex)
        {
            path = PathUtil::RemoveSlash(searchPaths[searchPathIndex]);

            bool foundConfigFile = false;
            for (size_t numLevelsGoneUp = 0; numLevelsGoneUp < numLevelsUp; ++numLevelsGoneUp)
            {
                string filePath = PathUtil::Make(path.c_str(), BOOTSTRAP_CFG_FILE);
                if (ParseConfig(filePath.c_str()))
                {
                    // nothing to do
                    m_rootFolder = path.c_str();
                    foundConfigFile = true;
                    break;
                }
                path = PathUtil::Make(path, "..");
            }

            if (foundConfigFile)
            {
                break;
            }
        }

        if (m_gameFolder.empty())
        {
            //if its not then its probably an error, but set the default:
            m_gameFolder = DEFAULT_GAMEFOLDER;
        }

        if (m_gameDLL.empty())
        {
            // you're allowed to assume that the game dll name is the same as the game folder.
            m_gameDLL = m_gameFolder;
        }
    }

    CEngineConfig(const char** sourcePaths = nullptr, size_t numSearchPaths = 0, size_t numLevelsUp = 3)
        : m_connectToRemote(false)
        , m_remoteFileIO(false)
        , m_remotePort(DEFAULT_REMOTEPORT)
        , m_waitForConnect(false)
        , m_remoteIP(DEFAULT_REMOTEIP)
    {
        LoadEngineConfig(sourcePaths, numSearchPaths, numLevelsUp);
    }

    void CopyToStartupParams(SSystemInitParams& startupParams) const
    {
        startupParams.remoteFileIO = m_remoteFileIO;
        startupParams.remotePort = m_remotePort;
        startupParams.connectToRemote = m_connectToRemote;
        startupParams.waitForConnection = m_waitForConnect;

        azstrncpy(startupParams.remoteIP, sizeof(startupParams.remoteIP), m_remoteIP.c_str(), m_remoteIP.length() + 1); // +1 for the null terminator
        azstrncpy(startupParams.assetsPlatform, sizeof(startupParams.assetsPlatform), m_assetPlatform.c_str(), m_assetPlatform.length() + 1); // +1 for the null terminator
        azstrncpy(startupParams.rootPath, sizeof(startupParams.rootPath), m_rootFolder.c_str(), m_rootFolder.length() + 1); // +1 for the null terminator
        azstrncpy(startupParams.gameFolderName, sizeof(startupParams.gameFolderName), m_gameFolder.c_str(), m_gameFolder.length() + 1); // +1 for the null terminator
        azstrncpy(startupParams.gameDLLName, sizeof(startupParams.gameDLLName), m_gameDLL.c_str(), m_gameDLL.length() + 1); // +1 for the null terminator
        azstrncpy(startupParams.branchToken, sizeof(startupParams.branchToken), m_branchToken.c_str(), m_branchToken.length() + 1); // +1 for the null terminator

        // compute assets path based on game folder name
        string gameFolderLower(m_gameFolder);
        gameFolderLower.MakeLower();
        azsnprintf(startupParams.assetsPath, sizeof(startupParams.assetsPath), "%s/%s", startupParams.rootPath, gameFolderLower.c_str());

        // compute where the cache should be located
        azsnprintf(startupParams.rootPathCache, sizeof(startupParams.rootPathCache), "%s/Cache/%s/%s", m_rootFolder.c_str(), m_gameFolder.c_str(), m_assetPlatform.c_str());
        azsnprintf(startupParams.assetsPathCache, sizeof(startupParams.assetsPathCache), "%s/%s", startupParams.rootPathCache, gameFolderLower.c_str());
    }

protected:

    // check if the config key is the expected key
    // returns true if either its an exact match or if
    // it matches with _CURRENTPLATFORM attached
    // so for example
    // KeyMatches("android_myKey", "myKey") == true if we're on android
    bool KeyMatches(const char* key, const char* searchFor) const
    {
        if (azstricmp(key, searchFor) == 0)
        {
            return true;
        }
        char searchForPlatformSpecific[128];
        azstrcpy(searchForPlatformSpecific, 128, CURRENT_PLATFORM_NAME "_");
        azstrcat(searchForPlatformSpecific, 128, searchFor);

        if (azstricmp(key, searchForPlatformSpecific) == 0)
        {
            return true;
        }

        return false;
    }

    void ReadConfigEntry(const string& key, const string& value, const char* checkKey, string& store)  const
    {
        if (KeyMatches(key.c_str(), checkKey))
        {
            store = value;
        }
    }

    void ReadConfigEntry(const string& key, const string& value, const char* checkKey, bool& store)  const
    {
        if (KeyMatches(key.c_str(), checkKey))
        {
            if (value.compareNoCase("1") == 0)
            {
                store = true;
            }
            else if (value.compareNoCase("0") == 0)
            {
                store = false;
            }
        }
    }

    void ReadConfigEntry(const string& key, const string& value, const char* checkKey, int& store)  const
    {
        if (KeyMatches(key.c_str(), checkKey))
        {
            azsscanf(value.c_str(), "%d", &store);
        }
    }

    void OnLoadConfigurationEntry(const string& strKey, const string& strValue, const string& /*strGroup*/) override
    {
        ReadConfigEntry(strKey, strValue, CONFIG_KEY_FOR_GAMEFOLDER, m_gameFolder);
        ReadConfigEntry(strKey, strValue, CONFIG_KEY_FOR_GAMEDLL, m_gameDLL);
        ReadConfigEntry(strKey, strValue, CONFIG_KEY_FOR_REMOTEFILEIO, m_remoteFileIO);
        ReadConfigEntry(strKey, strValue, CONFIG_KEY_WAIT_FOR_CONNECT, m_waitForConnect);
        ReadConfigEntry(strKey, strValue, CONFIG_KEY_FOR_CONNECTTOREMOTE, m_connectToRemote);
        ReadConfigEntry(strKey, strValue, CONFIG_KEY_FOR_REMOTEPORT, m_remotePort);
        ReadConfigEntry(strKey, strValue, CONFIG_KEY_FOR_REMOTEIP, m_remoteIP);
        ReadConfigEntry(strKey, strValue, CONFIG_KEY_FOR_ASSETS, m_assetPlatform);
        ReadConfigEntry(strKey, strValue, CONFIG_KEY_FOR_BRANCHTOKEN, m_branchToken);
    }
};
