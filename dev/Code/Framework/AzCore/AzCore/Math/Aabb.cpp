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

#include <AzCore/Math/Aabb.h>

#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Transform.h>

using namespace AZ;

//=======================================================================
//
// Aabb constructor
//
//=======================================================================
const Aabb Aabb::CreateFromObb(const Obb& obb)
{
    Vector3 tmp[8];
    tmp[0] = obb.GetPosition() + obb.GetAxisX() * obb.GetHalfLengthX()
        + obb.GetAxisY() * obb.GetHalfLengthY()
        + obb.GetAxisZ() * obb.GetHalfLengthZ();
    tmp[1] = tmp[0] - obb.GetAxisZ() * (2.0f * obb.GetHalfLengthZ());
    tmp[2] = tmp[0] - obb.GetAxisX() * (2.0f * obb.GetHalfLengthX());
    tmp[3] = tmp[1] - obb.GetAxisX() * (2.0f * obb.GetHalfLengthX());
    tmp[4] = tmp[0] - obb.GetAxisY() * (2.0f * obb.GetHalfLengthY());
    tmp[5] = tmp[1] - obb.GetAxisY() * (2.0f * obb.GetHalfLengthY());
    tmp[6] = tmp[2] - obb.GetAxisY() * (2.0f * obb.GetHalfLengthY());
    tmp[7] = tmp[3] - obb.GetAxisY() * (2.0f * obb.GetHalfLengthY());

    Vector3 min = tmp[0];
    Vector3 max = tmp[0];

    for (int i = 1; i < 8; ++i)
    {
        min = min.GetMin(tmp[i]);
        max = max.GetMax(tmp[i]);
    }

    return Aabb::CreateFromMinMax(min, max);
}

//=======================================================================
//
// Aabb - GetTransformedObb
//
//=======================================================================
const Obb Aabb::GetTransformedObb(const Transform& transform) const
{
    /// \todo This can be optimized, by transforming directly from the Aabb without converting to a Obb first
    Obb temp = Obb::CreateFromAabb(*this);
    return transform * temp;
}

//==========================================================================
//
// Aabb - ApplyTransform
//
//==========================================================================
void
Aabb::ApplyTransform(const Transform& transform)
{
    //float    a, b;
    Vector3 a, b, col;

    Vector3 newMin, newMax;
    newMin = newMax = transform.GetTranslation();

    // Find extreme points by considering product of
    // min and max with each component of M.
    for (int j = 0; j < 3; j++)
    {
        col = transform.GetRowAsVector3(j);
        a  = col * m_min;
        b  = col * m_max;

        newMin.SetElement(j, newMin(j) + a.GetMin(b).Dot(Vector3::CreateOne()));
        newMax.SetElement(j, newMax(j) + a.GetMax(b).Dot(Vector3::CreateOne()));
    }

    m_min = newMin;
    m_max = newMax;
}

#endif // #ifndef AZ_UNITY_BUILD