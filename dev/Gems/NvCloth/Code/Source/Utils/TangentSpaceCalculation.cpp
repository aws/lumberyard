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

#include <NvCloth_precompiled.h>

#include <AzCore/std/containers/array.h>

#include <Utils/TangentSpaceCalculation.h>
#include <Utils/MathConversion.h>

namespace NvCloth
{
    void TangentSpaceCalculation::Calculate(
        const AZStd::vector<SimParticleType>& vertices,
        const AZStd::vector<SimIndexType>& indices,
        const AZStd::vector<SimUVType>& uvs)
    {
        AZ_Error("TangentSpaceCalculation", (indices.size() % 3) == 0,
            "Size of list of indices (%d) is not a multiple of 3.",
            indices.size());

        AZ::u32 triangleCount = indices.size() / 3;
        AZ::u32 vertexCount = vertices.size();

        // Reset results with the right number of elements
        m_baseVectors = AZStd::vector<Base33>(vertexCount);

        using TriangleIndices = AZStd::array<SimIndexType, 3>;
        using TrianglePositions = AZStd::array<Vec3, 3>;
        using TriangleUVs = AZStd::array<Vec3, 3>;
        using TriangleEdges = AZStd::array<Vec3, 2>;

        AZStd::vector<TriangleIndices> trianglesIndices;
        AZStd::vector<TrianglePositions> trianglesPositions;
        AZStd::vector<TriangleUVs> trianglesUVs;
        AZStd::vector<TriangleEdges> trianglesEdges;

        trianglesIndices.reserve(triangleCount);
        trianglesPositions.reserve(triangleCount);
        trianglesUVs.reserve(triangleCount);
        trianglesEdges.reserve(triangleCount);

        // Precalculate triangles' indices, positions, UVs and edges.
        for (AZ::u32 i = 0; i < triangleCount; ++i)
        {
            const TriangleIndices triangleIndices =
            {{
                indices[i * 3 + 0],
                indices[i * 3 + 1],
                indices[i * 3 + 2]
            }};
            const TrianglePositions trianglePositions =
            {{
                PxMathConvert(vertices[triangleIndices[0]]),
                PxMathConvert(vertices[triangleIndices[1]]),
                PxMathConvert(vertices[triangleIndices[2]])
            }};
            const TriangleUVs triangleUVs =
            {{
                PxMathConvert(uvs[triangleIndices[0]]),
                PxMathConvert(uvs[triangleIndices[1]]),
                PxMathConvert(uvs[triangleIndices[2]])
            }};
            const TriangleEdges triangleEdges =
            {{
                trianglePositions[1] - trianglePositions[0],
                trianglePositions[2] - trianglePositions[0]
            }};
            trianglesIndices.push_back(AZStd::move(triangleIndices));
            trianglesPositions.push_back(AZStd::move(trianglePositions));
            trianglesUVs.push_back(AZStd::move(triangleUVs));
            trianglesEdges.push_back(AZStd::move(triangleEdges));
        }

        // base vectors per triangle
        AZStd::vector<Base33> triangleBases;
        triangleBases.reserve(triangleCount);

        // calculate the base vectors per triangle
        {
            const float identityInfluence = 0.01f;
            const Base33 identityBase(
                Vec3(identityInfluence, 0.0f, 0.0f),
                Vec3(0.0f, identityInfluence, 0.0f),
                Vec3(0.0f, 0.0f, identityInfluence));

            for (AZ::u32 i = 0; i < triangleCount; ++i)
            {
                const auto& trianglePositions = trianglesPositions[i];
                const auto& triangleUVs = trianglesUVs[i];
                const auto& triangleEdges = trianglesEdges[i];

                // calculate tangent vectors

                Vec3 normal = triangleEdges[0].cross(triangleEdges[1]);

                // Avoid situations where the edges are parallel resulting in an invalid normal.
                // This can happen if the simulation moves particles of triangle to the same spot or very far away.
                if (normal.IsZero(0.0001f))
                {
                    // Use the identity base with low influence to leave other valid triangles to
                    // affect these vertices. In case no other triangle affects the vertices the base
                    // will still be valid with identity values as it gets normalized later.
                    triangleBases.push_back(identityBase);
                    continue;
                }

                normal.normalize();

                const float deltaU1 = triangleUVs[1].x - triangleUVs[0].x;
                const float deltaU2 = triangleUVs[2].x - triangleUVs[0].x;
                const float deltaV1 = triangleUVs[1].y - triangleUVs[0].y;
                const float deltaV2 = triangleUVs[2].y - triangleUVs[0].y;

                const float div = (deltaU1 * deltaV2 - deltaU2 * deltaV1);

                if (_isnan(div))
                {
                    AZ_Error("TangentSpaceCalculation", false,
                        "Vertices 0,1,2 have broken texture coordinates v0:(%f : %f : %f) v1:(%f : %f : %f) v2:(%f : %f : %f)",
                        trianglePositions[0].x, trianglePositions[0].y, trianglePositions[0].z,
                        trianglePositions[1].x, trianglePositions[1].y, trianglePositions[1].z,
                        trianglePositions[2].x, trianglePositions[2].y, trianglePositions[2].z);
                    return;
                }

                Vec3 tangent, bitangent;

                if (div != 0.0f)
                {
                    // 2D triangle area = (u1*v2-u2*v1)/2
                    const float a = deltaV2; // /div was removed - no required because of normalize()
                    const float b = -deltaV1;
                    const float c = -deltaU2;
                    const float d = deltaU1;

                    // /fAreaMul2*fAreaMul2 was optimized away -> small triangles in UV should contribute less and
                    // less artifacts (no divide and multiply)
                    tangent = (triangleEdges[0] * a + triangleEdges[1] * b) * fsgnf(div);
                    bitangent = (triangleEdges[0] * c + triangleEdges[1] * d) * fsgnf(div);
                }
                else
                {
                    tangent = Vec3(1.0f, 0.0f, 0.0f);
                    bitangent = Vec3(0.0f, 1.0f, 0.0f);
                }

                triangleBases.push_back(Base33(tangent, bitangent, normal));
            }
        }

        // distribute the normals and uv vectors to the vertices
        {
            // we create a new tangent base for every vertex index that has a different normal (later we split further for mirrored use)
            // and sum the base vectors (weighted by angle and mirrored if necessary)
            for (AZ::u32 i = 0; i < triangleCount; ++i)
            {
                const auto& triangleIndices = trianglesIndices[i];
                const auto& trianglePositions = trianglesPositions[i];

                Base33& triBase = triangleBases[i];

                // for each triangle vertex
                for (AZ::u32 e = 0; e < 3; ++e)
                {
                    // weight by angle to fix the L-Shape problem
                    const float weight = CalcAngleBetween(
                        trianglePositions[(e + 2) % 3] - trianglePositions[e],
                        trianglePositions[(e + 1) % 3] - trianglePositions[e]);

                    triBase.m_normal *= AZStd::max(weight, 0.0001f);
                    triBase.m_tangent *= weight;
                    triBase.m_bitangent *= weight;

                    AddNormalToBase(triangleIndices[e], triBase.m_normal);
                    AddUVToBase(triangleIndices[e], triBase.m_tangent, triBase.m_bitangent);
                }
            }
        }

        // adjust the base vectors per vertex
        {
            for (auto& ref : m_baseVectors)
            {
                // rotate u and v in n plane

                Vec3 nOut = ref.m_normal;
                nOut.normalize();

                // project u in n plane
                // project v in n plane
                Vec3 uOut = ref.m_tangent - nOut * (nOut.Dot(ref.m_tangent));
                Vec3 vOut = ref.m_bitangent - nOut * (nOut.Dot(ref.m_bitangent));

                ref.m_tangent = uOut;
                ref.m_tangent.normalize();

                ref.m_bitangent = vOut;
                ref.m_bitangent.normalize();

                ref.m_normal = nOut;
            }
        }

        AZ_Error("TangentSpaceCalculation", GetBaseCount() == vertices.size(),
            "Number of tangent spaces (%d) doesn't match with the number of input vertices (%d).",
            GetBaseCount(), vertices.size());
    }

    size_t TangentSpaceCalculation::GetBaseCount() const
    {
        return m_baseVectors.size();
    }

    void TangentSpaceCalculation::GetBase(AZ::u32 index, Vec3& tangent, Vec3& bitangent, Vec3& normal) const
    {
        tangent = GetTangent(index);
        bitangent = GetBitangent(index);
        normal = GetNormal(index);
    }

    Vec3 TangentSpaceCalculation::GetTangent(AZ::u32 index) const
    {
        return m_baseVectors[index].m_tangent;
    }

    Vec3 TangentSpaceCalculation::GetBitangent(AZ::u32 index) const
    {
        return m_baseVectors[index].m_bitangent;
    }

    Vec3 TangentSpaceCalculation::GetNormal(AZ::u32 index) const
    {
        return m_baseVectors[index].m_normal;
    }

    void TangentSpaceCalculation::AddNormalToBase(AZ::u32 index, const Vec3& normal)
    {
        m_baseVectors[index].m_normal += normal;
    }

    void TangentSpaceCalculation::AddUVToBase(AZ::u32 index, const Vec3& u, const Vec3& v)
    {
        m_baseVectors[index].m_tangent += u;
        m_baseVectors[index].m_bitangent += v;
    }

    float TangentSpaceCalculation::CalcAngleBetween(const Vec3& a, const Vec3& b)
    {
        double lengthQ = sqrt(a.len2() * b.len2());

        // to prevent division by zero
        lengthQ = AZStd::max(lengthQ, 1e-8);

        double cosAngle = a.Dot(b) / lengthQ;

        // acosf is not available on every platform. acos_tpl clamps cosAngle to [-1,1].
        return static_cast<float>(acos_tpl(cosAngle));
    }

    TangentSpaceCalculation::Base33::Base33(const Vec3& tangent, const Vec3& bitangent, const Vec3& normal)
        : m_tangent(tangent)
        , m_bitangent(bitangent)
        , m_normal(normal)
    {
    }
} // namespace NvCloth
