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

#pragma once

#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <Editor/Assets/ScriptCanvasAssetHolder.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Variable/VariableData.h>

namespace ScriptCanvasEditor
{
    /*! EditorScriptCanvasComponent
    The user facing Editor Component for interfacing with ScriptCanvas
    It connects to the AssetCatalogEventBus in order to remove the ScriptCanvasAssetHolder asset reference
    when the asset is removed from the file system. The reason the ScriptCanvasAssetHolder holder does not
    remove the asset reference itself is because the ScriptCanvasEditor MainWindow has a ScriptCanvasAssetHolder
    which it uses to maintain the asset data in memory. Therefore removing an open ScriptCanvasAsset from the file system
    will remove the reference from the EditorScriptCanvasComponent, but not the reference from the MainWindow allowing the
    ScriptCanvas graph to still be modified while open

    Finally per graph instance variables values are stored on the EditorScriptCanvasComponent and injected into the runtime ScriptCanvas component in BuildGameEntity
    */
    class EditorScriptCanvasComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private EditorScriptCanvasRequestBus::Handler
        , private EditorContextMenuRequestBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
        , private EditorScriptCanvasAssetNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(EditorScriptCanvasComponent, "{C28E2D29-0746-451D-A639-7F113ECF5D72}", AzToolsFramework::Components::EditorComponentBase);

        EditorScriptCanvasComponent();
        EditorScriptCanvasComponent(AZ::Data::Asset<ScriptCanvasAsset> asset);
        ~EditorScriptCanvasComponent() override;
        //=====================================================================
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //=====================================================================


        //=====================================================================
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        void SetPrimaryAsset(const AZ::Data::AssetId&) override;
        //=====================================================================
        AZ::Data::Asset<ScriptCanvasAsset> GetAsset() const override;

        //=====================================================================
        // EditorScriptCanvasRequestBus
        void SetName(const AZStd::string& name) override { m_name = name; }
        const AZStd::string& GetName() const { return m_name; };
        AZ::EntityId GetEditorEntityId() const { return GetEntity() ? GetEntityId() : AZ::EntityId(); }
        //=====================================================================

        //=====================================================================
        // EditorContextMenuRequestBus
        AZ::EntityId GetGraphId() const override;
        //=====================================================================
        AZ::EntityId GetGraphEntityId() const;

    protected:
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            (void)required;
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            (void)incompatible;
        }

        //=====================================================================
        // AssetCatalogEventBus
        void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId) override;
        //=====================================================================
        void OnScriptCanvasAssetChanged(const AZ::Data::Asset<ScriptCanvasAsset>& asset);

        //=====================================================================
        // EditorScriptCanvasRequestBus
        void OpenEditor() override;
        void CloseGraph() override;
        //=====================================================================

        void UpdateName();

        //=====================================================================
        // EditorScriptCanvasAssetNotificationBus
        void OnScriptCanvasAssetReady(const AZ::Data::Asset<ScriptCanvasAsset>& asset);
        void OnScriptCanvasAssetReloaded(const AZ::Data::Asset<ScriptCanvasAsset>& asset);
        //=====================================================================

        void AddVariable(AZStd::string_view varName, const ScriptCanvas::VariableDatum& varDatum);
        void AddNewVariables(const ScriptCanvas::VariableData& graphVarData);
        void RemoveVariable(const ScriptCanvas::VariableId& varId);
        void RemoveOldVariables(const ScriptCanvas::VariableData& graphVarData);
        bool UpdateVariable(const ScriptCanvas::VariableDatum& graphDatum, ScriptCanvas::VariableDatum& updateDatum, ScriptCanvas::VariableDatum& originalDatum);
        void LoadVariables(AZ::Entity* scriptCanvasEntity);
        void ClearVariables();

    private:
        AZStd::string m_name;
        ScriptCanvasAssetHolder m_scriptCanvasAssetHolder;
        
        ScriptCanvas::EditableVariableData m_editableData;

        //< Contains a mapping of the EntityId value from the ScriptCanvasAsset stored as an AZ::u64 so that it does not get remapped
        //< to itself stored as an EntityId
        AZStd::unordered_map<AZ::u64, AZ::EntityId> m_variableEntityIdMap; 
    };
}