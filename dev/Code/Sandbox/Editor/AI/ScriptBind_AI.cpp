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
#include "ScriptBind_AI.h"

#include "IAgent.h"
#include "AI/AIManager.h"

#include <IScriptSystem.h>
#include <ScriptHelpers.h>


// TODO(marcio): Exterminate this file


CScriptBind_AI::CScriptBind_AI(ISystem* pSystem)
{
    Init(pSystem->GetIScriptSystem(), pSystem);

    SetGlobalName("EditorAI");

    RegisterFunction(
        "AssignPFPropertiesToPathType",
        functor(*this, static_cast<int(CScriptBind_AI::*)(IFunctionHandler*)>(&CScriptBind_AI::AssignPFPropertiesToPathType)));

    m_pSS->ExecuteFile("scripts/ai/pathfindProperties.lua", true, true);
}

int CScriptBind_AI::AssignPFPropertiesToPathType(IFunctionHandler* pH)
{
    SCRIPT_CHECK_PARAMETERS(19);

    const char* sPathType = "AIPATH_DEFAULT";

    if (pH->GetParamType(1) == svtNumber)
    {
        int nPathType = AIPATH_DEFAULT;
        pH->GetParam(1, nPathType);
        sPathType = GetPathTypeName(static_cast<EAIPathType>(nPathType));
    }
    else
    {
        pH->GetParam(1, sPathType);
    }

#define GET_PF_PARAM(n, s)                                              \
    if ((pH->GetParamCount() >= n) && (pH->GetParamType(n) != svtNull)) \
        pH->GetParam(n, s)

    AgentPathfindingProperties properties;

    int navCapMask = 0;
    GET_PF_PARAM(2, navCapMask);
    properties.navCapMask = navCapMask;

    GET_PF_PARAM(3, properties.triangularResistanceFactor);
    GET_PF_PARAM(4, properties.waypointResistanceFactor);
    GET_PF_PARAM(5, properties.flightResistanceFactor);
    GET_PF_PARAM(6, properties.volumeResistanceFactor);
    GET_PF_PARAM(7, properties.roadResistanceFactor);
    GET_PF_PARAM(8, properties.waterResistanceFactor);
    GET_PF_PARAM(9, properties.maxWaterDepth);
    GET_PF_PARAM(10, properties.minWaterDepth);
    GET_PF_PARAM(11, properties.exposureFactor);
    GET_PF_PARAM(12, properties.dangerCost);
    GET_PF_PARAM(13, properties.zScale);
    GET_PF_PARAM(14, properties.customNavCapsMask);
    GET_PF_PARAM(15, properties.radius);
    GET_PF_PARAM(16, properties.height);
    GET_PF_PARAM(17, properties.maxSlope);
    GET_PF_PARAM(18, properties.id);
    GET_PF_PARAM(19, properties.avoidObstacles);
    properties.navCapMask |= properties.id * (1 << 24);
#undef GET_PF_PARAM

    // IMPORTANT: If changes are made here be sure to update CScriptBind_AI::AssignPFPropertiesToPathType() in CryAISystem!

    AssignPFPropertiesToPathType(sPathType, properties);

    return pH->EndFunction();
}

//
//-----------------------------------------------------------------------------------------------------------
void CScriptBind_AI::AssignPFPropertiesToPathType(const string& sPathType, AgentPathfindingProperties& properties)
{
    GetIEditor()->GetAI()->AssignPFPropertiesToPathType(sPathType, properties);
}

const char* CScriptBind_AI::GetPathTypeName(EAIPathType pathType)
{
    switch (pathType)
    {
    default:
    case AIPATH_DEFAULT:
        return "AIPATH_DEFAULT";

    case AIPATH_HUMAN:
        return "AIPATH_HUMAN";
    case AIPATH_HUMAN_COVER:
        return "AIPATH_HUMAN_COVER";
    case AIPATH_CAR:
        return "AIPATH_CAR";
    case AIPATH_TANK:
        return "AIPATH_TANK";
    case AIPATH_BOAT:
        return "AIPATH_BOAT";
    case AIPATH_HELI:
        return "AIPATH_HELI";
    case AIPATH_3D:
        return "AIPATH_3D";
    case AIPATH_SCOUT:
        return "AIPATH_SCOUT";
    case AIPATH_TROOPER:
        return "AIPATH_TROOPER";
    case AIPATH_HUNTER:
        return "AIPATH_HUNTER";
    }
    assert(!!!"Should not be here");
    return 0;
}
