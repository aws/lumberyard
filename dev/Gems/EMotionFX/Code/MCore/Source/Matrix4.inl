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

MCORE_INLINE Matrix::Matrix(const Matrix& m)
{
#ifdef MCORE_PLATFORM_WII
    PSMTX44Copy(m.m44, m44);
#else
    MCore::MemCopy(m16, m.m16, sizeof(Matrix));
#endif
}


MCORE_INLINE void Matrix::operator = (const Matrix& right)
{
#ifdef MCORE_PLATFORM_WII
    PSMTX44Copy(right.m44, m44);
#else
    MCore::MemCopy(m16, right.m16, sizeof(Matrix));
#endif
}



MCORE_INLINE void Matrix::SetRight(float xx, float xy, float xz)
{
    TMAT(0, 0) = xx;
    TMAT(0, 1) = xy;
    TMAT(0, 2) = xz;
}



MCORE_INLINE void Matrix::SetUp(float yx, float yy, float yz)
{
    TMAT(1, 0) = yx;
    TMAT(1, 1) = yy;
    TMAT(1, 2) = yz;
}



MCORE_INLINE void Matrix::SetForward(float zx, float zy, float zz)
{
    TMAT(2, 0) = zx;
    TMAT(2, 1) = zy;
    TMAT(2, 2) = zz;
}



MCORE_INLINE void Matrix::SetTranslation(float tx, float ty, float tz)
{
    TMAT(3, 0) = tx;
    TMAT(3, 1) = ty;
    TMAT(3, 2) = tz;
}



MCORE_INLINE void Matrix::SetRight(const Vector3& x)
{
    TMAT(0, 0) = x.x;
    TMAT(0, 1) = x.y;
    TMAT(0, 2) = x.z;
}



MCORE_INLINE void Matrix::SetUp(const Vector3& y)
{
    TMAT(1, 0) = y.x;
    TMAT(1, 1) = y.y;
    TMAT(1, 2) = y.z;
}



MCORE_INLINE void Matrix::SetForward(const Vector3& z)
{
    TMAT(2, 0) = z.x;
    TMAT(2, 1) = z.y;
    TMAT(2, 2) = z.z;
}



MCORE_INLINE void Matrix::SetTranslation(const Vector3& t)
{
    TMAT(3, 0) = t.x;
    TMAT(3, 1) = t.y;
    TMAT(3, 2) = t.z;
}



MCORE_INLINE Vector3 Matrix::GetRight() const
{
    //return *reinterpret_cast<Vector3*>( m16/* + 0*/);
    return Vector3(TMAT(0, 0), TMAT(0, 1), TMAT(0, 2));
}



MCORE_INLINE Vector3 Matrix::GetForward() const
{
    //  return *reinterpret_cast<Vector3*>(m16+4);
    return Vector3(TMAT(1, 0), TMAT(1, 1), TMAT(1, 2));
}



MCORE_INLINE Vector3 Matrix::GetUp() const
{
    //  return *reinterpret_cast<Vector3*>(m16+8);
    return Vector3(TMAT(2, 0), TMAT(2, 1), TMAT(2, 2));
}



MCORE_INLINE Vector3 Matrix::GetTranslation() const
{
    //return *reinterpret_cast<Vector3*>(m16+12);
    return Vector3(TMAT(3, 0), TMAT(3, 1), TMAT(3, 2));
}



MCORE_INLINE Vector3 Matrix::Mul3x3(const Vector3& v) const
{
    return Vector3(
        v.x * TMAT(0, 0) + v.y * TMAT(1, 0) + v.z * TMAT(2, 0),
        v.x * TMAT(0, 1) + v.y * TMAT(1, 1) + v.z * TMAT(2, 1),
        v.x * TMAT(0, 2) + v.y * TMAT(1, 2) + v.z * TMAT(2, 2));
}



MCORE_INLINE void operator *= (Vector3& v, const Matrix& m)
{
    v = Vector3(
            v.x * MMAT(m, 0, 0) + v.y * MMAT(m, 1, 0) + v.z * MMAT(m, 2, 0) + MMAT(m, 3, 0),
            v.x * MMAT(m, 0, 1) + v.y * MMAT(m, 1, 1) + v.z * MMAT(m, 2, 1) + MMAT(m, 3, 1),
            v.x * MMAT(m, 0, 2) + v.y * MMAT(m, 1, 2) + v.z * MMAT(m, 2, 2) + MMAT(m, 3, 2));
}



MCORE_INLINE void operator *= (AZ::Vector4& v, const Matrix& m)
{
    v = AZ::Vector4(
            v.GetX() * MMAT(m, 0, 0) + v.GetY() * MMAT(m, 1, 0) + v.GetZ() * MMAT(m, 2, 0) + v.GetW() * MMAT(m, 3, 0),
            v.GetX() * MMAT(m, 0, 1) + v.GetY() * MMAT(m, 1, 1) + v.GetZ() * MMAT(m, 2, 1) + v.GetW() * MMAT(m, 3, 1),
            v.GetX() * MMAT(m, 0, 2) + v.GetY() * MMAT(m, 1, 2) + v.GetZ() * MMAT(m, 2, 2) + v.GetW() * MMAT(m, 3, 2),
            v.GetX() * MMAT(m, 0, 3) + v.GetY() * MMAT(m, 1, 3) + v.GetZ() * MMAT(m, 2, 3) + v.GetW() * MMAT(m, 3, 3));
}



MCORE_INLINE Vector3 operator * (const Vector3& v, const Matrix& m)
{
    return Vector3(
        v.x * MMAT(m, 0, 0) + v.y * MMAT(m, 1, 0) + v.z * MMAT(m, 2, 0) + MMAT(m, 3, 0),
        v.x * MMAT(m, 0, 1) + v.y * MMAT(m, 1, 1) + v.z * MMAT(m, 2, 1) + MMAT(m, 3, 1),
        v.x * MMAT(m, 0, 2) + v.y * MMAT(m, 1, 2) + v.z * MMAT(m, 2, 2) + MMAT(m, 3, 2));
}




// skin a vertex position
MCORE_INLINE void Matrix::Skin4x3(const MCore::Vector3& in, Vector3& out, float weight)
{
    out.x += (in.x * TMAT(0, 0) + in.y * TMAT(1, 0) + in.z * TMAT(2, 0) + TMAT(3, 0)) * weight;
    out.y += (in.x * TMAT(0, 1) + in.y * TMAT(1, 1) + in.z * TMAT(2, 1) + TMAT(3, 1)) * weight;
    out.z += (in.x * TMAT(0, 2) + in.y * TMAT(1, 2) + in.z * TMAT(2, 2) + TMAT(3, 2)) * weight;
}


// skin a position and normal
MCORE_INLINE void Matrix::Skin(const Vector3* inPos, const Vector3* inNormal, Vector3* outPos, Vector3* outNormal, float weight)
{
    const float mat00 = TMAT(0, 0);
    const float mat10 = TMAT(1, 0);
    const float mat20 = TMAT(2, 0);
    const float mat30 = TMAT(3, 0);
    const float mat01 = TMAT(0, 1);
    const float mat11 = TMAT(1, 1);
    const float mat21 = TMAT(2, 1);
    const float mat31 = TMAT(3, 1);
    const float mat02 = TMAT(0, 2);
    const float mat12 = TMAT(1, 2);
    const float mat22 = TMAT(2, 2);
    const float mat32 = TMAT(3, 2);

    outPos->x += (inPos->x * mat00 + inPos->y * mat10 + inPos->z * mat20 + mat30) * weight;
    outPos->y += (inPos->x * mat01 + inPos->y * mat11 + inPos->z * mat21 + mat31) * weight;
    outPos->z += (inPos->x * mat02 + inPos->y * mat12 + inPos->z * mat22 + mat32) * weight;

    outNormal->x += (inNormal->x * mat00 + inNormal->y * mat10 + inNormal->z * mat20) * weight;
    outNormal->y += (inNormal->x * mat01 + inNormal->y * mat11 + inNormal->z * mat21) * weight;
    outNormal->z += (inNormal->x * mat02 + inNormal->y * mat12 + inNormal->z * mat22) * weight;
}


// skin a position, normal, and tangent
MCORE_INLINE void Matrix::Skin(const Vector3* inPos, const Vector3* inNormal, const AZ::Vector4* inTangent, Vector3* outPos, Vector3* outNormal, AZ::Vector4* outTangent, float weight)
{
    const float mat00 = TMAT(0, 0);
    const float mat10 = TMAT(1, 0);
    const float mat20 = TMAT(2, 0);
    const float mat30 = TMAT(3, 0);
    const float mat01 = TMAT(0, 1);
    const float mat11 = TMAT(1, 1);
    const float mat21 = TMAT(2, 1);
    const float mat31 = TMAT(3, 1);
    const float mat02 = TMAT(0, 2);
    const float mat12 = TMAT(1, 2);
    const float mat22 = TMAT(2, 2);
    const float mat32 = TMAT(3, 2);

    outPos->x += (inPos->x * mat00 + inPos->y * mat10 + inPos->z * mat20 + mat30) * weight;
    outPos->y += (inPos->x * mat01 + inPos->y * mat11 + inPos->z * mat21 + mat31) * weight;
    outPos->z += (inPos->x * mat02 + inPos->y * mat12 + inPos->z * mat22 + mat32) * weight;

    outNormal->x += (inNormal->x * mat00 + inNormal->y * mat10 + inNormal->z * mat20) * weight;
    outNormal->y += (inNormal->x * mat01 + inNormal->y * mat11 + inNormal->z * mat21) * weight;
    outNormal->z += (inNormal->x * mat02 + inNormal->y * mat12 + inNormal->z * mat22) * weight;

    outTangent->SetX(outTangent->GetX() + (inTangent->GetX() * mat00 + inTangent->GetY() * mat10 + inTangent->GetZ() * mat20) * weight);
    outTangent->SetY(outTangent->GetY() + (inTangent->GetX() * mat01 + inTangent->GetY() * mat11 + inTangent->GetZ() * mat21) * weight);
    outTangent->SetZ(outTangent->GetZ() + (inTangent->GetX() * mat02 + inTangent->GetY() * mat12 + inTangent->GetZ() * mat22) * weight);
    //outTangent->w = inTangent->w;
}


// skin a normal
MCORE_INLINE void Matrix::Skin3x3(const MCore::Vector3& in, Vector3& out, float weight)
{
    out.x += (in.x * TMAT(0, 0) + in.y * TMAT(1, 0) + in.z * TMAT(2, 0)) * weight;
    out.y += (in.x * TMAT(0, 1) + in.y * TMAT(1, 1) + in.z * TMAT(2, 1)) * weight;
    out.z += (in.x * TMAT(0, 2) + in.y * TMAT(1, 2) + in.z * TMAT(2, 2)) * weight;
}


// multiply by a float
MCORE_INLINE Matrix& Matrix::operator *= (float value)
{
    for (uint32 i = 0; i < 16; ++i)
    {
        m16[i] *= value;
    }

    return *this;
}


// scale (uniform)
MCORE_INLINE void Matrix::Scale(const Vector3& scale)
{
    for (uint32 i = 0; i < 4; ++i)
    {
        TMAT(i, 0) *= scale.x;
        TMAT(i, 1) *= scale.y;
        TMAT(i, 2) *= scale.z;
    }
}


// returns a normalized version of this matrix
Matrix Matrix::Normalized() const
{
    Matrix result(*this);
    result.Normalize();
    return result;
}



