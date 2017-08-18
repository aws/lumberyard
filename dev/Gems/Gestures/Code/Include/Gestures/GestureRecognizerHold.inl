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
Gestures::RecognizerHold::RecognizerHold(Gestures::IHoldListener& listener, const Config& config)
    : m_listener(listener)
    , m_config(config)
    , m_startTime(0)
    , m_startPosition(ZERO)
    , m_currentPosition(ZERO)
    , m_currentState(State::Idle)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Gestures::RecognizerHold::~RecognizerHold()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerHold::OnPressedEvent(const Vec2& screenPosition, uint32_t pointerIndex)
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
        m_currentPosition = screenPosition;
        m_currentState = State::Pressed;
    }
    break;
    case State::Pressed:
    case State::Held:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerHold::OnPressedEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerHold::OnDownEvent(const Vec2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex != m_config.pointerIndex)
    {
        return false;
    }

    m_currentPosition = screenPosition;

    switch (m_currentState)
    {
    case State::Pressed:
    {
        const CTimeValue currentTime = gEnv->pTimer->GetFrameStartTime();
        if (screenPosition.GetDistance(m_startPosition) > m_config.maxPixelsMoved)
        {
            // Hold recognition failed.
            m_currentState = State::Idle;
        }
        else if (currentTime.GetDifferenceInSeconds(m_startTime) >= m_config.minSecondsHeld)
        {
            // Hold recognition succeeded.
            m_listener.OnHoldInitiated(*this);
            m_currentState = State::Held;
        }
    }
    break;
    case State::Held:
    {
        if (screenPosition.GetDistance(m_startPosition) > m_config.maxPixelsMoved)
        {
            // Hold recognition ended.
            m_listener.OnHoldEnded(*this);
            m_currentState = State::Idle;
        }
        else
        {
            m_listener.OnHoldUpdated(*this);
        }
    }
    break;
    case State::Idle:
    {
        // Hold recognition already ended or failed above.
    }
    break;
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerHold::OnDownEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerHold::OnReleasedEvent(const Vec2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex != m_config.pointerIndex)
    {
        return false;
    }

    m_currentPosition = screenPosition;

    switch (m_currentState)
    {
    case State::Pressed:
    {
        // We never actually started the hold
        m_currentState = State::Idle;
    }
    break;
    case State::Held:
    {
        m_listener.OnHoldEnded(*this);
        m_currentState = State::Idle;
    }
    break;
    case State::Idle:
    {
        // Hold recognition already ended or failed above.
    }
    break;
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerHold::OnReleasedEvent state logic failure");
    }
    break;
    }

    return false;
}
