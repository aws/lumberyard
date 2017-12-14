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

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Debug/Trace.h>

#include <AssetBuilderApplication.h>
#include <AssetBuilderInfo.h>
#include <AssetBuilderComponent.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/StreamerComponent.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/Input/System/InputSystemComponent.h>
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogComponent.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>

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
        AZ_Printf(AssetBuilderSDK::InfoWindow, "gameRoot not specified on the command line, assuming current directory.\n");
        AZ_Printf(AssetBuilderSDK::InfoWindow, "gameRoot is best specified as the full path to the game's asset folder.");
    }
    
    AzFramework::StringFunc::Path::Join(gameRoot.c_str(), "config/editor.xml", configFilePath);

    ReflectModulesFromAppDescriptor(configFilePath.c_str());

    // the asset builder app never writes source files, only assets, so there is no need to do any kind of asset upgrading
    AZ::Data::AssetManager::Instance().SetAssetInfoUpgradingEnabled(false);
}
