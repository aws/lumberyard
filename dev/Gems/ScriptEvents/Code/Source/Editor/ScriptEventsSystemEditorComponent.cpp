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

#include "ScriptEventsSystemEditorComponent.h"

#include <ScriptEvents/Internal/VersionedProperty.h>

#include <AzToolsFramework/UI/PropertyEditor/GenericComboBoxCtrl.h>
#include <ScriptEvents/ScriptEventDefinition.h>

#include <ScriptEvents/ScriptEvent.h>

namespace ScriptEventsEditor
{
    ////////////////////////////
    // ScriptEventAssetHandler
    ////////////////////////////

    ScriptEventAssetHandler::ScriptEventAssetHandler(const char* displayName, const char* group, const char* extension, const AZ::Uuid& componentTypeId, AZ::SerializeContext* serializeContext)    
        : AzFramework::GenericAssetHandler<ScriptEvents::ScriptEventsAsset>(displayName, group, extension, componentTypeId, serializeContext)
    {
    }

    AZ::Data::AssetPtr ScriptEventAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        if (type != azrtti_typeid<ScriptEvents::ScriptEventsAsset>())
        {
            return nullptr;
        }

        AZ::Data::AssetPtr assetPtr = AzFramework::GenericAssetHandler<ScriptEvents::ScriptEventsAsset>::CreateAsset(id, type);

        AZ_Assert(id.IsValid(), "Script Event asset must have a valid ID to be created.");
        if (!AzToolsFramework::AssetEditor::AssetEditorValidationRequestBus::MultiHandler::BusIsConnectedId(id))
        {
            AZ_TracePrintf("ScriptEvent", "Creating Asset with ID: %s - SCRIPTEVENT\n", id.ToString<AZStd::string>().c_str());
            AzToolsFramework::AssetEditor::AssetEditorValidationRequestBus::MultiHandler::BusConnect(id);
        }

        return assetPtr;
    }

    bool ScriptEventAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        bool loadedData = AzFramework::GenericAssetHandler<ScriptEvents::ScriptEventsAsset>::LoadAssetData(asset, assetPath, assetLoadFilterCB);

        if (loadedData)
        {
            ScriptEvents::ScriptEventsAsset* assetData = asset.GetAs<ScriptEvents::ScriptEventsAsset>();

            if (assetData)
            {
                auto busIter = m_previousEbusNames.find(asset.GetId());

                bool registerBus = true;

                if (busIter != m_previousEbusNames.end())
                {
                    if (busIter->second.m_version < assetData->m_definition.GetVersion())
                    {
                        ScriptEvents::Internal::Utils::DestroyScriptEventBehaviorEBus(busIter->second.m_previousName);
                        m_previousEbusNames.erase(busIter);
                    }
                    else
                    {
                        registerBus = false;
                    }
                }

                if (registerBus)
                {
                    if (ScriptEvents::Internal::Utils::ConstructAndRegisterScriptEventBehaviorEBus(assetData->m_definition))
                    {
                        PreviousNameSettings previousSettings;
                        previousSettings.m_previousName = assetData->m_definition.GetName().c_str();
                        previousSettings.m_version = assetData->m_definition.GetVersion();

                        m_previousEbusNames[asset.GetId()] = previousSettings;                        
                    }
                }
            }
        }

        return loadedData;
    }

    bool ScriptEventAssetHandler::SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream)
    {
        AZ_TracePrintf("ScriptEvent", "Trying to save Asset with ID: %s - SCRIPTEVENT", asset.Get()->GetId().ToString<AZStd::string>().c_str());

        // Attempt to Save the data to a temporary stream in order to see if any 
        AZ::Outcome<bool, AZStd::string> outcome = AZ::Failure(AZStd::string::format("AssetEditorValidationRequests is not connected ID: %s", asset.Get()->GetId().ToString<AZStd::string>().c_str()));

        // Verify that the asset is in a valid state that can be saved.
        AzToolsFramework::AssetEditor::AssetEditorValidationRequestBus::EventResult(outcome, asset.Get()->GetId(), &AzToolsFramework::AssetEditor::AssetEditorValidationRequests::IsAssetDataValid, asset);
        if (!outcome.IsSuccess())
        {
            AZ_Error("Asset Editor", false, "%s", outcome.GetError().c_str());
            return false;
        }

        return AzFramework::GenericAssetHandler<ScriptEvents::ScriptEventsAsset>::SaveAssetData(asset, stream);
    }

    AZ::Outcome<bool, AZStd::string> ScriptEventAssetHandler::IsAssetDataValid(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        ScriptEvents::ScriptEventsAsset* assetData = asset.GetAs<ScriptEvents::ScriptEventsAsset>();
        if (!assetData)
        {
            return AZ::Failure(AZStd::string::format("Unable to validate asset with id: %s it has not been registered with the Script Event system component.", asset.GetId().ToString<AZStd::string>().c_str()));
        }

        const ScriptEvents::ScriptEvent* definition = &assetData->m_definition;
        AZ_Assert(definition, "The AssetData should have a valid definition");

        return definition->Validate();
    }

    void ScriptEventAssetHandler::PreAssetSave(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        ScriptEvents::ScriptEventsAsset* scriptEventAsset = asset.GetAs<ScriptEvents::ScriptEventsAsset>();
        scriptEventAsset->m_definition.IncreaseVersion();
    }

    void ScriptEventAssetHandler::BeforePropertyEdit(AzToolsFramework::InstanceDataNode* node, AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        ScriptEventData::VersionedProperty* property = nullptr;
        AzToolsFramework::InstanceDataNode* parent = node;
        while (parent)
        {
            if (parent->GetClassMetadata()->m_typeId == azrtti_typeid<ScriptEventData::VersionedProperty>())
            {
                property = static_cast<ScriptEventData::VersionedProperty*>(parent->GetInstance(0));
                break;
            }
            parent = parent->GetParent();
        }

        if (property)
        {
            property->OnPropertyChange();
        }
    }

    ////////////////////
    // SystemComponent
    ////////////////////
    void SystemComponent::Activate()
    {
        ScriptEvents::SystemComponent::Activate();

        m_propertyHandlers.emplace_back(AzToolsFramework::RegisterGenericComboBoxHandler<ScriptEventData::VersionedProperty>());
    }

    void SystemComponent::Deactivate()
    {
        ScriptEvents::SystemComponent::Deactivate();

        for (auto&& propertyHandler : m_propertyHandlers)
        {
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, propertyHandler.get());
        }
        m_propertyHandlers.clear();
        
        m_editorAssetHandler->Unregister();
        m_editorAssetHandler.reset();
    }
}

