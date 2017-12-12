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

// Description : Simulation mode for AI movement by sending instructions to
//               an AI on where to pathfind in simulation mode


#ifndef CRYINCLUDE_EDITOR_AI_AIMOVESIMULATION_H
#define CRYINCLUDE_EDITOR_AI_AIMOVESIMULATION_H
#pragma once


#include <MovementRequestID.h>

class CAIMoveSimulation
{
public:
    CAIMoveSimulation();
    virtual ~CAIMoveSimulation();

    void OnSelectionChanged();
    void CancelMove();
    bool UpdateAIMoveSimulation(CViewport* pView, const QPoint& point);

private:
    bool GetAIMoveSimulationDestination(CViewport* pView, const QPoint& point, Vec3& outGotoPoint) const;
    bool SendAIMoveSimulation(IEntity* pEntity, const Vec3& vGotoPoint);

    Vec3 m_vGotoPoint;
    Vec3 m_vLastRefPoint;
    GUID m_selectedAI;
    MovementRequestID m_movementRequestID;
};

#endif // CRYINCLUDE_EDITOR_AI_AIMOVESIMULATION_H
