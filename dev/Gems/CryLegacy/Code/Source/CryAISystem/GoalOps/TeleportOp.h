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

#ifndef CRYINCLUDE_CRYAISYSTEM_GOALOPS_TELEPORTOP_H
#define CRYINCLUDE_CRYAISYSTEM_GOALOPS_TELEPORTOP_H
#pragma once

#include "EnterLeaveUpdateGoalOp.h"
#include "TimeValue.h"

// Teleports a pipe user to a specific location
class TeleportOp
    : public EnterLeaveUpdateGoalOp
{
public:
    TeleportOp();
    TeleportOp(const XmlNodeRef& node);
    TeleportOp(const TeleportOp& rhs);

    virtual void Update(CPipeUser& pipeUser) override;
    virtual void Serialize(TSerialize serializer) override;

    void SetDestination(const Vec3& position, const Vec3& direction);
    void SetWaitUntilAgentIsNotMovingBeforeTeleport();
private:

    void Teleport(CPipeUser& pipeUser);
    bool IsAgentMoving(CPipeUser& pipeUser);

    Vec3 m_destinationPosition;
    Vec3 m_destinationDirection;
    bool m_waitUntilAgentIsNotMoving;
};

#endif // CRYINCLUDE_CRYAISYSTEM_GOALOPS_TELEPORTOP_H
