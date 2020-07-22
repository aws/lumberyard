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

#ifdef AZ_TESTS_ENABLED

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <WhiteBox/WhiteBoxToolApi.h>
#include <tuple>

#include "Rendering/WhiteBoxRenderData.h"

namespace UnitTest
{
    class WhiteBoxTestFixture : public AllocatorsTestFixture
    {
    public:
        void SetUp() override final
        {
            AllocatorsTestFixture::SetUp();

            m_whiteBox = WhiteBox::Api::CreateWhiteBoxMesh();

            SetUpEditorFixtureImpl();
        }

        void TearDown() override final
        {
            TearDownEditorFixtureImpl();

            m_whiteBox.reset();

            AllocatorsTestFixture::TearDown();
        }

        virtual void SetUpEditorFixtureImpl() {}
        virtual void TearDownEditorFixtureImpl() {}

        WhiteBox::Api::WhiteBoxMeshPtr m_whiteBox;
    };

    struct FaceTestData
    {
        std::vector<AZ::Vector3> m_positions;
        size_t m_numCulledFaces;
    };

    class WhiteBoxVertexDataTestFixture
        : public WhiteBoxTestFixture
        , public testing::WithParamInterface<FaceTestData>
    {
    public:
        WhiteBox::WhiteBoxFaces ConstructFaceData(const FaceTestData& inFaces)
        {
            const auto& positionData = inFaces.m_positions;
            const size_t numVertices = positionData.size();
            const size_t numFaces = numVertices / 3;
            WhiteBox::WhiteBoxFaces outFaces;
            outFaces.resize(numFaces);

            // assemble the triangle primitives from the position data
            // (normals and uvs will be undefined)
            for (size_t faceIndex = 0, vertexIndex = 0; faceIndex < numFaces; faceIndex++, vertexIndex += 3)
            {
                WhiteBox::WhiteBoxFace& face = outFaces[faceIndex];
                face.m_v1.m_position = inFaces.m_positions[vertexIndex];
                face.m_v2.m_position = inFaces.m_positions[vertexIndex + 1];
                face.m_v3.m_position = inFaces.m_positions[vertexIndex + 2];
            }

            return outFaces;
        }
    };

    // AZ::Vector3 doesn't play ball with gtest's parameterized tests
    struct Vector3
    {
        float x;
        float y;
        float z;
    };

    // vector components to apply random noise to
    enum class NoiseSource
    {
        None,
        XComponent,
        YComponent,
        ZComponent,
        XYComponent,
        XZComponent,
        YZComponent,
        XYZComponent
    };

    // rotation of normal (45 degrees around each axis)
    enum class Rotation
    {
        Identity,
        XAxis,
        ZAxis,
        XZAxis
    };

    // noise and rotation parameters to be applied per permutation
    using WhiteBoxUVTestParams = std::tuple<Vector3, NoiseSource, Rotation>;

    class WhiteBoxUVTestFixture
        : public WhiteBoxTestFixture
        , public ::testing::WithParamInterface<WhiteBoxUVTestParams>
    {
    public:
    };
} // namespace UnitTest

#endif // AZ_TESTS_ENABLED
