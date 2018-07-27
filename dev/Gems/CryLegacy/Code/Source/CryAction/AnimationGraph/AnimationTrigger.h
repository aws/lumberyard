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

// Description : defines a spacial target for exact positioning, with operations we need
//               to perform on these things

#ifndef CRYINCLUDE_CRYACTION_ANIMATIONGRAPH_ANIMATIONTRIGGER_H
#define CRYINCLUDE_CRYACTION_ANIMATIONGRAPH_ANIMATIONTRIGGER_H
#pragma once

class CAnimationTrigger
{
public:
    CAnimationTrigger();
    CAnimationTrigger(const Vec3& pos, float width, const Vec3& triggerSize, const Quat& orient, float orientTolerance, float animMovementLength);

    void Update(float frameTime, Vec3 userPos, Quat userOrient, bool allowTriggering);
    void DebugDraw();

    bool IsReached() const { return m_state >= eS_Optimizing; }
    bool IsTriggered() const { return m_state >= eS_Triggered; }

    void ResetRadius(const Vec3& triggerSize, float orientTolerance);
    void SetDistanceErrorFactor(float factor) { m_distanceErrorFactor = factor; }

private:
    enum EState
    {
        eS_Invalid,
        eS_Initializing,
        eS_Before,
        eS_Optimizing,
        eS_Triggered
    };

    Vec3 m_pos;
    float m_width;
    Vec3 m_posSize;
    Quat m_orient;
    float m_cosOrientTolerance;
    float m_sideTime;
    float m_distanceErrorFactor;

    float m_animMovementLength;
    float m_distanceError;
    float m_orientError;
    float m_oldFwdDir;

    EState m_state;

    Vec3 m_userPos;
    Quat m_userOrient;
};

#endif // CRYINCLUDE_CRYACTION_ANIMATIONGRAPH_ANIMATIONTRIGGER_H
