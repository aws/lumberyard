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

#include <AzCore/Component/TransformBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include "EditorConstraintComponent.h"


namespace LmbrCentral
{
    void EditorConstraintConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorConstraintConfiguration, ConstraintConfiguration>()->
                Version(1)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<ConstraintConfiguration>("Configuration", "Constraint configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    // Constraint type
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ConstraintConfiguration::m_constraintType, "Constraint type", "The type of constraint.")
                        ->EnumAttribute(ConstraintType::Hinge, "Hinge")
                        ->EnumAttribute(ConstraintType::Ball, "Ball")
                        ->EnumAttribute(ConstraintType::Slider, "Slider")
                        ->EnumAttribute(ConstraintType::Plane, "Plane")
                        ->EnumAttribute(ConstraintType::Magnet, "Magnet")
                        ->EnumAttribute(ConstraintType::Fixed, "Fixed")
                        ->EnumAttribute(ConstraintType::Free, "Free")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ConstraintConfiguration::OnConstraintTypeChanged)

                    // Owner type
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ConstraintConfiguration::m_ownerType, "Owning entity", "Constrain self or another selected entity.")
                        ->EnumAttribute(OwnerType::Self, "Self")
                        ->EnumAttribute(OwnerType::OtherEntity, "Other entity")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ConstraintConfiguration::OnOwnerTypeChanged)

                    // Target type
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ConstraintConfiguration::m_targetType, "Target type", "Chose to constrain the owning entity to another entity (\"target\") or the world position of the constraint entity.")
                        ->EnumAttribute(TargetType::Entity, "Entity")
                        ->EnumAttribute(TargetType::World, "World space")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ConstraintConfiguration::OnTargetTypeChanged)

                    // Owner and target
                    ->DataElement(0, &ConstraintConfiguration::m_owningEntity, "Constrained entity", "The entity being constrained")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ConstraintConfiguration::GetOwningEntityVisibility)
                        ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC("PhysicsService", 0xa7350d22))
                    ->DataElement(0, &ConstraintConfiguration::m_targetEntity, "Constraint target", "An entity to constrain to (none if constrained to world space)")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ConstraintConfiguration::GetTargetEntityVisibility)
                        ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC("PhysicsService", 0xa7350d22))

                    // Axis
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ConstraintConfiguration::m_axis, "Axis", "The orientation of the constraint")
                        ->EnumAttribute(Axis::Z, "Z")
                        ->EnumAttribute(Axis::NegativeZ, "Negative Z")
                        ->EnumAttribute(Axis::Y, "Y")
                        ->EnumAttribute(Axis::NegativeY, "Negative Y")
                        ->EnumAttribute(Axis::X, "X")
                        ->EnumAttribute(Axis::NegativeX, "Negative X")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ConstraintConfiguration::GetAxisVisibility)

                    // Enable on activate
                    ->DataElement(0, &ConstraintConfiguration::m_enableOnActivate, "Enabled on activate", "Is this constraint enabled when it is activated?")

                    // Part Id
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Part ids")
                        ->DataElement(0, &ConstraintConfiguration::m_enableConstrainToPartId, "Enable", "Specify the animation partId(s) constrained to")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ConstraintConfiguration::OnPropertyChanged)
                        ->DataElement(0, &ConstraintConfiguration::m_ownerPartId, "Owner part id", "Part id on the owning entity to constrain to")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &ConstraintConfiguration::GetPartIdVisibility)
                        ->DataElement(0, &ConstraintConfiguration::m_targetPartId, "Target part id", "Part id on the target entity to constrain to (disabled if world space)")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &ConstraintConfiguration::GetPartIdVisibility)

                    // Force limits
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Force limits")
                        ->DataElement(0, &ConstraintConfiguration::m_enableForceLimits, "Enable", "Specify force and torque limits for this constraint?")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ConstraintConfiguration::OnPropertyChanged)
                        ->DataElement(0, &ConstraintConfiguration::m_maxPullForce, "Max pull force", "Max force before constraint is broken")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &ConstraintConfiguration::GetForceLimitVisibility)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " N")
                        ->DataElement(0, &ConstraintConfiguration::m_maxBendTorque, "Max bend torque", "Max bend torque before constraint is broken")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &ConstraintConfiguration::GetForceLimitVisibility)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " NM")

                    // Limits
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Movement limits")
                        ->DataElement(0, &ConstraintConfiguration::m_enableRotationLimits, "Enable", "Specify rotation limits per axis")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &ConstraintConfiguration::GetRotationLimitGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ConstraintConfiguration::OnPropertyChanged)
                        ->DataElement(0, &ConstraintConfiguration::m_xmin, "Min", "If less than max, the constraint will only move/rotate on the given axis between the specified min and max values")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &ConstraintConfiguration::GetRotationLimitVisibilityX)
                            ->Attribute(AZ::Edit::Attributes::Min, -360.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, &ConstraintConfiguration::GetXLimitUnits)
                        ->DataElement(0, &ConstraintConfiguration::m_xmax, "Max", "If greater than min, the constraint will only move/rotate on the given axis between the specified min and max values")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &ConstraintConfiguration::GetRotationLimitVisibilityX)
                            ->Attribute(AZ::Edit::Attributes::Min, -360.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, &ConstraintConfiguration::GetXLimitUnits)
                        ->DataElement(0, &ConstraintConfiguration::m_yzmax, "Half angle", "Rotation cone half angle which limits rotation to a cone centered about the selected axis ")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &ConstraintConfiguration::GetRotationLimitVisibilityYZ)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 90.0f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " deg")

                    // Search Radius
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Search radius")
                        ->DataElement(0, &ConstraintConfiguration::m_enableSearchRadius, "Enable", "Specify a search radius for the constraint")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &ConstraintConfiguration::GetSearchRadiusGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ConstraintConfiguration::OnPropertyChanged)
                        ->DataElement(0, &ConstraintConfiguration::m_searchRadius, "Search radius", "Radius of the spherical area to search for attachable objects")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &ConstraintConfiguration::GetSearchRadiusVisibility)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " m")

                    // Damping
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Damping")
                        ->DataElement(0, &ConstraintConfiguration::m_enableDamping, "Enable", "Specify a damping override")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ConstraintConfiguration::OnPropertyChanged)
                        ->DataElement(0, &ConstraintConfiguration::m_damping, "Damping", "Value typically between 0 (no additional damping) and 0.5 (visually overdamped) - use for objects that have trouble coming to rest")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &ConstraintConfiguration::GetDampingVisibility)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    // Other settings
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Additional settings")
                        ->DataElement(0, &ConstraintConfiguration::m_enableTargetCollisions, "Enable collision", "Enable collision checks between constrained objects")
                        ->DataElement(0, &ConstraintConfiguration::m_enableRelativeRotation, "Enable rotation", "Disable to fully constrain all rotation")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &ConstraintConfiguration::GetEnableRotationVisibility)
                        ->DataElement(0, &ConstraintConfiguration::m_enforceOnFastObjects, "Fast objects", "Constraint will be enforced on fast moving objects")
                        ->DataElement(0, &ConstraintConfiguration::m_breakable, "Breakable", "Constraint will be deleted when the force limit is reached")
                ;
            }
        }
    }

    void EditorConstraintComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorConstraintConfiguration::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorConstraintComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("Config", &EditorConstraintComponent::m_config)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorConstraintComponent>("Constraint", "The Constraint component creates a physical limitation or restriction between an entity and its target")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Physics")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/PhysicsConstraint.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/PhysicsConstraint.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))  
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-constraint.html")

                    ->DataElement(0, &EditorConstraintComponent::m_config, "Settings", "Constraint configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;
            }
        }
    }

    void EditorConstraintComponent::Init()
    {
        Base::Init();

        m_config.Init(GetEntityId());
    }

    void EditorConstraintComponent::Activate()
    {
        Base::Activate();

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorConstraintComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();

        Base::Deactivate();
    }

    void EditorConstraintComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        ConstraintComponent* constraintComponent = gameEntity->CreateComponent<ConstraintComponent>();

        if (constraintComponent)
        {
            constraintComponent->SetConfiguration(m_config);
        }
    }

    void EditorConstraintComponent::DisplayEntity(bool& handled)
    {
        handled = true;

        // Don't draw extra visualization unless selected.
        if (!IsSelected())
        {
            return;
        }

        auto* displayContext = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        AZ_Assert(displayContext, "Invalid display context.");

        const float sphereRadius = 0.5f;
        const float ballAlpha = 0.5f;
        const float lineAlpha = 1.0f;
        const AZ::Vector4 lineWhite(1.0f, 1.0f, 1.0f, lineAlpha);
        const AZ::Vector4 lineCyan(0.0f, 1.0f, 1.0f, lineAlpha);
        const AZ::Vector4 coneCyan(0.0f, 1.0f, 1.0f, 0.25f);
        const AZ::Vector4 ballGreen(0.0f, 1.0f, 0.0f, ballAlpha);
        const AZ::Vector4 ballBlue(0.0f, 0.0f, 1.0f, ballAlpha);
        const AZ::Vector4 ballYellow(1.0f, 1.0f, 0.0f, ballAlpha);

        const AZ::EntityId selfId = GetEntityId();
        AZ::Transform pivotTransform = AZ::Transform::Identity();
        EBUS_EVENT_ID_RESULT(pivotTransform, selfId, AZ::TransformBus, GetWorldTM);
        const AZ::Vector3 pivotWorldPos = pivotTransform.GetTranslation();

        // Draw owner-to-pivot
        if ((selfId != m_config.m_owningEntity) && m_config.m_owningEntity.IsValid())
        {
            AZ::Transform ownerTransform = AZ::Transform::Identity();
            EBUS_EVENT_ID_RESULT(ownerTransform, m_config.m_owningEntity, AZ::TransformBus, GetWorldTM);
            const AZ::Vector3 ownerWorldPos = ownerTransform.GetTranslation();

            // owner to pivot
            displayContext->SetColor(lineWhite);
            displayContext->DrawLine(ownerWorldPos, pivotWorldPos);

            // owner position
            displayContext->SetColor(ballGreen);
            displayContext->DrawBall(ownerWorldPos, sphereRadius);
        }

        // pivot position
        displayContext->SetColor(ballYellow);
        displayContext->DrawBall(pivotWorldPos, sphereRadius);

        // Draw target-to-pivot
        if (m_config.m_targetEntity.IsValid())
        {
            AZ::Transform targetTransform = AZ::Transform::Identity();
            EBUS_EVENT_ID_RESULT(targetTransform, m_config.m_targetEntity, AZ::TransformBus, GetWorldTM);
            const AZ::Vector3 targetWorldPos = targetTransform.GetTranslation();

            // target to pivot
            displayContext->SetColor(lineWhite);
            displayContext->DrawLine(targetWorldPos, pivotWorldPos);

            // pivot position
            displayContext->SetColor(ballBlue);
            displayContext->DrawBall(targetWorldPos, sphereRadius);
        }

        // Axis
        AZ::Vector3 defaultAxis;
        AZ::Quaternion worldFrame;
        if (m_config.m_constraintType == ConstraintConfiguration::ConstraintType::Ball)
        {
            // Ball socket always uses the model Z of the constraint owner
            defaultAxis = AZ::Vector3::CreateAxisZ(1.0f);

            AZ::Transform ownerTransform;
            if (m_config.m_owningEntity == selfId)
            {
                ownerTransform = pivotTransform;
            }
            else
            {
                ownerTransform = AZ::Transform::Identity();
                EBUS_EVENT_ID_RESULT(ownerTransform, m_config.m_owningEntity, AZ::TransformBus, GetWorldTM);
            }
            ownerTransform.ExtractScale();

            worldFrame = AZ::Quaternion::CreateFromTransform(ownerTransform);
        }
        else
        {
            defaultAxis = AZ::Vector3::CreateAxisX(1.0f); // all other constraint types use X as the default axis
            worldFrame = ConstraintConfiguration::GetWorldFrame(m_config.m_axis, pivotTransform);
        }

        const AZ::Vector3 axisVector = worldFrame * defaultAxis;

        displayContext->SetColor(lineCyan);
        displayContext->DrawArrow(pivotWorldPos, pivotWorldPos + (10.0f * axisVector));

        if (m_config.m_constraintType == ConstraintConfiguration::ConstraintType::Ball)
        {
            const float height = 7.5f;
            const float radius = height * tanf(AZ::DegToRad(m_config.m_yzmax)); // r = h tan(x)
            displayContext->SetColor(coneCyan);
            displayContext->DrawCone(pivotWorldPos + (axisVector * height), -axisVector, radius, height);
        }
    }

    void EditorConstraintConfiguration::Init(const AZ::EntityId& self)
    {
        m_self = self;

        if (m_ownerType == OwnerType::Self)
        {
            // Required on first-init for new Editor instances as we do not have the value of self until Init
            m_owningEntity = self;
        }
    }

    bool EditorConstraintConfiguration::GetOwningEntityVisibility() const
    {
        return (m_ownerType == OwnerType::OtherEntity);
    }

    bool EditorConstraintConfiguration::GetTargetEntityVisibility() const
    {
        return (m_targetType == TargetType::Entity);
    }

    bool EditorConstraintConfiguration::GetEnableRotationVisibility() const
    {
        return (m_constraintType == ConstraintType::Magnet || m_constraintType == ConstraintType::Slider || m_constraintType == ConstraintType::Plane);
    }

    bool EditorConstraintConfiguration::GetAxisVisibility() const
    {
        return (m_constraintType != ConstraintType::Ball && m_constraintType != ConstraintType::Magnet && m_constraintType != ConstraintType::Fixed && m_constraintType != ConstraintType::Free);
    }

    bool EditorConstraintConfiguration::GetPartIdVisibility() const
    {
        return m_enableConstrainToPartId;
    }

    bool EditorConstraintConfiguration::GetForceLimitVisibility() const
    {
        return m_enableForceLimits;
    }

    bool EditorConstraintConfiguration::GetRotationLimitGroupVisibility() const
    {
        return (m_constraintType == ConstraintType::Hinge || m_constraintType == ConstraintType::Slider || m_constraintType == ConstraintType::Ball);
    }

    bool EditorConstraintConfiguration::GetRotationLimitVisibilityX() const
    {
        return (m_enableRotationLimits && (m_constraintType == ConstraintType::Hinge || m_constraintType == ConstraintType::Slider));
    }

    bool EditorConstraintConfiguration::GetRotationLimitVisibilityYZ() const
    {
        return (m_enableRotationLimits && (m_constraintType == ConstraintType::Ball));
    }

    bool EditorConstraintConfiguration::GetSearchRadiusGroupVisibility() const
    {
        return (m_constraintType == ConstraintType::Magnet || m_constraintType == ConstraintType::Fixed);
    }

    bool EditorConstraintConfiguration::GetSearchRadiusVisibility() const
    {
        return (m_enableSearchRadius && EditorConstraintConfiguration::GetSearchRadiusGroupVisibility());
    }

    bool EditorConstraintConfiguration::GetDampingVisibility() const
    {
        return m_enableDamping;
    }

    AZ::Crc32 EditorConstraintConfiguration::OnConstraintTypeChanged()
    {
        // Clear state that becomes hidden to the user (and is invalid in another state) when switching to types
        switch (m_constraintType)
        {
            case ConstraintConfiguration::ConstraintType::Hinge :
            {
                m_enableSearchRadius = false;
                m_enableRelativeRotation = true;
                break;
            }
            case ConstraintConfiguration::ConstraintType::Ball :
            {
                m_enableSearchRadius = false;
                m_enableRelativeRotation = true;
                break;
            }
            case ConstraintConfiguration::ConstraintType::Slider :
            {
                m_enableSearchRadius = false;
                m_enableRelativeRotation = true;
                break;
            }
            default :
            {
                break;
            }
        };

        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::Crc32 EditorConstraintConfiguration::OnTargetTypeChanged()
    {
        m_targetEntity = AZ::EntityId();

        // This entity cannot own the constraint if constraining to world, as we use this entities location as the world constraint location
        if ((m_targetType == TargetType::World) && (m_owningEntity == m_self))
        {
            m_ownerType = OwnerType::OtherEntity;
            m_owningEntity = AZ::EntityId();
        }

        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::Crc32 EditorConstraintConfiguration::OnOwnerTypeChanged()
    {
        if (m_ownerType == OwnerType::Self)
        {
            m_owningEntity = m_self;

            // This entity cannot own the constraint if constraining to world, as we use this entities location as the world constraint location
            if (m_targetType == TargetType::World)
            {
                m_targetType = TargetType::Entity;
                AZ_Assert(!m_targetEntity.IsValid(), "No target entity should be assigned if target type is a world space position.");
            }
        }
        else
        {
            AZ_Assert(m_ownerType == OwnerType::OtherEntity, "OtherEntity expected, did you add a new entry to OwnerType?");
            m_owningEntity = AZ::EntityId();
        }

        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::Crc32 EditorConstraintConfiguration::OnPropertyChanged() const
    {
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    const char * EditorConstraintConfiguration::GetXLimitUnits() const
    {
        return (m_constraintType == ConstraintType::Slider) ? " m" : " deg";
    }
} // namespace LmbrCentral
