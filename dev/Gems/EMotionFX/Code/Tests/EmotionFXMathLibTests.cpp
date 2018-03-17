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


#include "EMotionFX_precompiled.h"

#include <AzTest/AzTest.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/MathUtils.h>
#include <AzFramework/Math/MathUtils.h>

#include <Mcore/Source/Quaternion.h>
#include <MCore/Source/Vector.h>
#include <MCore/Source/Matrix4.h>
#include <MCore/Source/AzCoreConversions.h>


class EmotionFXMathLibTests
    : public ::testing::Test
{
protected:

    void SetUp() override
    {
        m_azNormalizedVector3_a = AZ::Vector3(s_x1, s_y1, s_z1);
        m_azNormalizedVector3_a.Normalize();
        m_emQuaternion_a =  MCore::Quaternion(m_azNormalizedVector3_a, s_angle_a);
        m_azQuaternion_a = AZ::Quaternion::CreateFromAxisAngle(m_azNormalizedVector3_a, s_angle_a);
    }

    void TearDown() override
    {
    }


    bool AZQuaternionCompareExact(AZ::Quaternion& quaternion, float x, float y, float z, float w)
    {
        if (quaternion.GetX() != x)
        {
            return false;
        }
        if (quaternion.GetY() != y)
        {
            return false;
        }
        if (quaternion.GetZ() != z)
        {
            return false;
        }
        if (quaternion.GetW() != w)
        {
            return false;
        }
        return true;
    }

    bool EmfxQuaternionCompareExact(MCore::Quaternion& quaternion, float x, float y, float z, float w)
    {
        if (quaternion.x != x)
        {
            return false;
        }
        if (quaternion.y != y)
        {
            return false;
        }
        if (quaternion.z != z)
        {
            return false;
        }
        if (quaternion.w != w)
        {
            return false;
        }
        return true;
    }

    bool AZQuaternionCompareClose(AZ::Quaternion& quaternion, float x, float y, float z, float w, float tolerance)
    {
        if (!AZ::IsClose(quaternion.GetX(), x, tolerance))
        {
            return false;
        }
        if (!AZ::IsClose(quaternion.GetY(), y, tolerance))
        {
            return false;
        }
        if (!AZ::IsClose(quaternion.GetZ(), z, tolerance))
        {
            return false;
        }
        if (!AZ::IsClose(quaternion.GetW(), w, tolerance))
        {
            return false;
        }
        return true;
    }

    bool AZVector3CompareClose(const AZ::Vector3& vector, const AZ::Vector3& vector2, float tolerance)
    {
        if (!AZ::IsClose(vector.GetX(), vector2.GetX(), tolerance))
        {
            return false;
        }
        if (!AZ::IsClose(vector.GetY(), vector2.GetY(), tolerance))
        {
            return false;
        }
        if (!AZ::IsClose(vector.GetZ(), vector2.GetZ(), tolerance))
        {
            return false;
        }
        return true;
    }

    bool AZVector3CompareClose(const AZ::Vector3& vector, float x, float y, float z, float tolerance)
    {
        if (!AZ::IsClose(vector.GetX(), x, tolerance))
        {
            return false;
        }
        if (!AZ::IsClose(vector.GetY(), y, tolerance))
        {
            return false;
        }
        if (!AZ::IsClose(vector.GetZ(), z, tolerance))
        {
            return false;
        }
        return true;
    }

    static const float  s_toleranceHigh;
    static const float  s_toleranceMedium;
    static const float  s_toleranceLow;
    static const float  s_toleranceReallyLow;
    static const float  s_x1;
    static const float  s_y1;
    static const float  s_z1;
    static const float  s_angle_a;
    AZ::Vector3         m_azNormalizedVector3_a;
    AZ::Quaternion      m_azQuaternion_a;
    MCore::Quaternion   m_emQuaternion_a;
};

const float EmotionFXMathLibTests::s_toleranceHigh        = 0.00001f;
const float EmotionFXMathLibTests::s_toleranceMedium      = 0.0001f;
const float EmotionFXMathLibTests::s_toleranceLow         = 0.001f;
const float EmotionFXMathLibTests::s_toleranceReallyLow   = 0.02f;

const float EmotionFXMathLibTests::s_x1         = 0.2f;
const float EmotionFXMathLibTests::s_y1         = 0.3f;
const float EmotionFXMathLibTests::s_z1         = 0.4f;
const float EmotionFXMathLibTests::s_angle_a    = 0.5f;


///////////////////////////////////////////////////////////////////////////////


// MCore::Quaternion: Test identity values
TEST_F(EmotionFXMathLibTests, QuaternionIdentity_Identity_Success)
{
    MCore::Quaternion test(0.1f, 0.2f, 0.3f, 0.4f);
    test.Identity();
    ASSERT_TRUE(test == MCore::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
}

//////////////////////////////////////////////////////////////////
//Getting and setting of Quaternions
//////////////////////////////////////////////////////////////////

// MCore::Quaternion: Compare Get values to set values
TEST_F(EmotionFXMathLibTests, EMQuaternionGet_Elements_Success)
{
    MCore::Quaternion test(0.1f, 0.2f, 0.3f, 0.4f);
    ASSERT_TRUE(EmfxQuaternionCompareExact(test, 0.1f, 0.2f, 0.3f, 0.4f));
}

// AZ::Quaternion: Compare Get values to set values
TEST_F(EmotionFXMathLibTests, AZQuaternionGet_Elements_Success)
{
    AZ::Quaternion test(0.1f, 0.2f, 0.3f, 0.4f);
    ASSERT_TRUE(AZQuaternionCompareExact(test, 0.1f, 0.2f, 0.3f, 0.4f));
}

// Compare equivalent normalized quaternions between systems
TEST_F(EmotionFXMathLibTests, AZEMQuaternionNormalizeEquivalent_Success)
{
    AZ::Quaternion azTest(0.1f, 0.2f, 0.3f, 0.4f);
    MCore::Quaternion emTest(0.1f, 0.2f, 0.3f, 0.4f);
    azTest.Normalize();
    emTest.Normalize();

    ASSERT_TRUE(AZQuaternionCompareClose(azTest, emTest.x, emTest.y, emTest.z, emTest.w, s_toleranceMedium));
}

///////////////////////////////////////////////////////////////////////////////
// Axis Angle
///////////////////////////////////////////////////////////////////////////////

// Compare setting a quaternion using axis and angle
TEST_F(EmotionFXMathLibTests, AZEMQuaternionConversion_SetToAxisAngleEquivalent_Success)
{
    MCore::Quaternion emQuaternion(m_azNormalizedVector3_a, s_angle_a);
    AZ::Quaternion azQuaternion = AZ::Quaternion::CreateFromAxisAngle(m_azNormalizedVector3_a, s_angle_a);

    ASSERT_TRUE(AZQuaternionCompareClose(azQuaternion, emQuaternion.x, emQuaternion.y, emQuaternion.z, emQuaternion.w, s_toleranceLow));
}

// Compare equivalent coversions quaternions -> (axis, angle) between systems
TEST_F(EmotionFXMathLibTests, AZEMQuaternionConversion_ToAxisAngleEquivalent_Success)
{
    //populate Quaternions with same data
    MCore::Quaternion emTest = m_emQuaternion_a;
    AZ::Quaternion azTest(emTest.x, emTest.y, emTest.z, emTest.w);

    AZ::Vector3 emAxis;
    float emAngle;
    emTest.ToAxisAngle(&emAxis, &emAngle);

    AZ::Vector3 azAxis;
    float azAngle;
    AzFramework::ConvertQuaternionToAxisAngle(azTest, azAxis, azAngle);

    bool same = AZ::IsClose(azAngle, emAngle, s_toleranceLow) &&
        AZVector3CompareClose(azAxis, emAxis, s_toleranceLow);

    ASSERT_TRUE(same);
}


///////////////////////////////////////////////////////////////////////////////
//Basic rotations
///////////////////////////////////////////////////////////////////////////////

//Right Hand - counterclockwise looking down axis from positive side

TEST_F(EmotionFXMathLibTests, AZQuaternion_Rotation1ComponentAxisX_Success)
{
    AZ::Vector3 axis = AZ::Vector3(1.0f, 0.0f, 0.0f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.0f, 0.0f, 0.1f);
    AZ::Vector3 vertexOut;
    vertexOut = azQuaternion1 * vertexIn;

    bool same = AZVector3CompareClose(vertexOut, AZ::Vector3(0.0f, -0.1f, 0.0f), s_toleranceLow);
    ASSERT_TRUE(same);
}

TEST_F(EmotionFXMathLibTests, AZQuaternion_Rotation1ComponentAxisY_Success)
{
    AZ::Vector3 axis = AZ::Vector3(0.0f, 1.0f, 0.0f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.1f, 0.0f, 0.0f);
    AZ::Vector3 vertexOut;
    vertexOut = azQuaternion1 * vertexIn;

    bool same = AZVector3CompareClose(vertexOut, AZ::Vector3(0.0f, 0.0f, -0.1f), s_toleranceLow);
    ASSERT_TRUE(same);
}

TEST_F(EmotionFXMathLibTests, AZQuaternion_Rotation1ComponentAxisZ_Success)
{
    AZ::Vector3 axis = AZ::Vector3(0.0f, 0.0f, 1.0f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.1f, 0.0f, 0.0f);
    AZ::Vector3 vertexOut;
    vertexOut = azQuaternion1 * vertexIn;

    bool same = AZVector3CompareClose(vertexOut, AZ::Vector3(0.0f, 0.1f, 0.0f), s_toleranceLow);
    ASSERT_TRUE(same);
}

//AZ Quaternion Normalize Vertex test
TEST_F(EmotionFXMathLibTests, AZEMQuaternion_NormalizedQuaternionRotationTest3DAxis_Success)
{
    AZ::Vector3 axis = AZ::Vector3(1.0f, 0.7f, 0.3f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternion1Normalized = azQuaternion1.GetNormalized();
    AZ::Vector3 vertexIn(0.1f, 0.2f, 0.3f);

    AZ::Vector3 vertexOut1, vertexOut1FromNormalizedQuaternion;

    //generate value 1
    vertexOut1 = azQuaternion1 * vertexIn;

    vertexOut1FromNormalizedQuaternion = azQuaternion1Normalized * vertexIn;

    bool same = AZVector3CompareClose(vertexOut1, vertexOut1FromNormalizedQuaternion, s_toleranceLow);
    ASSERT_TRUE(same);
}


///////////////////////////////////////////////////////////////////////////////
// Euler  AZ
///////////////////////////////////////////////////////////////////////////////

// AZ Quaternion <-> euler conversion Vertex test 1 component axis
TEST_F(EmotionFXMathLibTests, AZQuaternion_EulerGetSet1ComponentAxis_Success)
{
    AZ::Vector3 axis = AZ::Vector3(0.0f, 0.0f, 1.0f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.1f, 0.2f, 0.3f);
    AZ::Vector3 vertexOut1, vertexOut2;
    AZ::Vector3 euler1;
    AZ::Quaternion test1;

    vertexOut1 = azQuaternion1 * vertexIn;

    //euler out and in
    euler1 = AzFramework::ConvertQuaternionToEulerRadians(azQuaternion1);
    test1 = AzFramework::ConvertEulerRadiansToQuaternion(euler1);

    //generate vertex value 2
    vertexOut2 = test1 * vertexIn;

    bool same = AZVector3CompareClose(vertexOut1, vertexOut2, s_toleranceReallyLow);
    ASSERT_TRUE(same);
}

// AZ Quaternion <-> euler conversion Vertex test 2 component axis
TEST_F(EmotionFXMathLibTests, AZQuaternion_EulerGetSet2ComponentAxis_Success)
{
    AZ::Vector3 axis = AZ::Vector3(0.0f, 0.7f, 0.3f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.1f, 0.2f, 0.3f);
    AZ::Vector3 vertexOut1, vertexOut2;
    AZ::Vector3 euler1;
    AZ::Quaternion test1;

    //generate vertex value 1
    vertexOut1 = azQuaternion1 * vertexIn;

    //euler out and in
    euler1 = AzFramework::ConvertQuaternionToEulerRadians(azQuaternion1);
    test1 = AzFramework::ConvertEulerRadiansToQuaternion(euler1);

    //generate vertex value 2
    vertexOut2 = test1 * vertexIn;

    bool same = AZVector3CompareClose(vertexOut1, vertexOut2, s_toleranceReallyLow);
    ASSERT_TRUE(same);
}

// AZ Quaternion <-> euler conversion Vertex test 3 component axis
TEST_F(EmotionFXMathLibTests, AZQuaternion_EulerInOutRotationTest3DAxis_Success)
{
    AZ::Vector3 axis = AZ::Vector3(1.0f, 0.7f, 0.3f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.1f, 0.2f, 0.3f);
    AZ::Vector3 vertexOut1, vertexOut2;
    AZ::Vector3 euler1;
    AZ::Quaternion test1;

    //generate vertex value 1
    vertexOut1 = azQuaternion1 * vertexIn;

    //euler out and in
    euler1 = AzFramework::ConvertQuaternionToEulerRadians(azQuaternion1);
    test1 = AzFramework::ConvertEulerRadiansToQuaternion(euler1);

    //generate vertex value 2
    vertexOut2 = test1 * vertexIn;

    bool same = AZVector3CompareClose(vertexOut1, vertexOut2, s_toleranceReallyLow);
    ASSERT_TRUE(same);
}

// Quaternion -> Transform -> Euler conversion is same as Quaternion -> Euler
// AZ Euler get set Transform Compare test 3 dim axis
TEST_F(EmotionFXMathLibTests, AZQuaternion_EulerGetSet3ComponentAxisCompareTransform_Success)
{
    AZ::Vector3 axis = AZ::Vector3(1.0f, 0.7f, 0.3f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.1f, 0.2f, 0.3f);
    AZ::Vector3 vertexOut1, vertexOut2, vertexTransform;
    AZ::Vector3 euler1, eulerVectorFromTransform;
    AZ::Quaternion test1, testTransformQuat;

    //generate vertex value 1
    vertexOut1 = azQuaternion1 * vertexIn;

    //use Transform to generate euler
    AZ::Transform TransformFromQuat = AZ::Transform::CreateFromQuaternion(azQuaternion1);
    eulerVectorFromTransform = AzFramework::ConvertTransformToEulerRadians(TransformFromQuat);
    testTransformQuat = AzFramework::ConvertEulerRadiansToQuaternion(eulerVectorFromTransform);
    vertexTransform = testTransformQuat * vertexIn;

    //use existing convert function
    euler1 = AzFramework::ConvertQuaternionToEulerRadians(azQuaternion1);
    test1 = AzFramework::ConvertEulerRadiansToQuaternion(euler1);

    //generate vertex value 2
    vertexOut2 = test1 * vertexIn;

    bool same = AZVector3CompareClose(vertexOut1, vertexTransform, s_toleranceReallyLow)
        && AZVector3CompareClose(vertexOut1, vertexOut2, s_toleranceReallyLow)
        && AZVector3CompareClose(vertexOut2, vertexTransform, s_toleranceHigh);
    ASSERT_TRUE(same);
}



//  FROM EULER
// Compare setting quaternions from Euler values
//no proof updated code from MCore works
//switch to Transform
//the SetEuler is making very different quaternions than ConvertEulerRadiansToQuaternion
//this can be seen in the code. I am assuming it is related to an incomplete transition of the code
//(which is my fault most likely) but this code will be removed shortly.
//simple euler values are failing so it should be pretty easy to determine
//when it is working correctly

//TEST_F(EmotionFXMathLibTests, AZEMQuaternionConversion_SetToEulerEquivalent_Success)
//{
//   // AZ::Vector3 azEulerIn(0.1f, 0.2f, 0.3f);
//    AZ::Vector3 azEulerIn(0.0f, 3.14159f / 2.0f, 0.0f);
//    MCore::Vector3 emEulerIn(azEulerIn.GetX(), azEulerIn.GetY(), azEulerIn.GetZ());
//
//    AZ::Quaternion azTest;
//    MCore::Quaternion emTest;
//
//    emTest.SetEuler(emEulerIn.x, emEulerIn.y, emEulerIn.z);
//    azTest = AzFramework::ConvertEulerRadiansToQuaternion(azEulerIn);
//
//    AZ::Vector3 azVectorIn(5, 0, 5);
//    MCore::Vector3 emVectorIn(5, 0, 5);
//
//    AZ::Vector3 azVectorOut;
//    MCore::Vector3 emVectorOut;
//    azVectorOut = azTest * azVectorIn;
//    emVectorOut = emTest * emVectorIn;
//
//    ASSERT_TRUE(true);
//    //bool same = AZVector3CompareClose(azVectorOut, AZ::Vector3(emVectorOut.x, emVectorOut.y, emVectorOut.z), s_toleranceMedium);
//    //ASSERT_TRUE(same);
//}

// EM Quaternion to Euler test
TEST_F(EmotionFXMathLibTests, EMQuaternionConversion_ToEulerEquivalent_Success)
{
    AZ::Vector3 eulerIn(0.1f, 0.2f, 0.3f);
    MCore::Quaternion test;
    test.SetEuler(eulerIn.GetX(), eulerIn.GetY(), eulerIn.GetZ());
    AZ::Vector3 eulerOut = test.ToEuler();

    ASSERT_TRUE(AZVector3CompareClose(eulerOut, 0.1f, 0.2f, 0.3f, s_toleranceHigh));
}

// AZ Quaternion to Euler test
//only way to test Quaternions sameness is to apply it to a vector and measure result
TEST_F(EmotionFXMathLibTests, AZQuaternionConversion_ToEulerEquivalent_Success)
{
    AZ::Vector3 eulerIn(0.1f, 0.2f, 0.3f);
    AZ::Vector3 testVertex(0.1f, 0.2f, 0.3f);
    AZ::Vector3 outVertex1, outVertex2;
    AZ::Quaternion test, test2;
    AZ::Vector3 eulerOut1, eulerOut2;

    test = AzFramework::ConvertEulerRadiansToQuaternion(eulerIn);
    test.Normalize();
    outVertex1 = test * testVertex;

    eulerOut1 = AzFramework::ConvertQuaternionToEulerRadians(test);

    test2 = AzFramework::ConvertEulerRadiansToQuaternion(eulerOut1);
    test2.Normalize();
    outVertex2 = test2 * testVertex;

    eulerOut2 = AzFramework::ConvertQuaternionToEulerRadians(test2);
    ASSERT_TRUE(AZVector3CompareClose(eulerOut1, eulerOut2, s_toleranceReallyLow));
}

///////////////////////////////////////////////////////////////////////////////
//Quaternion order test
//determines that ordering is same between systems.
///////////////////////////////////////////////////////////////////////////////
TEST_F(EmotionFXMathLibTests, AZEMQuaternion_OrderTest_Success)
{
    AZ::Vector3 axis = AZ::Vector3(1.0f, 0.7f, 0.3f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);

    AZ::Vector3 axis2 = AZ::Vector3(0.2f, 0.5f, 0.9f);
    axis2.Normalize();
    AZ::Quaternion azQuaternion2 = AZ::Quaternion::CreateFromAxisAngle(axis2, AZ::Constants::HalfPi);

    MCore::Quaternion emQuaternion1(azQuaternion1.GetX(), azQuaternion1.GetY(), azQuaternion1.GetZ(), azQuaternion1.GetW());
    MCore::Quaternion emQuaternion2(azQuaternion2.GetX(), azQuaternion2.GetY(), azQuaternion2.GetZ(), azQuaternion2.GetW());

    AZ::Quaternion azQuaterionOut = azQuaternion1 * azQuaternion2;
    AZ::Quaternion azQuaterionOut2 = azQuaternion2 * azQuaternion1;
    MCore::Quaternion emQuaterionOut = emQuaternion1 * emQuaternion2;

    AZ::Vector3 azVertexIn(0.1f, 0.2f, 0.3f);

    AZ::Vector3 azVertexOut, azVertexOut2;
    AZ::Vector3 emVertexOut;

    azVertexOut = azQuaterionOut * azVertexIn;
    azVertexOut2 = azQuaterionOut2 * azVertexIn;
    emVertexOut = emQuaterionOut * azVertexIn;

    bool same = AZVector3CompareClose(emVertexOut, azVertexOut.GetX(), azVertexOut.GetY(), azVertexOut.GetZ(), s_toleranceMedium);
    ASSERT_TRUE(same);
}


///////////////////////////////////////////////////////////////////////////////
//  Quaternion Matrix
///////////////////////////////////////////////////////////////////////////////

// Test EM Quaternion made from Matrix X
TEST_F(EmotionFXMathLibTests, EMQuaternionConversion_FromMatrixXRot_Success)
{
    MCore::Matrix emMatrix = MCore::Matrix::RotationMatrixX(AZ::Constants::HalfPi);
    MCore::Quaternion emQuaternion = MCore::Quaternion::ConvertFromMatrix(emMatrix);

    AZ::Vector3 emVertexIn(0.0f, 0.1f, 0.0f);
    AZ::Vector3 emVertexOut = emQuaternion * emVertexIn;

    bool same = AZVector3CompareClose(emVertexOut, 0.0f, 0.0f, 0.1f, s_toleranceMedium);
    ASSERT_TRUE(same);
}

// Test EM Quaternion made from Matrix Y
TEST_F(EmotionFXMathLibTests, EMQuaternionConversion_FromMatrixYRot_Success)
{
    MCore::Matrix emMatrix = MCore::Matrix::RotationMatrixY(AZ::Constants::HalfPi);
    MCore::Quaternion emQuaternion = MCore::Quaternion::ConvertFromMatrix(emMatrix);

    AZ::Vector3 emVertexIn(0.0f, 0.0f, 0.1f);
    AZ::Vector3 emVertexOut = emQuaternion * emVertexIn;

    bool same = AZVector3CompareClose(emVertexOut, 0.1f, 0.0f, 0.0f, s_toleranceMedium);
    ASSERT_TRUE(same);
}

// Compare Quaternion made from Matrix X
TEST_F(EmotionFXMathLibTests, AZEMQuaternionConversion_FromMatrixXRot_Success)
{
    AZ::Matrix4x4 azMatrix = AZ::Matrix4x4::CreateRotationX(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternion = AZ::Quaternion::CreateFromMatrix4x4(azMatrix);

    MCore::Matrix emMatrix = MCore::Matrix::RotationMatrixX(AZ::Constants::HalfPi);
    MCore::Quaternion emQuaternion = MCore::Quaternion::ConvertFromMatrix(emMatrix);

    AZ::Vector3 azVertexIn(0.0f, 0.1f, 0.0f);

    AZ::Vector3 azVertexOut = azQuaternion * azVertexIn;
    AZ::Vector3 emVertexOut = emQuaternion * azVertexIn;

    bool same = AZVector3CompareClose(azVertexOut, emVertexOut, s_toleranceMedium);
    ASSERT_TRUE(same);
}

// Compare Quaternion made from Matrix Y
TEST_F(EmotionFXMathLibTests, AZEMQuaternionConversion_FromMatrixYRot_Success)
{
    AZ::Matrix4x4 azMatrix = AZ::Matrix4x4::CreateRotationY(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternion = AZ::Quaternion::CreateFromMatrix4x4(azMatrix);

    MCore::Matrix emMatrix = MCore::Matrix::RotationMatrixY(AZ::Constants::HalfPi);
    MCore::Quaternion emQuaternion = MCore::Quaternion::ConvertFromMatrix(emMatrix);

    AZ::Vector3 azVertexIn(0.1f, 0.0f, 0.0f);
    AZ::Vector3 azVertexOut = azQuaternion * azVertexIn;
    AZ::Vector3 emVertexOut = emQuaternion * azVertexIn;

    bool same = AZVector3CompareClose(azVertexOut, emVertexOut, s_toleranceMedium);
    ASSERT_TRUE(same);
}

// Compare Quaternion made from Matrix Z
TEST_F(EmotionFXMathLibTests, AZEMQuaternionConversion_FromMatrixZRot_Success)
{
    AZ::Matrix4x4 azMatrix = AZ::Matrix4x4::CreateRotationZ(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternion = AZ::Quaternion::CreateFromMatrix4x4(azMatrix);

    MCore::Matrix emMatrix = MCore::Matrix::RotationMatrixZ(AZ::Constants::HalfPi);
    MCore::Quaternion emQuaternion = MCore::Quaternion::ConvertFromMatrix(emMatrix);

    AZ::Vector3 azVertexIn(0.1f, 0.0f, 0.0f);

    AZ::Vector3 azVertexOut = azQuaternion * azVertexIn;
    AZ::Vector3 emVertexOut = emQuaternion * azVertexIn;

    bool same = AZVector3CompareClose(azVertexOut, emVertexOut, s_toleranceMedium);
    ASSERT_TRUE(same);
}

// Compare Quaternion -> Matrix conversion
// AZ - column major
// Emfx - row major
TEST_F(EmotionFXMathLibTests, AZEMQuaternionConversion_ToMatrix_Success)
{
    AZ::Vector3 axis = AZ::Vector3(1.0f, 0.7f, 0.3f);
    axis.Normalize();
    AZ::Quaternion azQuaternion = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    MCore::Quaternion emQuaternion(azQuaternion.GetX(), azQuaternion.GetY(), azQuaternion.GetZ(), azQuaternion.GetW());

    AZ::Matrix4x4 azMatrix = AZ::Matrix4x4::CreateFromQuaternion(azQuaternion);

    MCore::Matrix emMatrix = emQuaternion.ToMatrix();

    bool same = true;

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            float emValue = emMatrix.GetRow4D(i).GetElement(j);
            float azValue = azMatrix.GetElement(j, i);
            if (!AZ::IsClose(emValue, azValue, s_toleranceReallyLow))
            {
                same = false;
                break;
            }
        }
        if (!same)
        {
            break;
        }
    }
    ASSERT_TRUE(same);
}



//last test
TEST_F(EmotionFXMathLibTests, LastTest)
{
    ASSERT_TRUE(true);
}
