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
#include <Mocks/IInputMock.h>
#include <Mocks/IRecognizerMock.h>
#include <Mocks/StubTimer.h>
#include <Tests/CryMathPrinters.h>
#include "BaseGestureTest.h"
#include "../Source/GestureManager.h"



////////////////////////////////////////////////////////////////////////////////////////////////////
class BaseGestureManagerTest
    : public ::testing::Test
{
public:

    void SetUp() override
    {
        gEnv = new(AZ_OS_MALLOC(sizeof(SSystemGlobalEnvironment), alignof(SSystemGlobalEnvironment))) SSystemGlobalEnvironment();

        m_stubInput = new InputMock();
        gEnv->pInput = m_stubInput;
        gEnv->pGame = nullptr;
    }

    void TearDown() override
    {
        delete m_stubInput;
        gEnv->pInput = nullptr;
        gEnv->~SSystemGlobalEnvironment();
        AZ_OS_FREE(gEnv);
        gEnv = nullptr;
    }

    InputMock* m_stubInput;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class GestureManagerTest
    : public BaseGestureManagerTest
{
};

TEST_F(GestureManagerTest, Register_NoneRegister_OneRecognizerRegistered)
{
    Gestures::RecognizerMock mockRecognizer;
    Gestures::Manager mgr;
    mgr.Register(mockRecognizer);
    ASSERT_EQ(1, mgr.GetRecognizerCount());
}

TEST_F(GestureManagerTest, Register_AlreadyRegistered_DoesNotRegisterTwice)
{
    Gestures::RecognizerMock mockRecognizer;
    Gestures::Manager mgr;
    mgr.Register(mockRecognizer);
    mgr.Register(mockRecognizer);
    ASSERT_EQ(1, mgr.GetRecognizerCount());
}

TEST_F(GestureManagerTest, Deregister_AlreadyRegistered_RemovesRecognizer)
{
    Gestures::RecognizerMock mockRecognizer;
    Gestures::Manager mgr;
    mgr.Register(mockRecognizer);
    ASSERT_EQ(1, mgr.GetRecognizerCount());
    mgr.Deregister(mockRecognizer);
    ASSERT_EQ(0, mgr.GetRecognizerCount());
}

TEST_F(GestureManagerTest, Event_Mouse1Pressed_PassedToRecognizerAsPressed0)
{
    using ::testing::_;

    Gestures::RecognizerMock mockRecognizer;
    Gestures::Manager mgr;
    EXPECT_CALL(mockRecognizer, OnPressedEvent(_, 0));

    mgr.Register(mockRecognizer);

    SInputEvent mouse1Pressed;
    mouse1Pressed.deviceType = EInputDeviceType::eIDT_Mouse;
    mouse1Pressed.state = EInputState::eIS_Pressed;
    mouse1Pressed.keyId = eKI_Mouse1;

    mgr.OnInputEvent(mouse1Pressed);
}

TEST_F(GestureManagerTest, Event_Mouse2Pressed_PassedToRecognizerAsPressed1)
{
    using ::testing::_;

    Gestures::RecognizerMock mockRecognizer;
    Gestures::Manager mgr;
    EXPECT_CALL(mockRecognizer, OnPressedEvent(_, 1));

    mgr.Register(mockRecognizer);

    SInputEvent mouse2Pressed;
    mouse2Pressed.deviceType = EInputDeviceType::eIDT_Mouse;
    mouse2Pressed.state = EInputState::eIS_Pressed;
    mouse2Pressed.keyId = eKI_Mouse2;

    mgr.OnInputEvent(mouse2Pressed);
}

TEST_F(GestureManagerTest, Event_MousePressed_PassedToAllSubscribedRecognizers)
{
    using ::testing::_;

    Gestures::RecognizerMock mockRecognizer1;
    Gestures::RecognizerMock mockRecognizer2;
    Gestures::RecognizerMock mockRecognizer3;
    Gestures::Manager mgr;
    EXPECT_CALL(mockRecognizer1, OnPressedEvent(_, 0));
    EXPECT_CALL(mockRecognizer2, OnPressedEvent(_, 0));
    EXPECT_CALL(mockRecognizer3, OnPressedEvent(_, 0));

    mgr.Register(mockRecognizer1);
    mgr.Register(mockRecognizer2);
    mgr.Register(mockRecognizer3);

    SInputEvent mousePressed;
    mousePressed.deviceType = EInputDeviceType::eIDT_Mouse;
    mousePressed.state = EInputState::eIS_Pressed;
    mousePressed.keyId = eKI_Mouse1;

    mgr.OnInputEvent(mousePressed);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
class GestureManagerTouchSimulationTest
    : public BaseGestureManagerTest
{
public:
    void SetUp() override
    {
        BaseGestureManagerTest::SetUp();

        // Gestures::Manager must be created after gEnv is set up by the
        // base test, or SEH will cause the test to fail.
        m_mgr = new Gestures::Manager();
    }

    void TearDown() override
    {
        delete m_mgr;

        BaseGestureManagerTest::TearDown();
    }

    enum Modifier
    {
        NONE = 0,
        ALT = EModifierMask::eMM_LAlt,
        SHIFT = EModifierMask::eMM_Shift
    };

    void AltPress()
    {
        SInputEvent leftAltPress;
        leftAltPress.deviceType = EInputDeviceType::eIDT_Keyboard;
        leftAltPress.state = EInputState::eIS_Pressed;
        leftAltPress.keyId = eKI_LAlt;
        leftAltPress.modifiers = ALT;
        leftAltPress.screenPosition = Vec2(ZERO);

        m_mgr->OnInputEvent(leftAltPress);
    }

    void ShiftPress()
    {
        SInputEvent shiftPress;
        shiftPress.deviceType = EInputDeviceType::eIDT_Keyboard;
        shiftPress.state = EInputState::eIS_Pressed;
        shiftPress.keyId = eKI_LShift;
        shiftPress.modifiers = SHIFT;
        shiftPress.screenPosition = Vec2(ZERO);

        m_mgr->OnInputEvent(shiftPress);
    }

    void ShiftRelease()
    {
        SInputEvent shiftRelease;
        shiftRelease.deviceType = EInputDeviceType::eIDT_Keyboard;
        shiftRelease.state = EInputState::eIS_Released;
        shiftRelease.keyId = eKI_LShift;
        shiftRelease.modifiers = 0;
        shiftRelease.screenPosition = Vec2(ZERO);

        m_mgr->OnInputEvent(shiftRelease);
    }

    void SendEnable(const Vec2& pos = ZERO)
    {
        // enable takes 2 steps (based on PC empirical analysis)
        // 1. track the mouse movement
        // 2. press the alt key

        MouseMove(pos);
        AltPress();
    }

    void MouseMove(const Vec2& pos)
    {
        SInputEvent mouseMove;
        mouseMove.deviceType = EInputDeviceType::eIDT_Mouse;
        mouseMove.state = EInputState::eIS_Changed;
        mouseMove.keyId = eKI_MousePosition;
        mouseMove.modifiers = 0;
        mouseMove.screenPosition = pos;

        m_mgr->OnInputEvent(mouseMove);
    }

    void AltRelease()
    {
        SInputEvent leftAltRelease;
        leftAltRelease.deviceType = EInputDeviceType::eIDT_Keyboard;
        leftAltRelease.state = EInputState::eIS_Released;
        leftAltRelease.keyId = eKI_LAlt;
        leftAltRelease.modifiers = 0;
        leftAltRelease.screenPosition = Vec2(ZERO);

        m_mgr->OnInputEvent(leftAltRelease);
    }

    void Press(uint32_t pointerIndex, const Vec2& pos = ZERO)
    {
        SendInputEvent(pointerIndex, pos, EInputState::eIS_Pressed);
    }

    void Down(uint32_t pointerIndex, const Vec2& pos = ZERO)
    {
        SendInputEvent(pointerIndex, pos, EInputState::eIS_Down);
    }

    void Release(uint32_t pointerIndex, const Vec2& pos = ZERO)
    {
        SendInputEvent(pointerIndex, pos, EInputState::eIS_Released);
    }

    void SendInputEvent(uint32_t pointerIndex, const Vec2& pos, EInputState state)
    {
        SInputEvent event;
        event.deviceType = EInputDeviceType::eIDT_Mouse;
        event.state = state;
        event.keyId = static_cast<EKeyId>(eKI_Mouse1 + pointerIndex);
        event.screenPosition = pos;

        m_mgr->OnInputEvent(event);
    }

protected:
    Gestures::Manager* m_mgr;
};

#if SIMULATE_TOUCH
TEST_F(GestureManagerTouchSimulationTest, IsSimulatingTouch_NoTrigger_False)
{
    ASSERT_FALSE(m_mgr->GetSimulator().IsSimulatingTouch());
}

TEST_F(GestureManagerTouchSimulationTest, IsSimulatingTouch_AltPressed_True)
{
    SendEnable(Vec2(ZERO));

    ASSERT_TRUE(m_mgr->GetSimulator().IsSimulatingTouch());
}

TEST_F(GestureManagerTouchSimulationTest, IsSimulatingTouch_AltPressedAndReleased_False)
{
    SendEnable();
    ASSERT_TRUE(m_mgr->GetSimulator().IsSimulatingTouch());
    AltRelease();
    ASSERT_FALSE(m_mgr->GetSimulator().IsSimulatingTouch());
}

TEST_F(GestureManagerTouchSimulationTest, OnPressMouse0_Simulating_BothIndex0And1OnPressedCalled)
{
    using ::testing::_;
    using ::testing::Return;

    Gestures::RecognizerMock mockRecognizer;
    EXPECT_CALL(mockRecognizer, OnPressedEvent(_, 0)).Times(1);
    EXPECT_CALL(mockRecognizer, OnPressedEvent(_, 1)).Times(1); // reflected pointer motion

    m_mgr->Register(mockRecognizer);

    SendEnable(); // begin reflection
    Press(0, Vec2(ZERO)); // 0 will be reflected
}

TEST_F(GestureManagerTouchSimulationTest, OnMouseMove_SimulatingReflectPointAtOrigin_Position0IsPosition1Reflected)
{
    using ::testing::_;
    using ::testing::Return;

    Gestures::RecognizerMock mockRecognizer;
    EXPECT_CALL(mockRecognizer, OnDownEvent(Vec2(1.f, 2.f), 0)).WillOnce(Return(true));
    EXPECT_CALL(mockRecognizer, OnDownEvent(Vec2(-1.f, -2.f), 1)).WillOnce(Return(true)); // reflected pointer motion

    m_mgr->Register(mockRecognizer);
    SendEnable(Vec2(ZERO)); // begin reflection
    Down(0, Vec2(1.f, 2.f));
}

TEST_F(GestureManagerTouchSimulationTest, AltRelease_ReflectActive_OnlyIndex1Released)
{
    using ::testing::_;
    using ::testing::Expectation;

    Gestures::RecognizerMock mockRecognizer;
    Expectation down0 = EXPECT_CALL(mockRecognizer, OnDownEvent(_, 0)).Times(1);
    Expectation down1 = EXPECT_CALL(mockRecognizer, OnDownEvent(_, 1)).Times(1);
    EXPECT_CALL(mockRecognizer, OnReleasedEvent(_, 1))
        .Times(1)
        .After(down0, down1);
    EXPECT_CALL(mockRecognizer, OnReleasedEvent(_, 0)).Times(0); // should never be called
    m_mgr->Register(mockRecognizer);

    SendEnable(Vec2(ZERO)); // begin reflection
    Down(0, Vec2(1.f, 2.f));
    AltRelease();
}

TEST_F(GestureManagerTouchSimulationTest, AltPressed_Index0AlreadyDown_Index1Pressed)
{
    using ::testing::_;
    using ::testing::Expectation;
    Gestures::RecognizerMock mockRecognizer;
    Expectation press0 = EXPECT_CALL(mockRecognizer, OnPressedEvent(_, 0)).Times(1);
    Expectation press1 = EXPECT_CALL(mockRecognizer, OnPressedEvent(_, 1)).Times(1); // .After(press1);
    m_mgr->Register(mockRecognizer);

    Press(0);
    AltPress();
}

TEST_F(GestureManagerTouchSimulationTest, ShiftPressed_SimulatingNoMouseDown_ReflectPointMoves)
{
    using ::testing::_;
    using ::testing::Expectation;
    Gestures::RecognizerMock mockRecognizer;
    m_mgr->Register(mockRecognizer);

    MouseMove(Vec2(1.f, 0.f));
    AltPress();
    MouseMove(Vec2(2.f, 0.f));
    ShiftPress();
    MouseMove(Vec2(4.f, 0.f));

    ASSERT_EQ(Vec2(3.f, 0.f), m_mgr->GetSimulator().GetReflectPoint());

    ShiftRelease();
    MouseMove(Vec2(5.f, 0.f));
    ASSERT_EQ(Vec2(3.f, 0.f), m_mgr->GetSimulator().GetReflectPoint());
}

TEST_F(GestureManagerTouchSimulationTest, ShiftPressed_SimulatingAndMouseDown_DownEventIsShiftedPositions)
{
    using ::testing::_;
    using ::testing::Expectation;
    using ::testing::InSequence;
    Gestures::RecognizerMock mockRecognizer;
    m_mgr->Register(mockRecognizer);

    {
        InSequence dummy;
        EXPECT_CALL(mockRecognizer, OnPressedEvent(Vec2(2.f, 0.f), 0));
        EXPECT_CALL(mockRecognizer, OnPressedEvent(Vec2(0.f, 0.f), 1));
        EXPECT_CALL(mockRecognizer, OnDownEvent(Vec2(3.f, 0.1f), 0));
        EXPECT_CALL(mockRecognizer, OnDownEvent(Vec2(1.f, 0.1f), 1));
    }

    MouseMove(Vec2(1.f, 0.f));
    AltPress();
    MouseMove(Vec2(2.f, 0.f));
    Press(0, Vec2(2.f, 0.f)); // press
    ShiftPress();
    Down(0, Vec2(3.f, 0.1f)); //
}
#endif // SIMULATE_TOUCH

