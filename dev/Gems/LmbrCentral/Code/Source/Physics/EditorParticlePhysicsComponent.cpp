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
#include "EditorParticlePhysicsComponent.h"
#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    void EditorParticlePhysicsConfiguration::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorParticlePhysicsConfiguration, ParticlePhysicsConfiguration>()
                ->Version(1)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<ParticlePhysicsConfiguration>(
                    "Particle Physics Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Physics")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->DataElement(0, &ParticlePhysicsConfiguration::m_enabledInitially,
                        "Enabled initially", "Whether the entity is initially enabled in the physics simulation.")

                    ->DataElement(0, &ParticlePhysicsConfiguration::m_mass,
                        "Total mass", "Total mass of entity")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " kg")

                    ->DataElement(0, &ParticlePhysicsConfiguration::m_radius,
                        "Radius", "Radius of entity")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")

                    ->DataElement(0, &ParticlePhysicsConfiguration::m_thicknessWhenLying,
                        "Thickness when lying", "The thickness of the entity, to use when it is lying on something or sliding.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")

                    ->DataElement(0, &ParticlePhysicsConfiguration::m_interactsWithTriggers,
                        "Interacts with triggers", "Indicates whether or not this entity can interact with proximity triggers.")

                    ->DataElement(0, &ParticlePhysicsConfiguration::m_collideWithCharacterCapsule,
                        "Collide with character capsule", "Indicates whether or not this entity can collide with character capsule (when disabled, entity collides with character's physical skeleton instead).")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_stopOnFirstContact, 
                        "Stop on first contact", "Stop dead on first contact with another entity")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_orientToTrajectory,
                        "Orient to trajectory", "Continuously align the particle along the current trajectory")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_allowSpin,
                        "Allow spin", "Allow the particle to incur spin when it collides with things")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_bounceSpeedThreshold,
                        "Bounce speed threshold", "Minimum speed below which the particle will stop bouncing")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m/s")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_sleepSpeedThreshold,
                        "Sleep speed threshold", "Minimum speed below which the particle will come to rest")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m/s")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_allowRolling,
                        "Allow rolling", "Whether the particle is allowed to roll. Otherwise it will only slide.")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_alignRollAxis,
                        "Align roll axis", "Whether to align the particle along the roll axis when it is rolling, or just let it rotate freely.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(AZ::Edit::UIHandlers::Vector3, &ParticlePhysicsConfiguration::m_rollAxis,
                        "Roll axis", "Local axis to align with the roll axis when rolling")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ParticlePhysicsConfiguration::m_alignRollAxis)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_applyHitImpulse,
                        "Apply hit impulse", "Apply an impulse to anything the particle hits. Impulse is proportional to speed and mass.")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_airResistance,
                        "Air resistance", "Resistance to movement when in air")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_waterResistance,
                        "Water resistance", "Resistance to movement when in water")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_surfaceIndex,
                        "Surface index", "The physics surface index to use")
                        ->Attribute(AZ::Edit::Attributes::Min, 0)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_useCustomGravity,
                        "Use custom gravity", "Use a custom gravity vector instead of the global world gravity")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(AZ::Edit::UIHandlers::Vector3, &ParticlePhysicsConfiguration::m_customGravity,
                        "Custom gravity", "The custom gravity vector to use")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ParticlePhysicsConfiguration::m_useCustomGravity)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m/s^2")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Collides with types:")
                        ->Attribute("AutoExpand", false)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_collideWithTerrain,
                        "Terrain", "Collide with terrain")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_collideWithStatic,
                        "Static", "Collide with static entities")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_collideWithRigid,
                        "Rigid body (active)", "Collide with active rigid bodies")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_collideWithSleepingRigid,
                        "Rigid body (sleeping)", "Collide with sleeping rigid bodies")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_collideWithLiving,
                        "Living", "Collide with living entities")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_collideWithIndependent,
                        "Independent", "Collide with independent entities")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticlePhysicsConfiguration::m_collideWithParticles,
                        "Particle", "Collide with particles")
                    ;
            }
        }
    }

    void EditorParticlePhysicsComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorParticlePhysicsConfiguration::Reflect(context);

        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorParticlePhysicsComponent, EditorPhysicsComponent>()
                ->Field("Configuration", &EditorParticlePhysicsComponent::m_configuration)
                ->Version(1)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorParticlePhysicsComponent>(
                    "Particle Physics", "The entity behaves as a particle object in the physics simulation.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Physics")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/RigidPhysics.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/RigidPhysics.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorParticlePhysicsComponent::m_configuration, "Configuration", "Configuration for particle physics.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    EditorParticlePhysicsComponent::EditorParticlePhysicsComponent(const EditorParticlePhysicsConfiguration& configuration)
        : m_configuration(configuration)
    {
    }

    void EditorParticlePhysicsComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<ParticlePhysicsComponent>(m_configuration);
    }
} // namespace LmbrCentral