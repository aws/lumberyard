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


#include "StdAfx.h"
#include "AIMoveSimulation.h"
#include "Objects/EntityObject.h"
#include "GameEngine.h"
#include "IAIObject.h"
#include "Viewport.h"
#include <IAIActorProxy.h>
#include <IMovementSystem.h>
#include <MovementRequest.h>

//////////////////////////////////////////////////////////////////////////
CAIMoveSimulation::CAIMoveSimulation()
    : m_vGotoPoint(ZERO)
    , m_vLastRefPoint(ZERO)
    , m_selectedAI(GUID_NULL)
    , m_movementRequestID(MovementRequestID::Invalid())
{
}

//////////////////////////////////////////////////////////////////////////
CAIMoveSimulation::~CAIMoveSimulation()
{
    CancelMove();
}

//////////////////////////////////////////////////////////////////////////
void CAIMoveSimulation::OnSelectionChanged()
{
    IEditor* pEditor = GetIEditor();
    CRY_ASSERT(pEditor);

    CBaseObject* pSelected = pEditor->GetSelectedObject();
    if (!pSelected || pSelected->GetId() != m_selectedAI)
    {
        CancelMove();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIMoveSimulation::CancelMove()
{
    if (gEnv && gEnv->pAISystem && gEnv->pAISystem->GetMovementSystem())
    {
        gEnv->pAISystem->GetMovementSystem()->CancelRequest(m_movementRequestID);
        m_movementRequestID = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CAIMoveSimulation::UpdateAIMoveSimulation(CViewport* pView, const QPoint& point)
{
    bool bResult = false;

    IEditor* pEditor = GetIEditor();
    CRY_ASSERT(pEditor);

    CGameEngine* pGameEngine = pEditor ? pEditor->GetGameEngine() : NULL;
    CRY_ASSERT(pGameEngine);

    // Cancel the current move order
    CancelMove();

    m_selectedAI = GUID_NULL;
    m_vGotoPoint.zero();

    CBaseObject* pSelected = pEditor->GetSelectedObject();
    if (pSelected && pGameEngine && pGameEngine->GetSimulationMode())
    {
        // Get AI object of the selected entity
        if (qobject_cast<CEntityObject*>(pSelected))
        {
            IEntity* pEntity = ((CEntityObject*)pSelected)->GetIEntity();
            Vec3 vGotoPoint(ZERO);
            if (pEntity && GetAIMoveSimulationDestination(pView, point, vGotoPoint))
            {
                if (SendAIMoveSimulation(pEntity, vGotoPoint))
                {
                    m_selectedAI = pSelected->GetId();
                    m_vGotoPoint = vGotoPoint;
                    bResult = true;
                }
            }
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CAIMoveSimulation::GetAIMoveSimulationDestination(CViewport* pView, const QPoint& point, Vec3& outGotoPoint) const
{
    HitContext hitInfo;
    pView->HitTest(point, hitInfo);

    // TODO Get point or projected point on hit object's bounds
    CBaseObject* pHitObj = hitInfo.object;
    if (pHitObj)
    {
        AABB bbox;
        pHitObj->GetBoundBox(bbox);

        // TODO Get closest approachable point to bounds
        outGotoPoint = pView->SnapToGrid(pView->ViewToWorld(point));
    }
    else
    {
        outGotoPoint = pView->SnapToGrid(pView->ViewToWorld(point));
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
MovementStyle::Speed GetSpeedToUseForSimulation()
{
    MovementStyle::Speed speedToUse = MovementStyle::Run;

    if (const bool bShift = Qt::ShiftModifier & QApplication::queryKeyboardModifiers())
    {
        speedToUse = MovementStyle::Walk;
    }

    if (const bool bControl = Qt::AltModifier & QApplication::queryKeyboardModifiers())
    {
        speedToUse = MovementStyle::Sprint;
    }

    return speedToUse;
}

bool CAIMoveSimulation::SendAIMoveSimulation(IEntity* pEntity, const Vec3& vGotoPoint)
{
    assert(pEntity);

    // ensure that a potentially running animation doesn't block the movement
    if (pEntity && pEntity->HasAI())
    {
        IAIObject* pAI = pEntity->GetAI();
        if (pAI)
        {
            if (IAIActorProxy* aiProxy = pAI->GetProxy())
            {
                aiProxy->ResetAGInput(AIAG_ACTION);
                aiProxy->SetAGInput(AIAG_ACTION, "idle", true);
            }
        }
    }

    MovementRequest request;
    request.type = MovementRequest::MoveTo;
    request.destination = vGotoPoint;
    request.style.SetSpeed(GetSpeedToUseForSimulation());
    request.style.SetStance(MovementStyle::Stand);
    request.entityID = pEntity->GetId();

    assert(gEnv && gEnv->pAISystem && gEnv->pAISystem->GetMovementSystem());
    m_movementRequestID = gEnv->pAISystem->GetMovementSystem()->QueueRequest(request);

    return true;
}

