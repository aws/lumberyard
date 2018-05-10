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

#include "CoreBuilderSystemComponent.h"
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <ScriptCanvas/Core/GraphAsset.h>
#include <ScriptCanvas/Core/GraphAssetHandler.h>

namespace ScriptCanvasBuilder
{
    CoreBuilderSystemComponent::CoreBuilderSystemComponent()
    {
        m_graphAssetHandler = AZStd::make_unique<ScriptCanvas::GraphAssetHandler>();
    }

    CoreBuilderSystemComponent::~CoreBuilderSystemComponent()
    {
    }

    void CoreBuilderSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CoreBuilderSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void CoreBuilderSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ScriptCanvasCoreBuilderService", 0x42195ec0));
    }

    void CoreBuilderSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
        required.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
    }

    void CoreBuilderSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("AssetCatalogService", 0xc68ffc57));
    }

    void CoreBuilderSystemComponent::Init()
    {
    }

    void CoreBuilderSystemComponent::Activate()
    {
        AZ::Data::AssetType assetType(azrtti_typeid<ScriptCanvas::GraphAsset>());
        if (AZ::Data::AssetManager::Instance().GetHandler(assetType))
        {
            return; // Asset Type already handled
        }

        AZ::Data::AssetManager::Instance().RegisterHandler(m_graphAssetHandler.get(), assetType);

        // Use AssetCatalog service to register ScriptCanvas asset type and extension
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, ScriptCanvas::GraphAsset::GetFileExtension());
    }

    void CoreBuilderSystemComponent::Deactivate()
    {
        // Finish all queued work
        AZ::Data::AssetBus::ExecuteQueuedEvents();
        AZ::Data::AssetManager::Instance().UnregisterHandler(m_graphAssetHandler.get());
    }
}