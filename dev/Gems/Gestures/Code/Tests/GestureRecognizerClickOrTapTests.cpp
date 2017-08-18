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
#include <Gestures/GestureRecognizerClickOrTap.h>
#include "BaseGestureTest.h"

namespace
{
    Gestures::RecognizerClickOrTap::Config singleTapOneSecond;
}

class MockListener
    : public Gestures::IClickOrTapListener
{
public:
    MockListener()
        : m_count(0)
    {
    }
    int m_count;

    void OnClickOrTapRecognized(const Gestures::RecognizerClickOrTap& recognizer) override
    {
        ++m_count;
    }
};

class SimpleTests
    : public BaseGestureTest
{
public:
    void SetUp() override
    {
        BaseGestureTest::SetUp();

        // configure
        singleTapOneSecond.maxSecondsHeld = 1.0f;
        singleTapOneSecond.minClicksOrTaps = 1;
    }

    void TearDown() override
    {
        BaseGestureTest::TearDown();
    }
};


TEST_F(SimpleTests, NoInput_DefaultConfig_NotRecognized)
{
    MockListener mockListener;
    Gestures::RecognizerClickOrTap recognizer(mockListener, singleTapOneSecond);
    ASSERT_EQ(0, mockListener.m_count);
}

TEST_F(SimpleTests, Tap_ZeroDuration_Recognized)
{
    MockListener mockListener;
    Gestures::RecognizerClickOrTap recognizer(mockListener, singleTapOneSecond);

    MouseDownAt(recognizer, 0.0f);
    MouseUpAt(recognizer, 0.0f);

    ASSERT_EQ(1, mockListener.m_count);
}

TEST_F(SimpleTests, Tap_LessThanMaxDuration_Recognized)
{
    MockListener mockListener;
    Gestures::RecognizerClickOrTap recognizer(mockListener, singleTapOneSecond);

    MouseDownAt(recognizer, 0.0f);
    MouseUpAt(recognizer, 0.9f);

    ASSERT_EQ(1, mockListener.m_count);
}

TEST_F(SimpleTests, Tap_GreaterThanMaxDuration_NotRecognized)
{
    MockListener mockListener;
    Gestures::RecognizerClickOrTap recognizer(mockListener, singleTapOneSecond);

    MouseDownAt(recognizer, 0.0f);
    MouseUpAt(recognizer, 1.1f);

    ASSERT_EQ(0, mockListener.m_count);
}

TEST_F(SimpleTests, Tap_MoveWithinLimits_Recognized)
{
    MockListener mockListener;
    singleTapOneSecond.maxPixelsMoved = 10.0f;
    Gestures::RecognizerClickOrTap recognizer(mockListener, singleTapOneSecond);

    MoveTo(0.0f, 0.0f);
    MouseDownAt(recognizer, 0.0f);
    MoveTo(9.9f, 0.0f);
    MouseUpAt(recognizer, 0.5f);

    ASSERT_EQ(1, mockListener.m_count);
}

TEST_F(SimpleTests, Tap_MoveOutsideLimits_NotRecognized)
{
    MockListener mockListener;
    singleTapOneSecond.maxPixelsMoved = 10.0f;
    Gestures::RecognizerClickOrTap recognizer(mockListener, singleTapOneSecond);

    MoveTo(0.0f, 0.0f);
    MouseDownAt(recognizer, 0.0f);
    MoveTo(10.1f, 0.0f);
    MouseUpAt(recognizer, 0.5f);

    ASSERT_EQ(0, mockListener.m_count);
}