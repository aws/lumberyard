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

#include <Asset/RuntimeAssetRegistry.h>
#include <AzCore/Asset/AssetManager.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>

namespace ScriptCanvas
{
    RuntimeAssetRegistry::RuntimeAssetRegistry()
    {
        m_runtimeAssetHandler = AZStd::make_unique<ScriptCanvas::RuntimeAssetHandler>();
    }

    RuntimeAssetRegistry::~RuntimeAssetRegistry()
    {
    }

    void RuntimeAssetRegistry::Register()
    {
        AZ::Data::AssetType assetType(azrtti_typeid<ScriptCanvas::RuntimeAsset>());
        if (AZ::Data::AssetManager::Instance().GetHandler(assetType))
        {
            return; // Asset Type already handled
        }

        AZ::Data::AssetManager::Instance().RegisterHandler(m_runtimeAssetHandler.get(), assetType);

        // Use AssetCatalog service to register ScriptCanvas asset type and extension
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, ScriptCanvas::RuntimeAsset::GetFileExtension());
    }

    void RuntimeAssetRegistry::Unregister()
    {
        AZ::Data::AssetManager::Instance().UnregisterHandler(m_runtimeAssetHandler.get());
    }

    AZ::Data::AssetHandler* RuntimeAssetRegistry::GetAssetHandler()
    {
        return m_runtimeAssetHandler.get();
    }
}
