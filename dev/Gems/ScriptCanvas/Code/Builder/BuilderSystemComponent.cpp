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

#include "precompiled.h"

#include "BuilderSystemComponent.h"
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AssetBuilderSDK/AssetBuilderBusses.h>

#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>

namespace ScriptCanvasBuilder
{
    BuilderSystemComponent::BuilderSystemComponent()
    {
        m_scriptCanvasAssetHandler = AZStd::make_unique<ScriptCanvasEditor::ScriptCanvasAssetHandler>();
    }

    BuilderSystemComponent::~BuilderSystemComponent()
    {
    }

    void BuilderSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BuilderSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void BuilderSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
        required.push_back(AZ_CRC("AssetCatalogService", 0xc68ffc57));
    }

    void BuilderSystemComponent::Init()
    {
    }

    void BuilderSystemComponent::Activate()
    {
        AZ::Data::AssetType assetType(azrtti_typeid<ScriptCanvasEditor::ScriptCanvasAsset>());
        if (AZ::Data::AssetManager::Instance().GetHandler(assetType))
        {
            return; // Asset Type already handled
        }

        AZ::Data::AssetManager::Instance().RegisterHandler(m_scriptCanvasAssetHandler.get(), assetType);

        // Use AssetCatalog service to register ScriptCanvas asset type and extension
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, ScriptCanvasEditor::ScriptCanvasAsset::GetFileExtension());
        
        AssetBuilderSDK::ToolsAssetSystemBus::Broadcast(&AssetBuilderSDK::ToolsAssetSystemRequests::RegisterSourceAssetType, assetType, ScriptCanvasEditor::ScriptCanvasAsset::GetFileFilter());
    }

    void BuilderSystemComponent::Deactivate()
    {
        // Finish all queued work
        AZ::Data::AssetBus::ExecuteQueuedEvents();

        AssetBuilderSDK::ToolsAssetSystemBus::Broadcast(&AssetBuilderSDK::ToolsAssetSystemRequests::UnregisterSourceAssetType, azrtti_typeid<ScriptCanvasEditor::ScriptCanvasAsset>());
        AZ::Data::AssetManager::Instance().UnregisterHandler(m_scriptCanvasAssetHandler.get());
    }
}