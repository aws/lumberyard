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

#include <Asset/EditorAssetRegistry.h>
#include <Editor/Assets/ScriptCanvasAssetHolder.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace ScriptCanvasEditor
{
    //=========================================================================
    void ScriptCanvasAssetHolder::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ScriptCanvasAssetHolder>()
                ->Version(1)
                ->Field("m_asset", &ScriptCanvasAssetHolder::m_scriptCanvasAsset)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ScriptCanvasAssetHolder>("Script Canvas", "Script Canvas Asset Holder")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasAssetHolder::m_scriptCanvasAsset, "Script Canvas Asset", "Script Canvas asset associated with this component")
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

    ScriptCanvasAssetHolder::ScriptCanvasAssetHolder(AZ::Data::Asset<ScriptCanvasAsset> asset, const ScriptChangedCB& scriptChangedCB)
        : m_scriptCanvasAsset(asset)
        , m_scriptNotifyCallback(scriptChangedCB)
    {
    }

    ScriptCanvasAssetHolder::~ScriptCanvasAssetHolder()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    void ScriptCanvasAssetHolder::Init(AZ::EntityId ownerId)
    {
        m_ownerId = ownerId;

        if (m_scriptCanvasAsset.GetId().IsValid())
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
        AzToolsFramework::OpenViewPane(LyViewPane::ScriptCanvas);

        AZ::Outcome<int, AZStd::string> openOutcome = AZ::Failure(AZStd::string());
        GeneralRequestBus::BroadcastResult(openOutcome, &GeneralRequests::OpenScriptCanvasAsset, m_scriptCanvasAsset, -1, m_ownerId);
        if (!openOutcome)
        {
            AZ_Warning("Script Canvas", openOutcome, "%s", openOutcome.GetError().data());
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

    void ScriptCanvasAssetHolder::SetScriptChangedCB(const ScriptChangedCB& scriptChangedCB)
    {
        m_scriptNotifyCallback = scriptChangedCB;
    }

    void ScriptCanvasAssetHolder::Load(bool loadBlocking)
    {
        if (!m_scriptCanvasAsset.IsReady())
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, m_scriptCanvasAsset.GetId());
            if (assetInfo.m_assetId.IsValid())
            {
                const AZ::Data::AssetType assetTypeId = azrtti_typeid<ScriptCanvasAsset>();
                auto& assetManager = AZ::Data::AssetManager::Instance();
                m_scriptCanvasAsset = assetManager.GetAsset(m_scriptCanvasAsset.GetId(), azrtti_typeid<ScriptCanvasAsset>(), true, nullptr, loadBlocking);
            }
        }
    }

    AZ::u32 ScriptCanvasAssetHolder::OnScriptChanged()
    {
        SetAsset(m_scriptCanvasAsset);
        Load(false);

        if (m_scriptNotifyCallback)
        {
            m_scriptNotifyCallback(m_scriptCanvasAsset);
        }
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    void ScriptCanvasAssetHolder::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        SetAsset(asset);
        EditorScriptCanvasAssetNotificationBus::Event(m_scriptCanvasAsset.GetId(), &EditorScriptCanvasAssetNotifications::OnScriptCanvasAssetReady, m_scriptCanvasAsset);
    }

    void ScriptCanvasAssetHolder::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        SetAsset(asset);
        EditorScriptCanvasAssetNotificationBus::Event(m_scriptCanvasAsset.GetId(), &EditorScriptCanvasAssetNotifications::OnScriptCanvasAssetReloaded, m_scriptCanvasAsset);
    }

    void ScriptCanvasAssetHolder::OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType)
    {
        EditorScriptCanvasAssetNotificationBus::Event(assetId, &EditorScriptCanvasAssetNotifications::OnScriptCanvasAssetUnloaded, assetId);
    }

    void ScriptCanvasAssetHolder::OnAssetSaved(AZ::Data::Asset<AZ::Data::AssetData> asset, bool isSuccessful)
    {
        SetAsset(m_scriptCanvasAsset);
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
}
