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

// float
template <>
MCORE_INLINE bool Compare<float>::CheckIfIsClose(const float& a, const float& b, float threshold)
{
    return (Math::Abs(a - b) <= threshold);
}


// Vector2
template <>
MCORE_INLINE bool Compare<AZ::Vector2>::CheckIfIsClose(const AZ::Vector2& a, const AZ::Vector2& b, float threshold)
{
    return ((a - b).GetLengthSq() <= threshold);
}


// Vector3
template <>
MCORE_INLINE bool Compare<Vector3>::CheckIfIsClose(const Vector3& a, const Vector3& b, float threshold)
{
    return ((a - b).SquareLength() <= threshold);
}


// Vector4
template <>
MCORE_INLINE bool Compare<AZ::Vector4>::CheckIfIsClose(const AZ::Vector4& a, const AZ::Vector4& b, float threshold)
{
    return ((a - b).GetLengthSq() <= threshold);
}


// Matrix
template <>
MCORE_INLINE bool Compare<Matrix>::CheckIfIsClose(const Matrix& a, const Matrix& b, float threshold)
{
    for (uint32 i = 0; i < 3; ++i)
    {
        if ((a.GetRow4D(i) - b.GetRow4D(i)).GetLengthSq() > threshold)
        {
            return false;
        }
    }
    return true;
}


// Quaternion
template <>
MCORE_INLINE bool Compare<MCore::Quaternion>::CheckIfIsClose(const MCore::Quaternion& a, const MCore::Quaternion& b, float threshold)
{
    /*
    if (a.Dot(b) < 0.0f)
        return Compare<Vector4>::CheckIfIsClose( Vector4(a.x, a.y, a.z, a.w), Vector4(-b.x, -b.y, -b.z, -b.w), threshold);
    else
        return Compare<Vector4>::CheckIfIsClose( Vector4(a.x, a.y, a.z, a.w), Vector4(b.x, b.y, b.z, b.w), threshold);
        */

    Vector3 axisA, axisB;
    float   angleA, angleB;

    // convert to an axis and angle representation
    a.ToAxisAngle(&axisA, &angleA);
    b.ToAxisAngle(&axisB, &angleB);

    // compare the axis and angles
    if (Math::Abs(angleA  - angleB) > threshold)
    {
        return false;
    }
    if (Math::Abs(axisA.x - axisB.x) > threshold)
    {
        return false;
    }
    if (Math::Abs(axisA.y - axisB.y) > threshold)
    {
        return false;
    }
    if (Math::Abs(axisA.z - axisB.z) > threshold)
    {
        return false;
    }

    // they are the same!
    return true;
}
