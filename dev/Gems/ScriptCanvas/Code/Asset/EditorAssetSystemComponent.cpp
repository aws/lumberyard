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

#include <LyViewPaneNames.h>

#include <Asset/EditorAssetSystemComponent.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/wildcard.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogBus.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>

namespace ScriptCanvasEditor
{
    EditorAssetSystemComponent::~EditorAssetSystemComponent()
    {
    }

    void EditorAssetSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorAssetSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void EditorAssetSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ScriptCanvasEditorAssetService", 0x4a1c043d));
    }

    void EditorAssetSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
        required.push_back(AZ_CRC("AssetCatalogService", 0xc68ffc57));
        required.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
    }

    void EditorAssetSystemComponent::Init()
    {
    }

    void EditorAssetSystemComponent::Activate()
    {
        m_editorAssetRegistry.Register();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
    }

    void EditorAssetSystemComponent::Deactivate()
    {
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        m_editorAssetRegistry.Unregister();
    }

    EditorAssetRegistry& EditorAssetSystemComponent::GetAssetRegistry()
    {
        return m_editorAssetRegistry;
    }

    static bool HandlesSource(const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* entry)
    {
        AZStd::string_view targetExtension = entry->GetExtension();
        AZStd::string_view scriptCanvasFileFilter = ScriptCanvasAssetHandler::GetFileFilter();
        if (AZStd::wildcard_match(scriptCanvasFileFilter.data(), targetExtension.data()))
        {
            return true;
        }

        return false;
    }

    void EditorAssetSystemComponent::AddSourceFileOpeners(const char* fullSourceFileName, const AZ::Uuid& sourceUuid, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
    {
        using namespace AzToolsFramework;
        using namespace AzToolsFramework::AssetBrowser;
        if (const SourceAssetBrowserEntry* source = SourceAssetBrowserEntry::GetSourceByAssetId(sourceUuid))  // get the full details of the source file based on its UUID.
        {
            if (!HandlesSource(source))
            {
                return;
            }
        }
        // You can push back any number of "Openers" - choose a unique identifier, and icon, and then a lambda which will be activated if the user chooses to open it with your opener:
        openers.push_back({ "ScriptCanvas_Editor_Asset_Edit", "Script Canvas Editor...", QIcon(ScriptCanvasAssetHandler().GetBrowserIcon()),
            [](const char*, const AZ::Uuid& scSourceUuid)
        {
            AzToolsFramework::OpenViewPane(LyViewPane::ScriptCanvas);
            AZ::Data::AssetId sourceAssetId(scSourceUuid, 0);

            auto& assetManager = AZ::Data::AssetManager::Instance();
            AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset = assetManager.GetAsset(sourceAssetId, azrtti_typeid<ScriptCanvasAsset>());
            AZ::Outcome<int, AZStd::string> openOutcome = AZ::Failure(AZStd::string());
            GeneralRequestBus::BroadcastResult(openOutcome, &GeneralRequests::OpenScriptCanvasAsset, scriptCanvasAsset, -1, AZ::EntityId());
            if (!openOutcome)
            {
                AZ_Warning("Script Canvas", openOutcome, "%s", openOutcome.GetError().data());
            }
        } });

    }
}
