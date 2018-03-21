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

#ifndef CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_COMMONDEFS_H
#define CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_COMMONDEFS_H
#pragma once


#define CRYAISYSTEM_EXPORTS

#include <platform.h>

#include <stdio.h>

#include <limits>
#include <vector>
#include <map>
#include <numeric>
#include <algorithm>
#include <list>
#include <set>
#include <deque>

// Reference additional interface headers your program requires here (not local headers)

#include "Cry_Math.h"
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
#include <IEntity.h>
#include <IEntitySystem.h>
#include <CryFile.h>
//#include "ICryPak.h"
#include <IXml.h>
#include <ISerialize.h>
#include <IRenderer.h>
#include <IRenderAuxGeom.h>
#include <MTPseudoRandom.h>

//////////////////////////////////////////////////////////////////////////

/// This frees the memory allocation for a vector (or similar), rather than just erasing the contents
template<typename T>
void ClearVectorMemory(T& container)
{
    T().swap(container);
}

// adding some headers here can improve build times... but at the cost of the compiler not registering
// changes to these headers if you compile files individually.
#include "AILog.h"
// NOTE: (MATT) Reduced this list to the minimum. {2007/07/18:16:24:59}
//////////////////////////////////////////////////////////////////////////


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


inline float LinStep(float a, float b, float x)
{
    x = (x - a) / (b - a);
    if (x < 0.0f)
    {
        x = 0;
    }
    if (x > 1.0f)
    {
        x = 1;
    }
    return x;
}

#endif // CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_COMMONDEFS_H

