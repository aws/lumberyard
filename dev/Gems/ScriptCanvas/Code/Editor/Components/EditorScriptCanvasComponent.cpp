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
#include <ScriptCanvas/Components/EditorScriptCanvasComponent.h>
#include <ScriptCanvas/Components/EditorGraph.h>

#include <Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Bus/RequestBus.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>


namespace ScriptCanvasEditor
{
    static bool EditorScriptCanvasComponentVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() <= 4)
        {
            int assetElementIndex = rootElement.FindElement(AZ::Crc32("m_asset"));
            if (assetElementIndex == -1)
            {
                return false;
            }

            auto assetElement = rootElement.GetSubElement(assetElementIndex);
            AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset;
            if (!assetElement.GetData(scriptCanvasAsset))
            {
                AZ_Error("Script Canvas", false, "Unable to find Script Canvas Asset on a Version %u Editor ScriptCanvas Component", rootElement.GetVersion());
                return false;
            }

            if (!rootElement.AddElementWithData(serializeContext, "m_assetHolder", ScriptCanvasAssetHolder(scriptCanvasAsset)))
            {
                AZ_Error("Script Canvas", false, "Unable to add ScriptCanvas Asset Holder element when converting from version %u", rootElement.GetVersion())
            }

            rootElement.RemoveElementByName(AZ_CRC("m_asset", 0x4e58e538));
            rootElement.RemoveElementByName(AZ_CRC("m_openEditorButton", 0x9bcb3d5b));
        }
        return true;
    }

    //=========================================================================
    void EditorScriptCanvasComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorScriptCanvasComponent, EditorComponentBase>()
                ->Version(5, &EditorScriptCanvasComponentVersionConverter)
                ->Field("m_name", &EditorScriptCanvasComponent::m_name)
                ->Field("m_assetHolder", &EditorScriptCanvasComponent::m_scriptCanvasAssetHolder)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorScriptCanvasComponent>("Script Canvas", "The Script Canvas component allows you to add a Script Canvas asset to a component, and have it execute on the specified entity.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/ScriptCanvas.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/ScriptCanvas/Viewport/ScriptCanvas.png")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, ScriptCanvasAssetHandler::GetAssetTypeStatic())
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorScriptCanvasComponent::m_scriptCanvasAssetHolder, "Script Canvas Asset", "Script Canvas asset associated with this component")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    EditorScriptCanvasComponent::EditorScriptCanvasComponent()
        : EditorScriptCanvasComponent(AZ::Data::Asset<ScriptCanvasAsset>())
    {
    }

    EditorScriptCanvasComponent::EditorScriptCanvasComponent(AZ::Data::Asset<ScriptCanvasAsset> asset)
        : m_scriptCanvasAssetHolder(asset)
    {
        m_scriptCanvasAssetHolder.SetScriptChangedCB([this](const AZ::Data::Asset<ScriptCanvasAsset>& asset) { OnScriptCanvasAssetChanged(asset); });
    }

    EditorScriptCanvasComponent::~EditorScriptCanvasComponent()
    {
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
    }

    void EditorScriptCanvasComponent::UpdateName()
    {
        if (GetAsset().IsReady())
        {
            // Pathname from the asset doesn't seem to return a value unless the asset has been loaded up once(which isn't done until we try to show it).
            // Using the Job system to determine the asset name instead.
            AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> jobOutcome = AZ::Failure();
            AzToolsFramework::AssetSystemJobRequestBus::BroadcastResult(jobOutcome, &AzToolsFramework::AssetSystemJobRequestBus::Events::GetAssetJobsInfoByAssetID, GetAsset().GetId(), false);

            AZStd::string assetPath;
            AZStd::string assetName;

            if (jobOutcome.IsSuccess())
            {
                AzToolsFramework::AssetSystem::JobInfoContainer& jobs = jobOutcome.GetValue();

                // Get the asset relative path
                if (!jobs.empty())
                {
                    assetPath = jobs[0].m_sourceFile;
                }

                // Get the asset file name
                assetName = assetPath;

                if (!assetPath.empty())
                {
                    AzFramework::StringFunc::Path::GetFileName(assetPath.c_str(), assetName);
                    SetName(assetName);
                }
            }
        }
    }

    void EditorScriptCanvasComponent::OpenEditor()
    {
        m_scriptCanvasAssetHolder.OpenEditor();
    }

    void EditorScriptCanvasComponent::CloseGraph()
    {
        if (GetAsset().GetId().IsValid())
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::CloseScriptCanvasAsset, GetAsset().GetId());
        }
    }

    void EditorScriptCanvasComponent::Init()
    {
        EditorComponentBase::Init();
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();

        m_scriptCanvasAssetHolder.Init();
    }

    //=========================================================================
    void EditorScriptCanvasComponent::Activate()
    {
        EditorComponentBase::Activate();
        EditorContextMenuRequestBus::Handler::BusConnect(GetEntityId());

        AZ::EntityId graphId(GetGraphId());
        if (graphId.IsValid())
        {
            EditorScriptCanvasRequestBus::Handler::BusConnect(graphId);
        }

        auto scriptCanvasAsset = GetAsset();
        if (scriptCanvasAsset.GetId().IsValid())
        {
            // Connections Policies which auto invoke an OnAssetReady function is dangerous, as it may
            // invoke the callback twice if the OnAssetReady function is queued in the AssetBus
            EditorScriptCanvasAssetNotificationBus::Handler::BusConnect(scriptCanvasAsset.GetId());

            // IsReady also checks the ReadyPreNotify state which signifies that the asset is ready
            // but the AssetBus::OnAssetReady event has not been dispatch
            // Here we only want to invoke the method if the OnAsseteReady event has been dispatch
            if (scriptCanvasAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::Ready)
            {
                OnScriptCanvasAssetReady(scriptCanvasAsset);
            }
        }
        UpdateName();
    }

    //=========================================================================
    void EditorScriptCanvasComponent::Deactivate()
    {
        EditorComponentBase::Deactivate();

        EditorScriptCanvasAssetNotificationBus::Handler::BusDisconnect();
        EditorContextMenuRequestBus::Handler::BusDisconnect();
        EditorScriptCanvasRequestBus::Handler::BusDisconnect();
    }

    //=========================================================================
    void EditorScriptCanvasComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        m_scriptCanvasAssetHolder.Load(true);
        auto scriptCanvasAsset = GetAsset();
        if (!scriptCanvasAsset.IsReady())
        {
            AZ_Warning("Script Canvas", !scriptCanvasAsset.GetId().IsValid(), "ScriptCanvasComponent on Entity %s is missing valid asset[%s]. There is no ScriptCanvas Graph Component that can be added to the game entity", GetEntityId().ToString().data(), scriptCanvasAsset.GetId().ToString<AZStd::string>().data());
            return;
        }

        AZ::Entity* buildGraphEntity = scriptCanvasAsset.Get()->GetScriptCanvasEntity();
        auto* buildGraph = buildGraphEntity ? AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvasEditor::Graph>(buildGraphEntity) : nullptr;
        if (buildGraph)
        {
            ScriptCanvas::Graph* newGraph = buildGraph->Compile(GetEntityId());
            if (newGraph)
            {
                gameEntity->AddComponent(newGraph);

                AZ::Data::AssetInfo assetInfo;
                AZStd::string rootFilePath;
                AzToolsFramework::AssetSystemRequestBus::Broadcast(&AzToolsFramework::AssetSystemRequestBus::Events::GetAssetInfoById, scriptCanvasAsset.GetId(), azrtti_typeid<ScriptCanvasAsset>(), assetInfo, rootFilePath);
                AZStd::string absolutePath;
                AzFramework::StringFunc::Path::Join(rootFilePath.data(), assetInfo.m_relativePath.data(), absolutePath);
                ScriptCanvasEditor::GeneralGraphEventBus::Broadcast(&ScriptCanvasEditor::GeneralGraphEvents::OnBuildGameEntity, absolutePath, buildGraph->GetUniqueId(), newGraph->GetUniqueId());
            }
        }
    }

    void EditorScriptCanvasComponent::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId)
    {
        // If the Asset gets removed from disk while the Editor is loaded clear out the asset reference.
        if (GetAsset().GetId() == assetId)
        {
            SetPrimaryAsset({});
        }
    }

    void EditorScriptCanvasComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
    {
        m_scriptCanvasAssetHolder.SetAsset({});

        if (assetId.IsValid())
        {
            auto scriptCanvasAsset = AZ::Data::AssetManager::Instance().FindAsset(assetId);
            if (!scriptCanvasAsset.IsReady())
            {
                scriptCanvasAsset = AZ::Data::AssetManager::Instance().GetAsset(assetId, ScriptCanvasAssetHandler::GetAssetTypeStatic(), true, &AZ::ObjectStream::AssetFilterDefault, false);
            }
            m_scriptCanvasAssetHolder.SetAsset(scriptCanvasAsset);
        }

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    AZ::EntityId EditorScriptCanvasComponent::GetGraphId() const
    {
        return m_scriptCanvasAssetHolder.GetGraphId();
    }

    AZ::EntityId EditorScriptCanvasComponent::GetGraphEntityId() const
    {
        AZ::Entity* scriptCanvasEntity{};
        if (GetAsset().IsReady())
        {
             scriptCanvasEntity = GetAsset().Get()->GetScriptCanvasEntity();
        }

        return scriptCanvasEntity ? scriptCanvasEntity->GetId() : AZ::EntityId();
    }

    void EditorScriptCanvasComponent::OnScriptCanvasAssetChanged(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset)
    {   
        EditorScriptCanvasAssetNotificationBus::Handler::BusDisconnect();
        if (scriptCanvasAsset.GetId().IsValid())
        {
            EditorScriptCanvasAssetNotificationBus::Handler::BusConnect(scriptCanvasAsset.GetId());
            
            // IsReady also checks the ReadyPreNotify state which signifies that the asset is ready
            // but the AssetBus::OnAssetReady event has not been dispatch
            // Here we only want to invoke the method if the OnAsseteReady event has been dispatch
            if (scriptCanvasAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::Ready)
            {
                OnScriptCanvasAssetReady(scriptCanvasAsset);
            }
        }

        AZ::EntityId graphId(GetGraphId());
        if (graphId.IsValid())
        {
            EditorScriptCanvasRequestBus::Handler::BusDisconnect();
            EditorScriptCanvasRequestBus::Handler::BusConnect(graphId);
        }

        UpdateName();
    }

    AZ::Data::Asset<ScriptCanvasAsset> EditorScriptCanvasComponent::GetAsset() const
    {
        return m_scriptCanvasAssetHolder.GetAsset();
    }

    void EditorScriptCanvasComponent::OnScriptCanvasAssetReady(const AZ::Data::Asset<ScriptCanvasAsset>& asset)
    {
        ScriptCanvasData& scriptCanvasData = asset.Get()->GetScriptCanvasData();
        AZ::Entity* scriptCanvasEntity = asset.Get()->GetScriptCanvasEntity();
        if (scriptCanvasEntity)
        {
            if (scriptCanvasEntity->GetState() == AZ::Entity::ES_CONSTRUCTED)
            {
                AZStd::unordered_map<AZ::EntityId, AZ::EntityId> m_entityIdMap;
                AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(&scriptCanvasData, m_entityIdMap);
            }

            // During startup the logging an Error message will caused StartupTraceHandler to eventually follow a path where 
            // ComponentApplication::Tick function is called therefore causing the OnAssetReady callback to be called for this ScriptCanvas Asset
            // The OnAssetReady function will call this function again causing the GenerateNewIdsAndFixRefs logic to be run twice
            // Therefore the ES_CONSTRUCTED state is checked again to workaround the issue where the recursive call Inits the ScriptCanvas Entity
            // that is loaded on the asset
            if (scriptCanvasEntity->GetState() == AZ::Entity::ES_CONSTRUCTED)
            {
                scriptCanvasEntity->Init();
            }

            if (scriptCanvasEntity->GetState() == AZ::Entity::ES_INIT)
            {
                scriptCanvasEntity->Activate();
            }
        }
    }

    void EditorScriptCanvasComponent::OnScriptCanvasAssetReloaded(const AZ::Data::Asset<ScriptCanvasAsset>& asset)
    {
        OnScriptCanvasAssetReady(asset);
    }
}