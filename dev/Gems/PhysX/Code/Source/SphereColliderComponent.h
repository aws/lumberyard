
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

#pragma once

#include <BaseColliderComponent.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector3.h>

namespace PhysX
{
    /// Component that provides sphere shape collider.
    /// Used in conjunction with a PhysX Rigid Body Component.
    class SphereColliderComponent
        : public BaseColliderComponent
    {
    public:
        using Configuration = Physics::SphereShapeConfiguration;
        AZ_COMPONENT(SphereColliderComponent, "{108CD341-E5C3-4AE1-B712-21E81ED6C277}", BaseColliderComponent);
        static void Reflect(AZ::ReflectContext* context);

        SphereColliderComponent() = default;
        explicit SphereColliderComponent(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::SphereShapeConfiguration& configuration);

        // ColliderComponentRequestBus
        AZStd::shared_ptr<Physics::ShapeConfiguration> CreateScaledShapeConfig() override;

    protected:
        Physics::SphereShapeConfiguration m_shapeConfiguration; ///< Sphere shape configuration.
    };
}
