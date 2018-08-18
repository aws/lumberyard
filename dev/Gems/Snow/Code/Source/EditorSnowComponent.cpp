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

#include "Snow_precompiled.h"
#include "EditorSnowComponent.h"

#include <Editor/Objects/BaseObject.h>
#include <Objects/EntityObject.h>

#include <MathConversion.h>

//Legacy for conversion
#include "Snow.h"

namespace Snow
{
    void EditorSnowComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSnowComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("Enabled", &EditorSnowComponent::m_enabled)
                ->Field("Options", &EditorSnowComponent::m_snowOptions)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorSnowComponent>("Snow", "Defines snow settings for the level ")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Environment")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Snow.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Snow.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/snow-component")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorSnowComponent::m_enabled, "Enabled", "Sets if the snow is enabled")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSnowComponent::UpdateSnow)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorSnowComponent::m_snowOptions, "Options", "Options for snow simulation")
                    ;

                editContext->Class<SnowOptions>("Snow Options", "Modify the snowfall appearance. Snow or Frost amount must be greater than 0 to enable")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Snow Surface")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SnowOptions::m_radius, "Radius", "Radius of the snow area")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 16000.f) //16K Max for large maps
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SnowOptions::OnChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SnowOptions::m_snowAmount, "Snow Amount", "Amount of snow on the surface")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SnowOptions::OnChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SnowOptions::m_frostAmount, "Frost Amount", "Amount of frost on the surface")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SnowOptions::OnChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SnowOptions::m_surfaceFreezing, "Surface Freezing", "Amount of surface freezing")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SnowOptions::OnChanged)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Snowfall Options")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SnowOptions::m_snowFlakeCount, "Snowflake Count", "Amount of snowflakes")
                        ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Max, 10000)
                        ->Attribute(AZ::Edit::Attributes::Step, 1)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SnowOptions::OnChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SnowOptions::m_snowFlakeSize, "Snowflake Size", "Size of the snowflakes")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SnowOptions::OnChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SnowOptions::m_snowFallBrightness, "Brightness", "Brightness of the snowfall")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SnowOptions::OnChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SnowOptions::m_snowFallGravityScale, "Gravity Scale", "Snowfall gravity modifier")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SnowOptions::OnChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SnowOptions::m_snowFallWindScale, "Wind Scale", "Snowfall wind force modifier")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SnowOptions::OnChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SnowOptions::m_snowFallTurbulence, "Turbulence", "Turbulence applied to snowfall")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SnowOptions::OnChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SnowOptions::m_snowFallTurbulenceFreq, "Turbulence Frequency", "Turbulence frequency modifier")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SnowOptions::OnChanged)
                    ;
            }
        }
    }

    void EditorSnowComponent::Activate()
    {
        EditorComponentBase::Activate();

        AzToolsFramework::Components::EditorComponentBase::Activate();
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AZ::TickBus::Handler::BusConnect();

        auto changeCallback = [this]() { this->UpdateSnow(); };
        m_snowOptions.m_changeCallback = changeCallback;

        //Get initial position
        m_currentWorldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentWorldTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
    }

    void EditorSnowComponent::Deactivate()
    {
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect(GetEntityId());
        AZ::TickBus::Handler::BusDisconnect();

        m_snowOptions.m_changeCallback = nullptr;

        TurnOffSnow();

        //If any other snow components exist, broadcast to them that they should re-update the snow params
        //This is useful if two snow components exist and one is deleted; some other component's snow params sould
        //still be applied. This is just for preview in-editor.
        Snow::SnowComponentRequestBus::Broadcast(&Snow::SnowComponentRequestBus::Events::UpdateSnow);

        EditorComponentBase::Deactivate();
    }

    void EditorSnowComponent::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
    {
        m_currentWorldTransform = world;
        UpdateSnow();
    }

    void EditorSnowComponent::UpdateSnow()
    {
        //If the snow component is disabled, set the snow amount to 0
        if (!m_enabled || gEnv->IsEditor() && !gEnv->IsEditorSimulationMode() && !gEnv->IsEditorGameMode())
        {
            TurnOffSnow();
            return;
        }

        UpdateSnowSettings(m_currentWorldTransform, m_snowOptions);
    }

    void EditorSnowComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto component = gameEntity->CreateComponent<SnowComponent>())
        {
            component->m_enabled = m_enabled;
            component->m_snowOptions = m_snowOptions;
        }
    }

    void EditorSnowComponent::DisplayEntity(bool& handled)
    {
        auto dc = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        if (dc == nullptr)
        {
            handled = false;
            return;
        }

        dc->SetColor(AZ::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        dc->DrawWireSphere(m_currentWorldTransform.GetPosition(), m_snowOptions.m_radius);

        handled = true;
    }

    void EditorSnowComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        UpdateSnow();
    }

    //////////////////////////////////////////////////////////////////////////

    AZ::LegacyConversion::LegacyConversionResult SnowConverter::ConvertEntity(CBaseObject* pEntityToConvert)
    {
        if (!((AZStd::string(pEntityToConvert->metaObject()->className()) == "CSnow" && AZStd::string(pEntityToConvert->metaObject()->className()) != "CEntityObject") ||
            (pEntityToConvert->inherits("CEntityObject") && static_cast<CEntityObject *>(pEntityToConvert)->GetEntityClass() == "Snow")))
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

        //Retrieve the underlying legacy CSnow object
        IEntity* legacyEntity = entityObject->GetIEntity();
        AZStd::shared_ptr<CSnow> snow = legacyEntity->GetComponent<CSnow>();
        if (!snow)
        {
            //Entity did not have a CSnow component
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

        EditorSnowComponent *component = entity->CreateComponent<EditorSnowComponent>();

        if (!component)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        // Step 2: extract the parameter data from the underlying CSnow object into
        // the component.

        // Because the snow object is a Lua based entity this is the easiest way to retrieve its parameters

        //Snow Surface Settings
        float radius;
        float snowAmount;
        float frostAmount;
        float surfaceFreezing;

        //Snow Fall Settings
        int snowFlakeCount;
        float snowFlakeSize;
        float snowFallBrightness;
        float snowFallGravityScale;
        float snowFallWindScale;
        float snowFallTurbulence;
        float snowFallTurbulenceFreq;

        snow->GetSnowSurfaceParams(radius, snowAmount, frostAmount, surfaceFreezing);
        snow->GetSnowFallParams(snowFlakeCount, snowFlakeSize, snowFallBrightness, snowFallGravityScale, snowFallWindScale, snowFallTurbulence, snowFallTurbulenceFreq);

        Snow::SnowOptions options;

        options.m_radius = radius;
        options.m_snowAmount = snowAmount;
        options.m_frostAmount = frostAmount;
        options.m_surfaceFreezing = surfaceFreezing;
        options.m_snowFlakeCount = snowFlakeCount;
        options.m_snowFlakeSize = snowFlakeSize;
        options.m_snowFallBrightness = snowFallBrightness;
        options.m_snowFallGravityScale = snowFallGravityScale;
        options.m_snowFallWindScale = snowFallWindScale;
        options.m_snowFallTurbulence = snowFallTurbulence;
        options.m_snowFallTurbulenceFreq = snowFallTurbulenceFreq;

        component->m_enabled = snow->GetEnabled();
        component->m_snowOptions = options;

        // Turn the entity back on before wrapping up.
        entity->Activate();

        // finally, we can delete the old entity since we handled this completely!
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, result.GetValue()->GetId());
        return AZ::LegacyConversion::LegacyConversionResult::HandledDeleteEntity;
    }

    bool SnowConverter::BeforeConversionBegins()
    {
        return true;
    }

    bool SnowConverter::AfterConversionEnds()
    {
        return true;
    }
}