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

#include "ScriptEventsSystemComponent.h"

#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/AssetEditor/AssetEditorBus.h>
#include <ScriptEvents/ScriptEventsAsset.h>
#include <AzToolsFramework/AssetEditor/AssetEditorBus.h>

namespace ScriptEventsEditor
{

    class ScriptEventAssetHandler 
        : public AzFramework::GenericAssetHandler<ScriptEvents::ScriptEventsAsset>
        , AzToolsFramework::AssetEditor::AssetEditorValidationRequestBus::MultiHandler
    {
    public:
        AZ_RTTI(ScriptEventAssetHandler, "{D81DE7D5-5ED0-4D70-8364-AA986E9C490E}", AzFramework::GenericAssetHandler<ScriptEvents::ScriptEventsAsset>);

        ScriptEventAssetHandler(const char* displayName, const char* group, const char* extension, const AZ::Uuid& componentTypeId = AZ::Uuid::CreateNull(), AZ::SerializeContext* serializeContext = nullptr);

        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override;

        bool LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
        bool SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream) override;

        // AssetEditorValidationRequestBus::Handler
        AZ::Outcome<bool, AZStd::string> IsAssetDataValid(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        void PreAssetSave(AZ::Data::Asset<AZ::Data::AssetData> asset);
        void BeforePropertyEdit(AzToolsFramework::InstanceDataNode* node, AZ::Data::Asset<AZ::Data::AssetData> asset) override;

    private:

        struct PreviousNameSettings
        {
            AZStd::string   m_previousName;
            AZ::u32         m_version;
        };

        AZStd::unordered_map< AZ::Data::AssetId, PreviousNameSettings > m_previousEbusNames;
    };


    class SystemComponent 
        : public ScriptEvents::SystemComponent
    {
    public:
        AZ_COMPONENT(SystemComponent, "{8BAD5292-56C3-4657-99F2-515A2BDE23C1}", ScriptEvents::SystemComponent);

    protected:

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override {}
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        void RegisterAssetHandler() override
        {
            m_editorAssetHandler = AZStd::make_unique<ScriptEventsEditor::ScriptEventAssetHandler>(ScriptEvents::ScriptEventsAsset::GetDisplayName(), ScriptEvents::ScriptEventsAsset::GetGroup(), ScriptEvents::ScriptEventsAsset::GetFileFilter(), AZ::AzTypeInfo<ScriptEventsEditor::SystemComponent>::Uuid());

            AZ::Data::AssetType assetType(azrtti_typeid<ScriptEvents::ScriptEventsAsset>());
            if (AZ::Data::AssetManager::Instance().GetHandler(assetType))
            {
                return; // Asset Type already handled
            }

            AZ::Data::AssetManager::Instance().RegisterHandler(m_editorAssetHandler.get(), assetType);

            // Use AssetCatalog service to register ScriptEvent asset type and extension
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, ScriptEvents::ScriptEventsAsset::GetFileFilter());
        }

        void UnregisterAssetHandler() override
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(m_editorAssetHandler.get());
        }

        AZStd::vector<AZStd::unique_ptr<AzToolsFramework::PropertyHandlerBase>> m_propertyHandlers;

        AZStd::unique_ptr<AzFramework::GenericAssetHandler<ScriptEvents::ScriptEventsAsset>> m_editorAssetHandler;

    };
}
