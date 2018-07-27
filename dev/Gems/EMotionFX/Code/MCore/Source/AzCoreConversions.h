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

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <MCore/Source/Color.h>
#include <MCore/Source/Vector.h>
#include <MCore/Source/Matrix4.h>
#include <MCore/Source/Quaternion.h>
#include <MCore/Source/MemoryObject.h>

#include <EMotionFX/Source/Transform.h>

namespace MCore
{
    AZ_FORCE_INLINE AZ::Color EmfxColorToAzColor(const MCore::RGBAColor& emfxColor)
    {
        return AZ::Color(emfxColor.r, emfxColor.g, emfxColor.b, emfxColor.a);
    }

    AZ_FORCE_INLINE MCore::RGBAColor AzColorToEmfxColor(const AZ::Color& azColor)
    {
        return MCore::RGBAColor(static_cast<float>(azColor.GetR()), static_cast<float>(azColor.GetG()), static_cast<float>(azColor.GetB()), static_cast<float>(azColor.GetA()));
    }

    AZ_FORCE_INLINE AZ::Quaternion EmfxQuatToAzQuat(const MCore::Quaternion& emfxQuat)
    {
        return AZ::Quaternion(emfxQuat.x, emfxQuat.y, emfxQuat.z, emfxQuat.w);
    }

    AZ_FORCE_INLINE MCore::Quaternion AzQuatToEmfxQuat(const AZ::Quaternion& azQuat)
    {
        return MCore::Quaternion(azQuat.GetX(), azQuat.GetY(), azQuat.GetZ(), azQuat.GetW());
    }

    AZ_FORCE_INLINE AZ::Transform EmfxTransformToAzTransform(const EMotionFX::Transform& emfxTransform)
    {
        AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
                EmfxQuatToAzQuat(emfxTransform.mRotation),
                emfxTransform.mPosition);
        transform.MultiplyByScale(emfxTransform.mScale);
        return transform;
    }

    AZ_FORCE_INLINE EMotionFX::Transform AzTransformToEmfxTransform(const AZ::Transform& azTransform)
    {
        return EMotionFX::Transform(
            azTransform.GetTranslation(),
            AzQuatToEmfxQuat(AZ::Quaternion::CreateFromTransform(azTransform)),
            azTransform.RetrieveScale());
    }

    AZ_FORCE_INLINE MCore::Quaternion AzEulerAnglesToEmfxQuat(const AZ::Vector3& eulerAngles)
    {
        MCore::Quaternion quat;
        // MCore::Quaternion::SetEuler expects order: pitch, yaw, roll.
        // In the LY coordinate system, that's X, Z, Y respectively.
        quat.SetEuler(eulerAngles.GetX(), eulerAngles.GetZ(), eulerAngles.GetY());
        return quat;
    }
} // namespace MCore
