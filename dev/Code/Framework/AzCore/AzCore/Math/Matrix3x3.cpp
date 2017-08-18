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

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Quaternion.h>

using namespace AZ;

void Matrix3x3::SetRotationPartFromQuaternion(const Quaternion& q)
{
    float   tx = q.GetX() * 2.0f;
    float   ty = q.GetY() * 2.0f;
    float   tz = q.GetZ() * 2.0f;
    float   twx = q.GetW() * tx;
    float   twy = q.GetW() * ty;
    float   twz = q.GetW() * tz;
    float   txx = q.GetX() * tx;
    float   txy = q.GetX() * ty;
    float   txz = q.GetX() * tz;
    float   tyy = q.GetY() * ty;
    float   tyz = q.GetY() * tz;
    float   tzz = q.GetZ() * tz;

    SetElement(0, 0, 1.0f - (tyy + tzz));    //1.0-2yy-2zz       2xy-2wz       2xz+2wy
    SetElement(0, 1, txy - twz);
    SetElement(0, 2, txz + twy);

    SetElement(1, 0, txy + twz);           //2xy+2wz   1.0-2xx-2zz       2yz-2wx
    SetElement(1, 1, 1.0f - (txx + tzz));
    SetElement(1, 2, tyz - twx);

    SetElement(2, 0, txz - twy);           //2xz-2wy       2yz+2wx   1.0-2xx-2yy
    SetElement(2, 1, tyz + twx);
    SetElement(2, 2, 1.0f - (txx + tyy));
}

const Matrix3x3 Matrix3x3::GetOrthogonalized() const
{
    Matrix3x3 out;
    out.SetRow(0, GetRow(1).Cross(GetRow(2)).GetNormalizedSafe());
    out.SetRow(1, GetRow(2).Cross(out.GetRow(0)).GetNormalizedSafe());
    out.SetRow(2, out.GetRow(0).Cross(out.GetRow(1)).GetNormalizedSafe());
    return out;
}

const Vector3 Matrix3x3::RetrieveScale() const
{
    return Vector3(GetBasisX().GetLength(), GetBasisY().GetLength(), GetBasisZ().GetLength());
}

const Vector3 Matrix3x3::ExtractScale()
{
    Vector3 x = GetBasisX();
    Vector3 y = GetBasisY();
    Vector3 z = GetBasisZ();
    VectorFloat lengthX = x.GetLength();
    VectorFloat lengthY = y.GetLength();
    VectorFloat lengthZ = z.GetLength();
    SetBasisX(x / lengthX);
    SetBasisY(y / lengthY);
    SetBasisZ(z / lengthZ);
    return Vector3(lengthX, lengthY, lengthZ);
}

void Matrix3x3::MultiplyByScale(const Vector3& scale)
{
    SetBasisX(scale.GetX() * GetBasisX());
    SetBasisY(scale.GetY() * GetBasisY());
    SetBasisZ(scale.GetZ() * GetBasisZ());
}

const Matrix3x3 Matrix3x3::GetPolarDecomposition() const
{
    const float precision = 0.00001f;
    const float epsilon = 0.0000001f;
    const int maxIterations = 16;
    Matrix3x3 u = (*this);// / GetDiagonal().GetLength(); - NR: this was failing, diagonal could be zero and still a valid matrix! Not sure where this came from originally...
    float det = u.GetDeterminant();
    if (det * det > epsilon)
    {
        for (int i = 0; i < maxIterations; ++i)
        {
            //already have determinant, so can use adjugate/det to get inverse
            u = 0.5f * (u + (u.GetAdjugate() / det).GetTranspose());
            float newDet = u.GetDeterminant();
            float diff = newDet - det;
            if (diff * diff < precision)
            {
                break;
            }
            det = newDet;
        }
        u = u.GetOrthogonalized();
    }
    else
    {
        u = Matrix3x3::CreateIdentity();
    }

    return u;
}

void Matrix3x3::GetPolarDecomposition(Matrix3x3* orthogonalOut, Matrix3x3* symmetricOut) const
{
    *orthogonalOut = GetPolarDecomposition();
    *symmetricOut = orthogonalOut->TransposedMultiply(*this);
    //could also refine symmetricOut further, by taking H = 0.5*(H + H^t)
}

bool Matrix3x3::IsOrthogonal(const VectorFloat& tolerance) const
{
    VectorFloat one = VectorFloat::CreateOne();
    VectorFloat zero = VectorFloat::CreateZero();
    VectorFloat toleranceSq = tolerance * tolerance;
    if (!GetRow(0).GetLengthSq().IsClose(one, toleranceSq) ||
        !GetRow(1).GetLengthSq().IsClose(one, toleranceSq) ||
        !GetRow(2).GetLengthSq().IsClose(one, toleranceSq))
    {
        return false;
    }
    if (!GetRow(0).Dot(GetRow(1)).IsClose(zero, tolerance) ||
        !GetRow(0).Dot(GetRow(2)).IsClose(zero, tolerance) ||
        !GetRow(1).Dot(GetRow(2)).IsClose(zero, tolerance))
    {
        return false;
    }
    return true;
}

#endif // #ifndef AZ_UNITY_BUILD