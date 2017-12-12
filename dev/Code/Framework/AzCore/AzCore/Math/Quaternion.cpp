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

    const Quaternion Quaternion::CreateShortestArc(const Vector3& v1, const Vector3& v2)
    {
        Vector3 c = v1.Cross(v2);
        float d = v1.Dot(v2);

        if (d < -0.99999f)
        {
            return Quaternion(0.0f, 1.0f, 0.0f, 0.0f); // just pick any vector
        }
        float s = sqrtf((1.0f + d) * 2.0f);
        float rs = 1.0f / s;

        return Quaternion::CreateFromVector3AndValue(c * rs, 0.5f * s);
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
}

#endif // #ifndef AZ_UNITY_BUILD