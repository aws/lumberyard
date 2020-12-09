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
#include "NvClothTest.h"

#include <AzCore/UnitTest/Helpers.h>
#include <AzCore/Interface/Interface.h>
#include <System/TangentSpaceHelper.h>

namespace UnitTest
{
    class NvClothTest
        : public ::testing::Test
    {
    protected:
        static const float Tolerance;

        // This will register AZ::Interface<ITangentSpaceHelper>
        NvCloth::TangentSpaceHelper tangentScpaceHelper;
    };

    const float NvClothTest::Tolerance = 1e-5f;

    TEST_F(NvClothTest, TangentSpaceHelper_CalculateTangentSpacePlaneXY_CalculationIsCorrect)
    {
        const float width = 1.0f;
        const float height = 1.0f;
        const AZ::u32 segmentsX = 5;
        const AZ::u32 segmentsY = 5;

        const NvCloth::TriangleInputPlane planeXY = NvCloth::TriangleInputPlane::CreatePlane(width, height, segmentsX, segmentsY);

        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        AZStd::vector<AZ::Vector3> normals;
        AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentSpace(
            planeXY.m_vertices, planeXY.m_indices, planeXY.m_uvs,
            tangents, bitangents, normals);

        const size_t numVertices = planeXY.m_vertices.size();

        EXPECT_EQ(tangents.size(), numVertices);
        EXPECT_EQ(bitangents.size(), numVertices);
        EXPECT_EQ(normals.size(), numVertices);
        for (size_t i = 0; i < numVertices; ++i)
        {
            EXPECT_THAT(tangents[i], IsClose(AZ::Vector3::CreateAxisX(), NvClothTest::Tolerance));
            EXPECT_THAT(bitangents[i], IsClose(AZ::Vector3::CreateAxisY(), NvClothTest::Tolerance));
            EXPECT_THAT(normals[i], IsClose(AZ::Vector3::CreateAxisZ(), NvClothTest::Tolerance));
        }
    }

    TEST_F(NvClothTest, TangentSpaceHelper_CalculateNormalsPlaneXY_CalculationIsCorrect)
    {
        const float width = 1.0f;
        const float height = 1.0f;
        const AZ::u32 segmentsX = 5;
        const AZ::u32 segmentsY = 5;

        const NvCloth::TriangleInputPlane planeXY = NvCloth::TriangleInputPlane::CreatePlane(width, height, segmentsX, segmentsY);

        AZStd::vector<AZ::Vector3> normals;
        AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateNormals(
            planeXY.m_vertices, planeXY.m_indices, normals);

        const size_t numVertices = planeXY.m_vertices.size();

        EXPECT_EQ(normals.size(), numVertices);
        for (size_t i = 0; i < numVertices; ++i)
        {
            EXPECT_THAT(normals[i], IsClose(AZ::Vector3::CreateAxisZ(), NvClothTest::Tolerance));
        }
    }

    TEST_F(NvClothTest, TangentSpaceHelper_CalculateTangentsAndBitangentsPlaneXY_CalculationIsCorrect)
    {
        const float width = 1.0f;
        const float height = 1.0f;
        const AZ::u32 segmentsX = 5;
        const AZ::u32 segmentsY = 5;

        const NvCloth::TriangleInputPlane planeXY = NvCloth::TriangleInputPlane::CreatePlane(width, height, segmentsX, segmentsY);

        AZStd::vector<AZ::Vector3> normals;
        AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateNormals(
            planeXY.m_vertices, planeXY.m_indices, normals);

        AZStd::vector<AZ::Vector3> tangents;
        AZStd::vector<AZ::Vector3> bitangents;
        AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateTangentsAndBitagents(
            planeXY.m_vertices, planeXY.m_indices, planeXY.m_uvs, normals,
            tangents, bitangents);

        const size_t numVertices = planeXY.m_vertices.size();

        EXPECT_EQ(tangents.size(), numVertices);
        EXPECT_EQ(bitangents.size(), numVertices);
        for (size_t i = 0; i < numVertices; ++i)
        {
            EXPECT_THAT(tangents[i], IsClose(AZ::Vector3::CreateAxisX(), NvClothTest::Tolerance));
            EXPECT_THAT(bitangents[i], IsClose(AZ::Vector3::CreateAxisY(), NvClothTest::Tolerance));
        }
    }
} // namespace UnitTest
