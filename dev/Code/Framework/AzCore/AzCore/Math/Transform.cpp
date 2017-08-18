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

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Matrix3x3.h>

using namespace AZ;

//////////////////////////////////////////////////////////////////////////
// Global identity transform
const Transform AZ::g_transformIdentity = Transform::CreateIdentity();
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// Transform from Matrix3x3
// [8/21/2008]
//=========================================================================
const Transform Transform::CreateFromMatrix3x3(const Matrix3x3& value)
{
    Transform result;
    for (int i = 0; i < 3; ++i)
    {
        result.SetRow(i, value.GetRow(i), 0.0f);
    }
    return result;
}

const Transform Transform::CreateFromMatrix3x3AndTranslation(const class Matrix3x3& value, const Vector3& p)
{
    Transform result;
    for (int i = 0; i < 3; ++i)
    {
        result.SetRow(i, value.GetRow(i), 0.0f);
    }
    result.SetPosition(p);
    return result;
}

void Transform::SetRotationPartFromQuaternion(const Quaternion& q)
{
    VectorFloat one = VectorFloat::CreateOne();
    VectorFloat two = VectorFloat(2.0f);
    VectorFloat tx = q.GetX() * two;
    VectorFloat ty = q.GetY() * two;
    VectorFloat tz = q.GetZ() * two;
    VectorFloat twx = q.GetW() * tx;
    VectorFloat twy = q.GetW() * ty;
    VectorFloat twz = q.GetW() * tz;
    VectorFloat txx = q.GetX() * tx;
    VectorFloat txy = q.GetX() * ty;
    VectorFloat txz = q.GetX() * tz;
    VectorFloat tyy = q.GetY() * ty;
    VectorFloat tyz = q.GetY() * tz;
    VectorFloat tzz = q.GetZ() * tz;

    SetElement(0, 0, one - (tyy + tzz)); //1.0-2yy-2zz  2xy-2wz   2xz+2wy
    SetElement(0, 1, txy - twz);
    SetElement(0, 2, txz + twy);

    SetElement(1, 0, txy + twz);       //2xy+2wz   1.0-2xx-2zz  2yz-2wx
    SetElement(1, 1, one - (txx + tzz));
    SetElement(1, 2, tyz - twx);

    SetElement(2, 0, txz - twy);       //2xz-2wy   2yz+2wx   1.0-2xx-2yy
    SetElement(2, 1, tyz + twx);
    SetElement(2, 2, one - (txx + tyy));
}

/**
*
*/
const Transform Transform::GetInverseFull() const
{
    Transform out;

    /* compute matrix of cofactors */
    /*    c  r */
    out.SetElement(0, 0, ((*this)(1, 1) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 1)));
    out.SetElement(1, 0, ((*this)(1, 2) * (*this)(2, 0) - (*this)(1, 0) * (*this)(2, 2)));
    out.SetElement(2, 0, ((*this)(1, 0) * (*this)(2, 1) - (*this)(1, 1) * (*this)(2, 0)));

    out.SetElement(0, 1, ((*this)(2, 1) * (*this)(0, 2) - (*this)(2, 2) * (*this)(0, 1)));
    out.SetElement(1, 1, ((*this)(2, 2) * (*this)(0, 0) - (*this)(2, 0) * (*this)(0, 2)));
    out.SetElement(2, 1, ((*this)(2, 0) * (*this)(0, 1) - (*this)(2, 1) * (*this)(0, 0)));

    out.SetElement(0, 2, ((*this)(0, 1) * (*this)(1, 2) - (*this)(0, 2) * (*this)(1, 1)));
    out.SetElement(1, 2, ((*this)(0, 2) * (*this)(1, 0) - (*this)(0, 0) * (*this)(1, 2)));
    out.SetElement(2, 2, ((*this)(0, 0) * (*this)(1, 1) - (*this)(0, 1) * (*this)(1, 0)));

    // Calculate the determinant
    VectorFloat det = (*this)(0, 0) * out(0, 0) + (*this)(1, 0) * out(0, 1) + (*this)(2, 0) * out(0, 2);

    // divide cofactors by determinant
    VectorFloat f = VectorFloat::CreateSelectCmpEqual(det, VectorFloat::CreateZero(), VectorFloat(10000000.0f), det.GetReciprocalExact());

    out.SetElement(0, 0, out(0, 0) * f);
    out.SetElement(0, 1, out(0, 1) * f);
    out.SetElement(0, 2, out(0, 2) * f);
    out.SetElement(1, 0, out(1, 0) * f);
    out.SetElement(1, 1, out(1, 1) * f);
    out.SetElement(1, 2, out(1, 2) * f);
    out.SetElement(2, 0, out(2, 0) * f);
    out.SetElement(2, 1, out(2, 1) * f);
    out.SetElement(2, 2, out(2, 2) * f);

    // Translation
    out.SetElement(0, 3, -((*this)(0, 3) * out(0, 0) + (*this)(1, 3) * out(0, 1) + (*this)(2, 3) * out(0, 2)));
    out.SetElement(1, 3, -((*this)(0, 3) * out(1, 0) + (*this)(1, 3) * out(1, 1) + (*this)(2, 3) * out(1, 2)));
    out.SetElement(2, 3, -((*this)(0, 3) * out(2, 0) + (*this)(1, 3) * out(2, 1) + (*this)(2, 3) * out(2, 2)));

    return out;
}

const Vector3 Transform::RetrieveScale() const
{
    return Vector3(GetBasisX().GetLength(), GetBasisY().GetLength(), GetBasisZ().GetLength());
}

const Vector3 Transform::RetrieveScaleExact() const
{
    return Vector3(GetBasisX().GetLengthExact(), GetBasisY().GetLengthExact(), GetBasisZ().GetLengthExact());
}

const Vector3 Transform::ExtractScale()
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

const Vector3 Transform::ExtractScaleExact()
{
    Vector3 x = GetBasisX();
    Vector3 y = GetBasisY();
    Vector3 z = GetBasisZ();
    VectorFloat lengthX = x.GetLengthExact();
    VectorFloat lengthY = y.GetLengthExact();
    VectorFloat lengthZ = z.GetLengthExact();
    SetBasisX(x / lengthX);
    SetBasisY(y / lengthY);
    SetBasisZ(z / lengthZ);
    return Vector3(lengthX, lengthY, lengthZ);
}


void Transform::MultiplyByScale(const Vector3& scale)
{
    SetBasisX(scale.GetX() * GetBasisX());
    SetBasisY(scale.GetY() * GetBasisY());
    SetBasisZ(scale.GetZ() * GetBasisZ());
}

const Transform Transform::GetPolarDecomposition() const
{
    Matrix3x3 m = Matrix3x3::CreateFromTransform(*this);
    Matrix3x3 d = m.GetPolarDecomposition();
    Transform result = Transform::CreateFromMatrix3x3(d);
    result.SetTranslation(GetTranslation());
    return result;
}

void Transform::GetPolarDecomposition(Transform* orthogonalOut, Transform* symmetricOut) const
{
    Matrix3x3 m = Matrix3x3::CreateFromTransform(*this);
    Matrix3x3 om, sm;
    m.GetPolarDecomposition(&om, &sm);
    *orthogonalOut = Transform::CreateFromMatrix3x3(om);
    orthogonalOut->SetTranslation(GetTranslation());
    *symmetricOut = Transform::CreateFromMatrix3x3(sm);
}

bool Transform::IsOrthogonal(const VectorFloat& tolerance) const
{
    VectorFloat one = VectorFloat::CreateOne();
    VectorFloat zero = VectorFloat::CreateZero();
    if (!GetRowAsVector3(0).Dot(GetRowAsVector3(0)).IsClose(one, tolerance) ||
        !GetRowAsVector3(1).Dot(GetRowAsVector3(1)).IsClose(one, tolerance) ||
        !GetRowAsVector3(2).Dot(GetRowAsVector3(2)).IsClose(one, tolerance) ||
        !GetRowAsVector3(0).Dot(GetRowAsVector3(1)).IsClose(zero, tolerance) ||
        !GetRowAsVector3(0).Dot(GetRowAsVector3(2)).IsClose(zero, tolerance) ||
        !GetRowAsVector3(1).Dot(GetRowAsVector3(2)).IsClose(zero, tolerance))
    {
        return false;
    }
    return true;
}

const Transform Transform::GetOrthogonalized() const
{
    Transform out;
    Vector3 translation = GetTranslation();
    out.SetRow(0, GetRowAsVector3(1).Cross(GetRowAsVector3(2)).GetNormalizedSafe(), translation.GetX());
    out.SetRow(1, GetRowAsVector3(2).Cross(out.GetRowAsVector3(0)).GetNormalizedSafe(), translation.GetY());
    out.SetRow(2, out.GetRowAsVector3(0).Cross(out.GetRowAsVector3(1)).GetNormalizedSafe(), translation.GetZ());
    return out;
}

const VectorFloat Transform::GetDeterminant3x3() const
{
    return Matrix3x3::CreateFromTransform(*this).GetDeterminant();
}

#endif // #ifndef AZ_UNITY_BUILD