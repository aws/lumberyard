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
#if !defined(AFX_STDAFX_H__81DAABA0_0054_42BF_8696_D99BA6832D03__INCLUDED_)
#define AFX_STDAFX_H__81DAABA0_0054_42BF_8696_D99BA6832D03__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>
#include <memory>
#include <limits>
#include <vector>
#include <map>
#include <numeric>
#include <algorithm>
#include <list>
#include <set>
#include <deque>

#define RWI_NAME_TAG "RayWorldIntersection(AI)"
#define PWI_NAME_TAG "PrimitiveWorldIntersection(AI)"

#define CRYAISYSTEM_EXPORTS

#include <platform.h>

#if !defined(_RELEASE)
#   define CRYAISYSTEM_DEBUG
#   define CRYAISYSTEM_VERBOSITY
# define COMPILE_WITH_MOVEMENT_SYSTEM_DEBUG
#endif
// Reference additional interface headers your program requires here (not local headers)

#include "Cry_Math.h"
#include <Cry_Camera.h>
#include <CryArray.h>
#include "Cry_XOptimise.h" // required by AMD64 compiler
#include <Cry_Geo.h>
#include <ISystem.h>
#include "IScriptSystem.h"
#include <IConsole.h>
#include <ILog.h>
#include <ISerialize.h>
#include <IFlowSystem.h>
#include <IAIAction.h>
#include <IPhysics.h>
#include <I3DEngine.h>
#include <ITimer.h>
#include <IAgent.h>
#include <IAIActorProxy.h>
#include <IAIGroupProxy.h>
#include <IEntity.h>
#include <IEntitySystem.h>
#include <CryFile.h>
#include <IXml.h>
#include <ISerialize.h>
#include <IRenderAuxGeom.h>
#include <Random.h>
#include "XMLUtils.h"

#include "Environment.h"
#include "CodeCoverageTracker.h"

#define AI_STRIP_OUT_LEGACY_LOOK_TARGET_CODE

/// This frees the memory allocation for a vector (or similar), rather than just erasing the contents
template<typename T>
void ClearVectorMemory(T& container)
{
    T().swap(container);
}

// adding some headers here can improve build times... but at the cost of the compiler not registering
// changes to these headers if you compile files individually.
#include "AILog.h"
#include "CAISystem.h"
#include "ActorLookUp.h"
// NOTE: (MATT) Reduced this list to the minimum. {2007/07/18:16:24:59}
//////////////////////////////////////////////////////////////////////////

class CAISystem;
ILINE CAISystem* GetAISystem()
{
    extern CAISystem* g_pAISystem;

    return g_pAISystem;
}

//====================================================================
// SetAABBCornerPoints
//====================================================================
inline void SetAABBCornerPoints(const AABB& b, Vec3* pts)
{
    pts[0].Set(b.min.x, b.min.y, b.min.z);
    pts[1].Set(b.max.x, b.min.y, b.min.z);
    pts[2].Set(b.max.x, b.max.y, b.min.z);
    pts[3].Set(b.min.x, b.max.y, b.min.z);

    pts[4].Set(b.min.x, b.min.y, b.max.z);
    pts[5].Set(b.max.x, b.min.y, b.max.z);
    pts[6].Set(b.max.x, b.max.y, b.max.z);
    pts[7].Set(b.min.x, b.max.y, b.max.z);
}


ILINE float LinStep(float a, float b, float x)
{
    float w = (b - a);
    if (w != 0.0f)
    {
        x = (x - a) / w;
        return min(1.0f, max(x, 0.0f));
    }
    return 0.0f;
}

//===================================================================
// HasPointInRange
// (To be replaced)
//===================================================================
ILINE bool  HasPointInRange(const std::vector<Vec3>& list, const Vec3& pos, float range)
{
    float   r(sqr(range));
    for (uint32 i = 0; i < list.size(); ++i)
    {
        if (Distance::Point_PointSq(list[i], pos) < r)
        {
            return true;
        }
    }
    return false;
}

#ifndef OUT
# define OUT
#endif

#ifndef IN
# define IN
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__81DAABA0_0054_42BF_8696_D99BA6832D03__INCLUDED_)

