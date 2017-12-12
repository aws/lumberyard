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
#include <ScriptCanvas/Bus/DocumentContextBus.h>

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
            if (!assetElement.GetDataHierarchy(serializeContext, scriptCanvasAsset))
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
                editContext->Class<EditorScriptCanvasComponent>("Script Canvas", "A Script Canvas")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/ScriptCanvas.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/ScriptCanvas/Viewport/ScriptCanvas.png")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, ScriptCanvasAssetHandler::GetAssetTypeStatic())
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                    ->DataElement("AssetRef", &EditorScriptCanvasComponent::m_scriptCanvasAssetHolder, "Script Canvas Asset", "Script Canvas asset associated with this component")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
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

    EditorScriptCanvasComponent::EditorScriptCanvasComponent()
        : EditorScriptCanvasComponent(AZ::Data::Asset<ScriptCanvasAsset>())
    {
    }

    EditorScriptCanvasComponent::EditorScriptCanvasComponent(AZ::Data::Asset<ScriptCanvasAsset> asset)
        : m_scriptCanvasAssetHolder(asset)
    {
    }

    EditorScriptCanvasComponent::~EditorScriptCanvasComponent()
    {
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect(); 
    }

    void EditorScriptCanvasComponent::Init()
    {
        EditorComponentBase::Init();
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        auto scriptCanvasAsset = GetAsset();
        if (!scriptCanvasAsset.IsReady())
        {
            // Load the Asset if the Id is valid
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, scriptCanvasAsset.GetId());
            if (assetInfo.m_assetId.IsValid())
            {
                DocumentContextRequestBus::BroadcastResult(scriptCanvasAsset, &DocumentContextRequests::LoadScriptCanvasAssetById, scriptCanvasAsset.GetId(), true);
                m_scriptCanvasAssetHolder.SetAsset(scriptCanvasAsset);
            }

            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree_NewContent);
        }

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

        UpdateName();
    }

    //=========================================================================
    void EditorScriptCanvasComponent::Deactivate()
    {
        EditorComponentBase::Deactivate();

        EditorContextMenuRequestBus::Handler::BusDisconnect();
        EditorScriptCanvasRequestBus::Handler::BusDisconnect();
    }

    //=========================================================================
    void EditorScriptCanvasComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        // A game entity can be created due to the slice compiler and therefore the m_scriptCanvasAsset may not be loaded
        // So attempt to loaded it and copy the Graph Component to the game entity
        auto scriptCanvasAsset = GetAsset();
        if (!scriptCanvasAsset.IsReady())
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, scriptCanvasAsset.GetId());
            if (assetInfo.m_assetId.IsValid())
            {
                // The DocumentContextRequestBus is not available in the SliceBuilder as the ScriptCanvasEditor::SystemComponent is not a required component in that application
                // Therefore a direct call to the AssetManager is used to load ScriptCanvas asset when adding a ScriptCanvasComponent to a dynamic slice
                scriptCanvasAsset = AZ::Data::AssetManager::Instance().GetAsset(assetInfo.m_assetId, ScriptCanvasAssetHandler::GetAssetTypeStatic(), true, &AZ::ObjectStream::AssetFilterDefault, true);
                m_scriptCanvasAssetHolder.SetAsset(scriptCanvasAsset);
            }
        }

        scriptCanvasAsset = GetAsset();
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
                ScriptCanvasEditor::GeneralGraphEventBus::Broadcast(&ScriptCanvasEditor::GeneralGraphEvents::OnBuildGameEntity, scriptCanvasAsset.Get()->GetPath(), buildGraph->GetUniqueId(), newGraph->GetUniqueId());
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
                DocumentContextRequestBus::BroadcastResult(scriptCanvasAsset, &DocumentContextRequests::LoadScriptCanvasAssetById, assetId, false);
            }
            m_scriptCanvasAssetHolder.SetAsset(scriptCanvasAsset);
        }

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    AZ::EntityId EditorScriptCanvasComponent::GetGraphId() const
    {
        return m_scriptCanvasAssetHolder.GetGraphId();
    }

    void EditorScriptCanvasComponent::SetAsset(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset)
    {
        m_scriptCanvasAssetHolder.SetAsset(scriptCanvasAsset);

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
}
