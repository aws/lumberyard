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

namespace AZ
{
    class Aabb;
}

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
            IsTrigger = 1 << 2,
            IsVisible = 1 << 3, ///< @deprecated This property will be removed in a future release.
            Offset = 1 << 4 ///< Whether the rotation and position offsets should be visible.
        };

        ColliderConfiguration() = default;
        ColliderConfiguration(const ColliderConfiguration&) = default;
        virtual ~ColliderConfiguration() = default;

        AZ::Crc32 GetPropertyVisibility(PropertyVisibility property) const;
        void SetPropertyVisibility(PropertyVisibility property, bool isVisible);

        AZ::Crc32 GetIsTriggerVisibility() const;
        AZ::Crc32 GetCollisionLayerVisibility() const;
        AZ::Crc32 GetMaterialSelectionVisibility() const;
        AZ::Crc32 GetOffsetVisibility() const;

        Physics::CollisionLayer m_collisionLayer; ///< Which collision layer is this collider on.
        Physics::CollisionGroups::Id m_collisionGroupId; ///< Which layers does this collider collide with.
        bool m_isTrigger = false; ///< Should this shape act as a trigger shape.
        bool m_isExclusive = true; ///< Can this collider be shared between multiple bodies?
        AZ::Vector3 m_position = AZ::Vector3::CreateZero(); /// Shape offset relative to the connected rigid body.
        AZ::Quaternion m_rotation = AZ::Quaternion::CreateIdentity(); ///< Shape rotation relative to the connected rigid body.
        Physics::MaterialSelection m_materialSelection; ///< Materials for the collider.
        AZ::u8 m_propertyVisibilityFlags = (std::numeric_limits<AZ::u8>::max)(); ///< Visibility flags for collider.
                                                                                 ///< Note: added parenthesis for std::numeric_limits is
                                                                                 ///< to avoid collision with `max` macro in uber builds.
        bool m_visible = false; ///< @deprecated This property will be removed in a future release. Display the collider in editor view.
        AZStd::string m_tag; ///< Identifaction tag for the collider
    };

    using ShapeConfigurationPair = AZStd::pair<AZStd::shared_ptr<ColliderConfiguration>, AZStd::shared_ptr<ShapeConfiguration>>;
    using ShapeConfigurationList = AZStd::vector<ShapeConfigurationPair>;

    struct RayCastRequest;
    struct RayCastHit;

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
        virtual AZStd::pair<AZ::Vector3, AZ::Quaternion> GetLocalPose() const = 0;

        virtual void* GetNativePointer() = 0;

        virtual AZ::Crc32 GetTag() const = 0;

        virtual void AttachedToActor(void* actor) = 0;
        virtual void DetachedFromActor() = 0;

        //! Raycast against this shape.
        //! @param request Ray parameters in world space.
        //! @param worldTransform World transform of this shape.
        virtual Physics::RayCastHit RayCast(const Physics::RayCastRequest& worldSpaceRequest, const AZ::Transform& worldTransform) = 0;

        //! Raycast against this shape using local coordinates.
        //! @param request Ray parameters in local space.
        virtual Physics::RayCastHit RayCastLocal(const Physics::RayCastRequest& localSpaceRequest) = 0;

        //! Fills in the vertices and indices buffers representing this shape.
        //! If vertices are returned but not indices you may assume the vertices are in triangle list format.
        //! @param vertices A buffer to be filled with vertices
        //! @param indices A buffer to be filled with indices
        //! @param optionalBounds Optional AABB that, if provided, will limit the mesh returned to that AABB.  
        //!                       Currently only supported by the heightfield shape.
        virtual void GetGeometry(AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, AZ::Aabb* optionalBounds = nullptr) = 0;

    };
} // namespace Physics
