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
#include <AzCore/Math/Quaternion.h>

#include <foundation/PxVec3.h>
#include <foundation/PxQuat.h>

namespace NvCloth
{
    AZ_INLINE physx::PxVec3 PxMathConvert(const AZ::Vector3& lyVec)
    {
        return physx::PxVec3(lyVec.GetX(), lyVec.GetY(), lyVec.GetZ());
    }

    AZ_INLINE AZ::Vector3 PxMathConvert(const physx::PxVec3& pxVec)
    {
        return AZ::Vector3(pxVec.x, pxVec.y, pxVec.z);
    }

    AZ_INLINE physx::PxQuat PxMathConvert(const AZ::Quaternion& lyQuat)
    {
        return physx::PxQuat(lyQuat.GetX(), lyQuat.GetY(), lyQuat.GetZ(), lyQuat.GetW());
    }
} // namespace NvCloth
