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

#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Data/Data.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzCore/Component/TransformBus.h>

namespace ScriptCanvasPhysics
{
    namespace WorldNodes
    {
        using Result = AZStd::tuple<
            bool /*true if an object was hit*/,
            AZ::Vector3 /*world space position*/,
            AZ::Vector3 /*surface normal*/,
            float /*distance to the hit*/,
            AZ::EntityId /*entity hit, if any*/
        >;

        AZ_INLINE Result RayCastWorldSpace(const AZ::Vector3& start, const AZ::Vector3& direction, float distance, AZ::EntityId ignore)
        {
            Physics::RayCastRequest request;
            request.m_start = start;
            request.m_direction = direction.GetNormalized();
            request.m_distance = distance;
            request.m_customFilterCallback = [ignore](const Physics::WorldBody* body, const Physics::Shape* shape) 
            {
                return body->GetEntityId() != ignore;
            };

            Physics::RayCastHit hit;
            Physics::WorldRequestBus::BroadcastResult(hit, &Physics::World::RayCast, request);
            
            AZ::EntityId id;
            if (hit)
            {
                id = hit.m_body->GetEntityId();
            }
            return AZStd::make_tuple(hit.m_body != nullptr, hit.m_position, hit.m_normal, hit.m_distance, id);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(RayCastWorldSpace,
            "PhysX/World",
            "{F75EF071-6755-40A3-8D5D-0603964774AE}",
            "Returns the first hit from start in direction",
            "Start",
            "Direction",
            "Distance",
            "Ignore",
            "Object hit",
            "Position",
            "Normal",
            "Distance");

        AZ_INLINE Result RayCastLocalSpace(const AZ::EntityId& fromEntityId, const AZ::Vector3& direction, float distance, AZ::EntityId ignore)
        {
            AZ::Transform worldSpaceTransform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldSpaceTransform, fromEntityId, &AZ::TransformInterface::GetWorldTM);

            Physics::RayCastRequest request;
            request.m_start = worldSpaceTransform.GetTranslation();
            request.m_direction = worldSpaceTransform.Multiply3x3(direction.GetNormalized());
            request.m_distance = distance;
            request.m_customFilterCallback = [ignore](const Physics::WorldBody* body, const Physics::Shape* shape)
            {
                return body->GetEntityId() != ignore;
            };

            Physics::RayCastHit hit;
            Physics::WorldRequestBus::BroadcastResult(hit, &Physics::World::RayCast, request);

            AZ::EntityId id;
            if (hit)
            {
                id = hit.m_body->GetEntityId();
            }
            return AZStd::make_tuple(hit.m_body != nullptr, hit.m_position, hit.m_normal, hit.m_distance, id);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(RayCastLocalSpace,
            "PhysX/World",
            "{1D6935AF-8EE9-4636-9F0C-E0485D935800}",
            "Returns the first hit from [fromEntityId] to direction",
            "Source",
            "Direction",
            "Distance",
            "Ignore",
            "Object hit",
            "Position",
            "Normal",
            "Distance");

        AZ_INLINE ScriptCanvas::Data::BooleanType OverlapSphere(const AZ::Vector3& position, float radius, AZ::EntityId ignore)
        {
            Physics::SphereShapeConfiguration sphere(radius);

            Physics::OverlapRequest request;
            request.m_pose = AZ::Transform::CreateTranslation(position);
            request.m_shapeConfiguration = &sphere;
            request.m_customFilterCallback = [ignore](const Physics::WorldBody* body, const Physics::Shape* shape)
            {
                return body->GetEntityId() != ignore;
            };

            AZStd::vector<Physics::OverlapHit> overlaps;
            Physics::WorldRequestBus::BroadcastResult(overlaps, &Physics::World::Overlap, request);

            return !overlaps.empty();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(OverlapSphere,
            "PhysX/World",
            "{ED1CE1E1-5BF1-46BA-8DFF-78966FFDBF75}",
            "Returns if any objects overlapping a sphere at a position",
            "Position",
            "Radius",
            "Ignore"
            );

        using Registrar = ScriptCanvas::RegistrarGeneric
            < 
            RayCastWorldSpaceNode,
            RayCastLocalSpaceNode,
            OverlapSphereNode
            >;
    }
}