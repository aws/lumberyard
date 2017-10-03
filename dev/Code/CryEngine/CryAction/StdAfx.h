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

// Description : include file for standard system include files,    or project
//               specific include files that are used frequently  but are
//               changed infrequently


#if !defined(AFX_STDAFX_H__B36C365D_F0EA_4545_B3BC_1E0EAB3B5E42__INCLUDED_)
#define AFX_STDAFX_H__B36C365D_F0EA_4545_B3BC_1E0EAB3B5E42__INCLUDED_

//on mac the precompiled header is auto included in every .c and .cpp file, no include line necessary.
//.c files don't like the cpp things in here
#ifdef __cplusplus

//#define _CRTDBG_MAP_ALLOC

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <memory>
#include <vector>
#include <map>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <array>
#include <string>

#define RWI_NAME_TAG "RayWorldIntersection(Action)"
#define PWI_NAME_TAG "PrimitiveWorldIntersection(Action)"

#define CRYACTION_EXPORTS

// Insert your headers here
#include <platform.h>

#include <ProjectDefines.h>
#include <StlUtils.h>
#include <Cry_Math.h>
#include <Cry_Camera.h>
#include <ISystem.h>
#include <INetwork.h>
#include <IScriptSystem.h>
#include <IEntitySystem.h>
#include <IEntityHelper.h>
#include <ICryPak.h>
#include <IConsole.h>
#include <ITimer.h>
#include <ILog.h>
#include <IRemoteControl.h>
#include <CryAction.h>
#include <IGameFramework.h>
#include <IActorSystem.h>
#include <IAnimatedCharacter.h>
#include <IGame.h>
#include <IItem.h>
#include <IItemSystem.h>
#include <IViewSystem.h>
#include <IVehicleSystem.h>
#include <IFlowSystem.h>
#include <IGameplayRecorder.h>
#include <GameWarning.h>
#include "GameUtils.h"

#include "CheatProtection.h"

#pragma warning(disable: 4018)  // conditional expression is constant
#pragma warning(disable: 4018)  // conditional expression is constant
#pragma warning(disable: 4503)  // decorated name length exceeded, name was truncated

#if !defined(RELEASE)
# define CRYACTION_AI_VERBOSITY
#endif

#ifdef CRYACTION_AI_VERBOSITY
# define AIWarningID gEnv->pAISystem->Warning
# define AIErrorID gEnv->pAISystem->Error
# define AILogProgressID gEnv->pAISystem->LogProgress
# define AILogEventID gEnv->pAISystem->LogEvent
# define AILogCommentID gEnv->pAISystem->LogComment
#else
# define AIWarningID (void)
# define AIErrorID (void)
# define AILogProgressID (void)
# define AILogEventID (void)
# define AILogCommentID (void)
#endif

inline bool IsClientActor(EntityId id)
{
    IActor* pActor = CCryAction::GetCryAction()->GetClientActor();
    if (pActor && pActor->GetEntity()->GetId() == id)
    {
        return true;
    }
    return false;
}

template<typename T>
bool inline GetAttr(const XmlNodeRef& node, const char* key, T& val)
{
    return node->getAttr(key, val);
}

bool inline GetTimeAttr(const XmlNodeRef& node, const char* key, time_t& val)
{
    const char* pVal = node->getAttr(key);
    if (!pVal)
    {
        return false;
    }
    val = GameUtils::stringToTime(pVal);
    return true;
}

template<>
bool inline GetAttr(const XmlNodeRef& node, const char* key, string& val)
{
    const char* pVal = node->getAttr(key);
    if (!pVal)
    {
        return false;
    }
    val = pVal;
    return true;
}


#endif //__cplusplus

#endif // !defined(AFX_STDAFX_H__B36C365D_F0EA_4545_B3BC_1E0EAB3B5E42__INCLUDED_)
