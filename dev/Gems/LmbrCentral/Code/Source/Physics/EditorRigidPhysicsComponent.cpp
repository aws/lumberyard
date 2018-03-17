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
#include "LmbrCentral_precompiled.h"
#include "EditorRigidPhysicsComponent.h"
#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    void EditorRigidPhysicsConfig::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorRigidPhysicsConfig, AzFramework::RigidPhysicsConfig>()
                ->Version(2)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<AzFramework::RigidPhysicsConfig>(
                    "Rigid Body Physics Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Physics")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->DataElement(0, &AzFramework::RigidPhysicsConfig::m_enabledInitially,
                        "Enabled initially", "Whether the entity is initially enabled in the physics simulation.")

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AzFramework::RigidPhysicsConfig::m_specifyMassOrDensity,
                        "Specify mass or density", "Whether total mass is specified, or calculated at spawn time based on density and volume")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->EnumAttribute(AzFramework::RigidPhysicsConfig::MassOrDensity::Mass, "Mass")
                        ->EnumAttribute(AzFramework::RigidPhysicsConfig::MassOrDensity::Density, "Density")

                    ->DataElement(0, &AzFramework::RigidPhysicsConfig::m_mass,
                        "Total mass (kg)", "Total mass of entity, in kg")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzFramework::RigidPhysicsConfig::UseMass)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)

                    ->DataElement(0, &AzFramework::RigidPhysicsConfig::m_density,
                        "Density (kg / cubic meter)", "Mass (kg) per cubic meter of the mesh's volume. Total mass of entity is calculated at spawn. (Water's density is 1000)")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzFramework::RigidPhysicsConfig::UseDensity)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)

                    ->DataElement(0, &AzFramework::RigidPhysicsConfig::m_atRestInitially,
                        "At rest initially", "True: Entity remains at rest until agitated.\nFalse: Entity falls after spawn.")

                    ->DataElement(0, &AzFramework::RigidPhysicsConfig::m_enableCollisionResponse,
                        "Collision response", "True: Entity's simulation is affected normally as a result of collisions with other bodies.\nFalse: Collision events are reported, but the entity's simulation is not affected by collisions.")

                    ->DataElement(0, &AzFramework::RigidPhysicsConfig::m_interactsWithTriggers,
                        "Interacts with triggers", "Indicates whether or not this entity can interact with proximity triggers.")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzFramework::RigidPhysicsConfig::m_recordCollisions, "Record collisions", "Whether or not to record and report collisions with this entity")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzFramework::RigidPhysicsConfig::m_maxRecordedCollisions, "Number of collisions", "Maximum number of collisions to record and report per frame")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzFramework::RigidPhysicsConfig::m_recordCollisions)
                        ->Attribute(AZ::Edit::Attributes::Min, 0)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Simulation")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(0, &AzFramework::RigidPhysicsConfig::m_simulationDamping, "Damping", "Uniform damping value applied to object's movement.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)

                    ->DataElement(0, &AzFramework::RigidPhysicsConfig::m_simulationMinEnergy, "Minimum energy", "The energy threshold under which the object will go to sleep.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Buoyancy")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(0, &AzFramework::RigidPhysicsConfig::m_buoyancyDamping, "Water damping", "Uniform damping value applied while in water.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)

                    ->DataElement(0, &AzFramework::RigidPhysicsConfig::m_buoyancyDensity, "Water density", "Water density strength.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)

                    ->DataElement(0, &AzFramework::RigidPhysicsConfig::m_buoyancyResistance, "Water resistance", "Water resistance strength.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                ;
            }
        }
    }

    void EditorRigidPhysicsComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorPhysicsComponent::Reflect(context);
        EditorRigidPhysicsConfig::Reflect(context);

        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorRigidPhysicsComponent, EditorPhysicsComponent>()
                ->Field("Configuration", &EditorRigidPhysicsComponent::m_configuration)
                ->Version(1)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorRigidPhysicsComponent>(
                    "Rigid Body Physics", "The Rigid Body Physics component is used to represent solid objects that move realistically when touched")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Physics")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/RigidPhysics.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/RigidPhysics.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-rigid-body.html")
                    ->DataElement(0, &EditorRigidPhysicsComponent::m_configuration, "Configuration", "Configuration for rigid body physics.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;
            }
        }
    }

    bool EditorRigidPhysicsComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const AzFramework::RigidPhysicsConfig*>(baseConfig))
        {
            static_cast<AzFramework::RigidPhysicsConfig&>(m_configuration) = *config;
            return true;
        }
        return false;
    }

    bool EditorRigidPhysicsComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<AzFramework::RigidPhysicsConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void EditorRigidPhysicsComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<RigidPhysicsComponent>()->SetConfiguration(m_configuration);
    }

} // namespace LmbrCentral