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

#include "LmbrCentral_precompiled.h"
#include <AzTest/AzTest.h>

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/MathUtils.h>

#include <ISystem.h>

#include <LmbrCentral/Animation/MotionExtraction.h>

using ::testing::NiceMock;

TEST(AnimationMotionParameterExtraction, BasicExtractionTest)
{
    AZ::Quaternion entityOrientation = AZ::Quaternion::CreateRotationZ(AZ::Constants::Pi);
    AZ::Vector3 entityPosition(10.f, 20.f, 30.f);

    AZ::Quaternion frameRotation = AZ::Quaternion::CreateRotationZ(-AZ::Constants::HalfPi);
    AZ::Vector3 frameTranslation(-1.f, 0.f, 0.f);

    // Ground is oriented downhill with respect to our movement direction.
    const float groundSlope = -0.2;
    AZ::Vector3 groundNormal = AZ::Quaternion::CreateRotationZ(AZ::Constants::HalfPi) * AZ::Quaternion::CreateRotationX(-groundSlope) * AZ::Vector3::CreateAxisZ();

    const float timeStep = 0.016f;

    LmbrCentral::Animation::MotionParameters params = LmbrCentral::Animation::ExtractMotionParameters(
        timeStep, 
        AZ::Transform::CreateFromQuaternionAndTranslation(entityOrientation, entityPosition),
        AZ::Transform::CreateFromQuaternionAndTranslation(frameRotation, frameTranslation),
        groundNormal);

    float travelAngle = atan2f(-params.m_relativeTravelDelta.GetX(), params.m_relativeTravelDelta.GetY());

    EXPECT_NEAR(params.m_groundAngleSigned, groundSlope, 0.01f);                       // -0.2 rads ground angle (slightly downhill).
    EXPECT_NEAR(travelAngle, AZ::Constants::HalfPi, 0.01f);                            // Traveling to our left (positive in blend-space land).
    EXPECT_NEAR(params.m_travelDistance, 1.f / cosf(groundSlope), 0.01f);              // 1 m travel distance on slight downward slope.
    EXPECT_NEAR(params.m_travelSpeed, (1.f / cosf(groundSlope)) / timeStep, 0.01f);    // 1 m / timeStep.
    EXPECT_NEAR(params.m_turnAngle, -AZ::Constants::HalfPi, 0.01f);                    // Turn angle is -HalfPi.
    EXPECT_NEAR(params.m_turnSpeed, -AZ::Constants::HalfPi / timeStep, 0.01f);         // Turn speed is -HalfPi / timeStep.
}
