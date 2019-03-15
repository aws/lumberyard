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

#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/PropertyTypes.h>

namespace Physics
{
    void ShapeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ShapeConfiguration>()
                ->Version(1)
                ->Field("Scale", &ShapeConfiguration::m_scale)
                ;
        }
    }

    void SphereShapeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SphereShapeConfiguration, ShapeConfiguration>()
                ->Version(1)
                ->Field("Radius", &SphereShapeConfiguration::m_radius)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SphereShapeConfiguration>("SphereShapeConfiguration", "Configuration for sphere collider")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SphereShapeConfiguration::m_radius, "Radius", "The radius of the sphere collider")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ;
            }
        }
    }

    SphereShapeConfiguration::SphereShapeConfiguration(float radius)
        : m_radius(radius)
    {
    }

    void BoxShapeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BoxShapeConfiguration, ShapeConfiguration>()
                ->Version(1)
                ->Field("Configuration", &BoxShapeConfiguration::m_dimensions)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BoxShapeConfiguration>("BoxShapeConfiguration", "Configuration for box collider")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BoxShapeConfiguration::m_dimensions, "Dimensions", "Lengths of the box sides")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ;
            }
        }
    }

    BoxShapeConfiguration::BoxShapeConfiguration(const AZ::Vector3& boxDimensions)
        : m_dimensions(boxDimensions)
    {
    }

    void CapsuleShapeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CapsuleShapeConfiguration, ShapeConfiguration>()
                ->Version(1)
                ->Field("Height", &CapsuleShapeConfiguration::m_height)
                ->Field("Radius", &CapsuleShapeConfiguration::m_radius)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CapsuleShapeConfiguration>("CapsuleShapeConfiguration", "Configuration for capsule collider")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CapsuleShapeConfiguration::m_height, "Height", "The height of the capsule, including caps at each end")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CapsuleShapeConfiguration::OnHeightChanged)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CapsuleShapeConfiguration::m_radius, "Radius", "The radius of the capsule")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CapsuleShapeConfiguration::OnRadiusChanged)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ;
            }
        }
    }

    CapsuleShapeConfiguration::CapsuleShapeConfiguration(float height, float radius)
        : m_height(height)
        , m_radius(radius)
    {
    }

    void CapsuleShapeConfiguration::OnHeightChanged()
    {
        // check that the height is greater than twice the radius
        m_height = AZ::GetMax(m_height, 2 * m_radius + static_cast<float>(AZ::g_fltEps));
    }

    void CapsuleShapeConfiguration::OnRadiusChanged()
    {
        // check that the radius is less than half the height
        m_radius = AZ::GetMin(m_radius, (0.5f - static_cast<float>(AZ::g_fltEps)) * m_height);
    }

    void PhysicsAssetShapeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysicsAssetShapeConfiguration, ShapeConfiguration>()
                ->Version(1)
                ->Field("PhysicsAsset", &PhysicsAssetShapeConfiguration::m_asset)
                ->Field("AssetScale", &PhysicsAssetShapeConfiguration::m_assetScale)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PhysicsAssetShapeConfiguration>("PhysicsAssetShapeConfiguration", "Configuration for asset shape collider")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PhysicsAssetShapeConfiguration::m_assetScale, "AssetScale", "The scale of the asset shape")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ;
            }
        }
    }

    Physics::ShapeType PhysicsAssetShapeConfiguration::GetShapeType() const
    {
        return ShapeType::PhysicsAsset;
    }

    void NativeShapeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<NativeShapeConfiguration, ShapeConfiguration>()
                ->Version(1)
                ->Field("Scale", &NativeShapeConfiguration::m_nativeShapeScale)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<NativeShapeConfiguration>("NativeShapeConfiguration", "Configuration for native shape collider")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &NativeShapeConfiguration::m_nativeShapeScale, "Scale", "The scale of the native shape")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ;
            }
        }
    }
}