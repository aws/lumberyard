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

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/MathUtils.h>

// we need to specify the namespace AZ, since the Wii SDK has a Quaternion defined struct in the global namespace)
namespace AZ
{
    const Quaternion Quaternion::CreateRotationX(float angle)
    {
        Quaternion result;
        angle *= 0.5f;
        result.Set(sinf(angle), 0.0f, 0.0f, cosf(angle));
        return result;
    }

    const Quaternion Quaternion::CreateRotationY(float angle)
    {
        Quaternion result;
        angle *= 0.5f;
        result.Set(0.0f, sinf(angle), 0.0f, cosf(angle));
        return result;
    }

    const Quaternion Quaternion::CreateRotationZ(float angle)
    {
        Quaternion result;
        angle *= 0.5f;
        result.Set(0.0f, 0.0f, sinf(angle), cosf(angle));
        return result;
    }

    const Quaternion Quaternion::CreateFromMatrix3x3(const Matrix3x3& m)
    {
        Quaternion result;

        float trace = m(0, 0) + m(1, 1) + m(2, 2);
        if (trace > 0.0f)
        {
            float s = sqrtf(trace + 1.0f);
            result.SetW(s * 0.5f);
            s = 0.5f / s;
            result.SetX((m(2, 1) - m(1, 2)) * s);
            result.SetY((m(0, 2) - m(2, 0)) * s);
            result.SetZ((m(1, 0) - m(0, 1)) * s);
        }
        else
        {
            static int  nxt[3] = {1, 2, 0};

            int i = 0;
            if (m(1, 1) > m(0, 0))
            {
                i = 1;
            }
            if (m(2, 2) > m(i, i))
            {
                i = 2;
            }
            int j = nxt[i];
            int k = nxt[j];
            float s = sqrtf((m(i, i) - (m(j, j) + m(k, k))) + 1.0f);
            result.SetElement(i, s * 0.5f);
            if (s != 0.0f)
            {
                s = 0.5f / s;
            }
            result.SetW((m(k, j) - m(j, k)) * s);
            result.SetElement(j, (m(j, i) + m(i, j)) * s);
            result.SetElement(k, (m(k, i) + m(i, k)) * s);
        }

        return result;
    }

    // efficient check to determine if a transform has scale applied or not.
    bool IsUnit(const Transform& t)
    {
        return IsCloseMag<AZ::VectorFloat>(t.GetBasisX().GetLengthSq(), AZ::VectorFloat::CreateOne(), AZ::g_simdTolerance)
            && IsCloseMag<AZ::VectorFloat>(t.GetBasisY().GetLengthSq(), AZ::VectorFloat::CreateOne(), AZ::g_simdTolerance)
            && IsCloseMag<AZ::VectorFloat>(t.GetBasisZ().GetLengthSq(), AZ::VectorFloat::CreateOne(), AZ::g_simdTolerance);
    }

    const Quaternion Quaternion::CreateRotationFromScaledTransform(Transform t)
    {
        t.ExtractScaleExact();
        return CreateFromTransform(t);
    }
    
    const Quaternion Quaternion::CreateRotationFromUnscaledTransform(const Transform& t)
    {
        AZ_Assert(IsUnit(t), "Creating quaternion from transform without normalized scale - "
            "the orientation will likely not be what you expect. Either extract the scale first, "
            "or prefer calling CreateRotationFromScaledTransform which will call ExtractScale for you");

        return CreateFromTransform(t);
    }

    const Quaternion Quaternion::CreateFromTransform(const Transform& t)
    {
        return CreateFromMatrix3x3(Matrix3x3::CreateFromTransform(t));
    }

    const Quaternion Quaternion::CreateFromMatrix4x4(const Matrix4x4& m)
    {
        return CreateFromMatrix3x3(Matrix3x3::CreateFromMatrix4x4(m));
    }

    const Quaternion Quaternion::CreateFromAxisAngle(const Vector3& axis, const VectorFloat& angle)
    {
        VectorFloat sin, cos;
        VectorFloat halfAngle = 0.5f * angle;
        halfAngle.GetSinCos(sin, cos);
        return CreateFromVector3AndValue(sin * axis, cos);
    }

    const Quaternion Quaternion::CreateFromAxisAngleExact(const Vector3& axis, const VectorFloat& angle)
    {
        const VectorFloat halfAngle = 0.5f * angle;
        return CreateFromVector3AndValue(sinf(halfAngle) * axis, cosf(halfAngle));
    }

    const Quaternion Quaternion::CreateShortestArc(const Vector3& v1, const Vector3& v2)
    {
        const float dotProduct = v1.Dot(v2);
        const Vector3 crossProduct = v1.Cross(v2);
        const float magnitudeProduct = sqrtf(v1.GetLengthSq() * v2.GetLengthSq());

        if (dotProduct >= 0.0f)
        {
            // The cross product is in the right direction for the vector part of the quaternion and has magnitude
            // |v1||v2|sin(t)
            // = 2|v1||v2|cos(t / 2) [sin(t / 2)]
            // Then, if we make a quaternion with the cross product as its vector part and scalar part given by
            // |v1||v2| + v1.v2
            // = |v1||v2| + |v1||v2|cos(t)
            // = |v1||v2|(1 + cos(t))
            // = |v1||v2|(2 cos(t / 2) ^ 2)
            // = 2|v1||v2|cos(t / 2) [cos(t / 2)]
            // then normalizing the quaternion will eliminate the common factor of 2|v1||v2|cos(t / 2) and give the
            // desired rotation.
            const float w = magnitudeProduct + dotProduct;
            return CreateFromVector3AndValue(crossProduct, w).GetNormalized();
        }

        // If the vectors aren't in the same hemisphere, return the product of a 180 degree rotation and the rotation
        // from -v1 to v2.  This gives much better accuracy in the case where v1 is very close to -v2.
        const float w = magnitudeProduct - dotProduct;
        const Vector3 orthogonalVector = v1.GetOrthogonalVector();
        Quaternion result = (CreateFromVector3AndValue(-crossProduct, w) * CreateFromVector3(orthogonalVector)).GetNormalized();
        return result.GetW() >= 0.0f ? result : -result;
    }

    const Quaternion Quaternion::NLerp(const Quaternion& dest, const VectorFloat& t) const
    {
        Quaternion result = Lerp(dest, t);
        result.Normalize();
        return result;
    }
    
    const Quaternion
    Quaternion::Slerp(const Quaternion& dest, float t) const
    {
        float DestDot = Dot(dest);
        float comp = DestDot >= 0.f ? 1.0f : -1.0f;
        float cosom =  DestDot * comp; // compensate

        float sclA, sclB;

        if (cosom < 0.9999f)
        {
            float omega = acosf(cosom);
            float sinom = 1.f / sinf(omega);
            sclA = sinf((1.f - t) * omega) * sinom;
            sclB = sinf(t * omega) * sinom;
        }
        else
        {
            /* Very close, just Lerp */
            sclA = 1.0f - t;
            sclB = t;
        }

        sclA *= comp; // compensate

        return (*this) * sclA + dest * sclB;
    }


    /**************************************************************************
     *
     * Squad(p,a,b,q,t) = Slerp(Slerp(p,q,t), Slerp(a,b,t); 2(1-t)t).
     *
     **************************************************************************/
    const Quaternion
    Quaternion::Squad(const Quaternion& dest, const Quaternion& in, const Quaternion& out, float t) const
    {
        float k = 2.0f * (1.0f - t) * t;
        Quaternion temp1 = in.Slerp(out, t);
        Quaternion temp2 = Slerp(dest, t);
        return temp1.Slerp(temp2, k);
    }

    VectorFloat Quaternion::GetAngle() const
    {
        VectorFloat one = VectorFloat::CreateOne();
        return VectorFloat(2.0f * acosf(GetW().GetClamp(-one, one)));
    }

    // Technique from published work available here
    // https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2012/07/euler-angles1.pdf (Extracting Euler Angles from a Rotation Matrix - Mike Day, Insomniac Games mday@insomniacgames.com)
    AZ::Vector3 Quaternion::GetEulerDegrees() const
    {
        AZ::Vector3 eulers = GetEulerRadians();
        eulers.Set(AZ::RadToDeg(eulers.GetX()), AZ::RadToDeg(eulers.GetY()), AZ::RadToDeg(eulers.GetZ()));
        return eulers;
    }

    AZ::Vector3 Quaternion::GetEulerRadians() const
    {
        AZ::Transform transform = AZ::Transform::CreateFromQuaternion(*this);
        return transform.GetEulerRadians();
    }

    void Quaternion::SetFromEulerRadians(const AZ::Vector3& eulerRadians)
    {
        AZ::VectorFloat half(0.5f);
        AZ::VectorFloat rotx = eulerRadians.GetX() * half;
        AZ::VectorFloat roty = eulerRadians.GetY() * half;
        AZ::VectorFloat rotz = eulerRadians.GetZ() * half;

        AZ::VectorFloat sx, cx, sy, cy, sz, cz;
        rotx.GetSinCos(sx, cx);
        roty.GetSinCos(sy, cy);
        rotz.GetSinCos(sz, cz);

        // rot = rotx * roty * rotz
        auto w = cx * cy * cz - sx * sy * sz;
        auto x = cx * sy * sz + sx * cy * cz;
        auto y = cx * sy * cz - sx * cy * sz;
        auto z = cx * cy * sz + sx * sy * cz;

        Set(x, y, z, w);
    }

    void Quaternion::SetFromEulerDegrees(const AZ::Vector3& eulerDegrees)
    {
        SetFromEulerRadians(Vector3DegToRad(eulerDegrees));
    }

    void Quaternion::ConvertToAxisAngle(AZ::Vector3& outAxis, float& outAngle) const
    {
        outAngle = 2.0f * acosf(GetW());

        const float sinHalfAngle = sinf(outAngle * 0.5f);
        if (sinHalfAngle > 0.0f)
        {
            const float invS = 1.0f / sinHalfAngle;
            outAxis.Set(GetX() * invS, GetY() * invS, GetZ() * invS);
        }
        else
        {
            outAxis.Set(0.0f, 1.0f, 0.0f);
            outAngle = 0.0f;
        }
    }

    // Non-member functionality belonging to the AZ namespace
    Vector3 ConvertQuaternionToEulerDegrees(const AZ::Quaternion& q)
    {
        return q.GetEulerDegrees();
    }

    AZ::Vector3 ConvertQuaternionToEulerRadians(const AZ::Quaternion& q)
    {
        return q.GetEulerRadians();
    }

    AZ::Quaternion ConvertEulerRadiansToQuaternion(const AZ::Vector3& eulerRadians)
    {
        Quaternion q;
        q.SetFromEulerRadians(eulerRadians);
        return q;
    }

    AZ::Quaternion ConvertEulerDegreesToQuaternion(const AZ::Vector3& eulerDegrees)
    {
        Quaternion q;
        q.SetFromEulerDegrees(eulerDegrees);
        return q;
    }

    void ConvertQuaternionToAxisAngle(const AZ::Quaternion& quat, AZ::Vector3& outAxis, float& outAngle)
    {
        quat.ConvertToAxisAngle(outAxis, outAngle);
    }
}

#endif // #ifndef AZ_UNITY_BUILD