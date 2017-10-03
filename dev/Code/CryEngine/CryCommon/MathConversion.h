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

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Component/EntityId.h>
#include <Cry_Math.h>
#include <Cry_Geo.h>
#include <Cry_Color.h>

inline AZ::Vector3 LYVec3ToAZVec3(const Vec3& source)
{
    return AZ::Vector3(source.x, source.y, source.z);
}

inline Vec3 AZVec3ToLYVec3(const AZ::Vector3& source)
{
    return Vec3(source.GetX(), source.GetY(), source.GetZ());
}

inline AZ::Color LYVec3ToAZColor(const Vec3& source)
{
    return AZ::Color(source.x, source.y, source.z, 1.0f);
}

inline Vec3 AZColorToLYVec3(const AZ::Color& source)
{
    return Vec3(source.GetR(), source.GetG(), source.GetB());
}

inline ColorF AZColorToLYColorF(const AZ::Color& source)
{
    return ColorF(source.ToU32());
}

inline AZ::Color LYColorFToAZColor(const ColorF& source)
{
    return AZ::Color(source.r, source.g, source.b, source.a);
}

inline AZ::Quaternion LYQuaternionToAZQuaternion(const Quat& source)
{
    const float f4[4] = { source.v.x, source.v.y, source.v.z, source.w };
    return AZ::Quaternion::CreateFromFloat4(f4);
}

inline Quat AZQuaternionToLYQuaternion(const AZ::Quaternion& source)
{
    float f4[4];
    source.StoreToFloat4(f4);
    return Quat(f4[3], f4[0], f4[1], f4[2]);
}

inline Matrix34 AZTransformToLYTransform(const AZ::Transform& source)
{
    return Matrix34::CreateFromVectors(
        AZVec3ToLYVec3(source.GetColumn(0)),
        AZVec3ToLYVec3(source.GetColumn(1)),
        AZVec3ToLYVec3(source.GetColumn(2)),
        AZVec3ToLYVec3(source.GetTranslation()));
}

inline Matrix33 AZMatrix3x3ToLYMatrix3x3(const AZ::Matrix3x3& source)
{
    return Matrix33::CreateFromVectors(
        AZVec3ToLYVec3(source.GetColumn(0)),
        AZVec3ToLYVec3(source.GetColumn(1)),
        AZVec3ToLYVec3(source.GetColumn(2)));
}

inline AZ::Matrix3x3 LyMatrix3x3ToAzMatrix3x3(const Matrix33& source)
{
    return AZ::Matrix3x3::CreateFromColumns(
        LYVec3ToAZVec3(source.GetColumn(0)),
        LYVec3ToAZVec3(source.GetColumn(1)),
        LYVec3ToAZVec3(source.GetColumn(2)));
}

inline AZ::Transform LYTransformToAZTransform(const Matrix34& source)
{
    return AZ::Transform::CreateFromRowMajorFloat12(source.GetData());
}

inline AZ::Transform LYQuatTToAZTransform(const QuatT& source)
{
    return AZ::Transform::CreateFromQuaternionAndTranslation(
        LYQuaternionToAZQuaternion(source.q),
        LYVec3ToAZVec3(source.t));
}

inline QuatT AZTransformToLYQuatT(const AZ::Transform& source)
{
    AZ::Transform sourceNoScale(source);
    sourceNoScale.ExtractScale();

    return QuatT(
        AZQuaternionToLYQuaternion(AZ::Quaternion::CreateFromTransform(sourceNoScale)),
        AZVec3ToLYVec3(source.GetTranslation()));
}

inline AABB AZAabbToLyAABB(const AZ::Aabb& source)
{
    return AABB(AZVec3ToLYVec3(source.GetMin()), AZVec3ToLYVec3(source.GetMax()));
}

inline AZ::Aabb LyAABBToAZAabb(const AABB& source)
{
    return AZ::Aabb::CreateFromMinMax(LYVec3ToAZVec3(source.min), LYVec3ToAZVec3(source.max));
}

// returns true if an entityId is a legacyId - 0 is considered not to be a valid legacy Id as it's the
// default (but invalid) value in the legacy system
inline bool IsLegacyEntityId(const AZ::EntityId& entityId)
{
    return (((static_cast<AZ::u64>(entityId) >> 32) == 0) && static_cast<AZ::u64>(entityId) != 0);
}

inline unsigned GetLegacyEntityId(const AZ::EntityId& entityId)
{
    return IsLegacyEntityId(entityId) ? static_cast<unsigned>(static_cast<AZ::u64>(entityId)) : 0;
}
