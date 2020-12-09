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

#include <Cry_Math.h> // Needed for Vec3

#include <System/DataTypes.h>

namespace NvCloth
{
    //! Calculates the tangent space base for a given mesh
    class TangentSpaceCalculation
    {
    public:
        void Calculate(
            const AZStd::vector<SimParticleType>& vertices,
            const AZStd::vector<SimIndexType>& indices,
            const AZStd::vector<SimUVType>& uvs);

        size_t GetBaseCount() const;

        // Returns an orthogonal base (perpendicular and normalized)
        void GetBase(AZ::u32 index, Vec3& tangent, Vec3& bitangent, Vec3& normal) const;
        Vec3 GetTangent(AZ::u32 index) const;
        Vec3 GetBitangent(AZ::u32 index) const;
        Vec3 GetNormal(AZ::u32 index) const;

    private:
        struct Base33
        {
            Base33() = default;
            Base33(const Vec3& tangent, const Vec3& bitangent, const Vec3& normal);

            Vec3 m_tangent = Vec3(0.0f, 0.0f, 0.0f);
            Vec3 m_bitangent = Vec3(0.0f, 0.0f, 0.0f);
            Vec3 m_normal = Vec3(0.0f, 0.0f, 0.0f);
        };

        void AddNormalToBase(AZ::u32 index, const Vec3& normal);
        void AddUVToBase(AZ::u32 index, const Vec3& u, const Vec3& v);
        float CalcAngleBetween(const Vec3& a, const Vec3& b);

        AZStd::vector<Base33> m_baseVectors;
    };
} // namespace NvCloth
