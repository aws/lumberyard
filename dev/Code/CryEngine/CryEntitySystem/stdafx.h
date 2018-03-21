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

// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__2E966576_9327_4B66_9CFD_329F604BE709__INCLUDED_)
#define AFX_STDAFX_H__2E966576_9327_4B66_9CFD_329F604BE709__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning (error : 4018) //Cannot align catch objects to greater than 16 bytes

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#pragma warning (disable : 4768) //Cannot align catch objects to greater than 16 bytes
#include <vector>
#include <list>
#include <iterator>
#include <algorithm>
#include <map>

#define RWI_NAME_TAG "RayWorldIntersection(EntitySys)"
#define PWI_NAME_TAG "PrimitiveWorldIntersection(EntitySys)"

#define CRYENTITYDLL_EXPORTS

#include <platform.h>

//////////////////////////////////////////////////////////////////////////

#include "Cry_Math.h"
#include <Cry_Camera.h>
#include "Cry_XOptimise.h"
#include "Cry_GeoIntersect.h"
#include <CrySizer.h>
#include <smartptr.h>
#include <StlUtils.h>

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


//#include "EntityDesc.h"
//#include <IEntitySystem.h>

#include <ISystem.h>
#include <IConsole.h>
#include <IPhysics.h>
#include <I3DEngine.h>
#include <TimeValue.h>
#include <PoolAllocator.h>
#include <IEntitySystem.h>
#include <IEntityHelper.h>

#include "EntityCVars.h"

#if !defined(_RELEASE)
#define INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE
#endif // !_RELEASE

//////////////////////////////////////////////////////////////////////////
class CEntitySystem;
extern CEntitySystem* g_pIEntitySystem;
ILINE CEntitySystem* GetIEntitySystem() { return g_pIEntitySystem; }
//////////////////////////////////////////////////////////////////////////

class CEntity;

//////////////////////////////////////////////////////////////////////////
//! Reports a Game Warning to validator with WARNING severity.
inline void EntityWarning(const char* format, ...) PRINTF_PARAMS(1, 2);
inline void EntityWarning(const char* format, ...)
{
    if (!format)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    gEnv->pSystem->WarningV(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, 0, NULL, format, args);
    va_end(args);
}

inline void EntityFileWarning(const char* file, const char* format, ...) PRINTF_PARAMS(2, 3);
inline void EntityFileWarning(const char* file, const char* format, ...)
{
    if (!format)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    gEnv->pSystem->WarningV(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, 0, file, format, args);
    va_end(args);
}


//////////////////////////////////////////////////////////////////////////
// Proxy creation functions.
//////////////////////////////////////////////////////////////////////////
struct IEntityScript;
extern IComponentPtr CreateScriptComponent(IEntity* pEntity, IEntityScript* pScript, SEntitySpawnParams* pSpawnParams);

#define ENTITY_PROFILER FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_ENTITY, g_bProfilerEnabled);
#define ENTITY_PROFILER_NAME(str) FRAME_PROFILER_FAST(str, GetISystem(), PROFILE_ENTITY, g_bProfilerEnabled);

#endif // !defined(AFX_STDAFX_H__2E966576_9327_4B66_9CFD_329F604BE709__INCLUDED_)
