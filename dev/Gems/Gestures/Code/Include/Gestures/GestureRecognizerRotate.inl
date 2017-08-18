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
Gestures::RecognizerRotate::RecognizerRotate(Gestures::IRotateListener& listener, const Config& config)
    : m_listener(listener)
    , m_config(config)
    , m_currentState(State::Idle)
{
    m_lastUpdateTimes[0] = 0;
    m_lastUpdateTimes[1] = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Gestures::RecognizerRotate::~RecognizerRotate()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerRotate::OnPressedEvent(const Vec2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex > s_maxRotatePointerIndex)
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
    case State::Rotating:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerRotate::OnPressedEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerRotate::OnDownEvent(const Vec2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex > s_maxRotatePointerIndex)
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
        const float distanceDeltaPixels = abs(GetCurrentDistance() - GetStartDistance());

        if (distanceDeltaPixels > m_config.maxPixelsMoved)
        {
            // The touches have moved too far towards or away from each other.
            // Reset the start positions so a rotate can still be initiated.
            m_startPositions[0] = m_currentPositions[0];
            m_startPositions[1] = m_currentPositions[1];
        }
        else if (abs(GetSignedRotationInDegrees()) >= m_config.minAngleDegrees)
        {
            // The imaginary line between the two touches has rotated by
            // an angle sufficient for a rotate gesture to be initiated.
            m_startPositions[0] = m_currentPositions[0];
            m_startPositions[1] = m_currentPositions[1];
            m_listener.OnRotateInitiated(*this);
            m_currentState = State::Rotating;
        }
    }
    break;
    case State::Rotating:
    {
        m_listener.OnRotateUpdated(*this);
    }
    break;
    case State::Pressed0:
    case State::Pressed1:
    case State::Idle:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerRotate::OnDownEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::RecognizerRotate::OnReleasedEvent(const Vec2& screenPosition, uint32_t pointerIndex)
{
    if (pointerIndex > s_maxRotatePointerIndex)
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
            CryLogAlways("RecognizerRotate::OnReleasedEvent state logic failure");
            break;
        }

        // We never actually started rotating
        m_currentPositions[pointerIndex] = screenPosition;
        m_currentState = State::Idle;
    }
    break;
    case State::PressedBoth:
    {
        // We never actually started rotating
        m_currentPositions[pointerIndex] = screenPosition;
        m_currentState = pointerIndex ? State::Pressed0 : State::Pressed1;
    }
    break;
    case State::Rotating:
    {
        m_currentPositions[pointerIndex] = screenPosition;
        m_listener.OnRotateEnded(*this);
        m_currentState = pointerIndex ? State::Pressed0 : State::Pressed1;
    }
    break;
    case State::Idle:
    default:
    {
        // Should not be possible, but not fatal if we happen to get here somehow.
        CryLogAlways("RecognizerRotate::OnReleasedEvent state logic failure");
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float Gestures::RecognizerRotate::GetSignedRotationInDegrees() const
{
    Vec2 vectorBetweenStartPositions = GetStartPosition1() - GetStartPosition0();
    Vec2 vectorBetweenCurrentPositions = GetCurrentPosition1() - GetCurrentPosition0();

    if (vectorBetweenStartPositions.IsZero() || vectorBetweenCurrentPositions.IsZero())
    {
        return 0.0f;
    }

    vectorBetweenStartPositions.Normalize();
    vectorBetweenCurrentPositions.Normalize();

    const float dotProduct = vectorBetweenStartPositions.Dot(vectorBetweenCurrentPositions);
    const float crossProduct = vectorBetweenStartPositions.Cross(vectorBetweenCurrentPositions);
    return RAD2DEG(atan2(crossProduct, dotProduct));
}
