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
#include "EditorLightningArcComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include <AzCore/Component/TransformBus.h>

#include <AzCore/Asset/AssetManagerBus.h>

#include <LegacyEntityConversion/LegacyEntityConversion.h>

//Legacy for conversion
#include "LightningArc.h"

namespace Lightning
{
    AZStd::string EditorLightningArcComponent::s_customFallbackName = "<Custom>";
    AZStd::vector<AZStd::string> EditorLightningArcComponent::s_presetNames;

    void EditorLightningArcConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorLightningArcConfiguration, LightningArcConfiguration>()
                ->Version(1)
                ->Field("ArcPresetName", &EditorLightningArcConfiguration::m_arcPresetName)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorLightningArcConfiguration>("Editor Lightning Arc Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorLightningArcConfiguration::m_arcPresetName, "Arc Preset Name", "The name of the parameter presets to load.")
                    ->Attribute(AZ::Edit::Attributes::StringList, &EditorLightningArcComponent::GetPresetNames)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorLightningArcConfiguration::OnPresetNameChange)

                    ;

                editContext->Class<LightningArcConfiguration>("Lightning Arc Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcConfiguration::m_enabled, "Enabled", "Sets if the lightning arc effect is enabled and will produce arcs.")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcConfiguration::m_targets, "Targets", "Entities that arcs will randomly target.")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcConfiguration::m_arcParams, "Arc Parameters", "A collection of parameters that describe the look and behavior of the arcs.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &LightningArcConfiguration::OnParamsChange)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcConfiguration::m_materialAsset, "Material", "The material used to render the lightning arcs.")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Timing")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &LightningArcConfiguration::m_delay, "Delay", "Time in seconds between arcs.")
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &LightningArcConfiguration::m_delayVariation, "DelayVariation", "A random variation applied to the Delay parameter.")
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ;
            }
        }
    }

    AZ::Crc32 EditorLightningArcConfiguration::OnPresetNameChange()
    {
        bool newParamsFound = false;
        const LightningArcParams* newParamsPtr = nullptr;

        LightningArcRequestBus::BroadcastResult(newParamsFound, &LightningArcRequestBus::Events::GetArcParamsForName, m_arcPresetName, newParamsPtr);

        if (newParamsFound)
        {
            m_arcParams = *newParamsPtr;
        }

        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    AZ::Crc32 EditorLightningArcConfiguration::OnParamsChange()
    {
        //Let the user know that since they have modified parameters, they are no longer
        //using a preset series of parameters
        m_arcPresetName = EditorLightningArcComponent::s_customFallbackName;

        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    void EditorLightningArcComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ_CRC("LightningArcService"));
    }

    void EditorLightningArcComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LightningArcService"));
    }

    void EditorLightningArcComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorLightningArcComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("Config", &EditorLightningArcComponent::m_config)
                ->Field("RefreshPresets", &EditorLightningArcComponent::m_refreshPresets)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorLightningArcComponent>("Lightning Arc", "Produces an arcing effect that jumps to a random target entity.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/LightningArc.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/LightningArc.png")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/lightning-arc-component")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorLightningArcComponent::m_config, "m_config", "No Description")

                    ->DataElement("Button", &EditorLightningArcComponent::m_refreshPresets, "RefreshPresets", "Reloads all presets off the disk.")
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Refresh Presets")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorLightningArcComponent::RefreshPresets)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<EditorLightningArcComponent>()->RequestBus("LightningArcRequestBus");
        }

        LightningArcParams::EditorReflect(context);
        EditorLightningArcConfiguration::Reflect(context);
    }

    void EditorLightningArcComponent::Activate()
    {
        LightningArcComponentRequestBus::Handler::BusConnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorLightningArcComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect(GetEntityId());
        LightningArcComponentRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    void EditorLightningArcComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<LightningArcComponent>(&m_config);
    }

    void EditorLightningArcComponent::SetMaterial(_smart_ptr<IMaterial> material)
    {
        if (material)
        {
            m_config.m_materialAsset.SetAssetPath(material->GetName());
        }
        else
        {
            m_config.m_materialAsset.SetAssetPath("");
        }
    }
    _smart_ptr<IMaterial> EditorLightningArcComponent::GetMaterial()
    {
        AZStd::string materialPath = m_config.m_materialAsset.GetAssetPath();
        if (materialPath.empty())
        {
            return nullptr;
        }

        return gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(materialPath.c_str());
    }

    void EditorLightningArcComponent::DisplayEntity(bool& handled)
    {
        auto* dc = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        AZ_Assert(dc, "Invalid display context.");

        AZ::Color color(1.0f, 1.0f, 0.0f, 1.0f);
        dc->SetColor(AZ::Vector4(color.GetR(), color.GetG(), color.GetB(), 1.f));

        AZ::Vector3 thisPos = AZ::Vector3::CreateZero();
        AZ::Vector3 targetPos = AZ::Vector3::CreateZero();

        AZ::TransformBus::EventResult(thisPos, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

        //Draw a line from this entity to each target entity
        for (AZ::EntityId entity : m_config.m_targets)
        {
            if (entity.IsValid())
            {
                AZ::TransformBus::EventResult(targetPos, entity, &AZ::TransformBus::Events::GetWorldTranslation);

                dc->DrawLine(thisPos, targetPos);
            }
        }

        handled = true;
    }

    AZ::Crc32 EditorLightningArcComponent::RefreshPresets()
    {
        LightningArcRequestBus::Broadcast(&LightningArcRequestBus::Events::LoadArcPresets);

        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    AZStd::vector<AZStd::string> EditorLightningArcComponent::GetPresetNames()
    {
        LightningArcRequestBus::BroadcastResult(s_presetNames, &LightningArcRequestBus::Events::GetArcPresetNames);

        s_presetNames.insert(s_presetNames.begin(), s_customFallbackName);

        return s_presetNames;
    }

    AZ::LegacyConversion::LegacyConversionResult LightningArcConverter::ConvertEntity(CBaseObject* entityToConvert)
    {
        if (!((AZStd::string(entityToConvert->metaObject()->className()) == "CEntityObject" &&
            AZStd::string(entityToConvert->metaObject()->className()) != "CEntityObject") ||
                (entityToConvert->inherits("CEntityObject") &&
                    static_cast<CEntityObject *>(entityToConvert)->GetEntityClass() == "LightningArc")))
        {
            // We don't know how to convert this entity, whatever it is
            return AZ::LegacyConversion::LegacyConversionResult::Ignored;
        }

        //Get the underlying entity object
        CEntityObject* entityObject = static_cast<CEntityObject*>(entityToConvert);
        if (!entityObject)
        {
            //Object was not an entity object but we did not ignore the object
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        //Retrieve the underlying legacy CLightningArc object
        IEntity* legacyEntity = entityObject->GetIEntity();
        AZStd::shared_ptr<CLightningArc> arc = legacyEntity->GetComponent<CLightningArc>();
        if (!arc)
        {
            //Entity did not have a CLightningArc component
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        // if we have a parent, and the parent is not yet converted, we ignore this entity!
        // this is not ALWAYS the case - there may be types you wish to create anyway and just put them in the world, so we cannot enforce this
        // inside the CreateConvertedEntity function.
        AZ::Outcome<AZ::Entity*, AZ::LegacyConversion::CreateEntityResult> result(nullptr);
        AZ::LegacyConversion::LegacyConversionRequestBus::BroadcastResult(result, &AZ::LegacyConversion::LegacyConversionRequests::CreateConvertedEntity, entityToConvert, true, AZ::ComponentTypeList());

        if (!result.IsSuccess())
        {
            // if we failed due to no parent, we keep processing
            return (result.GetError() == AZ::LegacyConversion::CreateEntityResult::FailedNoParent) ? AZ::LegacyConversion::LegacyConversionResult::Ignored : AZ::LegacyConversion::LegacyConversionResult::Failed;
        }
        AZ::Entity *entity = result.GetValue();
        entity->Deactivate();

        EditorLightningArcComponent *component = entity->CreateComponent<EditorLightningArcComponent>();

        if (!component)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        //Set parameters on new component

        AZStd::vector<AZ::EntityId> targetEntities;
        bool enabled = arc->IsEnabled();
        float delay = arc->GetDelay();
        float delayVariation = arc->GetDelayVariation();
        AZStd::string arcPresetName = arc->GetLightningPreset();
        AZStd::string materialPath = legacyEntity->GetMaterial()->GetName();

        bool arcParamsFound = false;
        const LightningArcParams* paramsPtr = nullptr;

        LightningArcRequestBus::BroadcastResult(arcParamsFound, &LightningArcRequestBus::Events::GetArcParamsForName, arcPresetName, paramsPtr);

        /*
            We can't actually determine the AZ::EntityIds of the
            linked legacy entities but we can push back the right number
            of targets into the vector.
        */
        IEntityLink* link = legacyEntity->GetEntityLinks();
        while (link != nullptr)
        {
            if (strcmp(link->name, "Target") == 0)
            {
                targetEntities.push_back(AZ::EntityId());
            }
            link = link->next;
        }

        component->m_config.m_enabled = enabled;
        component->m_config.m_delay = delay;
        component->m_config.m_delayVariation = delayVariation;
        component->m_config.m_arcPresetName = arcPresetName;
        component->m_config.m_targets = targetEntities;

        if (arcParamsFound)
        {
            component->m_config.m_arcParams = *paramsPtr;
        }

        if (!materialPath.empty())
        {
            component->m_config.m_materialAsset.SetAssetPath(materialPath.c_str());
        }

        entity->Activate();

        return AZ::LegacyConversion::LegacyConversionResult::HandledDeleteEntity;
    }

    bool LightningArcConverter::BeforeConversionBegins()
    {
        return true;
    }

    bool LightningArcConverter::AfterConversionEnds()
    {
        return true;
    }

} //namespace Lightning