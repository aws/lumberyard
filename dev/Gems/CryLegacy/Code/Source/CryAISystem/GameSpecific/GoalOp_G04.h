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

#ifndef CRYINCLUDE_CRYAISYSTEM_GAMESPECIFIC_GOALOP_G04_H
#define CRYINCLUDE_CRYAISYSTEM_GAMESPECIFIC_GOALOP_G04_H
#pragma once

#if 0
// deprecated and won't compile at all...
/********************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2008-2009.
-------------------------------------------------------------------------
File name:   GoalOp_G04.h
Description: Game 04 goalops
             These should move into GameDLL when interfaces allow!
-------------------------------------------------------------------------
History:
- 21:02:2008 - Created by Matthew Jack
- 2 Mar 2009 - Evgeny Adamenkov: Removed IRenderer
*********************************************************************/


#include "GoalOp.h"
#include "GoalOpFactory.h"

// Forward declarations
class COPPathFind;
class COPTrace;

/**
* Factory for G04 goalops
*
*/
class CGoalOpFactoryG04
    : public IGoalOpFactory
{
    IGoalOp* GetGoalOp(const char* sGoalOpName, IFunctionHandler* pH, int nFirstParam, GoalParameters& params) const;
    IGoalOp* GetGoalOp(EGoalOperations op, GoalParameters& params) const;
};



////////////////////////////////////////////////////////////////////////////
//
//              G4APPROACH - makes agent approach a target using his environment
//
////////////////////////////////////////////////////////////////////////////

class COPG4Approach
    : public CGoalOp
{
    enum EOPG4AMode
    {
        EOPG4AS_Evaluate, EOPG4AS_GoNearHidepoint, EOPG4AS_GoToHidepoint, EOPG4AS_Direct
    };

    EOPG4AMode m_eApproachMode;
    EAIRegister m_nReg;     // Register from which to derive the target
    float m_fLastDistance;
    float m_fMinEndDistance, m_fMaxEndDistance;
    float m_fEndAccuracy;
    bool m_bNeedHidespot;
    bool m_bForceDirect;
    float m_fNotMovingTime;
    CTimeValue m_fLastTime;
    int m_iApproachQueryID, m_iRegenerateCurrentQueryID;

    CStrongRef<CAIObject>   m_refHideTarget;
    COPTrace* m_pTraceDirective;
    COPPathFind* m_pPathfindDirective;
public:
    // fEndDistance - goalpipe finishes at this range
    COPG4Approach(float fMinEndDistance, float MaxEndDistance, bool bForceDirect, EAIRegister nReg);
    COPG4Approach(const XmlNodeRef& node);
    COPG4Approach(const COPG4Approach& rhs);
    virtual ~COPG4Approach();

    EGoalOpResult Execute(CPipeUser* pOperand);
    void DebugDraw(CPipeUser* pOperand) const;
    void ExecuteDry(CPipeUser* pOperand);
    void Reset(CPipeUser* pOperand);
    void Serialize(TSerialize ser);
};

#endif // 0

#endif // CRYINCLUDE_CRYAISYSTEM_GAMESPECIFIC_GOALOP_G04_H
