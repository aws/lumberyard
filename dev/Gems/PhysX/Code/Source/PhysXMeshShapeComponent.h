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

#include <AzCore/Component/TransformBus.h>
#include <Include/PhysX/PhysXMeshShapeComponentBus.h>

namespace PhysX
{
    class PhysXMeshShapeComponent
        : public AZ::Component
        , private LmbrCentral::ShapeComponentRequestsBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , public PhysXMeshShapeComponentRequestBus::Handler
    {
    public:
        AZ_COMPONENT(PhysXMeshShapeComponent, "{87A02711-8D7F-4966-87E1-77001EB6B29E}");

        PhysXMeshShapeComponent() = default;
        ~PhysXMeshShapeComponent() override = default;

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // ShapeComponent::Handler implementation
        AZ::Crc32 GetShapeType() override
        {
            return AZ_CRC("PhysXMesh", 0xe86bc8a6);
        }

        AZ::Aabb GetEncompassingAabb() override;
        bool IsPointInside(const AZ::Vector3& point) override;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) override;

        // Transform notification bus listener
        // Called when the local transform of the entity has changed. Local transform update always implies world transform change too.
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // PhysXMeshShapeComponentRequestBus implementation
        AZ::Data::Asset<Pipeline::PhysXMeshAsset> GetMeshAsset() override
        {
            return m_meshColliderAsset;
        }

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZ::Data::Asset<Pipeline::PhysXMeshAsset> m_meshColliderAsset;
        AZ::Transform m_currentTransform; ///< Caches the current transform for the entity on which this component lives.
    };
} // namespace PhysX