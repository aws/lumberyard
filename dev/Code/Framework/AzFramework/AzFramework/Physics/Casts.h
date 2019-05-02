/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <functional>

#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/EntityId.h>

#include <AzFramework/Physics/WorldBody.h>
#include <AzFramework/Physics/Collision.h>
#include <AzFramework/Physics/Material.h>

namespace Physics
{
    class WorldBody;
    class Shape;
    class ShapeConfiguration;
    
    using CustomFilterCallback = AZStd::function<bool(const Physics::WorldBody* body, const Physics::Shape* shape)>;

    /// Enum to specify which shapes are included in the query.
    enum class QueryType
    {
        Static, ///< Only test against static shapes
        Dynamic, ///< Only test against dynamic shapes
        StaticAndDynamic ///< Test against both static and dynamic shapes
    };

    /// Casts a ray from a starting pose along a direction returning objects that intersected with the ray.
    struct RayCastRequest
    {
        AZ_CLASS_ALLOCATOR(RayCastRequest, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(RayCastRequest, "{53EAD088-A391-48F1-8370-2A1DBA31512F}");

        float m_distance = 500.0f; ///< The distance along m_dir direction.
        AZ::Vector3 m_start = AZ::Vector3::CreateZero(); ///< World space point where ray starts from.
        AZ::Vector3 m_direction = AZ::Vector3::CreateZero(); ///< World space direction (normalized).
        CollisionGroup m_collisionGroup = CollisionGroup::All; ///< The layers to include in the query
        CustomFilterCallback m_customFilterCallback = nullptr; ///< Custom filtering function
        QueryType m_queryType = QueryType::StaticAndDynamic; ///< Object types to include in the query
    };
    
    /// Sweeps a shape from a starting pose along a direction returning objects that intersected with the shape.
    struct ShapeCastRequest
    {
        AZ_CLASS_ALLOCATOR(ShapeCastRequest, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(ShapeCastRequest, "{52F6C536-92F6-4C05-983D-0A74800AE56D}");

        float m_distance = 500.0f; /// The distance to cast along m_dir direction.
        AZ::Transform m_start = AZ::Transform::CreateIdentity(); ///< World space start position. Assumes only rotation + translation (no scaling).
        AZ::Vector3 m_direction = AZ::Vector3::CreateZero(); ///< World space direction (Should be normalized)
        ShapeConfiguration* m_shapeConfiguration = nullptr; ///< Shape information.
        CollisionGroup m_collisionGroup = CollisionGroup::All; ///< Collision filter for the query.
        CustomFilterCallback m_customFilterCallback = nullptr; ///< Custom filtering function
        QueryType m_queryType = QueryType::StaticAndDynamic; ///< Object types to include in the query
    };

    /// Searches a region enclosed by a specified shape for any overlapping objects in the scene.
    struct OverlapRequest
    {
        AZ_CLASS_ALLOCATOR(OverlapRequest, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(OverlapRequest, "{3DC986C2-316B-4C54-A0A6-8ABBB8ABCC4A}");

        AZ::Transform m_pose = AZ::Transform::CreateIdentity(); ///< Initial shape pose
        ShapeConfiguration* m_shapeConfiguration = nullptr; ///< Shape information.
        CollisionGroup m_collisionGroup = CollisionGroup::All; ///< Collision filter for the query.
        CustomFilterCallback m_customFilterCallback = nullptr; ///< Custom filtering function
        QueryType m_queryType = QueryType::StaticAndDynamic; ///< Object types to include in the query
    };

    /// Structure used to store the result from either a raycast or a shape cast.
    struct RayCastHit
    {
        AZ_CLASS_ALLOCATOR(RayCastHit, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(RayCastHit, "{A46CBEA6-6B92-4809-9363-9DDF0F74F296}");

        inline operator bool() const { return m_body != nullptr; }

        float m_distance = 0.0f; ///< The distance along the cast at which the hit occurred as given by Dot(m_normal, startPoint) - Dot(m_normal, m_point).
        AZ::Vector3 m_position = AZ::Vector3::CreateZero(); ///< The position of the hit in world space
        AZ::Vector3 m_normal = AZ::Vector3::CreateZero(); ///< The normal of the surface hit
        WorldBody* m_body = nullptr; ///< World body that was hit.
        Shape* m_shape = nullptr; ///< The shape on the body that was hit
        Material* m_material = nullptr; ///< The material on the shape (or face) that was hit
    };

    /// Overlap hit.
    struct OverlapHit
    {
        AZ_CLASS_ALLOCATOR(OverlapHit, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(OverlapHit, "{7A7201B9-67B5-438B-B4EB-F3EEBB78C617}");

        inline operator bool() const { return m_body != nullptr; }

        WorldBody* m_body = nullptr; ///< World body that was hit.
        Shape* m_shape = nullptr; ///< The shape on the body that was hit
        Material* m_material = nullptr; ///< The material on the shape (or face) that was hit
    };

} // namespace Physics
