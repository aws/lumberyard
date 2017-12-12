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

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

namespace AZ
{
    /**
     * Vector2 conversions
     */

    AZ_MATH_FORCE_INLINE Vector3 Vector2ToVector3(const Vector2& v)
    {
        return Vector3(v.GetX(), v.GetY(), 0);
    }
    AZ_MATH_FORCE_INLINE Vector3 Vector2ToVector3(const Vector2& v, const VectorFloat& z)
    {
        return Vector3(v.GetX(), v.GetY(), z);
    }
    AZ_MATH_FORCE_INLINE Vector4 Vector2ToVector4(const Vector2& v) 
    {
        return Vector4(v.GetX(), v.GetY(), 0, 0);
    }
    AZ_MATH_FORCE_INLINE Vector4 Vector2ToVector4(const Vector2& v, const VectorFloat& z)
    {
        return Vector4(v.GetX(), v.GetY(), z, 0);
    }
    AZ_MATH_FORCE_INLINE Vector4 Vector2ToVector4(const Vector2& v, const VectorFloat& z, const VectorFloat& w)
    {
        return Vector4(v.GetX(), v.GetY(), z, w);
    }
    
    /**
     * Vector3 conversions
     */

    AZ_MATH_FORCE_INLINE Vector2 Vector3ToVector2(const Vector3& v)
    {
        return Vector2(v.GetX(), v.GetY());
    }
    AZ_MATH_FORCE_INLINE Vector4 Vector3ToVector4(const Vector3& v) 
    {
        return Vector4(v.GetX(), v.GetY(), v.GetZ(), 0);
    }
    AZ_MATH_FORCE_INLINE Vector4 Vector3ToVector4(const Vector3& v, const VectorFloat& w)
    {
        return Vector4(v.GetX(), v.GetY(), v.GetZ(), w);
    }

    /**
     * Vector4 conversions
     */

    AZ_MATH_FORCE_INLINE Vector2 Vector4ToVector2(const Vector4& v)
    {
        return Vector2(v.GetX(), v.GetY());
    }
    AZ_MATH_FORCE_INLINE Vector3 Vector4ToVector3(const Vector4& v) 
    {
        return Vector3(v.GetX(), v.GetY(), v.GetZ());
    }

} //namespace AZ