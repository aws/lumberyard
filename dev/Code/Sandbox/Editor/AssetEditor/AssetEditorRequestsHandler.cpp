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
#include "StdAfx.h"

#include "AssetEditorWindow.h"

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/AssetEditor/AssetEditorWidget.h>

#include <Editor/AssetEditor/AssetEditorRequestsHandler.h>

AssetEditorRequestsHandler::AssetEditorRequestsHandler()
{
    AzToolsFramework::AssetEditor::AssetEditorRequestsBus::Handler::BusConnect();
    AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
}

AssetEditorRequestsHandler::~AssetEditorRequestsHandler()
{
    AzToolsFramework::AssetEditor::AssetEditorRequestsBus::Handler::BusDisconnect();
    AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
}

void AssetEditorRequestsHandler::NotifyRegisterViews()
{
    using namespace AzToolsFramework::AssetEditor;
    if (auto assetsWindowsToRestore = AZ::UserSettings::CreateFind<AssetEditorWindowSettings>(AZ::Crc32(AssetEditorWindowSettings::s_name), AZ::UserSettings::CT_GLOBAL))
    {
        //copy the current list and clear it since they will be re-added as we request to open them
        auto windowsToOpen = assetsWindowsToRestore->m_openAssets;
        assetsWindowsToRestore->m_openAssets.clear();
        for (auto&& assetRef : windowsToOpen)
        {
            AssetEditorWindow::RegisterViewClass(assetRef);
        }
    }
}

void AssetEditorRequestsHandler::CreateNewAsset(const AZ::Data::AssetType& assetType)
{
    using namespace AzToolsFramework::AssetEditor;
    AzToolsFramework::OpenViewPane(LyViewPane::AssetEditor);

    AssetEditorWidgetRequestsBus::Broadcast(&AssetEditorWidgetRequests::CreateAsset, assetType);
}

void AssetEditorRequestsHandler::OpenAssetEditor(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
{
    using namespace AzToolsFramework::AssetEditor;

    AssetEditorWindow::RegisterViewClass(asset);

    auto& assetName = asset.GetHint();
    const char* paneName = assetName.c_str();
    auto&& pane = QtViewPaneManager::instance()->OpenPane(paneName, QtViewPane::OpenMode::RestoreLayout);

}
