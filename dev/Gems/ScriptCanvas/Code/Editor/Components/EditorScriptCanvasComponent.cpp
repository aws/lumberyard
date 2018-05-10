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
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasDocumentContext.h>

#include <Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Bus/RequestBus.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Utils.h>

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

        if (rootElement.GetVersion() <= 6)
        {
            rootElement.RemoveElementByName(AZ_CRC("m_originalData", 0x2aee5989));
        }
        return true;
    }

    //=========================================================================
    void EditorScriptCanvasComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorScriptCanvasComponent, EditorComponentBase>()
                ->Version(7, &EditorScriptCanvasComponentVersionConverter)
                ->Field("m_name", &EditorScriptCanvasComponent::m_name)
                ->Field("m_assetHolder", &EditorScriptCanvasComponent::m_scriptCanvasAssetHolder)
                ->Field("m_variableData", &EditorScriptCanvasComponent::m_editableData)
                ->Field("m_variableEntityIdMap", &EditorScriptCanvasComponent::m_variableEntityIdMap)
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
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorScriptCanvasComponent::m_editableData, "Variables",
                        "Script Canvas graph variables")
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

        m_scriptCanvasAssetHolder.Init(GetEntityId());
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

            if (!scriptCanvasAsset.IsReady())
            {
                // IsReady also checks the ReadyPreNotify state which signifies that the asset is ready
                // but the AssetBus::OnAssetReady event has not been dispatch
                // Here we only want to invoke the method if the OnAsseteReady event has been dispatch
                DocumentContextRequestBus::Broadcast(&DocumentContextRequests::LoadScriptCanvasAssetById, scriptCanvasAsset.GetId(), false);
            }
            else
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
        auto scriptCanvasAsset = GetAsset();

        AZ::Data::AssetId runtimeAssetId(scriptCanvasAsset.GetId().m_guid, AZ_CRC("RuntimeData", 0x163310ae));
        AZ::Data::Asset<ScriptCanvas::RuntimeAsset> runtimeAsset(runtimeAssetId, azrtti_typeid<ScriptCanvas::RuntimeAsset>());
        auto executionComponent = gameEntity->CreateComponent<ScriptCanvas::RuntimeComponent>(runtimeAsset);
        ScriptCanvas::VariableData varData;

        for (const auto& varConfig : m_editableData.GetVariables())
        {
            varData.AddVariable(varConfig.m_varNameValuePair.m_varName, varConfig.m_varNameValuePair.m_varDatum);
        }

        executionComponent->SetVariableOverrides(varData);
        executionComponent->SetVariableEntityIdMap(m_variableEntityIdMap);
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
        ClearVariables();
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

        LoadVariables(scriptCanvasEntity);
    }

    void EditorScriptCanvasComponent::OnScriptCanvasAssetReloaded(const AZ::Data::Asset<ScriptCanvasAsset>& asset)
    {
        OnScriptCanvasAssetReady(asset);
    }

    /*! Start Variable Block Implementation */
    void EditorScriptCanvasComponent::AddVariable(AZStd::string_view varName, const ScriptCanvas::VariableDatum& varDatum)
    {
        bool exposeVariableToComponent = varDatum.ExposeAsComponentInput();
        if (!exposeVariableToComponent)
        {
            return;
        }

        const auto& variableId = varDatum.GetId();
        ScriptCanvas::EditableVariableConfiguration* originalVarNameValuePair = m_editableData.FindVariable(variableId);
        if (!originalVarNameValuePair)
        {
            m_editableData.AddVariable(varName, varDatum);
            originalVarNameValuePair = m_editableData.FindVariable(variableId);
        }

        if (!originalVarNameValuePair)
        {
            AZ_Error("Script Canvas", false, "Unable to find variable with id %s and name %s on the ScriptCanvas Component. There is an issue in AddVariable",
                variableId.ToString().data(), varName.data());
            return;
        }

        // Update the variable name as it may have changed
        originalVarNameValuePair->m_varNameValuePair.m_varName = varName;
        originalVarNameValuePair->m_varNameValuePair.m_varDatum.SetInputControlVisibility(AZ::Edit::PropertyVisibility::Hide);
    }

    void EditorScriptCanvasComponent::AddNewVariables(const ScriptCanvas::VariableData& graphVarData)
    {
        for (auto&& variablePair : graphVarData.GetVariables())
        {
            AddVariable(variablePair.second.m_varName, variablePair.second.m_varDatum);
        }
    }

    void EditorScriptCanvasComponent::RemoveVariable(const ScriptCanvas::VariableId& varId)
    {
        m_editableData.RemoveVariable(varId);
    }

    void EditorScriptCanvasComponent::RemoveOldVariables(const ScriptCanvas::VariableData& graphVarData)
    {
        AZStd::vector<ScriptCanvas::VariableId> oldVariableIds;
        for (auto varConfig : m_editableData.GetVariables())
        {
            const auto& variableId = varConfig.m_varNameValuePair.m_varDatum.GetId();

            auto nameValuePair = graphVarData.FindVariable(variableId);
            if (!nameValuePair || !nameValuePair->m_varDatum.ExposeAsComponentInput())
            {
                oldVariableIds.push_back(variableId);
            }

        }

        for (const auto& oldVariableId : oldVariableIds)
        {
            RemoveVariable(oldVariableId);
        }
    }

    bool EditorScriptCanvasComponent::UpdateVariable(const ScriptCanvas::VariableDatum& graphDatum, ScriptCanvas::VariableDatum& updateDatum, ScriptCanvas::VariableDatum& originalDatum)
    {
        // If the editable datum is the different than the original datum, then the "variable value" has been overridden on this component

        // Variable values only propagate from the Script Canvas graph to this component if the original "variable value" has not been overridden
        // by the editable "variable value" on this component and the "variable value" on the graph is different than the variable value on this component
        auto isNotOverridden = updateDatum.GetData() == originalDatum.GetData();
        auto scGraphIsModified = originalDatum.GetData() != graphDatum.GetData();
        if (isNotOverridden && scGraphIsModified)
        {
            originalDatum.GetData() = graphDatum.GetData();
            updateDatum.GetData() = graphDatum.GetData();
            return true;
        }
        return false;
    }

    void EditorScriptCanvasComponent::LoadVariables(AZ::Entity* scriptCanvasEntity)
    {
        auto variableComponent = scriptCanvasEntity ? AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::GraphVariableManagerComponent>(scriptCanvasEntity) : nullptr;
        if (variableComponent)
        {
            m_variableEntityIdMap.clear();
            for (const auto& varIdToVariablePair : variableComponent->GetVariableData()->GetVariables())
            {
                if (const AZ::EntityId* entityIdVariable = varIdToVariablePair.second.m_varDatum.GetData().GetAs<AZ::EntityId>())
                {
                    m_variableEntityIdMap.emplace(static_cast<AZ::u64>(*entityIdVariable), *entityIdVariable);
                }
            }
            // Add properties from the SC Asset to the SC Component if they do not exist on the SC Component
            AddNewVariables(*variableComponent->GetVariableData());
            RemoveOldVariables(*variableComponent->GetVariableData());
        }

        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
    }

    void EditorScriptCanvasComponent::ClearVariables()
    {
        m_editableData.Clear();
    }
    /* End Variable Block Implementation*/
}