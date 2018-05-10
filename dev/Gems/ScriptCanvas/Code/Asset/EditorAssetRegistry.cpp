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

#include "EditorAssetRegistry.h"
#include <AzCore/Asset/AssetManager.h>

#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogBus.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>

namespace ScriptCanvasEditor
{
    EditorAssetRegistry::EditorAssetRegistry()
    {
        m_editorAssetHandler = AZStd::make_unique<ScriptCanvasEditor::ScriptCanvasAssetHandler>();
    }

    EditorAssetRegistry::~EditorAssetRegistry()
    {
    }

    void EditorAssetRegistry::Register()
    {
        AZ::Data::AssetType assetType(azrtti_typeid<ScriptCanvasEditor::ScriptCanvasAsset>());
        if (AZ::Data::AssetManager::Instance().GetHandler(assetType))
        {
            return; // Asset Type already handled
        }

        AZ::Data::AssetManager::Instance().RegisterHandler(m_editorAssetHandler.get(), assetType);

        // Use AssetCatalog service to register ScriptCanvas asset type and extension
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, ScriptCanvasEditor::ScriptCanvasAsset::GetFileExtension());
    }

    void EditorAssetRegistry::Unregister()
    {
        AZ::Data::AssetManager::Instance().UnregisterHandler(m_editorAssetHandler.get());
    }

    AZ::Data::AssetHandler* EditorAssetRegistry::GetAssetHandler()
    {
        return m_editorAssetHandler.get();
    }
}
