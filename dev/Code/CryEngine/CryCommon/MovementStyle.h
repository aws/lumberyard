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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_MOVEMENTSTYLE_H
#define CRYINCLUDE_CRYCOMMON_MOVEMENTSTYLE_H
#pragma once

#include "XMLAttrReader.h"
#include <IAIActor.h>

// Defines how an agent should move to the destination.
// Moving to cover, running, crouching and so on.
class MovementStyle
{
public:
    // When you change these, change ReadFromXml as well.
    enum Stance
    {
        Relaxed,
        Alerted,
        Stand,
        Crouch
    };

    enum Speed
    {
        Walk,
        Run,
        Sprint
    };

public:
    MovementStyle()
        : m_stance(Stand)
        , m_speed(Run)
        , m_speedLiteral(0.0f)
        , m_hasSpeedLiteral(false)
        , m_bodyOrientationMode(HalfwayTowardsAimOrLook)
        , m_movingToCover(false)
        , m_movingAlongDesignedPath(false)
        , m_turnTowardsMovementDirectionBeforeMoving(false)
        , m_strafe(false)
        , m_hasExactPositioningRequest(false)
        , m_glanceInMovementDirection(false)
    {
    }

    void ConstructDictionariesIfNotAlreadyDone();
    void ReadFromXml(const XmlNodeRef& node) {}

    void SetStance(const Stance stance) { m_stance = stance; }
    void SetSpeed(const Speed speed) { m_speed = speed; }
    void SetSpeedLiteral(float speedLiteral) { m_speedLiteral = speedLiteral; m_hasSpeedLiteral = true; }
    void SetMovingToCover(const bool movingToCover) { m_movingToCover = movingToCover; }
    void SetTurnTowardsMovementDirectionBeforeMoving(const bool enabled) { m_turnTowardsMovementDirectionBeforeMoving = enabled; }

    void SetMovingAlongDesignedPath(const bool movingAlongDesignedPath) { m_movingAlongDesignedPath = movingAlongDesignedPath; }
    void SetExactPositioningRequest(const SAIActorTargetRequest* const pExactPositioningRequest)
    {
        if (pExactPositioningRequest != 0)
        {
            m_hasExactPositioningRequest = true;
            m_exactPositioningRequest = *pExactPositioningRequest;
        }
        else
        {
            m_hasExactPositioningRequest = false;
        }
    }

    Stance GetStance() const { return m_stance; }
    Speed GetSpeed() const { return m_speed; }
    bool HasSpeedLiteral() const { return m_hasSpeedLiteral; }
    float GetSpeedLiteral() const { return m_speedLiteral; }
    EBodyOrientationMode GetBodyOrientationMode() const { return m_bodyOrientationMode; }
    bool IsMovingToCover() const { return m_movingToCover; }
    bool IsMovingAlongDesignedPath() const { return m_movingAlongDesignedPath; }
    bool ShouldTurnTowardsMovementDirectionBeforeMoving() const { return m_turnTowardsMovementDirectionBeforeMoving; }
    bool ShouldStrafe() const { return m_strafe; }
    bool ShouldGlanceInMovementDirection() const { return m_glanceInMovementDirection; }
    const SAIActorTargetRequest* GetExactPositioningRequest() const { return m_hasExactPositioningRequest ? &m_exactPositioningRequest : NULL; }

private:
    Stance m_stance;
    Speed m_speed;
    float m_speedLiteral;
    bool m_hasSpeedLiteral;
    EBodyOrientationMode m_bodyOrientationMode;
    SAIActorTargetRequest m_exactPositioningRequest;
    bool m_movingToCover;
    bool m_movingAlongDesignedPath;
    bool m_turnTowardsMovementDirectionBeforeMoving;
    bool m_strafe;
    bool m_hasExactPositioningRequest;
    bool m_glanceInMovementDirection;
};

#endif // CRYINCLUDE_CRYCOMMON_MOVEMENTSTYLE_H
