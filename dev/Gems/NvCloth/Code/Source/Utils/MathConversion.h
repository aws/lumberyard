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

#include <Cry_Math.h> // Needed for Vec3 and Vec2

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>

#include <foundation/PxVec3.h>
#include <foundation/PxQuat.h>
#include <foundation/PxMat44.h>

#include <MCore/Source/AzCoreConversions.h>

#include <System/DataTypes.h>

namespace NvCloth
{
    AZ_INLINE physx::PxVec3 PxMathConvert(const AZ::Vector3& lyVec)
    {
        return physx::PxVec3(lyVec.GetX(), lyVec.GetY(), lyVec.GetZ());
    }

    AZ_INLINE physx::PxQuat PxMathConvert(const AZ::Quaternion& lyQuat)
    {
        return physx::PxQuat(lyQuat.GetX(), lyQuat.GetY(), lyQuat.GetZ(), lyQuat.GetW());
    }

    AZ_INLINE physx::PxMat44 PxMathConvert(const EMotionFX::Transform& emfxTransform)
    {
        AZ::Vector3 col0, col1, col2, col3;
        MCore::EmfxTransformToAzTransform(emfxTransform).GetColumns(&col0, &col1, &col2, &col3);
        return physx::PxMat44(
            PxMathConvert(col0),
            PxMathConvert(col1),
            PxMathConvert(col2),
            PxMathConvert(col3));
    }

    AZ_INLINE Vec3 PxMathConvert(const SimParticleType& simParticle)
    {
        return Vec3(simParticle.x, simParticle.y, simParticle.z);
    }

    AZ_INLINE Vec2 PxMathConvert(const SimUVType& uv)
    {
        return Vec2(uv.GetX(), uv.GetY());
    }
} // namespace NvCloth
