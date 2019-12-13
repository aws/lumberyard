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
#include <AzFramework/Physics/SystemBus.h>
#include <LyViewPaneNames.h>
#include <EditorShapeColliderComponent.h>
#include <ShapeColliderComponent.h>
#include <EditorRigidBodyComponent.h>
#include <PhysX/ColliderComponentBus.h>
#include <Editor/ConfigurationWindowBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <AzCore/std/algorithm.h>

namespace PhysX
{
    EditorShapeColliderComponent::EditorShapeColliderComponent()
    {
        m_colliderConfig.SetPropertyVisibility(Physics::ColliderConfiguration::Offset, false);
    }

    void EditorShapeColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorShapeColliderComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("ColliderConfiguration", &EditorShapeColliderComponent::m_colliderConfig)
                ->Field("DebugDrawSettings", &EditorShapeColliderComponent::m_colliderDebugDraw)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorShapeColliderComponent>(
                    "PhysX Shape Collider", "Creates geometry in the PhysX simulation based on an attached shape component")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/PhysXCollider.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/PhysXCollider.svg")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorShapeColliderComponent::m_colliderConfig,
                        "Collider configuration", "Configuration of the collider")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorShapeColliderComponent::OnConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorShapeColliderComponent::m_colliderDebugDraw,
                        "Debug draw settings", "Debug draw settings")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void EditorShapeColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
        provided.push_back(AZ_CRC("PhysXTriggerService", 0x3a117d7b));
    }

    void EditorShapeColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        required.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
    }

    void EditorShapeColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        // Not compatible with Legacy Cry Physics services
        incompatible.push_back(AZ_CRC("ColliderService", 0x902d4e93));
        incompatible.push_back(AZ_CRC("LegacyCryPhysicsService", 0xbb370351));
    }

    const Physics::ColliderConfiguration& EditorShapeColliderComponent::GetColliderConfiguration() const
    {
        return m_colliderConfig;
    }

    void EditorShapeColliderComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto shapeColliderComponent = gameEntity->CreateComponent<ShapeColliderComponent>();
        shapeColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(
            AZStd::make_shared<Physics::ColliderConfiguration>(m_colliderConfig),
            Utils::CreateScaledShapeConfig(GetEntityId())
        ) });
    }

    void EditorShapeColliderComponent::CreateStaticEditorCollider()
    {
        // We're ignoring dynamic bodies in the editor for now
        auto rigidBody = GetEntity()->FindComponent<PhysX::EditorRigidBodyComponent>();
        if (rigidBody)
        {
            return;
        }

        AZStd::shared_ptr<Physics::World> editorWorld;
        Physics::EditorWorldBus::BroadcastResult(editorWorld, &Physics::EditorWorldRequests::GetEditorWorld);
        if (editorWorld)
        {
            AZ::Transform colliderTransform = GetWorldTM();
            colliderTransform.ExtractScale();

            Physics::WorldBodyConfiguration configuration;
            configuration.m_orientation = AZ::Quaternion::CreateFromTransform(colliderTransform);
            configuration.m_position = colliderTransform.GetPosition();
            configuration.m_entityId = GetEntityId();
            configuration.m_debugName = GetEntity()->GetName();

            Physics::SystemRequestBus::BroadcastResult(m_editorBody, &Physics::SystemRequests::CreateStaticRigidBody,
                configuration);

            AZStd::shared_ptr<Physics::ShapeConfiguration> scaledShapeConfig = Utils::CreateScaledShapeConfig(GetEntityId());
            if (scaledShapeConfig)
            {
                AZStd::shared_ptr<Physics::Shape> shape;
                Physics::SystemRequestBus::BroadcastResult(shape, &Physics::SystemRequests::CreateShape, m_colliderConfig,
                    *scaledShapeConfig);
                m_editorBody->AddShape(shape);
                editorWorld->AddBody(*m_editorBody);

                Physics::EditorWorldBus::Broadcast(&Physics::EditorWorldRequests::MarkEditorWorldDirty);
            }
        }
    }

    AZ::u32 EditorShapeColliderComponent::OnConfigurationChanged()
    {
        m_colliderConfig.m_materialSelection.SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
        CreateStaticEditorCollider();
        ColliderComponentEventBus::Event(GetEntityId(), &ColliderComponentEvents::OnColliderChanged);
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    void EditorShapeColliderComponent::CheckSupportedShapeTypes()
    {
        if (m_shapeTypeWarningIssued)
        {
            return;
        }

        AZ::Crc32 shapeType;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(shapeType, GetEntityId(), &LmbrCentral::ShapeComponentRequests::GetShapeType);

        if (shapeType != AZ::Crc32())
        {
            const AZStd::vector<AZ::Crc32> supportedShapeTypes = {
                ShapeConstants::Box,
                ShapeConstants::Capsule,
                ShapeConstants::Sphere
            };

            if (AZStd::find(supportedShapeTypes.begin(), supportedShapeTypes.end(), shapeType) != supportedShapeTypes.end())
            {
                return;
            }

            AZ_Warning("PhysX Shape Collider Component", false, "Unsupported shape type for entity \"%s\". "
                "The following shapes are currently supported - box, capsule, sphere.", GetEntity()->GetName().c_str());
            m_shapeTypeWarningIssued = true;
        }
    }

    // AZ::Component
    void EditorShapeColliderComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        CheckSupportedShapeTypes();

        // Debug drawing
        m_colliderDebugDraw.Connect(GetEntityId());
        m_colliderDebugDraw.SetDisplayCallback(this);

        m_scale = AZ::Vector3(GetTransform()->GetLocalScale().GetMaxElement());
        CreateStaticEditorCollider();

        ColliderComponentEventBus::Event(GetEntityId(), &ColliderComponentEvents::OnColliderChanged);
    }

    void EditorShapeColliderComponent::Deactivate()
    {
        m_colliderDebugDraw.Disconnect();

        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        m_editorBody = nullptr;
        Physics::EditorWorldBus::Broadcast(&Physics::EditorWorldRequests::MarkEditorWorldDirty);
    }

    // AzToolsFramework::EntitySelectionEvents
    void EditorShapeColliderComponent::OnSelected()
    {
        PhysX::ConfigurationNotificationBus::Handler::BusConnect();
    }

    void EditorShapeColliderComponent::OnDeselected()
    {
        PhysX::ConfigurationNotificationBus::Handler::BusDisconnect();
    }

    // TransformBus
    void EditorShapeColliderComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        m_scale = AZ::Vector3(GetTransform()->GetLocalScale().GetMaxElement());
        CreateStaticEditorCollider();
        ColliderComponentEventBus::Event(GetEntityId(), &ColliderComponentEvents::OnColliderChanged);
    }

    // PhysX::ConfigurationNotificationBus
    void EditorShapeColliderComponent::OnConfigurationRefreshed(const Configuration& /*configuration*/)
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
            AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
    }

    void EditorShapeColliderComponent::OnDefaultMaterialLibraryChanged(const AZ::Data::AssetId& defaultMaterialLibrary)
    {
        m_colliderConfig.m_materialSelection.OnDefaultMaterialLibraryChanged(defaultMaterialLibrary);
        ColliderComponentEventBus::Event(GetEntityId(), &ColliderComponentEvents::OnColliderChanged);
    }

    // LmbrCentral::ShapeComponentNotificationBus
    void EditorShapeColliderComponent::OnShapeChanged(LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons changeReason)
    {
        CheckSupportedShapeTypes();
        CreateStaticEditorCollider();
        ColliderComponentEventBus::Event(GetEntityId(), &ColliderComponentEvents::OnColliderChanged);
    }

    // DisplayCallback
    void EditorShapeColliderComponent::Display(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        AZ::Crc32 shapeType;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(shapeType, GetEntityId(), &LmbrCentral::ShapeComponentRequests::GetShapeType);

        if (shapeType == ShapeConstants::Box)
        {
            AZ::Vector3 boxDimensions = AZ::Vector3::CreateZero();
            LmbrCentral::BoxShapeComponentRequestsBus::EventResult(boxDimensions, GetEntityId(),
                &LmbrCentral::BoxShapeComponentRequests::GetBoxDimensions);
            Physics::BoxShapeConfiguration boxShapeConfig(boxDimensions);
            m_colliderDebugDraw.DrawBox(debugDisplay, m_colliderConfig, boxShapeConfig, m_scale);
        }

        else if (shapeType == ShapeConstants::Capsule)
        {
            LmbrCentral::CapsuleShapeConfiguration lmbrCentralCapsuleShapeConfig;
            LmbrCentral::CapsuleShapeComponentRequestsBus::EventResult(lmbrCentralCapsuleShapeConfig, GetEntityId(),
                &LmbrCentral::CapsuleShapeComponentRequests::GetCapsuleConfiguration);
            m_colliderDebugDraw.DrawCapsule(debugDisplay, m_colliderConfig,
                Utils::ConvertFromLmbrCentralCapsuleConfig(lmbrCentralCapsuleShapeConfig), m_scale);
        }

        else if (shapeType == ShapeConstants::Sphere)
        {
            float radius = 0.0f;
            LmbrCentral::SphereShapeComponentRequestsBus::EventResult(radius, GetEntityId(),
                &LmbrCentral::SphereShapeComponentRequests::GetRadius);
            m_colliderDebugDraw.DrawSphere(debugDisplay, m_colliderConfig, Physics::SphereShapeConfiguration(radius), m_scale);
        }
    }
} // namespace PhysX
