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

#include <AzTest/AzTest.h>

#include <AzCore/UnitTest/TestTypes.h>

#include <Utils/MathConversion.h>
#include <Utils/TangentSpaceCalculation.h>

namespace NvCloth
{
    class NvClothTest
        : public UnitTest::AllocatorsTestFixture
    {
    protected:
        static const float Tolerance;
    };

    const float NvClothTest::Tolerance = 1e-5f;

    TEST_F(NvClothTest, MathConversion_ConvertAzVector3ToPxVec3_ConversionIsCorrect)
    {
        const AZ::Vector3 lyA(3.0f, -4.0f, 12.0f);
        const AZ::Vector3 lyB(-8.0f, 1.0f, -4.0f);

        const physx::PxVec3 pxA = PxMathConvert(lyA);
        const physx::PxVec3 pxB = PxMathConvert(lyB);

        EXPECT_NEAR(pxA.x, lyA.GetX(), NvClothTest::Tolerance);
        EXPECT_NEAR(pxA.y, lyA.GetY(), NvClothTest::Tolerance);
        EXPECT_NEAR(pxA.z, lyA.GetZ(), NvClothTest::Tolerance);
        EXPECT_NEAR(pxB.x, lyB.GetX(), NvClothTest::Tolerance);
        EXPECT_NEAR(pxB.y, lyB.GetY(), NvClothTest::Tolerance);
        EXPECT_NEAR(pxB.z, lyB.GetZ(), NvClothTest::Tolerance);
    }

    TEST_F(NvClothTest, MathConversion_ConvertPxVec3ToAzVector3_ConversionIsCorrect)
    {
        const physx::PxVec3 pxA(3.0f, -4.0f, 12.0f);
        const physx::PxVec3 pxB(-8.0f, 1.0f, -4.0f);

        const AZ::Vector3 lyA = PxMathConvert(pxA);
        const AZ::Vector3 lyB = PxMathConvert(pxB);

        EXPECT_NEAR(lyA.GetX(), pxA.x, NvClothTest::Tolerance);
        EXPECT_NEAR(lyA.GetY(), pxA.y, NvClothTest::Tolerance);
        EXPECT_NEAR(lyA.GetZ(), pxA.z, NvClothTest::Tolerance);
        EXPECT_NEAR(lyB.GetX(), pxB.x, NvClothTest::Tolerance);
        EXPECT_NEAR(lyB.GetY(), pxB.y, NvClothTest::Tolerance);
        EXPECT_NEAR(lyB.GetZ(), pxB.z, NvClothTest::Tolerance);
    }

    TEST_F(NvClothTest, MathConversion_ConvertAzQuaternionToPxQuat_ConversionIsCorrect)
    {
        const AZ::Quaternion lyQ = AZ::Quaternion(9.0f, -8.0f, -4.0f, 8.0f) / 15.0f;

        const physx::PxQuat pxQ = PxMathConvert(lyQ);

        EXPECT_NEAR(pxQ.x, lyQ.GetX(), NvClothTest::Tolerance);
        EXPECT_NEAR(pxQ.y, lyQ.GetY(), NvClothTest::Tolerance);
        EXPECT_NEAR(pxQ.z, lyQ.GetZ(), NvClothTest::Tolerance);
        EXPECT_NEAR(pxQ.w, lyQ.GetW(), NvClothTest::Tolerance);
    }

    // Class to provide triangle input data for tests
    struct TriangleInputPlane
    {
        // Creates triangle data for a plane in XY axis with any dimensions and segments.
        static TriangleInputPlane CreatePlane(float width, float height, AZ::u32 segmentsX, AZ::u32 segmentsY)
        {
            TriangleInputPlane plane;

            plane.m_vertices.resize((segmentsX + 1) * (segmentsY + 1));
            plane.m_uvs.resize((segmentsX + 1) * (segmentsY + 1));
            plane.m_indices.resize((segmentsX * segmentsY * 2) * 3);

            const SimParticleType topLeft(
                -width * 0.5f,
                -height * 0.5f,
                0.0f,
                0.0f);

            // Vertices and UVs
            for (AZ::u32 y = 0; y < segmentsY + 1; ++y)
            {
                for (AZ::u32 x = 0; x < segmentsX + 1; ++x)
                {
                    const AZ::u32 segmentIndex = x + y * (segmentsX + 1);
                    const float fractionX = ((float)x / (float)segmentsX);
                    const float fractionY = ((float)y / (float)segmentsY);

                    SimParticleType position(
                        fractionX * width,
                        fractionY * height,
                        0.0f,
                        0.0f);

                    plane.m_vertices[segmentIndex] = topLeft + position;
                    plane.m_uvs[segmentIndex] = SimUVType(fractionX, fractionY);
                }
            }

            // Triangles indices
            for (AZ::u32 y = 0; y < segmentsY; ++y)
            {
                for (AZ::u32 x = 0; x < segmentsX; ++x)
                {
                    const AZ::u32 segmentIndex = (x + y * segmentsX) * 2 * 3;

                    const AZ::u32 firstTriangleStartIndex = segmentIndex;
                    const AZ::u32 secondTriangleStartIndex = segmentIndex + 3;

                    //Top left to bottom right

                    plane.m_indices[firstTriangleStartIndex + 0] = static_cast<SimIndexType>((x + 0) + (y + 0) * (segmentsX + 1));
                    plane.m_indices[firstTriangleStartIndex + 1] = static_cast<SimIndexType>((x + 1) + (y + 0) * (segmentsX + 1));
                    plane.m_indices[firstTriangleStartIndex + 2] = static_cast<SimIndexType>((x + 1) + (y + 1) * (segmentsX + 1));

                    plane.m_indices[secondTriangleStartIndex + 0] = static_cast<SimIndexType>((x + 0) + (y + 0) * (segmentsX + 1));
                    plane.m_indices[secondTriangleStartIndex + 1] = static_cast<SimIndexType>((x + 1) + (y + 1) * (segmentsX + 1));
                    plane.m_indices[secondTriangleStartIndex + 2] = static_cast<SimIndexType>((x + 0) + (y + 1) * (segmentsX + 1));
                }
            }

            return plane;
        }

        AZStd::vector<SimParticleType> m_vertices;
        AZStd::vector<SimIndexType> m_indices;
        AZStd::vector<SimUVType> m_uvs;
    };

    TEST_F(NvClothTest, TangentSpaceCalculation_CalculateTangentScapePlaneXY_CalculationIsCorrect)
    {
        const float width = 1.0f;
        const float height = 1.0f;
        const AZ::u32 segmentsX = 5;
        const AZ::u32 segmentsY = 5;

        const TriangleInputPlane planeXY = TriangleInputPlane::CreatePlane(width, height, segmentsX, segmentsY);

        TangentSpaceCalculation tangentSpacePlaneXY;
        tangentSpacePlaneXY.Calculate(planeXY.m_vertices, planeXY.m_indices, planeXY.m_uvs);

        EXPECT_EQ(tangentSpacePlaneXY.GetBaseCount(), planeXY.m_vertices.size());
        for (size_t i = 0; i < tangentSpacePlaneXY.GetBaseCount(); ++i)
        {
            Vec3 tangent, bitangent, normal;
            tangentSpacePlaneXY.GetBase(i, tangent, bitangent, normal);
            EXPECT_NEAR(tangent.x, 1.0f, NvClothTest::Tolerance);
            EXPECT_NEAR(tangent.y, 0.0f, NvClothTest::Tolerance);
            EXPECT_NEAR(tangent.z, 0.0f, NvClothTest::Tolerance);
            EXPECT_NEAR(bitangent.x, 0.0f, NvClothTest::Tolerance);
            EXPECT_NEAR(bitangent.y, 1.0f, NvClothTest::Tolerance);
            EXPECT_NEAR(bitangent.z, 0.0f, NvClothTest::Tolerance);
            EXPECT_NEAR(normal.x, 0.0f, NvClothTest::Tolerance);
            EXPECT_NEAR(normal.y, 0.0f, NvClothTest::Tolerance);
            EXPECT_NEAR(normal.z, 1.0f, NvClothTest::Tolerance);
        }
    }

    AZ_UNIT_TEST_HOOK();
    
} // namespace NvCloth
