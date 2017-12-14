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

#include <PxPhysicsAPI.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>

inline physx::PxVec3 PxVec3FromLYVec3(const AZ::Vector3& lyVec)
{
    return physx::PxVec3(lyVec.GetX(), lyVec.GetY(), lyVec.GetZ());
}

inline AZ::Vector3 LYVec3FromPxVec3(const physx::PxVec3& pxVec)
{
    return AZ::Vector3(pxVec.x, pxVec.y, pxVec.z);
}

inline physx::PxQuat PxQuatFromLYQuat(const AZ::Quaternion& lyQuat)
{
    return physx::PxQuat(lyQuat.GetX(), lyQuat.GetY(), lyQuat.GetZ(), lyQuat.GetW());
}

inline AZ::Quaternion LYQuatFromPxQuat(const physx::PxQuat& pxQuat)
{
    return AZ::Quaternion(pxQuat.x, pxQuat.y, pxQuat.z, pxQuat.w);
}

inline physx::PxTransform PxTransformFromLYTransform(const AZ::Transform& lyTransform)
{
    AZ::Quaternion lyQuat = AZ::Quaternion::CreateFromTransform(lyTransform);
    AZ::Vector3 lyVec3 = lyTransform.GetPosition();
    return physx::PxTransform(PxVec3FromLYVec3(lyVec3), PxQuatFromLYQuat(lyQuat));
}

inline AZ::Transform LYTransformFromPxTransform(const physx::PxTransform& pxTransform)
{
    return AZ::Transform::CreateFromQuaternionAndTranslation(LYQuatFromPxQuat(pxTransform.q), LYVec3FromPxVec3(pxTransform.p));
}
