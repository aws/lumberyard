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
Gestures::RecognizerPinch::RecognizerPinch(Gestures::IPinchListener& listener, const Config& config)
    : m_listener(listener)
    , m_config(config)
    , m_currentState(State::Idle)
{
    m_lastUpdateTimes[0] = 0;
    m_lastUpdateTimes[1] = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Gestures::RecognizerPinch::~RecognizerPinch()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerPinch::OnPressedEvent(const Vec2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex > s_maxPinchPointerIndex)
    {
        return false;
    }

    switch (m_currentState)
    {
    case State::Idle:
    {
        m_currentPositions[pointerIndex] = screenPosition;
        m_currentState = pointerIndex ? State::Pressed1 : State::Pressed0;
    }
    break;
    case State::Pressed0:
    case State::Pressed1:
    {
        m_currentPositions[pointerIndex] = screenPosition;
        if (static_cast<uint32_t>(m_currentState) != pointerIndex)
        {
            m_startPositions[0] = m_currentPositions[0];
            m_startPositions[1] = m_currentPositions[1];
            m_currentState = State::PressedBoth;
        }
    }
    break;
    case State::PressedBoth:
    case State::Pinching:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerPinch::OnPressedEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline float AngleInDegreesBetweenVectors(const Vec2& vec0, const Vec2& vec1)
{
    if (vec0.IsZero() || vec1.IsZero())
    {
        return 0.0f;
    }

    return RAD2DEG(acosf(abs(vec0.GetNormalized().Dot(vec1.GetNormalized()))));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerPinch::OnDownEvent(const Vec2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex > s_maxPinchPointerIndex)
    {
        return false;
    }

    m_currentPositions[pointerIndex] = screenPosition;
    m_lastUpdateTimes[pointerIndex] = gEnv->pTimer->GetFrameStartTime().GetValue();
    if (m_lastUpdateTimes[0] != m_lastUpdateTimes[1])
    {
        // We need to wait until both touches have been updated this frame.
        return false;
    }

    switch (m_currentState)
    {
    case State::PressedBoth:
    {
        const Vec2 vectorBetweenStartPositions = GetStartPosition1() - GetStartPosition0();
        const Vec2 vectorBetweenCurrentPositions = GetCurrentPosition1() - GetCurrentPosition0();
        const float distanceDeltaPixels = abs(GetCurrentDistance() - GetStartDistance());

        if (AngleInDegreesBetweenVectors(vectorBetweenStartPositions, vectorBetweenCurrentPositions) > m_config.maxAngleDegrees)
        {
            // The touches are not moving towards or away from each other.
            // Reset the start positions so a pinch can still be initiated.
            m_startPositions[0] = m_currentPositions[0];
            m_startPositions[1] = m_currentPositions[1];
        }
        else if (distanceDeltaPixels >= m_config.minPixelsMoved)
        {
            // The touches have moved towards or away from each other a
            // sufficient distance for a pinch gesture to be initiated.
            m_startPositions[0] = m_currentPositions[0];
            m_startPositions[1] = m_currentPositions[1];
            m_listener.OnPinchInitiated(*this);
            m_currentState = State::Pinching;
        }
    }
    break;
    case State::Pinching:
    {
        m_listener.OnPinchUpdated(*this);
    }
    break;
    case State::Pressed0:
    case State::Pressed1:
    case State::Idle:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerPinch::OnDownEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerPinch::OnReleasedEvent(const Vec2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex > s_maxPinchPointerIndex)
    {
        return false;
    }

    switch (m_currentState)
    {
    case State::Pressed0:
    case State::Pressed1:
    {
        if (static_cast<uint32_t>(m_currentState) != pointerIndex)
        {
            // Should not be possible, but not fatal if we happen to get here somehow.
            CryLogAlways("RecognizerPinch::OnReleasedEvent state logic failure");
            break;
        }

        // We never actually started pinching
        m_currentPositions[pointerIndex] = screenPosition;
        m_currentState = State::Idle;
    }
    break;
    case State::PressedBoth:
    {
        // We never actually started pinching
        m_currentPositions[pointerIndex] = screenPosition;
        m_currentState = pointerIndex ? State::Pressed0 : State::Pressed1;
    }
    break;
    case State::Pinching:
    {
        m_currentPositions[pointerIndex] = screenPosition;
        m_listener.OnPinchEnded(*this);
        m_currentState = pointerIndex ? State::Pressed0 : State::Pressed1;
    }
    break;
    case State::Idle:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerPinch::OnReleasedEvent state logic failure");
    }
    break;
    }

    return false;
}
