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

#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/Collision.h>

namespace Physics
{
    class Material;

    class ColliderConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(ColliderConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(ColliderConfiguration, "{16206828-F867-4DA9-9E4E-549B7B2C6174}");
        static void Reflect(AZ::ReflectContext* context);

        enum PropertyVisibility : AZ::u8
        {
            CollisionLayer = 1 << 0,
            MaterialSelection = 1 << 1,
            IsTrigger = 1 << 2
        };

        ColliderConfiguration() = default;
        ColliderConfiguration(const ColliderConfiguration&) = default;
        virtual ~ColliderConfiguration() = default;

        AZ::Crc32 GetPropertyVisibility(PropertyVisibility property) const;
        void SetPropertyVisibility(PropertyVisibility property, bool isVisible);

        AZ::Crc32 GetIsTriggerVisibility() const;
        AZ::Crc32 GetCollisionLayerVisibility() const;
        AZ::Crc32 GetMaterialSelectionVisibility() const;

        Physics::CollisionLayer m_collisionLayer; ///< Which collision layer is this collider on
        Physics::CollisionGroups::Id m_collisionGroupId; ///< Which layers does this collider collide with
        bool m_isTrigger = false; ///< Should this shape act as a trigger shape
        bool m_isExclusive = true; ///< Can this collider be shared between multiple bodies?
        AZ::Vector3 m_position = AZ::Vector3::CreateZero(); /// Shape offset relative to the connected rigid body
        AZ::Quaternion m_rotation = AZ::Quaternion::CreateIdentity(); ///< Shape rotation relative to the connected rigid body
        Physics::MaterialSelection m_materialSelection; ///< Materials for the collider
        AZ::u8 m_propertyVisibilityFlags = (std::numeric_limits<AZ::u8>::max)();

        bool m_visible = false; ///< Display the collider in editor view.
    };
    using ShapeConfigurationPair = AZStd::pair<AZStd::shared_ptr<ColliderConfiguration>, AZStd::shared_ptr<ShapeConfiguration>>;
    using ShapeConfigurationList = AZStd::vector<ShapeConfigurationPair>;

    class Shape
    {
    public:
        AZ_CLASS_ALLOCATOR(Shape, AZ::SystemAllocator, 0);
        AZ_RTTI(Shape, "{0A47DDD6-2BD7-43B3-BF0D-2E12CC395C13}");
        virtual ~Shape() = default;

        virtual void SetMaterial(const AZStd::shared_ptr<Material>& material) = 0;
        virtual AZStd::shared_ptr<Material> GetMaterial() const = 0;

        virtual void SetCollisionLayer(const Physics::CollisionLayer& layer) = 0;
        virtual Physics::CollisionLayer GetCollisionLayer() const = 0;

        virtual void SetCollisionGroup(const Physics::CollisionGroup& group) = 0;
        virtual Physics::CollisionGroup GetCollisionGroup() const = 0;

        virtual void SetName(const char* name) = 0;

        virtual void SetLocalPose(const AZ::Vector3& offset, const AZ::Quaternion& rotation) = 0;

        virtual void* GetNativePointer() = 0;
    };
} // namespace Physics
