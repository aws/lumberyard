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

#include <PhysXCharacters_precompiled.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <API/CharacterController.h>
#include <Components/EditorCharacterControllerComponent.h>
#include <Components/CharacterControllerComponent.h>
#include <LmbrCentral/Geometry/GeometrySystemComponentBus.h>

namespace PhysXCharacters
{
    void EditorProxyShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorProxyShapeConfig>()
                ->Version(1)
                ->Field("ShapeType", &EditorProxyShapeConfig::m_shapeType)
                ->Field("Box", &EditorProxyShapeConfig::m_box)
                ->Field("Capsule", &EditorProxyShapeConfig::m_capsule)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorProxyShapeConfig>(
                    "EditorProxyShapeConfig", "PhysX character controller shape")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorProxyShapeConfig::m_shapeType, "Shape",
                        "The shape associated with the character controller")
                    ->EnumAttribute(Physics::ShapeType::Capsule, "Capsule")
                    ->EnumAttribute(Physics::ShapeType::Box, "Box")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyShapeConfig::m_box, "Box",
                        "Configuration of box shape")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorProxyShapeConfig::IsBoxConfig)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyShapeConfig::m_capsule, "Capsule",
                        "Configuration of capsule shape")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorProxyShapeConfig::IsCapsuleConfig)
                    ;
            }
        }
    }

    bool EditorProxyShapeConfig::IsBoxConfig() const
    {
        return m_shapeType == Physics::ShapeType::Box;
    }

    bool EditorProxyShapeConfig::IsCapsuleConfig() const
    {
        return m_shapeType == Physics::ShapeType::Capsule;
    }

    const Physics::ShapeConfiguration& EditorProxyShapeConfig::GetCurrent() const
    {
        switch (m_shapeType)
        {
        case Physics::ShapeType::Box:
            return m_box;
        case Physics::ShapeType::Capsule:
            return m_capsule;
        default:
            AZ_Warning("EditorProxyShapeConfig", false, "Unsupported shape type.");
            return m_capsule;
        }
    }

    void EditorCharacterControllerComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorProxyShapeConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorCharacterControllerComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("Configuration", &EditorCharacterControllerComponent::m_configuration)
                ->Field("ShapeConfig", &EditorCharacterControllerComponent::m_proxyShapeConfiguration)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorCharacterControllerComponent>(
                    "PhysX Character Controller", "PhysX Character Controller")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/PhysXCharacter.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/PhysXCharacter.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorCharacterControllerComponent::m_configuration,
                        "Configuration", "Configuration for the character controller")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorCharacterControllerComponent::m_proxyShapeConfiguration,
                        "Shape Configuration", "The configuration for the shape associated with the character controller")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void EditorCharacterControllerComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
    }

    void EditorCharacterControllerComponent::Deactivate()
    {
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
    }

    // AzToolsFramework::EntitySelectionEvents
    void EditorCharacterControllerComponent::OnSelected()
    {
        PhysX::ConfigurationNotificationBus::Handler::BusConnect();
    }

    void EditorCharacterControllerComponent::OnDeselected()
    {
        PhysX::ConfigurationNotificationBus::Handler::BusDisconnect();
    }

    // AzFramework::EntityDebugDisplayEventBus
    void EditorCharacterControllerComponent::DisplayEntity(bool& handled)
    {
        if (IsSelected())
        {
            AzFramework::EntityDebugDisplayRequests* displayContext = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
            AZ_Assert(displayContext, "Invalid display context.");

            const AZ::Vector3 upDirectionNormalized = m_configuration.m_upDirection.IsZero()
                ? AZ::Vector3::CreateAxisZ()
                : m_configuration.m_upDirection.GetNormalized();

            // PhysX uses the x-axis as the height direction of the controller, and so takes the shortest arc from the 
            // x-axis to the up direction.  To obtain the same orientation in the LY co-ordinate system (which uses z
            // as the height direction), we need to combine a rotation from the x-axis to the up direction with a 
            // rotation from the z-axis to the x-axis.
            const AZ::Quaternion upDirectionQuat = AZ::Quaternion::CreateShortestArc(AZ::Vector3::CreateAxisX(),
                upDirectionNormalized) * AZ::Quaternion::CreateRotationY(AZ::Constants::HalfPi);

            if (m_proxyShapeConfiguration.IsCapsuleConfig())
            {
                const auto& capsuleConfig = static_cast<const Physics::CapsuleShapeConfiguration&>(m_proxyShapeConfiguration.GetCurrent());
                const float heightOffset = 0.5f * capsuleConfig.m_height + m_configuration.m_contactOffset;
                const AZ::Transform controllerTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
                    upDirectionQuat, GetWorldTM().GetPosition() + heightOffset * upDirectionNormalized);

                displayContext->PushMatrix(controllerTransform);

                // draw the actual shape
                LmbrCentral::CapsuleGeometrySystemRequestBus::Broadcast(
                    &LmbrCentral::CapsuleGeometrySystemRequestBus::Events::GenerateCapsuleMesh,
                    m_configuration.m_scaleCoefficient * capsuleConfig.m_radius,
                    m_configuration.m_scaleCoefficient * capsuleConfig.m_height,
                    16, 8,
                    m_vertexBuffer,
                    m_indexBuffer,
                    m_lineBuffer
                );

                displayContext->SetLineWidth(2.0f);
                displayContext->DrawTrianglesIndexed(m_vertexBuffer, m_indexBuffer, AzFramework::ViewportColors::SelectedColor);
                displayContext->DrawLines(m_lineBuffer, AzFramework::ViewportColors::WireColor);

                // draw the shape inflated by the contact offset
                LmbrCentral::CapsuleGeometrySystemRequestBus::Broadcast(
                    &LmbrCentral::CapsuleGeometrySystemRequestBus::Events::GenerateCapsuleMesh,
                    m_configuration.m_scaleCoefficient * capsuleConfig.m_radius + m_configuration.m_contactOffset,
                    m_configuration.m_scaleCoefficient * capsuleConfig.m_height + 2.0f * m_configuration.m_contactOffset,
                    16, 8,
                    m_vertexBuffer,
                    m_indexBuffer,
                    m_lineBuffer
                );

                displayContext->DrawLines(m_lineBuffer, AzFramework::ViewportColors::WireColor);
                displayContext->PopMatrix();
            }

            if (m_proxyShapeConfiguration.IsBoxConfig())
            {
                const auto& boxConfig = static_cast<const Physics::BoxShapeConfiguration&>(m_proxyShapeConfiguration.GetCurrent());
                const float heightOffset = 0.5f * boxConfig.m_dimensions.GetZ() + m_configuration.m_contactOffset;
                const AZ::Transform controllerTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
                    upDirectionQuat, GetWorldTM().GetPosition() + heightOffset * upDirectionNormalized);

                const AZ::Vector3 boxHalfExtentsScaled = 0.5f * m_configuration.m_scaleCoefficient * boxConfig.m_dimensions;
                const AZ::Vector3 boxHalfExtentsScaledWithContactOffset = boxHalfExtentsScaled +
                    m_configuration.m_contactOffset * AZ::Vector3::CreateOne();

                displayContext->PushMatrix(controllerTransform);

                // draw the actual shape
                displayContext->SetLineWidth(2.0f);
                displayContext->SetColor(AzFramework::ViewportColors::SelectedColor);
                displayContext->DrawSolidBox(-boxHalfExtentsScaled, boxHalfExtentsScaled);
                displayContext->SetColor(AzFramework::ViewportColors::WireColor);
                displayContext->DrawWireBox(-boxHalfExtentsScaled, boxHalfExtentsScaled);

                // draw the shape inflated by the contact offset
                displayContext->DrawWireBox(-boxHalfExtentsScaledWithContactOffset, boxHalfExtentsScaledWithContactOffset);
                displayContext->PopMatrix();
            }

            handled = true;
        }
    }

    // PhysX::ConfigurationNotificationBus
    void EditorCharacterControllerComponent::OnConfigurationRefreshed(const PhysX::Configuration& configuration)
    {
        AZ_UNUSED(configuration);
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
            AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
    }

    // EditorComponentBase
    void EditorCharacterControllerComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        switch (m_proxyShapeConfiguration.m_shapeType)
        {
        case Physics::ShapeType::Box:
            gameEntity->CreateComponent<CharacterControllerComponent>(
                AZStd::make_unique<CharacterControllerConfiguration>(m_configuration),
                AZStd::make_unique<Physics::BoxShapeConfiguration>(m_proxyShapeConfiguration.m_box));
            break;
        case Physics::ShapeType::Capsule:
            gameEntity->CreateComponent<CharacterControllerComponent>(
                AZStd::make_unique<CharacterControllerConfiguration>(m_configuration),
                AZStd::make_unique<Physics::CapsuleShapeConfiguration>(m_proxyShapeConfiguration.m_capsule));
            break;
        }
    }
} // namespace PhysXCharacters 
