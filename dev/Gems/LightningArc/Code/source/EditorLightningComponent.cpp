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

#include "LightningArc_precompiled.h"
#include "EditorLightningComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include <AzCore/Asset/AssetManagerBus.h>

#include <LmbrCentral/Scripting/SpawnerComponentBus.h>
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>
#include <LmbrCentral/Scripting/RandomTimedSpawnerComponentBus.h>

#include <AzToolsFramework/API/EntityCompositionRequestBus.h>

#include <LegacyEntityConversion/LegacyEntityConversion.h>

namespace Lightning
{
    void EditorLightningComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ_CRC("LightningService"));
    }
    void EditorLightningComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LightningService"));
    }

    void EditorLightningConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorLightningConfiguration, LightningConfiguration>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                AZStd::vector<AZ::Crc32> requiredAudioServices = { AZ_CRC("AudioTriggerService"), AZ_CRC("AudioRtpcService") };

                editContext->Class<EditorLightningConfiguration>("Lightning Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editContext->Class<LightningConfiguration>("Lightning Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &LightningConfiguration::m_startOnActivate, "Start on Activate", "Should the lightning effect start on component activation or when StartEffect is called.")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &LightningConfiguration::m_relativeToPlayer, "Relative To Player", "Will the effect be rendered relative to the player camera.")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &LightningConfiguration::m_lightningDuration, "Duration", "The duration of the entire lightning effect.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.000000)
                    
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Lightning Bolt")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LightningConfiguration::m_lightningParticleEntity, "Particle Entity", "Entity with a particle component that provides the bolt.")
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC("ParticleService"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LightningConfiguration::m_particleSizeVariation, "Size Variation", "Random particle size variation from 0% to 100%.")
                    ->Attribute(AZ::Edit::Attributes::Max, 1.000000)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.000000)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "SkyHighlight")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LightningConfiguration::m_skyHighlightEntity, "Sky Highlight Entity", "Entity with a sky highlight component.")
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC("SkyHighlightService"))

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Light")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LightningConfiguration::m_lightEntity, "Light Entity", "Entity with a point light component.")
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC("LightService"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LightningConfiguration::m_lightRadiusVariation, "Radius Variation", "Random light radius variation from 0% to 100%.")
                    ->Attribute(AZ::Edit::Attributes::Max, 1.000000)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.000000)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LightningConfiguration::m_lightIntensityVariation, "Intensity Variation", "Random light intensity variation from 0% to 100%.")
                    ->Attribute(AZ::Edit::Attributes::Max, 1.000000)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.000000)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Audio")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LightningConfiguration::m_audioThunderEntity, "Audio Entity", "Entity with Audio Trigger, Audio Proxy and Audio RTPC components.")
                    ->Attribute(AZ::Edit::Attributes::RequiredService, requiredAudioServices)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LightningConfiguration::m_speedOfSoundScale, "Speed Of Sound Scale", "Scale to apply to the speed of sound for this affect.")
                    ->Attribute(AZ::Edit::Attributes::Max, 100.000000)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.000000)
                    ;
            }
        }
    }

    void EditorLightningComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorLightningComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("m_config", &EditorLightningComponent::m_config)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorLightningComponent>("Lightning", "Controls timings of other components to produce a lightning bolt effect.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Environment")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Lightning.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Lightning.png")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/lightning-component")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorLightningComponent::m_config, "m_config", "No Description")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<EditorLightningComponent>()->RequestBus("LightningRequestBus");
        }

        EditorLightningConfiguration::Reflect(context);
    }

    void EditorLightningComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<LightningComponent>(&m_config);
    }

    AZ::LegacyConversion::LegacyConversionResult LightningConverter::ConvertEntity(CBaseObject* entityToConvert)
    {
        if (!((AZStd::string(entityToConvert->metaObject()->className()) == "CEntityObject" && AZStd::string(entityToConvert->metaObject()->className()) != "CEntityObject") ||
            (entityToConvert->inherits("CEntityObject") && static_cast<CEntityObject *>(entityToConvert)->GetEntityClass() == "Lightning")))
        {
            // We don't know how to convert this entity, whatever it is
            return AZ::LegacyConversion::LegacyConversionResult::Ignored;
        }
        
        /*
            Note about lightning conversion:
            Because legacy Lightning entities are almost 100% Lua we can't directly convert
            it to a component as we have no way to access the Lua params here.
            Instead default parameters will be used as part of the LightningStrike.slice 
            provided as part of the LightningArc Gem.

            The Lightning system has changed to be more flexible and component based. Instead
            of a component that schedules lightning strikes explicitly, the system has been 
            broken up. There is now a RandomTimedSpawner component that will work with 
            a Spawner component to handle spawning a slice at a given timing and with 
            a given random distribution. Slices can only be spawned in the given volume
            which here will be defined by a Cylinder shape but which could also be 
            defined by a Box shape (or any other shape in the future).
        */

        AZ::Outcome<AZ::Entity*, AZ::LegacyConversion::CreateEntityResult> result(nullptr);

        /// Create an AZ Entity with the necessary components
        AZ::ComponentTypeList entityComponentList = {LmbrCentral::EditorCylinderShapeComponentTypeId};
        AZ::LegacyConversion::LegacyConversionRequestBus::BroadcastResult(result, &AZ::LegacyConversion::LegacyConversionRequests::CreateConvertedEntity, entityToConvert, true, entityComponentList);

        if (!result.IsSuccess())
        {
            // if we failed due to no parent, we keep processing
            return (result.GetError() == AZ::LegacyConversion::CreateEntityResult::FailedNoParent) ? AZ::LegacyConversion::LegacyConversionResult::Ignored : AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        AZ::Entity* entity = result.GetValue();
        AZ::EntityId entityId = entity->GetId();  

        //Change the cylinder size to match the default of the lightning entity

        //Lightning Entity would only spawn in a circle so set the height of this cylinder to a reasonably small value
        LmbrCentral::CylinderShapeComponentRequestsBus::Event(entityId, &LmbrCentral::CylinderShapeComponentRequestsBus::Events::SetHeight, 0.01f);
        LmbrCentral::CylinderShapeComponentRequestsBus::Event(entityId, &LmbrCentral::CylinderShapeComponentRequestsBus::Events::SetRadius, 10.0f);

        //Set the default lightning slice included with the Gem onto the spawner component
        AZ::Data::AssetId sliceAssetId;
        AZStd::string sliceFileName = "slices/lightningstrike.dynamicslice";
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(sliceAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, sliceFileName.c_str(), AZ::Data::s_invalidAssetType, false);

        if (!sliceAssetId.IsValid())
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        AZ::Uuid sliceAssetUuid = AZ::AzTypeInfo<AZ::DynamicSliceAsset>::Uuid();
        AZ::Data::Asset<AZ::Data::AssetData> sliceData(sliceAssetId, sliceAssetUuid);

        //Entity cannot be active while we add this spawner component
        entity->Deactivate();

        //The Spawner component does not have an editor-time component equivalent so we add one based off of a configuration
        AZ::Component* spawnerComponent = nullptr;
        AZ::ComponentDescriptorBus::EventResult(spawnerComponent, LmbrCentral::SpawnerComponentTypeId, &AZ::ComponentDescriptorBus::Events::CreateComponent);

        LmbrCentral::SpawnerConfig spawnerConfig;
        spawnerConfig.m_sliceAsset = sliceData;
        spawnerComponent->SetConfiguration(spawnerConfig);

        AzToolsFramework::EntityCompositionRequestBus::Broadcast(&AzToolsFramework::EntityCompositionRequestBus::Events::AddExistingComponentsToEntity, entity, AZStd::vector<AZ::Component*>{spawnerComponent});

        //Manually add the RandomTimedSpawnerComponent after its dependencies have been met

        entity->CreateComponent(LmbrCentral::EditorRandomTimedSpawnerComponentTypeId);

        //Re-activate here so we can interact with objects on the bus
        entity->Activate();

        //Change the RandomTimedSpawner component to have the same defaults as the lightning entity
        //This has to be done after activate so that the component can actually activate
        //It don't do so unless all required components are available
        LmbrCentral::RandomTimedSpawnerComponentRequestBus::Event(entityId, &LmbrCentral::RandomTimedSpawnerComponentRequestBus::Events::SetSpawnDelay, 5.0f);
        LmbrCentral::RandomTimedSpawnerComponentRequestBus::Event(entityId, &LmbrCentral::RandomTimedSpawnerComponentRequestBus::Events::SetSpawnDelayVariation, 0.5f);

        // finally, we can delete the old entity since we handled this completely!
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, result.GetValue()->GetId());
        return AZ::LegacyConversion::LegacyConversionResult::HandledDeleteEntity;
    }

    bool LightningConverter::BeforeConversionBegins()
    {
        return true;
    }

    bool LightningConverter::AfterConversionEnds()
    {
        return true;
    }

} //namespace Lightning
