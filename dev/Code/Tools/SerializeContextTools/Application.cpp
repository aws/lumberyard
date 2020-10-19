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

#include <Application.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/API/BootstrapReaderBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    namespace SerializeContextTools
    {
        Application::Application(int* argc, char*** argv)
            : AzToolsFramework::ToolsApplication(argc, argv)
        {
        }

        void Application::StartCommon(AZ::Entity* systemEntity)
        {
            AzToolsFramework::ToolsApplication::StartCommon(systemEntity);

            if (!ReadGameFolderFromBootstrap(m_gameRoot))
            {
                AZ_Error("Serialize Context Tools", false, "Unable to determine the game root automatically. "
                    "Make sure a default project has been set or provide a default option on the command line. (See -help for more info.)");
                return;
            }

            AZStd::string configFilePath = "config/editor.xml";
            if (m_commandLine.HasSwitch("config"))
            {
                configFilePath = m_commandLine.GetSwitchValue("config", 0);
            }

            if (AzFramework::StringFunc::Path::Join(m_gameRoot, configFilePath.c_str(), configFilePath))
            {
                ReflectModulesFromAppDescriptor(configFilePath.c_str());
                azstrcpy(m_configFilePath, AZ_MAX_PATH_LEN, configFilePath.c_str());
            }
            else
            {
                AZ_Error("Serialize Context Tools", false, "Unable to resolve path to config file.");
            }
        }

        const char* Application::GetGameRoot() const
        {
            return m_gameRoot;
        }

        const char* Application::GetConfigFilePath() const
        {
            return m_configFilePath;
        }

        bool Application::ReadGameFolderFromBootstrap(char* result) const
        {
            AZStd::string assetRoot;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(assetRoot, &AzFramework::ApplicationRequests::GetAssetRoot);
            if (assetRoot.empty())
            {
                return false;
            }
            AZStd::string gameFolder;
            AzFramework::BootstrapReaderRequestBus::Broadcast(&AzFramework::BootstrapReaderRequestBus::Events::SearchConfigurationForKey, "sys_game_folder", false, gameFolder);
            if (gameFolder.empty())
            {
                return false;
            }

            AZStd::string fullPath;
            if (AzFramework::StringFunc::Path::Join(assetRoot.c_str(), gameFolder.c_str(), fullPath))
            {
                azstrcpy(result, AZ_MAX_PATH_LEN, fullPath.c_str());
                return true;
            }
            else
            {
                AZ_Error("Serialize Context Tools", false, "Unable to resolve path to game folder.");
                return false;
            }
        }
    } // namespace SerializeContextTools
} // namespace AZ