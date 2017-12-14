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
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Quaternion.h>

using namespace AZ;

void Matrix4x4::SetRotationPartFromQuaternion(const Quaternion& q)
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

/**
*
*/
const Matrix4x4 Matrix4x4::CreateProjection(float fovY, float aspectRatio, float nearDist, float farDist)
{
    //Some notes about the camera matrices and stuff, because this is usually confused. Our camera space
    // has the camera looking down the *positive* z-axis, the x-axis points towards the left of the screen,
    // and the y-axis points towards the top of the screen.
    //
    // The projection matrix transforms vectors in camera space into clip space. In clip space, x=-1 is the
    // left of the screen, x=+1 is the right of the screen, y=-1 is the bottom of the screen, y=+1 is the top
    // of the screen. Points at the near plane are mapped to z=-1, and points at the far plane are mapped to
    // z=+1. There is also an alternative projection matrix below which maps to the z=0,1 range instead, which
    // is used by Direct3D.
    //
    // The relationship between fovX and fovY is non-linear. If we used a linear relationship when constructing
    // the projection matrix there would be some slight squashing. An alternative correct method which doesn't use
    // the fov constructs the projection using the screen window size projected onto the near plane. I prefer the fov
    // method since it seems more natural, especially when it comes to constructing the frustum.
    //
    float cotY = cosf(0.5f * fovY) / sinf(0.5f * fovY);
    float cotX = cotY / aspectRatio;//cosf(0.5f*fovX) / sinf(0.5f*fovX);
    return CreateProjectionInternal(cotX, cotY, nearDist, farDist);
}

const Matrix4x4 Matrix4x4::CreateProjectionFov(float fovX, float fovY, float nearDist, float farDist)
{
    //note that the relationship between fovX and fovY is not linear with the aspect ratio, so prefer the
    // aspect ratio version of the projection matrix, to avoid unwanted stretching, unless you really know both
    // FOV's exactly
    float cotX = cosf(0.5f * fovX) / sinf(0.5f * fovX);
    float cotY = cosf(0.5f * fovY) / sinf(0.5f * fovY);
    return CreateProjectionInternal(cotX, cotY, nearDist, farDist);
}

const Matrix4x4 Matrix4x4::CreateProjectionInternal(float cotX, float cotY, float nearDist, float farDist)
{
    AZ_Assert(nearDist > 0.0f, "Near plane distance must be greater than 0");
    AZ_Assert(farDist > nearDist, "Far plane distance must be greater than near plane distance");

    //construct projection matrix
    Matrix4x4 result;
    float invfn = 1.0f / (farDist - nearDist);
#if defined(RENDER_WINNT_DIRECTX)
    //this matrix maps the z values into the range [0,1]
    result.SetRow(0, -cotX, 0.0f, 0.0f,         0.0f);
    result.SetRow(1, 0.0f, cotY, 0.0f,          0.0f);
    result.SetRow(2, 0.0f, 0.0f, farDist * invfn, -farDist * nearDist * invfn);
    result.SetRow(3, 0.0f, 0.0f, 1.0f,          0.0f);
#else
    //this matrix maps the z values into the range [-1,1]
    result.SetRow(0, -cotX, 0.0f, 0.0f,                     0.0f);
    result.SetRow(1, 0.0f, cotY, 0.0f,                      0.0f);
    result.SetRow(2, 0.0f, 0.0f, (farDist + nearDist) * invfn,  -2.0f * farDist * nearDist * invfn);
    result.SetRow(3, 0.0f, 0.0f, 1.0f,                      0.0f);
#endif
    return result;
}

/**
 * Set the projection matrix with a volume offset
 */
const Matrix4x4 Matrix4x4::CreateProjectionOffset(float left, float right, float bottom, float top, float nearDist, float farDist)
{
    AZ_Assert(nearDist > 0.0f, "Near plane distance must be greater than 0");
    AZ_Assert(farDist > nearDist, "Far plane distance must be greater than near plane distance");

    //construct projection matrix
    Matrix4x4 result;
    float invfn = 1.0f / (farDist - nearDist);
#if defined(RENDER_WINNT_DIRECTX)
    //this matrix maps the z values into the range [0,1]
    result.SetRow(0, -2.0f * nearDist / (right - left),   0.0f,                       (left + right) / (left - right),  0.0f);
    result.SetRow(1, 0.0f,                          2.0f * nearDist / (top - bottom), (top + bottom) / (bottom - top),  0.0f);
    result.SetRow(2, 0.0f,                          0.0f,                       farDist * invfn,              -farDist * nearDist * invfn);
    result.SetRow(3, 0.0f,                          0.0f,                       1.0f,                       0.0f);
#else
    //this matrix maps the z values into the range [-1,1]
    result.SetRow(0, -2.0f * nearDist / (right - left),   0.0f,                       (left + right) / (left - right),  0.0f);
    result.SetRow(1, 0.0f,                          2.0f * nearDist / (top - bottom), (top + bottom) / (bottom - top),  0.0f);
    result.SetRow(2, 0.0f,                          0.0f,                       (farDist + nearDist) * invfn,   -2.0f * farDist * nearDist * invfn);
    result.SetRow(3, 0.0f,                          0.0f,                       1.0f,                       0.0f);
#endif
    return result;
}

/**
 *
 */
const Matrix4x4 Matrix4x4::GetInverseTransform() const
{
    Matrix4x4 out;

    AZ_Assert((*this)(3, 0) == VectorFloat::CreateZero() && (*this)(3, 1) == VectorFloat::CreateZero() && (*this)(3, 2) == VectorFloat::CreateZero() && (*this)(3, 3) == VectorFloat::CreateOne(),
        "For this matrix you sould use GetInverseFull");

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

    /* Calculate the determinant */
    VectorFloat det = (*this)(0, 0) * out(0, 0) + (*this)(1, 0) * out(0, 1) + (*this)(2, 0) * out(0, 2);

    /* divide cofactors by determinant */
    VectorFloat f = VectorFloat::CreateSelectCmpEqual(det, VectorFloat::CreateZero(), VectorFloat(10000000.0f), det.GetReciprocal());

    out.SetElement(0, 0, out(0, 0) * f);
    out.SetElement(0, 1, out(0, 1) * f);
    out.SetElement(0, 2, out(0, 2) * f);
    out.SetElement(1, 0, out(1, 0) * f);
    out.SetElement(1, 1, out(1, 1) * f);
    out.SetElement(1, 2, out(1, 2) * f);
    out.SetElement(2, 0, out(2, 0) * f);
    out.SetElement(2, 1, out(2, 1) * f);
    out.SetElement(2, 2, out(2, 2) * f);

    /* Translation */
    out.SetElement(0, 3, -((*this)(0, 3) * out(0, 0) + (*this)(1, 3) * out(0, 1) + (*this)(2, 3) * out(0, 2)));
    out.SetElement(1, 3, -((*this)(0, 3) * out(1, 0) + (*this)(1, 3) * out(1, 1) + (*this)(2, 3) * out(1, 2)));
    out.SetElement(2, 3, -((*this)(0, 3) * out(2, 0) + (*this)(1, 3) * out(2, 1) + (*this)(2, 3) * out(2, 2)));

    /* The remaining parts of the 4x4 matrix */
    out.SetRow(3, 0.0f, 0.0f, 0.0f, 1.0f);

    return out;
}

/**
 *
 *
 */
const Matrix4x4 Matrix4x4::GetInverseFull() const
{
    float det;
    float d12, d13, d23, d24, d34, d41;
    Matrix4x4 out;

    // Inverse = adjoint / det.

    // pre-compute 2x2 dets for last two rows when computing
    // cofactors of first two rows.
    d12 = ((*this)(2, 0) * (*this)(3, 1) - (*this)(3, 0) * (*this)(2, 1));
    d13 = ((*this)(2, 0) * (*this)(3, 2) - (*this)(3, 0) * (*this)(2, 2));
    d23 = ((*this)(2, 1) * (*this)(3, 2) - (*this)(3, 1) * (*this)(2, 2));
    d24 = ((*this)(2, 1) * (*this)(3, 3) - (*this)(3, 1) * (*this)(2, 3));
    d34 = ((*this)(2, 2) * (*this)(3, 3) - (*this)(3, 2) * (*this)(2, 3));
    d41 = ((*this)(2, 3) * (*this)(3, 0) - (*this)(3, 3) * (*this)(2, 0));

    out.SetElement(0, 0,  ((*this)(1, 1) * d34 - (*this)(1, 2) * d24 + (*this)(1, 3) * d23));
    out.SetElement(1, 0, -((*this)(1, 0) * d34 + (*this)(1, 2) * d41 + (*this)(1, 3) * d13));
    out.SetElement(2, 0,  ((*this)(1, 0) * d24 + (*this)(1, 1) * d41 + (*this)(1, 3) * d12));
    out.SetElement(3, 0, -((*this)(1, 0) * d23 - (*this)(1, 1) * d13 + (*this)(1, 2) * d12));

    // Compute determinant as early as possible using these cofactors.
    det = (*this)(0, 0) * out(0, 0) + (*this)(0, 1) * out(1, 0) + (*this)(0, 2) * out(2, 0) + (*this)(0, 3) * out(3, 0);

    // Run singularity test.
    if (det == 0.0f)
    {
        out = CreateIdentity();
    }
    else
    {
        float invDet = 1.0f / det;

        // Compute rest of inverse.
        out.SetElement(0, 0, out(0, 0) * invDet);
        out.SetElement(1, 0, out(1, 0) * invDet);
        out.SetElement(2, 0, out(2, 0) * invDet);
        out.SetElement(3, 0, out(3, 0) * invDet);

        out.SetElement(0, 1, -((*this)(0, 1) * d34 - (*this)(0, 2) * d24 + (*this)(0, 3) * d23) * invDet);
        out.SetElement(1, 1,  ((*this)(0, 0) * d34 + (*this)(0, 2) * d41 + (*this)(0, 3) * d13) * invDet);
        out.SetElement(2, 1, -((*this)(0, 0) * d24 + (*this)(0, 1) * d41 + (*this)(0, 3) * d12) * invDet);
        out.SetElement(3, 1,  ((*this)(0, 0) * d23 - (*this)(0, 1) * d13 + (*this)(0, 2) * d12) * invDet);

        // Pre-compute 2x2 dets for first two rows when computing
        // cofactors of last two rows.
        d12 = (*this)(0, 0) * (*this)(1, 1) - (*this)(1, 0) * (*this)(0, 1);
        d13 = (*this)(0, 0) * (*this)(1, 2) - (*this)(1, 0) * (*this)(0, 2);
        d23 = (*this)(0, 1) * (*this)(1, 2) - (*this)(1, 1) * (*this)(0, 2);
        d24 = (*this)(0, 1) * (*this)(1, 3) - (*this)(1, 1) * (*this)(0, 3);
        d34 = (*this)(0, 2) * (*this)(1, 3) - (*this)(1, 2) * (*this)(0, 3);
        d41 = (*this)(0, 3) * (*this)(1, 0) - (*this)(1, 3) * (*this)(0, 0);

        out.SetElement(0, 2,  ((*this)(3, 1) * d34 - (*this)(3, 2) * d24 + (*this)(3, 3) * d23) * invDet);
        out.SetElement(1, 2, -((*this)(3, 0) * d34 + (*this)(3, 2) * d41 + (*this)(3, 3) * d13) * invDet);
        out.SetElement(2, 2,  ((*this)(3, 0) * d24 + (*this)(3, 1) * d41 + (*this)(3, 3) * d12) * invDet);
        out.SetElement(3, 2, -((*this)(3, 0) * d23 - (*this)(3, 1) * d13 + (*this)(3, 2) * d12) * invDet);
        out.SetElement(0, 3, -((*this)(2, 1) * d34 - (*this)(2, 2) * d24 + (*this)(2, 3) * d23) * invDet);
        out.SetElement(1, 3,  ((*this)(2, 0) * d34 + (*this)(2, 2) * d41 + (*this)(2, 3) * d13) * invDet);
        out.SetElement(2, 3, -((*this)(2, 0) * d24 + (*this)(2, 1) * d41 + (*this)(2, 3) * d12) * invDet);
        out.SetElement(3, 3,  ((*this)(2, 0) * d23 - (*this)(2, 1) * d13 + (*this)(2, 2) * d12) * invDet);
    }

    return out;
}

const Vector3 Matrix4x4::RetrieveScale() const
{
    return Vector3(GetBasisX().GetLength(), GetBasisY().GetLength(), GetBasisZ().GetLength());
}

const Vector3 Matrix4x4::ExtractScale()
{
    Vector4 x = GetBasisX();
    Vector4 y = GetBasisY();
    Vector4 z = GetBasisZ();
    VectorFloat lengthX = x.GetLength();
    VectorFloat lengthY = y.GetLength();
    VectorFloat lengthZ = z.GetLength();
    SetBasisX(x / lengthX);
    SetBasisY(y / lengthY);
    SetBasisZ(z / lengthZ);
    return Vector3(lengthX, lengthY, lengthZ);
}

void Matrix4x4::MultiplyByScale(const Vector3& scale)
{
    SetBasisX(scale.GetX() * GetBasisX());
    SetBasisY(scale.GetY() * GetBasisY());
    SetBasisZ(scale.GetZ() * GetBasisZ());
}

const Matrix4x4 Matrix4x4::CreateInterpolated(const Matrix4x4& m1, const Matrix4x4& m2, float t)
{
    Matrix3x3 m1Copy = Matrix3x3::CreateFromMatrix4x4(m1);
    Vector3 s1 = m1Copy.ExtractScale();
    Vector3 t1 = m1.GetTranslation();
    Quaternion q1 = Quaternion::CreateFromMatrix3x3(m1Copy);

    Matrix3x3 m2Copy = Matrix3x3::CreateFromMatrix4x4(m2);
    Vector3 s2 = m2Copy.ExtractScale();
    Vector3 t2 = m2.GetTranslation();
    Quaternion q2 = Quaternion::CreateFromMatrix3x3(m2Copy);

    s1 = s1.Lerp(s2, t);
    t1 = t1.Lerp(t2, t);
    q1 = q1.Slerp(q2, t);
    q1.Normalize();
    Matrix4x4 result = Matrix4x4::CreateFromQuaternionAndTranslation(q1, t1);
    result.MultiplyByScale(s1);
    return result;
}

#endif // #ifndef AZ_UNITY_BUILD