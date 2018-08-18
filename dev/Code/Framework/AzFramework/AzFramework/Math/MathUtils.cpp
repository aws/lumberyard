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

#include "MathUtils.h"

namespace AzFramework
{
#pragma warning( push )         // disable deprecated warning from our own internal calls until they are removed
#pragma warning(disable: 4996)

    // Technique from published work available here
    // https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2012/07/euler-angles1.pdf (Extracting Euler Angles from a Rotation Matrix - Mike Day, Insomniac Games mday@insomniacgames.com)
    AZ::Vector3 ConvertTransformToEulerDegrees(const AZ::Transform& transform)
    {
        AZ::Vector3 radians = AzFramework::ConvertTransformToEulerRadians(transform);
        return RadToDeg(radians);
    }

    AZ::Vector3 ConvertTransformToEulerRadians(const AZ::Transform& transform)
    {
        auto rotx = -atan2(transform(1, 2), transform(2, 2));
        AZ::Vector2 temp{
            transform(0, 0), transform(0, 1)
        };
        auto c2 = temp.GetLength();
        auto roty = -atan2(-transform(0, 2), c2);
        float s1 = sin(-rotx);
        float c1 = cos(rotx);
        auto rotz = -atan2(-c1 * transform(1, 0) + s1 * transform(2, 0), c1 * transform(1, 1) - s1 * transform(2, 1));
        rotx = AZ::Wrap(rotx, 0.f, AZ::Constants::TwoPi);
        roty = AZ::Wrap(roty, 0.f, AZ::Constants::TwoPi);
        rotz = AZ::Wrap(rotz, 0.f, AZ::Constants::TwoPi);
        return AZ::Vector3(rotx, roty, rotz);
    }

    AZ::Transform ConvertEulerDegreesToTransform(const AZ::Vector3& eulerDegrees)
    {
        AZ::Vector3 eulerRadians = AzFramework::DegToRad(eulerDegrees);

        return AZ::Transform::CreateRotationX(eulerRadians.GetX())
               * AZ::Transform::CreateRotationY(eulerRadians.GetY())
               * AZ::Transform::CreateRotationZ(eulerRadians.GetZ());
    }

    AZ::Transform ConvertEulerRadiansToTransformPrecise(const AZ::Vector3& eulerRadians)
    {
        AZ::Transform finalRotation;

        // X basis
        const AZ::VectorFloat xSin = sinf(eulerRadians.GetX());
        const AZ::VectorFloat xCos = cosf(eulerRadians.GetX());
        finalRotation.SetRow(0, 1.0f, 0.0f, 0.0f, 0.0f);
        finalRotation.SetRow(1, 0.0f, xCos, -xSin, 0.0f);
        finalRotation.SetRow(2, 0.0f, xSin, xCos, 0.0f);

        // Y basis
        AZ::Transform yRotation;
        const AZ::VectorFloat ySin = sinf(eulerRadians.GetY());
        const AZ::VectorFloat yCos = cosf(eulerRadians.GetY());
        yRotation.SetRow(0, yCos, 0.0f, ySin, 0.0f);
        yRotation.SetRow(1, 0.0f, 1.0f, 0.0f, 0.0f);
        yRotation.SetRow(2, -ySin, 0.0f, yCos, 0.0f);
        finalRotation *= yRotation;

        // Z basis
        AZ::Transform zRotation;
        const AZ::VectorFloat zSin = sinf(eulerRadians.GetZ());
        const AZ::VectorFloat zCos = cosf(eulerRadians.GetZ());
        zRotation.SetRow(0, zCos, -zSin, 0.0f, 0.0f);
        zRotation.SetRow(1, zSin, zCos, 0.0f, 0.0f);
        zRotation.SetRow(2, 0.0f, 0.0f, 1.0f, 0.0f);
        finalRotation *= zRotation;

        return finalRotation;
    }

    AZ::Transform ConvertEulerDegreesToTransformPrecise(const AZ::Vector3& eulerDegrees)
    {
        AZ::Vector3 eulerRadians = AzFramework::DegToRad(eulerDegrees);
        AZ::Transform result = AzFramework::ConvertEulerRadiansToTransformPrecise(eulerRadians);
        return result;
    }

    AZ::Vector3 ConvertQuaternionToEulerDegrees(const AZ::Quaternion& q)
    {
        AZ::Vector3 eulers = AzFramework::ConvertQuaternionToEulerRadians(q);
        eulers.Set(AZ::RadToDeg(eulers.GetX()), AZ::RadToDeg(eulers.GetY()), AZ::RadToDeg(eulers.GetZ()));
        return eulers;
    }

    AZ::Vector3 ConvertQuaternionToEulerRadians(const AZ::Quaternion& q)
    {
        AZ::Transform transform = AZ::Transform::CreateFromQuaternion(q);
        return AzFramework::ConvertTransformToEulerRadians(transform);
    }

    AZ::Quaternion ConvertEulerRadiansToQuaternion(const AZ::Vector3& eulerRadians)
    {
        AZ::VectorFloat half(0.5f);
        AZ::VectorFloat rotx = eulerRadians.GetX() * half;
        AZ::VectorFloat roty = eulerRadians.GetY() * half;
        AZ::VectorFloat rotz = eulerRadians.GetZ() * half;

        AZ::VectorFloat sx, cx, sy, cy, sz, cz;
        rotx.GetSinCos(sx, cx);
        roty.GetSinCos(sy, cy);
        rotz.GetSinCos(sz, cz);

        // rot = rotx * roty * rotz
        auto w = cx * cy * cz - sx * sy * sz;
        auto x = cx * sy * sz + sx * cy * cz;
        auto y = cx * sy * cz - sx * cy * sz;
        auto z = cx * cy * sz + sx * sy * cz;

        AZ::Quaternion q(x, y, z, w);
        return q;
    }

    AZ::Quaternion ConvertEulerDegreesToQuaternion(const AZ::Vector3& eulerDegrees)
    {
        return AzFramework::ConvertEulerRadiansToQuaternion(AzFramework::DegToRad(eulerDegrees));
    }

    void ConvertQuaternionToAxisAngle(const AZ::Quaternion& quat, AZ::Vector3& outAxis, float& outAngle)
    {
        outAngle = 2.0f * acosf(quat.GetW());

        const float sinHalfAngle = sinf(outAngle * 0.5f);
        if (sinHalfAngle > 0.0f)
        {
            const float invS = 1.0f / sinHalfAngle;
            outAxis.Set(quat.GetX() * invS, quat.GetY() * invS, quat.GetZ() * invS);
        }
        else
        {
            outAxis.Set(0.0f, 1.0f, 0.0f);
            outAngle = 0.0f;
        }
    }

    AZ::Transform CreateLookAt(const AZ::Vector3& from, const AZ::Vector3& to, Axis forwardAxis)
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();

        // Get desired forward
        AZ::Vector3 targetForward = (to - from);
        // If 'look-at' vector is zero, error and return Identity transform.
        if (targetForward.IsZero())
        {
            AZ_Assert(!targetForward.IsZero(), "Can't create look-at transform when 'to' and 'from' positions are equal!");
            return transform;
        }

        targetForward.Normalize();

        // Ly is Z-up and is right-handed.  We assume that convention here,
        // which affects the ordering of the cross products and choosing an appropriate
        // 'up' basis vector.
        AZ::Vector3 up = AZ::Vector3::CreateAxisZ();

        // We have a degenerate case if target forward is parallel to the up axis,
        // in which case we'll have to pick a new up vector
        float absDot = targetForward.Dot(up).GetAbs();
        if (absDot > 1.0f - 0.001f)
        {
            up = targetForward.CrossYAxis();
        }

        AZ::Vector3 right = targetForward.Cross(up);
        right.Normalize();
        up = right.Cross(targetForward);
        up.Normalize();

        // Passing in forwardAxis allows you to force a particular local-space axis to look
        // at the target point.  In Ly, the default is forward is along Y+.
        switch (forwardAxis)
        {
        case AzFramework::Axis::XPositive:
            transform.SetBasisAndPosition(targetForward, -right, up, from);
            break;
        case AzFramework::Axis::XNegative:
            transform.SetBasisAndPosition(-targetForward, right, up, from);
            break;
        case AzFramework::Axis::YPositive:
            transform.SetBasisAndPosition(right, targetForward, up, from);
            break;
        case AzFramework::Axis::YNegative:
            transform.SetBasisAndPosition(-right, -targetForward, up, from);
            break;
        case AzFramework::Axis::ZPositive:
            transform.SetBasisAndPosition(right, -up, targetForward, from);
            break;
        case AzFramework::Axis::ZNegative:
            transform.SetBasisAndPosition(right, up, -targetForward, from);
            break;
        default:
            transform.SetBasisAndPosition(right, targetForward, up, from);
            break;
        }

        return transform;
    }

#pragma warning( pop )
} // namespace AzFramework
