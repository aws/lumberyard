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

////////////////////////////////////////////////////////////////////////////////////////////////////
Gestures::RecognizerSwipe::RecognizerSwipe(Gestures::ISwipeListener& listener, const Config& config)
    : m_listener(listener)
    , m_config(config)
    , m_startPosition(ZERO)
    , m_endPosition(ZERO)
    , m_startTime(0)
    , m_endTime(0)
    , m_currentState(State::Idle)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Gestures::RecognizerSwipe::~RecognizerSwipe()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerSwipe::OnPressedEvent(const Vec2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex != m_config.pointerIndex)
    {
        return false;
    }

    switch (m_currentState)
    {
    case State::Idle:
    {
        m_startTime = gEnv->pTimer->GetFrameStartTime().GetValue();
        m_startPosition = screenPosition;
        m_endPosition = screenPosition;
        m_currentState = State::Pressed;
    }
    break;
    case State::Pressed:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerDrag::OnPressedEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerSwipe::OnDownEvent(const Vec2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex != m_config.pointerIndex)
    {
        return false;
    }

    switch (m_currentState)
    {
    case State::Pressed:
    {
        const CTimeValue currentTime = gEnv->pTimer->GetFrameStartTime();
        if (currentTime.GetDifferenceInSeconds(m_startTime) > m_config.maxSecondsHeld)
        {
            // Swipe recognition failed because we took too long.
            m_currentState = State::Idle;
        }
    }
    break;
    case State::Idle:
    {
        // Swipe recognition already failed above.
    }
    break;
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerSwipe::OnDownEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerSwipe::OnReleasedEvent(const Vec2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex != m_config.pointerIndex)
    {
        return false;
    }

    switch (m_currentState)
    {
    case State::Pressed:
    {
        const CTimeValue currentTime = gEnv->pTimer->GetFrameStartTime();
        if ((currentTime.GetDifferenceInSeconds(m_startTime) <= m_config.maxSecondsHeld) &&
            (screenPosition.GetDistance(m_startPosition) >= m_config.minPixelsMoved))
        {
            // Swipe recognition succeeded.
            m_endTime = currentTime.GetValue();
            m_endPosition = screenPosition;
            m_listener.OnSwipeRecognized(*this);
            m_currentState = State::Idle;
        }
        else
        {
            // Swipe recognition failed because we took too long or didn't move enough.
            m_currentState = State::Idle;
        }
    }
    break;
    case State::Idle:
    {
        // Swipe recognition already failed above.
    }
    break;
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerSwipe::OnReleasedEvent state logic failure");
    }
    break;
    }

    return false;
}
