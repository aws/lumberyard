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

#include "Rain_precompiled.h"
#include "EditorRainComponent.h"

#include <Editor/Objects/BaseObject.h>
#include <Objects/EntityObject.h>

#include <MathConversion.h>

//Legacy for conversion
#include "Rain.h"

namespace Rain
{
    void EditorRainComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorRainComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("Enabled", &EditorRainComponent::m_enabled)
                ->Field("Options", &EditorRainComponent::m_rainOptions)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorRainComponent>("Rain", "Defines rain settings for the level ")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Environment")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Rain.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Rain.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/rain-component")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorRainComponent::m_enabled, "Enabled", "Sets if the rain is enabled")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorRainComponent::m_rainOptions, "Options", "Options for rain simulation")
                    ;

                editContext->Class<RainOptions>("Rain Options", "Options for rain simulation")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &RainOptions::m_useVisAreas, "Use VisAreas", "Should rain render when the player is inside of a VisArea")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RainOptions::m_disableOcclusion, "Disable Occlusion", "Should allow objects to block rainfall")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RainOptions::m_radius, "Radius", "Radius of the rain area")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10000.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RainOptions::m_amount, "Amount", "Overall amount of rain")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RainOptions::m_diffuseDarkening, "Diffuse Darkening", "Amount of darkening due to surface wetness")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Drops")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RainOptions::m_rainDropsAmount, "Amount", "Number of rain drops that can be seen in the air")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RainOptions::m_rainDropsSpeed, "Speed", "Speed of falling rain drops")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RainOptions::m_rainDropsLighting, "Lighting", "Brightness/Back-lighting of rain drops")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Puddles")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RainOptions::m_puddlesAmount, "Amount", "Depth and brightness of puddles generated")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RainOptions::m_puddlesMaskAmount, "Mask Amount", "Strength of the puddle mask")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RainOptions::m_puddlesRippleAmount, "Ripple Amount", "Strength and frequency of ripples in puddles")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RainOptions::m_splashesAmount, "Splashes Amount", "Strength of the splash effect generated by rain drops")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ;
            }
        }
    }

    void EditorRainComponent::Activate()
    {
        EditorComponentBase::Activate();

        AzToolsFramework::Components::EditorComponentBase::Activate();
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());

        m_currentWorldPos = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(m_currentWorldPos, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);
    }

    void EditorRainComponent::Deactivate()
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect(GetEntityId());

        EditorComponentBase::Deactivate();
    }

    void EditorRainComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentWorldPos = world.GetPosition();
    }

    void EditorRainComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto component = gameEntity->CreateComponent<RainComponent>())
        {
            component->m_enabled = m_enabled;
            component->m_rainOptions = m_rainOptions;
        }
    }

    void EditorRainComponent::DisplayEntity(bool& handled)
    {
        auto dc = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        if (dc == nullptr)
        {
            handled = false;
            return;
        }

        dc->SetColor(AZ::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        dc->DrawWireSphere(m_currentWorldPos, m_rainOptions.m_radius);

        handled = true;
    }

    AZ::LegacyConversion::LegacyConversionResult RainConverter::ConvertEntity(CBaseObject* pEntityToConvert)
    {
        if (!((AZStd::string(pEntityToConvert->metaObject()->className()) == "CRain" && AZStd::string(pEntityToConvert->metaObject()->className()) != "CEntityObject") ||
            (pEntityToConvert->inherits("CEntityObject") && static_cast<CEntityObject *>(pEntityToConvert)->GetEntityClass() == "Rain")))
        {
            // We don't know how to convert this entity, whatever it is
            return AZ::LegacyConversion::LegacyConversionResult::Ignored;
        }

        //Get the underlying entity object
        CEntityObject* entityObject = static_cast<CEntityObject*>(pEntityToConvert);
        if (!entityObject)
        {
            //Object was not an entity object but we did not ignore the object
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        //Retrieve the underlying legacy CRain object
        using namespace LYGame;
        IEntity* legacyEntity = entityObject->GetIEntity();
        AZStd::shared_ptr<CRain> rain = legacyEntity->GetComponent<CRain>();
        if (!rain)
        {
            //Entity did not have a CRain component
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        // Step 1, create an entity with no components.

        // if we have a parent, and the parent is not yet converted, we ignore this entity!
        // this is not ALWAYS the case - there may be types you wish to create anyway and just put them in the world, so we cannot enforce this
        // inside the CreateConvertedEntity function.
        AZ::Outcome<AZ::Entity*, AZ::LegacyConversion::CreateEntityResult> result(nullptr);
        AZ::LegacyConversion::LegacyConversionRequestBus::BroadcastResult(result, &AZ::LegacyConversion::LegacyConversionRequests::CreateConvertedEntity, pEntityToConvert, true, AZ::ComponentTypeList());

        if (!result.IsSuccess())
        {
            // if we failed due to no parent, we keep processing
            return (result.GetError() == AZ::LegacyConversion::CreateEntityResult::FailedNoParent) ? AZ::LegacyConversion::LegacyConversionResult::Ignored : AZ::LegacyConversion::LegacyConversionResult::Failed;
        }
        AZ::Entity *entity = result.GetValue();
        entity->Deactivate();

        EditorRainComponent *component = entity->CreateComponent<EditorRainComponent>();

        if (!component)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        // Step 2: extract the parameter data from the underlying CRain object into
        // the component.

        // Because the rain object is a Lua based entity this is the easiest way to retrieve its parameters

        SRainParams legacyParams = rain->GetRainParams();
        Rain::RainOptions options;

        options.m_useVisAreas = !legacyParams.bIgnoreVisareas;
        options.m_disableOcclusion = legacyParams.bDisableOcclusion;
        options.m_radius = legacyParams.fRadius;
        options.m_amount = legacyParams.fAmount;
        options.m_diffuseDarkening = legacyParams.fDiffuseDarkening;
        options.m_puddlesAmount = legacyParams.fPuddlesAmount;
        options.m_puddlesMaskAmount = legacyParams.fPuddlesMaskAmount;
        options.m_puddlesRippleAmount = legacyParams.fPuddlesRippleAmount;
        options.m_rainDropsAmount = legacyParams.fRainDropsAmount;
        options.m_rainDropsSpeed = legacyParams.fRainDropsSpeed;
        options.m_rainDropsLighting = legacyParams.fRainDropsLighting;
        options.m_splashesAmount = legacyParams.fSplashesAmount;

        component->m_enabled = rain->GetEnabled();
        component->m_rainOptions = options;

        // Turn the entity back on before wrapping up.
        entity->Activate();

        // finally, we can delete the old entity since we handled this completely!
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, result.GetValue()->GetId());
        return AZ::LegacyConversion::LegacyConversionResult::HandledDeleteEntity;
    }

    bool RainConverter::BeforeConversionBegins()
    {
        return true;
    }

    bool RainConverter::AfterConversionEnds()
    {
        return true;
    }
}