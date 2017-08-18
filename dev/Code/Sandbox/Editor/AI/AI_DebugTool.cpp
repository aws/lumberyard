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

#include "StdAfx.h"
#include "AI_DebugTool.h"
#include "Viewport.h"
#include "Objects/EntityObject.h"

#include <IEntitySystem.h>
#include <IAISystem.h>
#include <IAgent.h>
#include "IAIObject.h"
#include "IAIActor.h"


//////////////////////////////////////////////////////////////////////////
CAI_DebugTool::CAI_DebugTool()
{
    m_statusText = "AI Debug Tool";

    m_pSelectedAI = 0;

    m_currentOpMode = eMode_GoTo;

    CBaseObject* pSelObj = GetIEditor()->GetSelectedObject();
    if (pSelObj)
    {
        if (qobject_cast<CEntityObject*>(pSelObj))
        {
            IEntity* pEntity = ((CEntityObject*)pSelObj)->GetIEntity();
            if (pEntity && pEntity->GetAI())
            {
                m_pSelectedAI = pEntity;
            }
        }
    }
    m_mouseDownPos.Set(0, 0, 0);
    m_curPos.Set(0, 0, 0);
    m_fAISpeed = AISPEED_WALK;
}

//////////////////////////////////////////////////////////////////////////
CAI_DebugTool::~CAI_DebugTool()
{
}

//////////////////////////////////////////////////////////////////////////
void CAI_DebugTool::BeginEditParams(IEditor* ie, int flags)
{
}

//////////////////////////////////////////////////////////////////////////
void CAI_DebugTool::EndEditParams()
{
}

//////////////////////////////////////////////////////////////////////////
void CAI_DebugTool::Display(DisplayContext& dc)
{
    dc.SetColor(1, 0, 0);
    dc.DrawTerrainCircle(m_mouseDownPos, 0.5f, 0.2f);

    dc.SetColor(0, 1, 0);
    dc.DrawTerrainCircle(m_curPos, 0.5f, 0.2f);

    if (m_pSelectedAI)
    {
        dc.SetColor(1, 0, 0, 0.8f);
        dc.DrawLine(m_pSelectedAI->GetWorldPos() + Vec3(0, 0, 1), m_mouseDownPos);
    }
    dc.Draw2dTextLabel(0, 0, 1, "AI Debug Tool");
}

//////////////////////////////////////////////////////////////////////////
bool CAI_DebugTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    bool bHit = false;
    HitContext hitInfo;
    bHit = view->HitTest(point, hitInfo);

    m_curPos = view->ViewToWorld(point);

    if (event == eMouseLDown)
    {
        m_mouseDownPos = m_curPos;
        if (!m_pSelectedAI)
        {
            m_pSelectedAI = GetNearestAI(m_curPos);
        }

        switch (m_currentOpMode)
        {
        case eMode_GoTo:
            if (m_pSelectedAI)
            {
                GotoToPosition(m_pSelectedAI, m_curPos, true);
            }
            break;
        }
    }
    else if (event == eMouseMove)
    {
        switch (m_currentOpMode)
        {
        case eMode_GoTo:
            if (m_pSelectedAI && (flags & MK_LBUTTON))
            {
                GotoToPosition(m_pSelectedAI, m_curPos, false);
            }
            break;
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CAI_DebugTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    switch (nChar)
    {
    case VK_NUMPAD1:
        m_fAISpeed = AISPEED_SLOW;
        return true;
        break;
    case VK_NUMPAD2:
        m_fAISpeed = AISPEED_WALK;
        return true;
        break;
    case VK_NUMPAD3:
        m_fAISpeed = AISPEED_RUN;
        return true;
        break;
    case VK_NUMPAD4:
        m_fAISpeed = AISPEED_SPRINT;
        return true;
        break;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
IEntity* CAI_DebugTool::GetNearestAI(Vec3 pos)
{
    IEntity* pNearestAI = 0;

    float fMinDist = 10000.0f;
    SEntityProximityQuery query;
    query.box = AABB(pos - Vec3(100, 100, 100), pos + Vec3(100, 100, 100));
    query.nEntityFlags = ENTITY_FLAG_HAS_AI;
    gEnv->pEntitySystem->QueryProximity(query);
    for (int i = 0; i < query.nCount; i++)
    {
        IEntity* pEntity = query.pEntities[i];
        if (pEntity->GetAI() && pEntity->GetAI()->CastToIPipeUser())
        {
            if (pos.GetDistance(pEntity->GetWorldPos()) < fMinDist)
            {
                fMinDist = pos.GetDistance(pEntity->GetWorldPos());
                pNearestAI = pEntity;
            }
        }
    }

    return pNearestAI;
}

//////////////////////////////////////////////////////////////////////////
void CAI_DebugTool::GotoToPosition(IEntity* pEntity, Vec3 pos, bool bSendSignal)
{
    IAIObject* pAI = pEntity->GetAI();
    if (pAI)
    {
        m_mouseDownPos = pos;
        IPipeUser* pPipeUser = pAI->CastToIPipeUser();
        if (pPipeUser)
        {
            // First check is the current goal pipe the one which belongs to this node
            pPipeUser->SetRefPointPos(pos);

            unsigned short nType = pAI->GetAIType();
            if (nType == AIOBJECT_VEHICLE)
            {
                if (bSendSignal)
                {
                    pAI->Event(AIEVENT_DRIVER_IN, NULL);  // enabling vehicle's full update to process signals, even if there is no driver
                }
            }
            else
            {
                IAIActor* pAIActor = pAI->CastToIAIActor();
                if (pAIActor)
                {
                    pAIActor->GetState().fMovementUrgency = m_fAISpeed;

                    if (bSendSignal)
                    {
                        IAISignalExtraData* pData = gEnv->pAISystem->CreateSignalExtraData();
                        pData->point = pos;
                        pData->fValue = 0;
                        pData->point2.x = 0;

                        bool res = false;
                        IPipeUser* pPipeUser = CastToIPipeUserSafe(pAI);
                        if (pPipeUser)
                        {
                            pPipeUser->SelectPipe(0, "do_nothing");  // Should reset previous pipes.
                        }
                        if (bSendSignal)
                        {
                            pAIActor->SetSignal(10, "ACT_GOTO", pEntity, pData);   // 10 means this signal must be sent (but sent[!], not set)
                        }
                    }
                }
            }
        }
    }
}

REGISTER_QT_CLASS_DESC_SYSTEM_ID(CAI_DebugTool, "EditTool.AI_DebugTool", "Debug", ESYSTEM_CLASS_EDITTOOL);

#include <AI/AI_DebugTool.moc>