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
Gestures::RecognizerClickOrTap::RecognizerClickOrTap(Gestures::IClickOrTapListener& listener, const Config& config)
    : m_listener(listener)
    , m_config(config)
    , m_timeOfLastEvent(0)
    , m_positionOfFirstEvent(ZERO)
    , m_positionOfLastEvent(ZERO)
    , m_currentCount(0)
    , m_currentState(State::Idle)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Gestures::RecognizerClickOrTap::~RecognizerClickOrTap()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerClickOrTap::OnPressedEvent(const Vec2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex != m_config.pointerIndex)
    {
        return false;
    }

    const CTimeValue currentTime = gEnv->pTimer->GetFrameStartTime();
    switch (m_currentState)
    {
    case State::Idle:
    {
        m_timeOfLastEvent = currentTime.GetValue();
        m_positionOfFirstEvent = screenPosition;
        m_positionOfLastEvent = screenPosition;
        m_currentCount = 0;
        m_currentState = State::Pressed;
    }
    break;
    case State::Released:
    {
        if ((currentTime.GetDifferenceInSeconds(m_timeOfLastEvent) > m_config.maxSecondsBetweenClicksOrTaps) ||
            (screenPosition.GetDistance(m_positionOfFirstEvent) > m_config.maxPixelsBetweenClicksOrTaps))
        {
            // Treat this as the start of a new tap sequence.
            m_currentCount = 0;
            m_positionOfFirstEvent = screenPosition;
        }

        m_timeOfLastEvent = currentTime.GetValue();
        m_positionOfLastEvent = screenPosition;
        m_currentState = State::Pressed;
    }
    break;
    case State::Pressed:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerClickOrTap::OnPressedEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerClickOrTap::OnDownEvent(const Vec2& screenPosition, uint32_t pointerIndex)
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
        if ((currentTime.GetDifferenceInSeconds(m_timeOfLastEvent) > m_config.maxSecondsHeld) ||
            (screenPosition.GetDistance(m_positionOfLastEvent) > m_config.maxPixelsMoved))
        {
            // Tap recognition failed.
            m_currentCount = 0;
            m_currentState = State::Idle;
        }
    }
    break;
    case State::Idle:
    {
        // Tap recognition already failed above.
    }
    break;
    case State::Released:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerClickOrTap::OnDownEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerClickOrTap::OnReleasedEvent(const Vec2& screenPosition, uint32_t pointerIndex)
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
        if ((currentTime.GetDifferenceInSeconds(m_timeOfLastEvent) > m_config.maxSecondsHeld) ||
            (screenPosition.GetDistance(m_positionOfLastEvent) > m_config.maxPixelsMoved))
        {
            // Tap recognition failed.
            m_currentCount = 0;
            m_currentState = State::Idle;
        }
        else if (++m_currentCount >= m_config.minClicksOrTaps)
        {
            // Tap recognition succeeded, inform the listener.
            m_timeOfLastEvent = currentTime.GetValue();
            m_positionOfLastEvent = screenPosition;
            m_listener.OnClickOrTapRecognized(*this);

            // Now reset to the default state.
            m_currentCount = 0;
            m_currentState = State::Idle;
        }
        else
        {
            // More taps are needed.
            m_timeOfLastEvent = currentTime.GetValue();
            m_positionOfLastEvent = screenPosition;
            m_currentState = State::Released;
        }
    }
    break;
    case State::Idle:
    {
        // Tap recognition already failed above.
    }
    break;
    case State::Released:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerClickOrTap::OnDownEvent state logic failure");
    }
    break;
    }

    return false;
}
