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
Gestures::RecognizerDrag::RecognizerDrag(Gestures::IDragListener& listener, const Config& config)
    : m_listener(listener)
    , m_config(config)
    , m_startTime(0)
    , m_startPosition(ZERO)
    , m_currentPosition(ZERO)
    , m_currentState(State::Idle)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Gestures::RecognizerDrag::~RecognizerDrag()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerDrag::OnPressedEvent(const Vec2& screenPosition, uint32_t pointerIndex)
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
    case State::Dragging:
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
bool Gestures::RecognizerDrag::OnDownEvent(const Vec2& screenPosition, uint32_t pointerIndex)
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
        if ((currentTime.GetDifferenceInSeconds(m_startTime) >= m_config.minSecondsHeld) &&
            (GetDistance() >= m_config.minPixelsMoved))
        {
            m_startTime = currentTime.GetValue();
            m_startPosition = m_currentPosition;
            m_listener.OnDragInitiated(*this);
            m_currentState = State::Dragging;
        }
    }
    break;
    case State::Dragging:
    {
        m_listener.OnDragUpdated(*this);
    }
    break;
    case State::Idle:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerDrag::OnDownEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerDrag::OnReleasedEvent(const Vec2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex != m_config.pointerIndex)
    {
        return false;
    }

    switch (m_currentState)
    {
    case State::Pressed:
    {
        // We never actually started dragging
        m_currentPosition = screenPosition;
        m_currentState = State::Idle;
    }
    break;
    case State::Dragging:
    {
        m_currentPosition = screenPosition;
        m_listener.OnDragEnded(*this);
        m_currentState = State::Idle;
    }
    break;
    case State::Idle:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerDrag::OnReleasedEvent state logic failure");
    }
    break;
    }

    return false;
}
