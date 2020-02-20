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

#include <Utils/TangentSpaceCalculation.h>

namespace NvCloth
{
    TangentSpaceCalculation::Error TangentSpaceCalculation::CalculateTangentSpace(const TriangleInputProxy& input, AZStd::string& errorMessage)
    {
        AZ::u32 triCount = input.GetTriangleCount();
        AZ::u32 vertexCount = input.GetVertexCount();

        // Reset results
        m_baseVectors = AZStd::vector<Base33>(vertexCount);

        // base vectors per triangle
        AZStd::vector<Base33> triangleBases;

        // calculate the base vectors per triangle
        {
            const float identityInfluence = 0.01f;
            const Base33 identityBase(
                Vec3(identityInfluence, 0.0f, 0.0f),
                Vec3(0.0f, identityInfluence, 0.0f),
                Vec3(0.0f, 0.0f, identityInfluence));

            for (AZ::u32 i = 0; i < triCount; ++i)
            {
                TriangleIndices triangleIndices = input.GetTriangleIndices(i);

                AZStd::array<Vec3, 3> trianglePos;
                AZStd::array<Vec2, 3> triangleUVs;
                for (AZ::u32 e = 0; e < 3; ++e)
                {
                    trianglePos[e] = input.GetPosition(triangleIndices[e]);
                    triangleUVs[e] = input.GetUV(triangleIndices[e]);
                }

                // calculate tangent vectors

                AZStd::array<Vec3, 2> triangleEdges =
                {{
                    trianglePos[1] - trianglePos[0],
                    trianglePos[2] - trianglePos[0]
                }};

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

                float deltaU1 = triangleUVs[1].x - triangleUVs[0].x;
                float deltaU2 = triangleUVs[2].x - triangleUVs[0].x;
                float deltaV1 = triangleUVs[1].y - triangleUVs[0].y;
                float deltaV2 = triangleUVs[2].y - triangleUVs[0].y;

                float div = (deltaU1 * deltaV2 - deltaU2 * deltaV1);

                if (_isnan(div))
                {
                    errorMessage = AZStd::string::format(
                        "Vertices 0,1,2 have broken texture coordinates v0:(%f : %f : %f) v1:(%f : %f : %f) v2:(%f : %f : %f)\n",
                        trianglePos[0].x, trianglePos[0].y, trianglePos[0].z,
                        trianglePos[1].x, trianglePos[1].y, trianglePos[1].z,
                        trianglePos[2].x, trianglePos[2].y, trianglePos[2].z);
                    return Error::BrokenTextureCoordinates;
                }

                Vec3 tangent, bitangent;

                if (div != 0.0f)
                {
                    // 2D triangle area = (u1*v2-u2*v1)/2
                    float a = deltaV2; // /div was removed - no required because of normalize()
                    float b = -deltaV1;
                    float c = -deltaU2;
                    float d = deltaU1;

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
            for (AZ::u32 i = 0; i < triCount; ++i)
            {
                TriangleIndices triangleIndices = input.GetTriangleIndices(i);

                Base33 triBase = triangleBases[i];

                AZStd::array<Vec3, 3> trianglePos;
                for (AZ::u32 e = 0; e < 3; ++e)
                {
                    trianglePos[e] = input.GetPosition(triangleIndices[e]);
                }

                // for each triangle vertex
                for (AZ::u32 e = 0; e < 3; ++e)
                {
                    // weight by angle to fix the L-Shape problem
                    float weight = CalcAngleBetween(
                        trianglePos[(e + 2) % 3] - trianglePos[e],
                        trianglePos[(e + 1) % 3] - trianglePos[e]);

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
                Vec3 uOut, vOut, nOut;

                nOut = ref.m_normal;
                nOut.normalize();

                // project u in n plane
                // project v in n plane
                uOut = ref.m_tangent - nOut * (nOut.Dot(ref.m_tangent));
                vOut = ref.m_bitangent - nOut * (nOut.Dot(ref.m_bitangent));

                ref.m_tangent = uOut;
                ref.m_tangent.normalize();

                ref.m_bitangent = vOut;
                ref.m_bitangent.normalize();

                ref.m_normal = nOut;
            }
        }

        return Error::NoErrors;
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
