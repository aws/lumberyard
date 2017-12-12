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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Math/Obb.h>

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>

using namespace AZ;

//=======================================================================
//
// Obb constructor
//
//=======================================================================
const Obb Obb::CreateFromAabb(const Aabb& aabb)
{
    Obb result;
    result.m_position = aabb.GetCenter();
    Vector3 extents = aabb.GetExtents();
    result.m_axes[0].m_axis.Set(1.0f, 0.0f, 0.0f);
    result.m_axes[0].m_halfLength = 0.5f * extents.GetX();
    result.m_axes[1].m_axis.Set(0.0f, 1.0f, 0.0f);
    result.m_axes[1].m_halfLength = 0.5f * extents.GetY();
    result.m_axes[2].m_axis.Set(0.0f, 0.0f, 1.0f);
    result.m_axes[2].m_halfLength = 0.5f * extents.GetZ();
    return result;
}

//=======================================================================
//
// Obb - operator*
//
//=======================================================================
const Obb AZ::operator*(const AZ::Transform& transform, const AZ::Obb& obb)
{
    return Obb::CreateFromPositionAndAxes(transform * obb.GetPosition(),
        transform.Multiply3x3(obb.GetAxisX()), obb.GetHalfLengthX(),
        transform.Multiply3x3(obb.GetAxisY()), obb.GetHalfLengthY(),
        transform.Multiply3x3(obb.GetAxisZ()), obb.GetHalfLengthZ());
}

bool Obb::IsFinite() const
{
    for (int i = 0; i < 3; ++i)
    {
        if (!m_axes[i].m_axis.IsFinite() || !IsFiniteFloat(m_axes[i].m_halfLength))
        {
            return false;
        }
    }
    return m_position.IsFinite();
}

bool Obb::operator==(const Obb& rhs) const
{
    for (int i = 0; i < 3; ++i)
    {
        if (!m_axes[i].m_axis.IsClose(rhs.m_axes[i].m_axis) ||
            !AZ::IsClose(m_axes[i].m_halfLength, rhs.m_axes[i].m_halfLength, AZ_FLT_EPSILON))
        {
            return false;
        }
    }

    return m_position.IsClose(rhs.m_position);
}

bool Obb::operator!=(const Obb& rhs) const
{
    return !(*this == rhs);
}

#endif // #ifndef AZ_UNITY_BUILD