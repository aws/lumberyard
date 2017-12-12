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

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/MathUtils.h>

using namespace AZ;

const Vector3 Vector3::Slerp(const Vector3& dest, const VectorFloat& t) const
{
    // Dot product - the cosine of the angle between 2 vectors.
    // Clamp it to be in the range of Acos()
    // This may be unnecessary, but floating point
    // precision can be a fickle mistress.
    float dot = AZ::GetClamp<float>(dest.Dot(*this), -1.0f, 1.0f);
    // Acos(dot) returns the angle between start and end,
    // And multiplying that by percent returns the angle between
    // start and the final result.
    float theta = acosf(dot) * t;
    Vector3 relativeVec = (dest - (*this) * dot).GetNormalizedSafe(); // Orthonormal basis
    // The final result.
    return (((*this) * cosf(theta)) + (relativeVec * sinf(theta)));
}

void Vector3::BuildTangentBasis(Vector3& tangent, Vector3& bitangent) const
{
    AZ_Assert(IsNormalized(), "BuildTangentBasis needs normalized vectors");

    if (fabsf(GetX()) < 0.9f)
    {
        tangent = CrossXAxis();
    }
    else
    {
        tangent = CrossYAxis();
    }
    tangent.Normalize();
    bitangent = Cross(tangent);
    bitangent.Normalize();
}

#endif // #ifndef AZ_UNITY_BUILD