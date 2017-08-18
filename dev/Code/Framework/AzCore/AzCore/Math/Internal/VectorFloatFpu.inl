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

#include <AzCore/Math/MathUtils.h>
#include <math.h>

namespace AZ
{
    AZ_MATH_FORCE_INLINE VectorFloat::VectorFloat(float value)
        : m_x(value) { }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::CreateZero()            { return VectorFloat(0.0f); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::CreateOne()         { return VectorFloat(1.0f); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::CreateFromFloat(const float* ptr) { return VectorFloat(*ptr); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::CreateSelectCmpEqual(const VectorFloat& cmp1, const VectorFloat& cmp2, const VectorFloat& vA, const VectorFloat& vB)                { return (cmp1.m_x == cmp2.m_x) ? vA.m_x : vB.m_x; }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::CreateSelectCmpGreaterEqual(const VectorFloat& cmp1, const VectorFloat& cmp2, const VectorFloat& vA, const VectorFloat& vB)     { return (cmp1.m_x >= cmp2.m_x) ? vA.m_x : vB.m_x; }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::CreateSelectCmpGreater(const VectorFloat& cmp1, const VectorFloat& cmp2, const VectorFloat& vA, const VectorFloat& vB)          { return (cmp1.m_x > cmp2.m_x) ? vA.m_x : vB.m_x; }

    AZ_MATH_FORCE_INLINE VectorFloat::operator float() const                    { return m_x; }

    AZ_MATH_FORCE_INLINE void VectorFloat::StoreToFloat(float* ptr) const { *ptr = m_x; }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::operator-() const                           { return -m_x; }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::operator+(const VectorFloat& rhs) const { return m_x + rhs.m_x; }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::operator-(const VectorFloat& rhs) const { return m_x - rhs.m_x; }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::operator*(const VectorFloat& rhs) const { return m_x * rhs.m_x; }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::operator/(const VectorFloat& rhs) const { return m_x / rhs.m_x; }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetMin(const VectorFloat& v) const      { return AZ::GetMin(m_x, v.m_x); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetMax(const VectorFloat& v) const      { return AZ::GetMax(m_x, v.m_x); }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetAngleMod() const { return (m_x >= 0.0f) ? (fmodf(m_x + Constants::Pi, Constants::TwoPi) - Constants::Pi) : (fmodf(m_x - Constants::Pi, Constants::TwoPi) + Constants::Pi); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetSin() const      { return sinf(m_x); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetCos() const      { return cosf(m_x); }
    AZ_MATH_FORCE_INLINE void VectorFloat::GetSinCos(VectorFloat& sin, VectorFloat& cos) const  { sin = sinf(m_x); cos = cosf(m_x); }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetAbs() const          { return VectorFloat(fabsf(m_x)); }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetReciprocal() const       { return VectorFloat(1.0f / m_x); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetReciprocalApprox() const { return VectorFloat(1.0f / m_x); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetReciprocalExact() const  { return VectorFloat(1.0f / m_x); }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetSqrt() const       { return VectorFloat(sqrtf(m_x)); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetSqrtApprox() const { return VectorFloat(sqrtf(m_x)); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetSqrtExact() const  { return VectorFloat(sqrtf(m_x)); }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetSqrtReciprocal() const       { return VectorFloat(1.0f / sqrtf(m_x)); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetSqrtReciprocalApprox() const { return VectorFloat(1.0f / sqrtf(m_x)); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetSqrtReciprocalExact() const  { return VectorFloat(1.0f / sqrtf(m_x)); }

    AZ_MATH_FORCE_INLINE bool VectorFloat::operator==(const VectorFloat& rhs) const         { return (m_x == rhs.m_x); }
    AZ_MATH_FORCE_INLINE bool VectorFloat::operator!=(const VectorFloat& rhs) const         { return (m_x != rhs.m_x); }
    AZ_MATH_FORCE_INLINE bool VectorFloat::IsLessThan(const VectorFloat& rhs) const         { return (m_x < rhs.m_x); }
    AZ_MATH_FORCE_INLINE bool VectorFloat::IsLessEqualThan(const VectorFloat& rhs) const        { return (m_x <= rhs.m_x); }
    AZ_MATH_FORCE_INLINE bool VectorFloat::IsGreaterThan(const VectorFloat& rhs) const      { return (m_x > rhs.m_x); }
    AZ_MATH_FORCE_INLINE bool VectorFloat::IsGreaterEqualThan(const VectorFloat& rhs) const { return (m_x >= rhs.m_x); }
}
