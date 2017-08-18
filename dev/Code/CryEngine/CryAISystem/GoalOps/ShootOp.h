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

#ifndef CRYINCLUDE_CRYAISYSTEM_GOALOPS_SHOOTOP_H
#define CRYINCLUDE_CRYAISYSTEM_GOALOPS_SHOOTOP_H
#pragma once

#include "EnterLeaveUpdateGoalOp.h"
#include "TimeValue.h"

// Shoot at the attention target or at a point in the actor's local-space.
// The shooting will go on for a specified amount of time, and after that
// the fire mode will be set to 'off' and the goal op will finish.
class ShootOp
    : public EnterLeaveUpdateGoalOp
{
public:
    ShootOp();
    ShootOp(const XmlNodeRef& node);
    ShootOp(const ShootOp& rhs);

    virtual void Enter(CPipeUser& pipeUser);
    virtual void Leave(CPipeUser& pipeUser);
    virtual void Update(CPipeUser& pipeUser);
    virtual void Serialize(TSerialize serializer);

    enum ShootAt
    {
        AttentionTarget,
        ReferencePoint,
        LocalSpacePosition,

        ShootAtSerializationHelperLast,
        ShootAtSerializationHelperFirst = AttentionTarget
    };

private:
    void SetupFireTargetForLocalPosition(CPipeUser& pipeUser);
    void SetupFireTargetForReferencePoint(CPipeUser& pipeUser);

    Vec3 m_position;
    CStrongRef<CAIObject> m_dummyTarget;
    CTimeValue m_timeWhenShootingShouldEnd;
    float m_duration;
    ShootAt m_shootAt;
    EFireMode m_fireMode;
    EStance m_stance;
};

#endif // CRYINCLUDE_CRYAISYSTEM_GOALOPS_SHOOTOP_H
