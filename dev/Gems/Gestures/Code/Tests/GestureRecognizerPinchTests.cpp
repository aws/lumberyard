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
#include "StdAfx.h"
#include <AzTest/AzTest.h>
#include <Mocks/StubTimer.h>
#include <Gestures/GestureRecognizerPinch.h>
#include "BaseGestureTest.h"

namespace
{
    Gestures::RecognizerPinch::Config defaultConfig;
}


class PinchTests
    : public BaseGestureTest
{
public:
    void SetUp() override
    {
        BaseGestureTest::SetUp();
    }

    void TearDown() override
    {
        BaseGestureTest::TearDown();
    }
};

class MockPinchListener
    : public Gestures::IPinchListener
{
public:
    MockPinchListener()
        : m_initCount(0)
        , m_updateCount(0)
        , m_endCount(0)
    {
    }

    int m_initCount;
    int m_updateCount;
    int m_endCount;

    void OnPinchInitiated(const Gestures::RecognizerPinch& recognizer) override { ++m_initCount; }
    void OnPinchUpdated(const Gestures::RecognizerPinch& recognizer) override { ++m_updateCount; }
    void OnPinchEnded(const Gestures::RecognizerPinch& recognizer) override { ++m_endCount; }
};


TEST_F(PinchTests, Sanity_Pass)
{
    ASSERT_EQ(0, 0);
}

TEST_F(PinchTests, NoInput_DefaultConfig_NotRecognized)
{
    MockPinchListener mockListener;
    Gestures::RecognizerPinch pinch(mockListener, defaultConfig);
    ASSERT_EQ(0, mockListener.m_initCount);
}

TEST_F(PinchTests, Touch_OneFinger_InitNotCalled)
{
    MockPinchListener mockListener;
    Gestures::RecognizerPinch pinch(mockListener, defaultConfig);

    Press(pinch, 0, Vec2(ZERO), 0.0f);

    ASSERT_EQ(0, mockListener.m_initCount);
}

TEST_F(PinchTests, Touch_TwoFingersSlightlyApartNoMovement_InitNotCalled)
{
    MockPinchListener mockListener;
    Gestures::RecognizerPinch pinch(mockListener, defaultConfig);

    Press(pinch, 0, Vec2(ZERO), 0.0f);
    Press(pinch, 1, Vec2(0.5f), 0.0f);

    // both are down, but they haven't moved the "min pixels moved" distance
    // so the gesture has not been initialized
    ASSERT_EQ(0, mockListener.m_initCount);
}

TEST_F(PinchTests, PinchOutward_GreaterThanMinDistance_InitCalled)
{
    MockPinchListener mockListener;
    Gestures::RecognizerPinch::Config config;
    config.minPixelsMoved = 10.0f;
    Gestures::RecognizerPinch pinch(mockListener, config);

    Press(pinch, 0, Vec2(ZERO), 0.0f);
    Press(pinch, 1, Vec2(ZERO), 0.0f);
    Move(pinch, 0, Vec2(-5.01f, 0.0f), 1.0f);
    Move(pinch, 1, Vec2(5.01f, 0.0f), 1.0f);

    ASSERT_EQ(1, mockListener.m_initCount);
}

TEST_F(PinchTests, PinchInward_GreaterThanMinDistance_InitCalled)
{
    MockPinchListener mockListener;
    Gestures::RecognizerPinch::Config config;
    config.minPixelsMoved = 10.0f;
    Gestures::RecognizerPinch pinch(mockListener, config);

    Press(pinch, 0, Vec2(-15.01f, 0.0f), 0.0f);
    Press(pinch, 1, Vec2(15.01f, 0.0f), 0.0f);
    Move(pinch, 0, Vec2(-10.00f, 0.0f), 1.0f);
    Move(pinch, 1, Vec2(10.00f, 0.0f), 1.0f);

    ASSERT_EQ(1, mockListener.m_initCount);
}

TEST_F(PinchTests, ReleaseBothTouches_AfterInitialized_EndedCalled)
{
    MockPinchListener mockListener;
    Gestures::RecognizerPinch::Config config;
    config.minPixelsMoved = 10.0f;
    Gestures::RecognizerPinch pinch(mockListener, config);

    const Vec2 end(5.01f, 0.0f);
    Press(pinch, 0, Vec2(ZERO), 0.0f);
    Press(pinch, 1, Vec2(ZERO), 0.0f);
    Move(pinch, 0, -end, 1.0f);
    Move(pinch, 1, end, 1.0f);
    Release(pinch, 0, -end, 2.0f);
    Release(pinch, 1, end, 2.0f);

    ASSERT_EQ(1, mockListener.m_initCount);
    ASSERT_EQ(1, mockListener.m_endCount);
}

TEST_F(PinchTests, ReleaseOneTouch_AfterInitialized_EndedCalled)
{
    MockPinchListener mockListener;
    Gestures::RecognizerPinch::Config config;
    config.minPixelsMoved = 10.0f;
    Gestures::RecognizerPinch pinch(mockListener, config);

    const Vec2 end(5.01f, 0.0f);
    Press(pinch, 0, Vec2(ZERO), 0.0f);
    Press(pinch, 1, Vec2(ZERO), 0.0f);
    Move(pinch, 0, -end, 1.0f);
    Move(pinch, 1, end, 1.0f);
    Release(pinch, 0, -end, 2.0f);
    //Release(pinch, 1, end, 2.0f);

    ASSERT_EQ(1, mockListener.m_initCount);
    ASSERT_EQ(1, mockListener.m_endCount);
}

TEST_F(PinchTests, ReleaseTouches_PinchNeverStarted_NoInitNoEnd)
{
    MockPinchListener mockListener;
    Gestures::RecognizerPinch::Config config;
    config.minPixelsMoved = 10.0f;
    Gestures::RecognizerPinch pinch(mockListener, config);

    const Vec2 start(10.0f, 0.0f);
    const Vec2 end(9.0f, 0.0f); // not enough to initiate a pinch

    Press(pinch, 0, -start, 0.0f);
    Press(pinch, 1, start, 0.0f);
    Move(pinch, 0, -end, 1.0f);
    Move(pinch, 1, end, 1.0f);
    Release(pinch, 0, -end, 2.0f);
    Release(pinch, 1, end, 2.0f);

    ASSERT_EQ(0, mockListener.m_initCount);
    ASSERT_EQ(0, mockListener.m_endCount);
}