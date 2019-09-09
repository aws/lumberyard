
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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Source/BoxColliderComponent.h>

namespace PhysX
{
    void BoxColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BoxColliderComponent, BaseColliderComponent>()
                ->Version(1)
                ->Field("Dimensions", &BoxColliderComponent::m_shapeConfiguration)
                ;
        }
    }

    BoxColliderComponent::BoxColliderComponent(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::BoxShapeConfiguration& configuration)
        : BaseColliderComponent(colliderConfiguration)
        , m_shapeConfiguration(configuration)
    {
    }

    AZStd::shared_ptr<Physics::ShapeConfiguration> BoxColliderComponent::CreateScaledShapeConfig()
    {
        auto scaledBoxShape = AZStd::make_shared<Physics::BoxShapeConfiguration>(m_shapeConfiguration);
        scaledBoxShape->m_scale = GetNonUniformScale();
        return scaledBoxShape;
    }
}
