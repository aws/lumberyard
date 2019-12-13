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
#include <ShapeColliderComponent.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>

namespace PhysX
{
    namespace Utils
    {
        Physics::CapsuleShapeConfiguration ConvertFromLmbrCentralCapsuleConfig(
            const LmbrCentral::CapsuleShapeConfig& inputCapsuleConfig)
        {
            return Physics::CapsuleShapeConfiguration(inputCapsuleConfig.m_height, inputCapsuleConfig.m_radius);
        }

        AZStd::shared_ptr<Physics::ShapeConfiguration> CreateScaledShapeConfig(AZ::EntityId entityId)
        {
            AZ::Crc32 shapeType;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(shapeType, entityId,
                &LmbrCentral::ShapeComponentRequests::GetShapeType);

            // all currently supported shape types scale uniformly based on the largest element of the non-uniform scale
            AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
            AZ::TransformBus::EventResult(nonUniformScale, entityId, &AZ::TransformBus::Events::GetLocalScale);
            AZ::Vector3 uniformScale = AZ::Vector3(nonUniformScale.GetMaxElement());

            if (shapeType == ShapeConstants::Box)
            {
                AZ::Vector3 boxDimensions = AZ::Vector3::CreateZero();
                LmbrCentral::BoxShapeComponentRequestsBus::EventResult(boxDimensions, entityId,
                    &LmbrCentral::BoxShapeComponentRequests::GetBoxDimensions);
                auto boxShapeConfig = AZStd::make_shared<Physics::BoxShapeConfiguration>(boxDimensions);
                boxShapeConfig->m_scale = uniformScale;
                return boxShapeConfig;
            }

            else if (shapeType == ShapeConstants::Capsule)
            {
                LmbrCentral::CapsuleShapeConfig lmbrCentralCapsuleShapeConfig;
                LmbrCentral::CapsuleShapeComponentRequestsBus::EventResult(lmbrCentralCapsuleShapeConfig, entityId,
                    &LmbrCentral::CapsuleShapeComponentRequests::GetCapsuleConfiguration);
                Physics::CapsuleShapeConfiguration capsuleShapeConfig =
                    Utils::ConvertFromLmbrCentralCapsuleConfig(lmbrCentralCapsuleShapeConfig);
                capsuleShapeConfig.m_scale = uniformScale;
                return AZStd::make_shared<Physics::CapsuleShapeConfiguration>(capsuleShapeConfig);
            }

            else if (shapeType == ShapeConstants::Sphere)
            {
                float radius = 0.0f;
                LmbrCentral::SphereShapeComponentRequestsBus::EventResult(radius, entityId,
                    &LmbrCentral::SphereShapeComponentRequests::GetRadius);
                auto sphereShapeConfig = AZStd::make_shared<Physics::SphereShapeConfiguration>(radius);
                sphereShapeConfig->m_scale = uniformScale;
                return sphereShapeConfig;
            }

            return nullptr;
        }
    } // namespace Utils

    void ShapeColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ShapeColliderComponent, BaseColliderComponent>()
                ->Version(1)
                ;
        }
    }

    // BaseColliderComponent
    void ShapeColliderComponent::UpdateScaleForShapeConfigs()
    {
        if (m_shapeConfigList.size() != 1)
        {
            AZ_Error("PhysX Shape Collider Component", false,
                "Expected exactly one collider/shape configuration for entity \"%s\".", GetEntity()->GetName().c_str());
            return;
        }

        if (m_shapeConfigList[0].second)
        {
            // all currently supported shape types scale uniformly based on the largest element of the non-uniform scale
            AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
            AZ::TransformBus::EventResult(nonUniformScale, GetEntityId(), &AZ::TransformBus::Events::GetLocalScale);
            AZ::Vector3 uniformScale = AZ::Vector3(nonUniformScale.GetMaxElement());
            m_shapeConfigList[0].second->m_scale = uniformScale;
        }
    }
} // namespace PhysX
