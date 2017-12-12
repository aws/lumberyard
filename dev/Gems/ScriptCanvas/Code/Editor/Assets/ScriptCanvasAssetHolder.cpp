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

#include <Editor/Assets/ScriptCanvasAssetHolder.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Bus/DocumentContextBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace ScriptCanvasEditor
{
    //=========================================================================
    void ScriptCanvasAssetHolder::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ScriptCanvasAssetHolder>()
                ->Version(0)
                ->Field("m_asset", &ScriptCanvasAssetHolder::m_scriptCanvasAsset)
                ->Field("m_openEditorButton", &ScriptCanvasAssetHolder::m_openEditorButton)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ScriptCanvasAssetHolder>("Script Canvas", "Script Canvas Asset Holder")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement("AssetRef", &ScriptCanvasAssetHolder::m_scriptCanvasAsset, "Script Canvas Asset", "Script Canvas asset associated with this component")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ScriptCanvasAssetHolder::OnScriptChanged)
                    ->Attribute("EditButton", "Editor/Icons/PropertyEditor/open_in.png")
                    ->Attribute("EditDescription", "Open in Script Canvas Editor")
                    ->Attribute("EditCallback", &ScriptCanvasAssetHolder::LaunchScriptCanvasEditor)
                    ;
            }
        }
    }

    ScriptCanvasAssetHolder::ScriptCanvasAssetHolder()
        : ScriptCanvasAssetHolder(AZ::Data::Asset<ScriptCanvasAsset>())
    {
    }

    ScriptCanvasAssetHolder::ScriptCanvasAssetHolder(AZ::Data::Asset<ScriptCanvasAsset> asset)
        : m_scriptCanvasAsset(asset)
    {
    }

    ScriptCanvasAssetHolder::~ScriptCanvasAssetHolder()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    void ScriptCanvasAssetHolder::Init()
    {
        if (!m_scriptCanvasAsset.IsReady())
        {
            // Load the Asset if the Id is valid
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, m_scriptCanvasAsset.GetId());
            if (assetInfo.m_assetId.IsValid())
            {
                DocumentContextRequestBus::BroadcastResult(m_scriptCanvasAsset, &DocumentContextRequests::LoadScriptCanvasAssetById, m_scriptCanvasAsset.GetId(), true);
            }

        }

        if (m_scriptCanvasAsset.IsReady())
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::Handler::BusConnect(m_scriptCanvasAsset.GetId());
        }
    }

    void ScriptCanvasAssetHolder::LaunchScriptCanvasEditor(const AZ::Data::AssetId&, const AZ::Data::AssetType&) const
    {
        OpenEditor();
    }

    void ScriptCanvasAssetHolder::OpenEditor() const
    {
        AzToolsFramework::OpenViewPane(GetViewPaneName());

        if (GetAsset().IsReady())
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::OpenScriptCanvasAsset, m_scriptCanvasAsset, -1);
        }
    }

    AZ::EntityId ScriptCanvasAssetHolder::GetGraphId() const
    {
        AZ::EntityId graphId;
        if (m_scriptCanvasAsset.IsReady())
        {
            ScriptCanvas::SystemRequestBus::BroadcastResult(graphId, &ScriptCanvas::SystemRequests::FindGraphId, m_scriptCanvasAsset.Get()->GetScriptCanvasEntity());
        }
        return graphId;
    }

    AZ::u32 ScriptCanvasAssetHolder::OnScriptChanged()
    {
        SetAsset(m_scriptCanvasAsset);
        if (!m_scriptCanvasAsset.IsReady())
        {
            DocumentContextRequestBus::Broadcast(&DocumentContextRequests::LoadScriptCanvasAssetById, m_scriptCanvasAsset.GetId(), false);
        }
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    void ScriptCanvasAssetHolder::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        SetAsset(asset);
        ActivateAssetData();
        EditorScriptCanvasAssetNotificationBus::Event(m_scriptCanvasAsset.GetId(), &EditorScriptCanvasAssetNotifications::OnScriptCanvasAssetReady, m_scriptCanvasAsset);
    }

    void ScriptCanvasAssetHolder::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        SetAsset(asset);
        ActivateAssetData();
        EditorScriptCanvasAssetNotificationBus::Event(m_scriptCanvasAsset.GetId(), &EditorScriptCanvasAssetNotifications::OnScriptCanvasAssetReloaded, m_scriptCanvasAsset);
    }

    void ScriptCanvasAssetHolder::OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType)
    {
        EditorScriptCanvasAssetNotificationBus::Event(assetId, &EditorScriptCanvasAssetNotifications::OnScriptCanvasAssetUnloaded, assetId);
    }

    void ScriptCanvasAssetHolder::OnAssetSaved(AZ::Data::Asset<AZ::Data::AssetData> asset, bool isSuccessful)
    {
        SetAsset(m_scriptCanvasAsset);
        ActivateAssetData();
        auto currentBusId = AZ::Data::AssetBus::GetCurrentBusId();
        if (currentBusId)
        {
            EditorScriptCanvasAssetNotificationBus::Event(*currentBusId, &EditorScriptCanvasAssetNotifications::OnScriptCanvasAssetSaved, asset, isSuccessful);
        }
    }

    void ScriptCanvasAssetHolder::SetAsset(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset)
    {
        m_scriptCanvasAsset = scriptCanvasAsset;

        if (!AZ::Data::AssetBus::Handler::BusIsConnectedId(m_scriptCanvasAsset.GetId()))
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
            if (m_scriptCanvasAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::Handler::BusConnect(m_scriptCanvasAsset.GetId());
            }
        }
    }

    AZ::Data::Asset<ScriptCanvasAsset> ScriptCanvasAssetHolder::GetAsset() const
    {
        return m_scriptCanvasAsset;
    }

    void ScriptCanvasAssetHolder::ActivateAssetData()
    {
        if (m_scriptCanvasAsset.IsReady())
        {
            ScriptCanvasData& scriptCanvasData = m_scriptCanvasAsset.Get()->GetScriptCanvasData();
            AZ::Entity* scriptCanvasEntity = m_scriptCanvasAsset.Get()->GetScriptCanvasEntity();
            if (scriptCanvasEntity)
            {
                if (scriptCanvasEntity->GetState() == AZ::Entity::ES_CONSTRUCTED)
                {
                    AZStd::unordered_map<AZ::EntityId, AZ::EntityId> m_entityIdMap;
                    AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(&scriptCanvasData, m_entityIdMap);
                    scriptCanvasEntity->Init();
                }

                if (scriptCanvasEntity->GetState() == AZ::Entity::ES_INIT)
                {
                    scriptCanvasEntity->Activate();
                    EditorScriptCanvasAssetNotificationBus::Event(m_scriptCanvasAsset.GetId(), &EditorScriptCanvasAssetNotifications::OnScriptCanvasAssetActivated, m_scriptCanvasAsset);
                }
            }
        }
    }
}
