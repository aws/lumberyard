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

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzCore/Component/TickBus.h>

#include <ScriptCanvas/Assets/EditorGraphAsset.h>
#include <ScriptCanvas/Bus/EditorGraphBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <Include/EditorCoreAPI.h>

namespace ScriptCanvasEditor
{
    //=========================================================================
    // EditorGraphComponent
    //=========================================================================
    class EditorGraphComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AZ::Data::AssetBus::Handler
        , private EditorGraphRequestBus::Handler
    {
    public:
        AZ_COMPONENT(EditorGraphComponent, "{C28E2D29-0746-451D-A639-7F113ECF5D72}", AzToolsFramework::Components::EditorComponentBase);

        EditorGraphComponent();
        EditorGraphComponent(AZ::Data::Asset<EditorGraphAsset> asset);
        ~EditorGraphComponent() override;
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
        void SetAsset(const AZ::Data::Asset<EditorGraphAsset>& asset) override;

        //=====================================================================
        // EditorGraphRequestBus
        void SetName(const AZStd::string& name) override { m_name = name; }
        const AZStd::string& GetName() const  override{ return m_name; }
        //=====================================================================

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
        // AZ::Data::AssetBus
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType) override;
        void OnAssetSaved(AZ::Data::Asset<AZ::Data::AssetData> asset, bool isSuccessful) override;
        //=====================================================================

        void LaunchScriptCanvasEditor(const AZ::Data::AssetId&, const AZ::Data::AssetType&);

        //=====================================================================
        // EditorGraphRequestBus
        void OpenEditor() override;
        void CloseGraph() override;
        //=====================================================================

        void UpdateName();

        //! Reloads the Script From the AssetData if it has changed
        AZ::u32 OnScriptChanged();

        AZStd::string m_name;
        AZ::Data::Asset<EditorGraphAsset> m_scriptCanvasAsset;
        // virtual property to implement button function
        bool m_openEditorButton;
    };

}