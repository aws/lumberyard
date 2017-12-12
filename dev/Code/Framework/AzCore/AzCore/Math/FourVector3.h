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
#ifndef AZCORE_MATH_FOURVECTOR3_H
#define AZCORE_MATH_FOURVECTOR3_H 1

namespace AZ
{
    /**
     * Contains 4 separate Vector3's, in structure-of-arrays form to fully leverage 4 way SIMD instructions.
     */
    class FourVector3
    {
    public:
        explicit FourVector3(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector3& v3);
        explicit FourVector3(const Vector4& x, const Vector4& y, const Vector4& z);
        AZ_MATH_FORCE_INLINE const FourVector3 operator+(const FourVector3& rhs) const
        {
            return FourVector3(m_x + rhs.m_x, m_y + rhs.m_y, m_z + rhs.m_z);
        }
    private:
        Vector4 m_x;
        Vector4 m_y;
        Vector4 m_z;
    };
}

#endif
#pragma once