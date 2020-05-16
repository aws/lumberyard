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

#include <AzCore/UnitTest/TestTypes.h>
#include <AtomCore/Math/Frustum.h>
#include <AtomCore/Math/ShapeIntersection.h>

namespace AZ
{
    std::ostream& operator<< (std::ostream& stream, const Vector3& vector)
    {
        stream << "x: " << vector.GetX() << " y: " << vector.GetY() << " z: " << vector.GetZ();
        return stream;
    }

    std::ostream& operator<< (std::ostream& stream, const Sphere& sphere)
    {
        stream << "center: " << sphere.GetCenter() << " radius: " << sphere.GetRadius();
        return stream;
    }
}

namespace UnitTest
{
    enum FrustumPlanes
    {
        Near,
        Far,
        Left,
        Right,
        Top,
        Bottom,
        NumPlanes
    };

    struct FrustumTestCase
    {
        AZ::Vector3 nearTopLeft;
        AZ::Vector3 nearTopRight;
        AZ::Vector3 nearBottomLeft;
        AZ::Vector3 nearBottomRight;
        AZ::Vector3 farTopLeft;
        AZ::Vector3 farTopRight;
        AZ::Vector3 farBottomLeft;
        AZ::Vector3 farBottomRight;
        // Used to set which plane is being used
        FrustumPlanes plane;
        // Used to set a test case name
        std::string testCaseName;
    };

    std::ostream& operator<< (std::ostream& stream, const UnitTest::FrustumTestCase& frustumTestCase)
    {
        stream << "NearTopLeft: " << frustumTestCase.nearTopLeft 
            << std::endl << "NearTopRight: " << frustumTestCase.nearTopRight
            << std::endl << "NearBottomRight: " << frustumTestCase.nearBottomRight
            << std::endl << "NearBottomLeft: " << frustumTestCase.nearBottomLeft
            << std::endl << "FarTopLeft: " << frustumTestCase.farTopLeft
            << std::endl << "FarTopRight: " << frustumTestCase.farTopRight
            << std::endl << "FarBottomLeft: " << frustumTestCase.farBottomLeft
            << std::endl << "FarBottomRight: " << frustumTestCase.farBottomRight;
        return stream;
    }

    class FrustumIntersectionTests
        : public ::testing::WithParamInterface <FrustumTestCase>
        , public UnitTest::AllocatorsTestFixture
    {

    protected:
        void SetUp() override
        {
            UnitTest::AllocatorsTestFixture::SetUp();

            // Build a frustum from 8 points
            // This allows us to generate test cases in each of the 9 regions in the 3x3 grid divided by the 6 planes,
            // as well as test cases that span across those regions
            m_testCase = GetParam();

            // Planes can be generated from triangles. Points must wind counter-clockwise (right-handed) for the normal to point in the correct direction.
            // The Frustum class assumes the plane normals point inwards
            m_planes[FrustumPlanes::Near] = AZ::Plane::CreateFromTriangle(m_testCase.nearTopLeft, m_testCase.nearTopRight, m_testCase.nearBottomRight);
            m_planes[FrustumPlanes::Far] = AZ::Plane::CreateFromTriangle(m_testCase.farTopRight, m_testCase.farTopLeft, m_testCase.farBottomLeft);
            m_planes[FrustumPlanes::Left] = AZ::Plane::CreateFromTriangle(m_testCase.nearTopLeft, m_testCase.nearBottomLeft, m_testCase.farTopLeft);
            m_planes[FrustumPlanes::Right] = AZ::Plane::CreateFromTriangle(m_testCase.nearTopRight, m_testCase.farTopRight, m_testCase.farBottomRight);
            m_planes[FrustumPlanes::Top] = AZ::Plane::CreateFromTriangle(m_testCase.nearTopLeft, m_testCase.farTopLeft, m_testCase.farTopRight);
            m_planes[FrustumPlanes::Bottom] = AZ::Plane::CreateFromTriangle(m_testCase.nearBottomLeft, m_testCase.nearBottomRight, m_testCase.farBottomRight);
            
            // AZ::Plane::CreateFromTriangle uses Vector3::GetNormalized to create a normal, which is not perfectly normalized.
            // Since the distance value is set by float dist = -(normal.Dot(v0));, this means that an imperfect normal actually gives you a slightly different distance
            // and thus a slightly different plane. Correct that here.
            m_planes[FrustumPlanes::Near] = AZ::Plane::CreateFromNormalAndPoint(m_planes[FrustumPlanes::Near].GetNormal().GetNormalizedExact(), m_testCase.nearTopLeft);
            m_planes[FrustumPlanes::Far] = AZ::Plane::CreateFromNormalAndPoint(m_planes[FrustumPlanes::Far].GetNormal().GetNormalizedExact(), m_testCase.farTopRight);
            m_planes[FrustumPlanes::Left] = AZ::Plane::CreateFromNormalAndPoint(m_planes[FrustumPlanes::Left].GetNormal().GetNormalizedExact(), m_testCase.nearTopLeft);
            m_planes[FrustumPlanes::Right] = AZ::Plane::CreateFromNormalAndPoint(m_planes[FrustumPlanes::Right].GetNormal().GetNormalizedExact(), m_testCase.nearTopRight);
            m_planes[FrustumPlanes::Top] = AZ::Plane::CreateFromNormalAndPoint(m_planes[FrustumPlanes::Top].GetNormal().GetNormalizedExact(), m_testCase.nearTopLeft);
            m_planes[FrustumPlanes::Bottom] = AZ::Plane::CreateFromNormalAndPoint(m_planes[FrustumPlanes::Bottom].GetNormal().GetNormalizedExact(), m_testCase.nearBottomLeft);

            // Create the frustum itself
            m_frustum.SetNearPlane(m_planes[FrustumPlanes::Near]);
            m_frustum.SetFarPlane(m_planes[FrustumPlanes::Far]);
            m_frustum.SetLeftPlane(m_planes[FrustumPlanes::Left]);
            m_frustum.SetRightPlane(m_planes[FrustumPlanes::Right]);
            m_frustum.SetTopPlane(m_planes[FrustumPlanes::Top]);
            m_frustum.SetBottomPlane(m_planes[FrustumPlanes::Bottom]);

            // Get the center points
            m_centerPoints[FrustumPlanes::Near] = m_testCase.nearTopLeft + 0.5f * (m_testCase.nearBottomRight - m_testCase.nearTopLeft);
            m_centerPoints[FrustumPlanes::Far] = m_testCase.farTopLeft + 0.5f * (m_testCase.farBottomRight - m_testCase.farTopLeft);
            m_centerPoints[FrustumPlanes::Left] = m_testCase.nearTopLeft + 0.5f * (m_testCase.farBottomLeft - m_testCase.nearTopLeft);
            m_centerPoints[FrustumPlanes::Right] = m_testCase.nearTopRight + 0.5f * (m_testCase.farBottomRight - m_testCase.nearTopRight);
            m_centerPoints[FrustumPlanes::Top] = m_testCase.nearTopLeft + 0.5f * (m_testCase.farTopRight - m_testCase.nearTopLeft);
            m_centerPoints[FrustumPlanes::Bottom] = m_testCase.nearBottomLeft + 0.5f * (m_testCase.farBottomRight - m_testCase.nearBottomLeft);

            // Get the shortest edge of the frustum
            AZStd::vector<AZ::Vector3> edges;
            // Near plane
            edges.push_back(m_testCase.nearTopLeft - m_testCase.nearTopRight);
            edges.push_back(m_testCase.nearTopRight - m_testCase.nearBottomRight);
            edges.push_back(m_testCase.nearBottomRight - m_testCase.nearBottomLeft);
            edges.push_back(m_testCase.nearBottomLeft - m_testCase.nearTopLeft);

            // Edges from near plane to far plane
            edges.push_back(m_testCase.nearTopLeft - m_testCase.farTopLeft);
            edges.push_back(m_testCase.nearTopRight - m_testCase.farTopRight);
            edges.push_back(m_testCase.nearBottomRight - m_testCase.farBottomRight);
            edges.push_back(m_testCase.nearBottomLeft - m_testCase.farBottomLeft);

            // Far plane
            edges.push_back(m_testCase.farTopLeft - m_testCase.farTopRight);
            edges.push_back(m_testCase.farTopRight - m_testCase.farBottomRight);
            edges.push_back(m_testCase.farBottomRight - m_testCase.farBottomLeft);
            edges.push_back(m_testCase.farBottomLeft - m_testCase.farTopLeft);
            
            m_minEdgeLength = std::numeric_limits<float>::max();
            for (const AZ::Vector3& edge : edges)
            {
                m_minEdgeLength = AZ::GetMin(m_minEdgeLength, static_cast<float>(edge.GetLengthExact()));
            }
        }

        AZ::Sphere GenerateSphereOutsidePlane(const AZ::Plane& plane, const AZ::Vector3& planeCenter)
        {
            // Get a radius small enough for the entire sphere to fit outside the plane without extending past the other planes in the frustum
            float radius = 0.25f * m_minEdgeLength;

            // Get the outward pointing normal of the plane
            AZ::Vector3 normal = -1.0f * plane.GetNormal();

            // Create a sphere that is outside the plane
            AZ::Vector3 center = planeCenter + normal * (radius + m_marginOfErrorOffset);

            return AZ::Sphere(center, radius);
        }

        AZ::Sphere GenerateSphereInsidePlane(const AZ::Plane& plane, const AZ::Vector3& planeCenter)
        {
            // Get a radius small enough for the entire sphere to fit outside the plane without extending past the other planes in the frustum
            float radius = 0.25f * m_minEdgeLength;

            // Get the inward pointing normal of the plane
            AZ::Vector3 normal = plane.GetNormal();

            // Create a sphere that is inside the plane
            AZ::Vector3 center = planeCenter + normal * (radius + m_marginOfErrorOffset);

            return AZ::Sphere(center, radius);
        }

        // The points used to generate the test cases
        FrustumTestCase m_testCase;

        // The planes generated from the points.
        // Keep these around for access to the normals, which are used to generate shapes inside/outside of the frustum
        AZ::Plane m_planes[FrustumPlanes::NumPlanes];

        // The center points on the frustum for each plane
        AZ::Vector3 m_centerPoints[FrustumPlanes::NumPlanes];

        // The length of the shortest edge in the frustum
        float m_minEdgeLength = std::numeric_limits<float>::max();

        // Shapes generated inside/outside the frustum will be offset by this much as a margin of error
        // Through trial and error determined that, for the test cases below, the sphere needs to be offset from the frustum by at least 0.05625f to be guaranteed to pass,
        // which gives a reasonable idea of how precise these intersection tests are.
        // For the box shaped frustum, the tests passed when offset by FLT_EPSILON, but the frustums further away from the origin were less precise.
        float m_marginOfErrorOffset = 0.05625f;

        // The frustum under test
        AZ::Frustum m_frustum;
    };

    // Tests that a frustum does not contain a sphere that is outside the frustum
    TEST_P(FrustumIntersectionTests, FrustumContainsSphere_SphereOutsidePlane_False)
    {
        AZ::Sphere testSphere = GenerateSphereOutsidePlane(m_planes[m_testCase.plane], m_centerPoints[m_testCase.plane]);
        EXPECT_FALSE(AZ::ShapeIntersection::Contains(m_frustum, testSphere)) << "Frustum contains sphere even though sphere is completely outside the frustum." << std::endl << "Frustum:" << std::endl << m_testCase << std::endl << "Sphere:" << std::endl << testSphere << std::endl;
    }

    // Tests that a sphere outside the frustum does not overlap the frustum
    TEST_P(FrustumIntersectionTests, SphereOverlapsFrustum_SphereOutsidePlane_False)
    {
        AZ::Sphere testSphere = GenerateSphereOutsidePlane(m_planes[m_testCase.plane], m_centerPoints[m_testCase.plane]);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(testSphere, m_frustum)) << "Sphere overlaps frustum even though sphere is completely outside the frustum." << std::endl << "Frustum:" << std::endl << m_testCase << std::endl << "Sphere:" << std::endl << testSphere << std::endl;
    }

    // Tests that a frustum contains a sphere that is inside the frustum
    TEST_P(FrustumIntersectionTests, FrustumContainsSphere_SphereInsidePlane_True)
    {
        AZ::Sphere testSphere = GenerateSphereInsidePlane(m_planes[m_testCase.plane], m_centerPoints[m_testCase.plane]);
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(m_frustum, testSphere)) << "Frustum does not contain sphere even though sphere is completely inside the frustum." << std::endl << "Frustum:" << std::endl << m_testCase << std::endl << "Sphere:" << std::endl << testSphere << std::endl;
    }

    // Tests that a sphere inside the frustum overlaps the frustum
    TEST_P(FrustumIntersectionTests, SphereOverlapsFrustum_SphereInsidePlane_True)
    {
        AZ::Sphere testSphere = GenerateSphereInsidePlane(m_planes[m_testCase.plane], m_centerPoints[m_testCase.plane]);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(testSphere, m_frustum)) << "Sphere does not overlap frustum even though sphere is completely inside the frustum." << std::endl << "Frustum:" << std::endl << m_testCase << std::endl << "Sphere:" << std::endl << testSphere << std::endl;
    }

    // Tests that a frustum does not contain a sphere that is half inside half outside the frustum
    TEST_P(FrustumIntersectionTests, FrustumContainsSphere_SphereHalfInsideHalfOutsidePlane_False)
    {
        AZ::Sphere testSphere(m_centerPoints[m_testCase.plane], m_minEdgeLength * .25f);
        EXPECT_FALSE(AZ::ShapeIntersection::Contains(m_frustum, testSphere)) << "Frustum contains sphere even though sphere is partially outside the frustum." << std::endl << "Frustum:" << std::endl << m_testCase << std::endl << "Sphere:" << std::endl << testSphere << std::endl;
    }

    // Tests that a sphere half inside half outside the frustum overlaps the frustum
    TEST_P(FrustumIntersectionTests, SphereOverlapsFrustum_SphereHalfInsideHalfOutsidePlane_True)
    {
        AZ::Sphere testSphere(m_centerPoints[m_testCase.plane], m_minEdgeLength * .25f);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(testSphere, m_frustum)) << "Sphere does not overlap frustum even though sphere is partially inside the frustum." << std::endl << "Frustum:" << std::endl << m_testCase << std::endl << "Sphere:" << std::endl << testSphere << std::endl;
    }

    std::vector<FrustumTestCase> GenerateFrustumIntersectionTestCases()
    {
        std::vector<FrustumTestCase> testCases;

        std::vector<FrustumTestCase> frustums;
        // 2x2x2 box (Z-up coordinate system)
        FrustumTestCase box;
        box.nearTopLeft = AZ::Vector3(-1.0f, -1.0f, 1.0f);
        box.nearTopRight = AZ::Vector3(1.0f, -1.0f, 1.0f);
        box.nearBottomLeft = AZ::Vector3(-1.0f, -1.0f, -1.0f);
        box.nearBottomRight = AZ::Vector3(1.0f, -1.0f, -1.0f);
        box.farTopLeft = AZ::Vector3(-1.0f, 1.0f, 1.0f);
        box.farTopRight = AZ::Vector3(1.0f, 1.0f, 1.0f);
        box.farBottomLeft = AZ::Vector3(-1.0f, 1.0f, -1.0f);
        box.farBottomRight = AZ::Vector3(1.0f, 1.0f, -1.0f);
        box.testCaseName = "BoxShapedFrustum";
        frustums.push_back(box);

        // Default values in a CCamera from Cry_Camera.h
        FrustumTestCase defaultCameraFrustum;
        defaultCameraFrustum.nearTopLeft = AZ::Vector3(-0.204621f, 0.200000f, 0.153465f);
        defaultCameraFrustum.nearTopRight = AZ::Vector3(0.204621f, 0.200000f, 0.153465f);
        defaultCameraFrustum.nearBottomLeft = AZ::Vector3(-0.204621f, 0.200000f, -0.153465f);
        defaultCameraFrustum.nearBottomRight = AZ::Vector3(0.204621f, 0.200000f, -0.153465f);
        defaultCameraFrustum.farTopLeft = AZ::Vector3(-1047.656982f, 1024.000000f, 785.742737f);
        defaultCameraFrustum.farTopRight = AZ::Vector3(1047.656982f, 1024.000000f, 785.742737f);
        defaultCameraFrustum.farBottomLeft = AZ::Vector3(-1047.656982f, 1024.000000f, -785.742737f);
        defaultCameraFrustum.farBottomRight = AZ::Vector3(1047.656982f, 1024.000000f, -785.742737f);
        defaultCameraFrustum.testCaseName = "DefaultCameraFrustum";
        frustums.push_back(defaultCameraFrustum);
        
        // These frustums were generated from flying around StarterGame and dumping the frustum values for the viewport camera and shadow cascade frustums to a log file
        FrustumTestCase starterGame0;
        starterGame0.nearTopLeft = AZ::Vector3(41656.351563f, 794907.750000f, -604483.687500f);
        starterGame0.nearTopRight = AZ::Vector3(41662.343750f, 794907.375000f, -604483.687500f);
        starterGame0.nearBottomLeft = AZ::Vector3(41656.164063f, 794904.125000f, -604488.437500f);
        starterGame0.nearBottomRight = AZ::Vector3(41662.156250f, 794903.750000f, -604488.437500f);
        starterGame0.farTopLeft = AZ::Vector3(41677.691406f, 795314.937500f, -604793.375000f);
        starterGame0.farTopRight = AZ::Vector3(41683.683594f, 795314.562500f, -604793.375000f);
        starterGame0.farBottomLeft = AZ::Vector3(41677.503906f, 795311.312500f, -604798.125000f);
        starterGame0.farBottomRight = AZ::Vector3(41683.496094f, 795310.937500f, -604798.125000f);
        starterGame0.testCaseName = "StarterGameFrustum0";
        frustums.push_back(starterGame0);

        FrustumTestCase starterGame1;
        starterGame1.nearTopLeft = AZ::Vector3(0.166996f, 0.240156f, 0.117468f);
        starterGame1.nearTopRight = AZ::Vector3(0.226816f, -0.184736f, 0.117423f);
        starterGame1.nearBottomLeft = AZ::Vector3(0.169258f, 0.240499f, -0.113460f);
        starterGame1.nearBottomRight = AZ::Vector3(0.229078f, -0.184393f, -0.113506f);
        starterGame1.farTopLeft = AZ::Vector3(83.497986f, 120.077904f, 58.734165f);
        starterGame1.farTopRight = AZ::Vector3(113.408234f, -92.368172f, 58.711285f);
        starterGame1.farBottomLeft = AZ::Vector3(84.628860f, 120.249550f, -56.730221f);
        starterGame1.farBottomRight = AZ::Vector3(114.539108f, -92.196526f, -56.753101f);
        starterGame1.testCaseName = "StarterGameFrustum1";
        frustums.push_back(starterGame1);

        FrustumTestCase starterGame2;
        starterGame2.nearTopLeft = AZ::Vector3(-0.007508f, 0.091724f, -0.043323f);
        starterGame2.nearTopRight = AZ::Vector3(0.018772f, 0.090096f, -0.043323f);
        starterGame2.nearBottomLeft = AZ::Vector3(-0.008393f, 0.077435f, -0.065421f);
        starterGame2.nearBottomRight = AZ::Vector3(0.017887f, 0.075807f, -0.065421f);
        starterGame2.farTopLeft = AZ::Vector3(-11.637362f, 142.172897f, -67.151001f);
        starterGame2.farTopRight = AZ::Vector3(29.096817f, 139.649338f, -67.151001f);
        starterGame2.farBottomLeft = AZ::Vector3(-13.009484f, 120.024765f, -101.403290f);
        starterGame2.farBottomRight = AZ::Vector3(27.724697f, 117.501205f, -101.403290f);
        starterGame2.testCaseName = "StarterGameFrustum2";
        frustums.push_back(starterGame2);

        FrustumTestCase starterGame3;
        starterGame3.nearTopLeft = AZ::Vector3(0.211831f, -0.207308f, 0.107297f);
        starterGame3.nearTopRight = AZ::Vector3(-0.217216f, -0.201659f, 0.107297f);
        starterGame3.nearBottomLeft = AZ::Vector3(0.211954f, -0.197980f, -0.123455f);
        starterGame3.nearBottomRight = AZ::Vector3(-0.217093f, -0.192331f, -0.123455f);
        starterGame3.farTopLeft = AZ::Vector3(8473.246094f, -8292.310547f, 4291.892578f);
        starterGame3.farTopRight = AZ::Vector3(-8688.623047f, -8066.359863f, 4291.892578f);
        starterGame3.farBottomLeft = AZ::Vector3(8478.158203f, -7919.196777f, -4938.199219f);
        starterGame3.farBottomRight = AZ::Vector3(-8683.710938f, -7693.245605f, -4938.199219f);
        starterGame3.testCaseName = "StarterGameFrustum3";
        frustums.push_back(starterGame3);

        // For each test frustum, create test cases for every plane
        for (FrustumTestCase frustum : frustums)
        {
            for (int i = 0; i < FrustumPlanes::NumPlanes; ++i)
            {
                frustum.plane = static_cast<FrustumPlanes>(i);
                testCases.push_back(frustum);
            }
        }

        return testCases;
    }

    std::string GenerateFrustumIntersectionTestCaseName(const ::testing::TestParamInfo<FrustumTestCase>& info)
    {
        std::string testCaseName = info.param.testCaseName;
        switch (info.param.plane)
        {
        case FrustumPlanes::Near:
            testCaseName += "_NearPlane";
            break;
        case FrustumPlanes::Far:
            testCaseName += "_FarPlane";
            break;
        case FrustumPlanes::Left:
            testCaseName += "_LeftPlane";
            break;
        case FrustumPlanes::Right:
            testCaseName += "_RightPlane";
            break;
        case FrustumPlanes::Top:
            testCaseName += "_TopPlane";
            break;
        case FrustumPlanes::Bottom:
            testCaseName += "_BottomPlane";
            break;
        }
        return testCaseName;
    }

    INSTANTIATE_TEST_CASE_P(FrustumIntersection, FrustumIntersectionTests, ::testing::ValuesIn(GenerateFrustumIntersectionTestCases()), GenerateFrustumIntersectionTestCaseName);
}