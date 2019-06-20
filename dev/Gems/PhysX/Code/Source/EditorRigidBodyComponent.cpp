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

#include <PhysX_precompiled.h>

#include <AzCore/Serialization/EditContext.h>
#include <Source/EditorRigidBodyComponent.h>
#include <Source/EditorColliderComponent.h>
#include <Source/EditorSystemComponent.h>
#include <Source/RigidBodyComponent.h>
#include <Editor/InertiaPropertyHandler.h>
#include <Editor/EditorClassConverters.h>

namespace PhysX
{
    void EditorRigidBodyConfiguration::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorRigidBodyConfiguration, Physics::RigidBodyConfiguration>()
                ->Version(2, &ClassConverters::EditorRigidBodyConfigVersionConverter)
                ->Field("Debug Draw Center of Mass", &EditorRigidBodyConfiguration::m_centerOfMassDebugDraw)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<Physics::RigidBodyConfiguration>(
                    "PhysX Rigid Body Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Physics::RigidBodyConfiguration::m_initialLinearVelocity,
                    "Initial linear velocity", "Initial linear velocity")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Physics::RigidBodyConfiguration::GetInitialVelocitiesVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Physics::RigidBodyConfiguration::m_initialAngularVelocity,
                    "Initial angular velocity", "Initial angular velocity")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Physics::RigidBodyConfiguration::GetInitialVelocitiesVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RigidBodyConfiguration::m_computeCenterOfMass,
                    "Compute COM", "Whether to automatically compute the center of mass")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Physics::RigidBodyConfiguration::GetInertiaSettingsVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RigidBodyConfiguration::m_centerOfMassOffset,
                    "Center Of Mass Offset", "Center of mass offset in local frame")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &RigidBodyConfiguration::GetCoMVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Physics::RigidBodyConfiguration::m_mass,
                    "Mass", "The mass of the object must be positive. A mass of zero is treated as infinite")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RigidBodyConfiguration::m_computeInertiaTensor,
                    "Compute inertia", "Whether to automatically compute the inertia values based on the mass and shape of the rigid body")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Physics::RigidBodyConfiguration::GetInertiaSettingsVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(Editor::InertiaHandler, &Physics::RigidBodyConfiguration::m_inertiaTensor,
                    "Inertia diagonal", "Inertia diagonal")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Physics::RigidBodyConfiguration::GetInertiaVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Physics::RigidBodyConfiguration::m_linearDamping,
                    "Linear damping", "Linear damping (must be non-negative)")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Physics::RigidBodyConfiguration::GetDampingVisibility)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Physics::RigidBodyConfiguration::m_angularDamping,
                    "Angular damping", "Angular damping (must be non-negative)")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Physics::RigidBodyConfiguration::GetDampingVisibility)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Physics::RigidBodyConfiguration::m_sleepMinEnergy,
                    "Sleep threshold", "Kinetic energy per unit mass below which body goes to sleep (must be non-negative)")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Physics::RigidBodyConfiguration::GetSleepOptionsVisibility)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Physics::RigidBodyConfiguration::m_startAsleep,
                    "Start Asleep", "The rigid body will be asleep when spawned")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Physics::RigidBodyConfiguration::GetSleepOptionsVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Physics::RigidBodyConfiguration::m_interpolateMotion,
                    "Interpolate Motion", "Makes object motion look smoother")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Physics::RigidBodyConfiguration::GetInterpolationVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Physics::RigidBodyConfiguration::m_gravityEnabled,
                    "Gravity Enabled", "Rigid body will be affected by gravity")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Physics::RigidBodyConfiguration::GetGravityVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Physics::RigidBodyConfiguration::m_kinematic,
                    "Kinematic", "Rigid body is kinematic")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Physics::RigidBodyConfiguration::GetKinematicVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Physics::RigidBodyConfiguration::m_ccdEnabled,
                    "CCD Enabled", "Whether continuous collision detection is enabled for this body")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Physics::RigidBodyConfiguration::GetCCDVisibility)
                ;

                editContext->Class<EditorRigidBodyConfiguration>(
                    "PhysX Rigid Body Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorRigidBodyConfiguration::m_centerOfMassDebugDraw,
                    "Debug Draw COM", "Whether to debug draw the center of mass for this body")
                ;
            }
        }
    }

    void EditorRigidBodyComponent::RefreshEditorRigidBody()
    {
        m_editorBody.reset();
        Physics::EditorWorldBus::Broadcast(&Physics::EditorWorldRequests::MarkEditorWorldDirty);
    }

    void EditorRigidBodyComponent::Init()
    {
    }

    void EditorRigidBodyComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        EditorRigidBodyRequestBus::Handler::BusConnect(GetEntityId());
    }

    void EditorRigidBodyComponent::Deactivate()
    {
        EditorRigidBodyRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        m_editorBody.reset();
    }

    void EditorRigidBodyComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorRigidBodyConfiguration::Reflect(context);

        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorRigidBodyComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Field("Configuration", &EditorRigidBodyComponent::m_config)
                ->Version(1)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorRigidBodyComponent>(
                    "PhysX Rigid Body Physics", "The entity behaves as a movable rigid object in PhysX.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/PhysXRigidBody.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/PhysXRigidBody.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-physx-rigid-body-physics.html")
                    ->DataElement(0, &EditorRigidBodyComponent::m_config, "Configuration", "Configuration for rigid body physics.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRigidBodyComponent::RefreshEditorRigidBody)
                ;
            }
        }
    }

    EditorRigidBodyComponent::EditorRigidBodyComponent(const EditorRigidBodyConfiguration& config)
        : m_config(config)
    {
    }

    void EditorRigidBodyComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<RigidBodyComponent>(m_config);
    }

    void EditorRigidBodyComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (!m_editorBody)
        {
            CreateEditorWorldRigidBody();
        }
        else
        {
            if (m_config.m_centerOfMassDebugDraw)
            {
                PhysX::Configuration configuration;
                PhysX::ConfigurationRequestBus::BroadcastResult(configuration, &PhysX::ConfigurationRequests::GetConfiguration);

                debugDisplay.DepthTestOff();
                debugDisplay.SetColor(configuration.m_editorConfiguration.m_centerOfMassDebugColor);
                debugDisplay.DrawBall(m_editorBody->GetCenterOfMassWorld(), configuration.m_editorConfiguration.m_centerOfMassDebugSize);
                debugDisplay.DepthTestOn();
            }
        }
    }

    void EditorRigidBodyComponent::CreateEditorWorldRigidBody()
    {
        AZStd::shared_ptr<Physics::World> editorWorld;
        Physics::EditorWorldBus::BroadcastResult(editorWorld, &Physics::EditorWorldRequests::GetEditorWorld);

        AZ_Assert(editorWorld, "Attempting to create an edit time rigid body without an editor world.");

        if (editorWorld)
        {
            AZ::Transform colliderTransform = GetWorldTM();

            Physics::RigidBodyConfiguration configuration;
            configuration.m_orientation = AZ::Quaternion::CreateFromTransform(colliderTransform);
            configuration.m_position = colliderTransform.GetPosition();
            configuration.m_entityId = GetEntityId();
            configuration.m_debugName = GetEntity()->GetName();
            configuration.m_centerOfMassOffset = m_config.m_centerOfMassOffset;
            configuration.m_computeCenterOfMass = m_config.m_computeCenterOfMass;
            configuration.m_computeInertiaTensor = m_config.m_computeInertiaTensor;
            configuration.m_inertiaTensor = m_config.m_inertiaTensor;
            configuration.m_simulated = false;

            Physics::SystemRequestBus::BroadcastResult(m_editorBody, &Physics::SystemRequests::CreateRigidBody, configuration);
            AZStd::vector<EditorColliderComponent*> colliders = GetEntity()->FindComponents<EditorColliderComponent>();
            for (EditorColliderComponent* collider : colliders)
            {
                AZStd::shared_ptr<Physics::Shape> shape;
                EditorProxyShapeConfig shapeConfigurationProxy = collider->GetShapeConfiguration();
                Physics::ShapeConfiguration& shapeConfiguration = shapeConfigurationProxy.GetCurrent();
                Physics::SystemRequestBus::BroadcastResult(shape, &Physics::SystemRequests::CreateShape, collider->GetColliderConfiguration(), shapeConfiguration);
                m_editorBody->AddShape(shape);
            }

            m_editorBody->UpdateCenterOfMassAndInertia(m_config.m_computeCenterOfMass, m_config.m_centerOfMassOffset, m_config.m_computeInertiaTensor, m_config.m_inertiaTensor);
            editorWorld->AddBody(*m_editorBody);

            Physics::EditorWorldBus::Broadcast(&Physics::EditorWorldRequests::MarkEditorWorldDirty);
        }
    }

    void EditorRigidBodyComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        if (m_editorBody)
        {
            m_editorBody.reset();
            CreateEditorWorldRigidBody();
            Physics::EditorWorldBus::Broadcast(&Physics::EditorWorldRequests::MarkEditorWorldDirty);
        }
    }
} // namespace PhysX
