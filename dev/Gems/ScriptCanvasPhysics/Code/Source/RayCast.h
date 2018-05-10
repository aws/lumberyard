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

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <AzFramework/Physics/PhysicsSystemComponentBus.h>
#include <ScriptCanvas/Data/Data.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>

#include <AzCore/Component/ComponentApplicationBus.h>

namespace ScriptCanvasPhysics
{
    namespace RayCastNodes
    {
        AZ_INLINE void SetDefaultsForRayCastFromEntityToDirection(ScriptCanvas::Node& node)
        {
            // Direction.
            ScriptCanvas::Node::SetDefaultValuesByIndex<1>::_(node, AZ::Vector3(0.f, 1.f, 0.f));

            // Offset from origin.
            ScriptCanvas::Node::SetDefaultValuesByIndex<2>::_(node, 0.);

            // Max distance.
            ScriptCanvas::Node::SetDefaultValuesByIndex<3>::_(node, 100.);
        }

        AZ_INLINE void SetDefaultsForRayCastFromWorldSpacePositionToEntity(ScriptCanvas::Node& node)
        {
        }

        AZ_INLINE void SetDefaultsForRayCastFromEntityToWorldSpacePosition(ScriptCanvas::Node& node)
        {
        }

        AZ_INLINE void SetDefaultsForRayCastFromWorldSpacePositionToWorldSpacePosition(ScriptCanvas::Node& node)
        {
            // From.
            ScriptCanvas::Node::SetDefaultValuesByIndex<0>::_(node, AZ::Vector3::CreateZero());
            
            // To.
            ScriptCanvas::Node::SetDefaultValuesByIndex<1>::_(node, AZ::Vector3(1.f, 0.f, 0.f));
        }

        using HitResult = AZStd::tuple<float /*distance from the origin of the raycast*/,
                    AZ::Vector3 /*world space position*/,
                    AZ::Vector3 /*surface normal*/,
                    AZ::EntityId /*entity hit, if any*/>;

        HitResult RayCast(ScriptCanvas::Data::Vector3Type origin,
            ScriptCanvas::Data::Vector3Type direction,
            float maxDistance,
            AZStd::vector<AZ::EntityId> ignoreEntityIds)
        {
            AzFramework::PhysicsSystemRequests::RayCastConfiguration rayCastConfiguration;
            rayCastConfiguration.m_origin = origin;
            rayCastConfiguration.m_direction = direction;
            rayCastConfiguration.m_maxDistance = maxDistance;
            rayCastConfiguration.m_ignoreEntityIds = ignoreEntityIds;
            rayCastConfiguration.m_maxHits = 1;

            AzFramework::PhysicsSystemRequests::RayCastResult result;
            AzFramework::PhysicsSystemRequestBus::BroadcastResult(result, &AzFramework::PhysicsSystemRequests::RayCast, rayCastConfiguration);

            HitResult hitResult;
            if (result.GetHitCount())
            {
                const AzFramework::PhysicsSystemRequests::RayCastHit* hit = result.GetHit(0);

                hitResult = AZStd::make_tuple(hit->m_distance, hit->m_position, hit->m_normal, hit->m_entityId);
            }

            return hitResult;
        }

        AZ_INLINE HitResult RayCastFromEntityToEntity(AZ::EntityId fromEntityId, AZ::EntityId toEntityId, AZ::EntityId ignoreEntityId)
        {
            if (fromEntityId == toEntityId)
            {
                // Nothing to do.
                return HitResult();
            }

            ScriptCanvas::Data::Vector3Type fromWorldSpacePosition(ScriptCanvas::Data::Vector3Type::CreateZero());
            AZ::TransformBus::EventResult(fromWorldSpacePosition, fromEntityId, &AZ::TransformInterface::GetWorldTranslation);

            ScriptCanvas::Data::Vector3Type toWorldSpacePosition(ScriptCanvas::Data::Vector3Type::CreateZero());
            AZ::TransformBus::EventResult(toWorldSpacePosition, toEntityId, &AZ::TransformInterface::GetWorldTranslation);

            ScriptCanvas::Data::Vector3Type worldSpaceDirection(toWorldSpacePosition - fromWorldSpacePosition);
            float distance = worldSpaceDirection.GetLength();
            worldSpaceDirection.Normalize();

            return RayCast(fromWorldSpacePosition, worldSpaceDirection, distance, {fromEntityId, ignoreEntityId});
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(RayCastFromEntityToEntity, "Physics/RayCast", "{ADF6E642-487E-4ADB-B930-49941A355A00}", "Returns the first hit from [fromEntityId] to [toEntityId]", "From", "To", "Ignore", "Hit Distance From Start", "Hit World-space Position", "Hit Surface Normal", "Hit EntityId");

        AZ_INLINE HitResult RayCastFromEntityToLocalSpaceDirection(AZ::EntityId fromEntityId, ScriptCanvas::Data::Vector3Type localSpaceDirection, ScriptCanvas::Data::NumberType offsetFromOrigin, ScriptCanvas::Data::NumberType maxDistance, AZ::EntityId ignoreEntityId)
        {
            localSpaceDirection.Normalize();

            AZ::Transform worldSpaceTransform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldSpaceTransform, fromEntityId, &AZ::TransformInterface::GetWorldTM);

            ScriptCanvas::Data::Vector3Type worldSpaceDirection = worldSpaceTransform.Multiply3x3(localSpaceDirection);
            worldSpaceDirection.Normalize();

            ScriptCanvas::Data::Vector3Type fromWorldSpacePosition = worldSpaceTransform.GetTranslation() + worldSpaceTransform.Multiply3x3(localSpaceDirection * aznumeric_cast<float>(offsetFromOrigin));

            return RayCast(fromWorldSpacePosition, worldSpaceDirection, aznumeric_cast<float>(maxDistance), {fromEntityId, ignoreEntityId});
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(RayCastFromEntityToLocalSpaceDirection, SetDefaultsForRayCastFromEntityToDirection, "Physics/RayCast", "{E972C607-8DC5-40EB-B9A8-8545CE361462}", "Returns the first hit from [fromEntityId] in the [localSpaceDirection], the ray's origin is offset along the direction by [offsetFromOrigin], and at-most [maxDistance] from the starting point", "From", "Local-space Direction", "Offset from the ray's origin in the specified direction,", "Max Distance", "Ignore", "Hit Distance From Start", "Hit World-space Position", "Hit Surface Normal", "Hit EntityId");

        AZ_INLINE HitResult RayCastFromEntityToWorldSpaceDirection(AZ::EntityId fromEntityId, ScriptCanvas::Data::Vector3Type worldSpaceDirection, ScriptCanvas::Data::NumberType offsetFromOrigin, ScriptCanvas::Data::NumberType maxDistance, AZ::EntityId ignoreEntityId)
        {
            worldSpaceDirection.Normalize();

            AZ::Transform worldSpaceTransform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldSpaceTransform, fromEntityId, &AZ::TransformInterface::GetWorldTM);

            ScriptCanvas::Data::Vector3Type fromWorldSpacePosition = worldSpaceTransform.GetTranslation() + (worldSpaceDirection * aznumeric_cast<float>(offsetFromOrigin));

            return RayCast(fromWorldSpacePosition, worldSpaceDirection, aznumeric_cast<float>(maxDistance), {fromEntityId, ignoreEntityId});
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(RayCastFromEntityToWorldSpaceDirection, SetDefaultsForRayCastFromEntityToDirection, "Physics/RayCast", "{E74A541E-B42C-43A3-9B7C-83124DC2F257}", "Returns the first hit from [fromEntityId] in the [worldSpaceDirection], the ray's origin is offset along the direction by [offsetFromOrigin], and at-most [maxDistance] from the starting point", "From", "World-space Direction", "Offset from the ray's origin in the specified direction", "Max Distance", "Ignore", "Hit Distance From Start", "Hit World-space Position", "Hit Surface Normal", "Hit EntityId");

        AZ_INLINE HitResult RayCastFromWorldSpacePositionToEntity(ScriptCanvas::Data::Vector3Type fromWorldSpacePosition,
            AZ::EntityId toEntityId,
            AZ::EntityId ignoreEntityId)
        {
            AZ::Transform worldSpaceTransform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldSpaceTransform, toEntityId, &AZ::TransformInterface::GetWorldTM);

            ScriptCanvas::Data::Vector3Type worldSpaceDirection = ScriptCanvas::Data::Vector3Type(worldSpaceTransform.GetTranslation() - fromWorldSpacePosition);
            float maxDistance = worldSpaceDirection.GetLength();
            worldSpaceDirection.Normalize();

            return RayCast(fromWorldSpacePosition, worldSpaceDirection, maxDistance, {ignoreEntityId});
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(RayCastFromWorldSpacePositionToEntity,
            SetDefaultsForRayCastFromWorldSpacePositionToEntity,
            "Physics/RayCast",
            "{3543F7C5-42D9-4472-8DB0-B69EE8AA31C1}",
            "Returns the first hit from [fromWorldSpacePosition] to [toEntityId]",
            "Position", "Target", "Ignore",
            "Distance", "Position", "Normal", "Entity");

        AZ_INLINE HitResult RayCastFromEntityToWorldSpacePosition(AZ::EntityId fromEntityId,
            ScriptCanvas::Data::Vector3Type toWorldSpacePosition,
            AZ::EntityId ignoreEntityId)
        {
            AZ::Transform worldSpaceTransform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldSpaceTransform, fromEntityId, &AZ::TransformInterface::GetWorldTM);

            ScriptCanvas::Data::Vector3Type worldSpaceDirection = ScriptCanvas::Data::Vector3Type(toWorldSpacePosition - worldSpaceTransform.GetTranslation());
            float maxDistance = worldSpaceDirection.GetLength();
            worldSpaceDirection.Normalize();

            return RayCast(worldSpaceTransform.GetTranslation(), worldSpaceDirection, maxDistance, {ignoreEntityId});
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(RayCastFromEntityToWorldSpacePosition,
            SetDefaultsForRayCastFromEntityToWorldSpacePosition,
            "Physics/RayCast",
            "{62E6F71D-6330-42EE-AC41-71CD14D1BF98}",
            "Returns the first hit from [fromEntityId] to [toWorldSpacePosition]",
            "Source", "Position", "Ignore",
            "Distance", "Position", "Normal", "Entity");

        AZ_INLINE HitResult RayCastFromWorldSpacePositionToWorldSpacePosition(ScriptCanvas::Data::Vector3Type fromWorldSpacePosition,
            ScriptCanvas::Data::Vector3Type toWorldSpacePosition,
            AZ::EntityId ignoreEntityId)
        {
            ScriptCanvas::Data::Vector3Type worldSpaceDirection = ScriptCanvas::Data::Vector3Type(toWorldSpacePosition - fromWorldSpacePosition);
            const float distance = worldSpaceDirection.GetLength();
            worldSpaceDirection.Normalize();

            return RayCast(fromWorldSpacePosition, worldSpaceDirection, distance, { ignoreEntityId });
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(RayCastFromWorldSpacePositionToWorldSpacePosition,
            SetDefaultsForRayCastFromWorldSpacePositionToWorldSpacePosition,
            "Physics/RayCast",
            "{F4DF407A-65E2-4ED7-9FC3-21FFFAC9CC2D}",
            "Returns the first hit between [fromWorldSpacePosition] and [toWorldSpacePosition]",
            "From", "To", "Ignore",
            "Distance", "Position", "Normal", "Entity");


        using Registrar = ScriptCanvas::RegistrarGeneric
                < RayCastFromEntityToEntityNode
                    , RayCastFromEntityToLocalSpaceDirectionNode
                    , RayCastFromEntityToWorldSpaceDirectionNode
                    , RayCastFromWorldSpacePositionToEntityNode
                    , RayCastFromEntityToWorldSpacePositionNode
                    , RayCastFromWorldSpacePositionToWorldSpacePositionNode
                >;
    } // namespace RayCastNodes
} // namespace ScriptCanvasPhysics
