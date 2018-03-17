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

#include <EditorPhysXRigidBodyComponent.h>
#include <PhysXRigidBodyComponent.h>
#include <AzCore/Serialization/EditContext.h>

namespace PhysX
{
    void EditorPhysXRigidBodyConfiguration::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorPhysXRigidBodyConfiguration, PhysXRigidBodyConfiguration>()
                ->Version(1)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<PhysXRigidBodyConfiguration>(
                    "PhysX Rigid Body Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &PhysXRigidBodyConfiguration::m_motionType, "Motion type", "Motion type")
                    ->EnumAttribute(Physics::MotionType::Dynamic, "Dynamic")
                    ->EnumAttribute(Physics::MotionType::Static, "Static")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PhysXRigidBodyConfiguration::m_initialLinearVelocity, "Initial linear velocity", "Initial linear velocity")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXRigidBodyConfiguration::ShowDynamicOnlyAttributes)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PhysXRigidBodyConfiguration::m_initialAngularVelocity, "Initial angular velocity", "Initial angular velocity")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXRigidBodyConfiguration::ShowDynamicOnlyAttributes)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PhysXRigidBodyConfiguration::m_mass, "Mass", "Mass (>= 0, 0 interpreted as infinite)")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXRigidBodyConfiguration::ShowDynamicOnlyAttributes)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PhysXRigidBodyConfiguration::m_computeInertiaDiagonal, "Compute inertia", "Whether to automatically compute the inertia values based on the mass and shape of the rigid body")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXRigidBodyConfiguration::ShowDynamicOnlyAttributes)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PhysXRigidBodyConfiguration::m_inertiaDiagonal, "Inertia diagonal values", "Diagonal values of the inertia tensor (>= 0, 0 interpreted as infinite)")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXRigidBodyConfiguration::ShowInertia)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PhysXRigidBodyConfiguration::m_centreOfMassOffset, "COM offset", "Center of mass offset in local frame")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXRigidBodyConfiguration::ShowDynamicOnlyAttributes)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PhysXRigidBodyConfiguration::m_linearDamping, "Linear damping", "Linear damping (>= 0)")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXRigidBodyConfiguration::ShowDynamicOnlyAttributes)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PhysXRigidBodyConfiguration::m_angularDamping, "Angular damping", "Angular damping (>= 0)")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXRigidBodyConfiguration::ShowDynamicOnlyAttributes)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PhysXRigidBodyConfiguration::m_sleepMinEnergy, "Sleep threshold", "Kinetic energy per unit mass below which body goes to sleep (>= 0)")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXRigidBodyConfiguration::ShowDynamicOnlyAttributes)
                ;
            }
        }
    }

    void EditorPhysXRigidBodyComponent::Init()
    {
    }

    void EditorPhysXRigidBodyComponent::Activate()
    {
    }

    void EditorPhysXRigidBodyComponent::Deactivate()
    {
    }

    void EditorPhysXRigidBodyComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorPhysXRigidBodyConfiguration::Reflect(context);

        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorPhysXRigidBodyComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Field("Configuration", &EditorPhysXRigidBodyComponent::m_config)
                ->Version(1)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorPhysXRigidBodyComponent>(
                    "PhysX Rigid Body Physics", "The entity behaves as a movable rigid object in PhysX.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/RigidPhysics.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/RigidPhysics.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorPhysXRigidBodyComponent::m_config, "Configuration", "Configuration for rigid body physics.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;
            }
        }
    }

    EditorPhysXRigidBodyComponent::EditorPhysXRigidBodyComponent(const EditorPhysXRigidBodyConfiguration& config)
        : m_config(config)
    {
    }

    void EditorPhysXRigidBodyComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<PhysXRigidBodyComponent>(m_config);
    }
} // namespace PhysX
