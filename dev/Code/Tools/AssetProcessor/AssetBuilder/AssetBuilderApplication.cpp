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

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/StreamerComponent.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzFramework/API/BootstrapReaderBus.h>
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/Input/System/InputSystemComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogComponent.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderApplication.h>
#include <AssetBuilderComponent.h>
#include <AssetBuilderInfo.h>

AZ::ComponentTypeList AssetBuilderApplication::GetRequiredSystemComponents() const
{
    AZ::ComponentTypeList components = AzFramework::Application::GetRequiredSystemComponents();

    for (auto iter = components.begin(); iter != components.end();)
    {
        if (*iter == azrtti_typeid<AZ::StreamerComponent>()
            || *iter == azrtti_typeid<AZ::UserSettingsComponent>()
            || *iter == azrtti_typeid<AzFramework::InputSystemComponent>()
            || *iter == azrtti_typeid<AzFramework::AssetCatalogComponent>()
            )
        {
            iter = components.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    components.insert(components.end(), {
        azrtti_typeid<AssetBuilderComponent>(),
        azrtti_typeid<AssetProcessor::ToolsAssetCatalogComponent>(),
        azrtti_typeid<AzToolsFramework::AssetSystem::AssetSystemComponent>(),
    });

    return components;
}

AssetBuilderApplication::AssetBuilderApplication(int* argc, char*** argv)
: AzToolsFramework::ToolsApplication(argc, argv)
{
}

void AssetBuilderApplication::RegisterCoreComponents()
{
    AzToolsFramework::ToolsApplication::RegisterCoreComponents();
    
    RegisterComponentDescriptor(AssetBuilderComponent::CreateDescriptor());
    RegisterComponentDescriptor(AssetProcessor::ToolsAssetCatalogComponent::CreateDescriptor());
}

void AssetBuilderApplication::StartCommon(AZ::Entity* systemEntity)
{
    AzToolsFramework::ToolsApplication::StartCommon(systemEntity);

    AZStd::string configFilePath;
    AZStd::string gameRoot;
    
    if (m_commandLine.GetNumSwitchValues("gameRoot") > 0)
    {
        gameRoot = m_commandLine.GetSwitchValue("gameRoot", 0);
    }
    
    if (gameRoot.empty())
    {
        if (IsInDebugMode())
        {
            if (!ReadGameFolderFromBootstrap(gameRoot))
            {
                AZ_Error("AssetBuilder", false, "Unable to determine the game root automatically. "
                    "Make sure a default project has been set or provide a default option on the command line. (See -help for more info.)");
                return;
            }
        }
        else
        {
            AZ_Printf(AssetBuilderSDK::InfoWindow, "gameRoot not specified on the command line, assuming current directory.\n");
            AZ_Printf(AssetBuilderSDK::InfoWindow, "gameRoot is best specified as the full path to the game's asset folder.");
        }
    }
    
    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    if (fileIO)
    {
        fileIO->SetAlias("@devassets@", gameRoot.c_str());
    }

    AzFramework::StringFunc::Path::Join(gameRoot.c_str(), "config/editor.xml", configFilePath);
    ReflectModulesFromAppDescriptor(configFilePath.c_str());

    // once we load all the modules from the app descriptor, create an entity with the builders inside gems
    CreateAndAddEntityFromComponentTags(AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }), "AssetBuilders Entity");

    // the asset builder app never writes source files, only assets, so there is no need to do any kind of asset upgrading
    AZ::Data::AssetManager::Instance().SetAssetInfoUpgradingEnabled(false);
}

bool AssetBuilderApplication::IsInDebugMode() const
{
    return AssetBuilderComponent::IsInDebugMode(m_commandLine);
}

bool AssetBuilderApplication::ReadGameFolderFromBootstrap(AZStd::string& result) const
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

    return AzFramework::StringFunc::Path::Join(assetRoot.c_str(), gameFolder.c_str(), result);
}