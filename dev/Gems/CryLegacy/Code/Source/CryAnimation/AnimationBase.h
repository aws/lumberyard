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

#ifndef CRYINCLUDE_CRYANIMATION_ANIMATIONBASE_H
#define CRYINCLUDE_CRYANIMATION_ANIMATIONBASE_H
#pragma once

#include <ICryAnimation.h>

#include "FrameProfiler.h"
#include "cvars.h"
#include "AnimationManager.h"

#ifdef CRYANIMATION_EXPORTS
#define CRYANIMATION_API DLL_EXPORT
#else
#define CRYANIMATION_API DLL_IMPORT
#endif


#define APX_NUM_OF_CGA_ANIMATIONS (200)  //to avoid unnecessary resizing of a dynamic array
#define MAX_FEET_AMOUNT (4)     //this is used for feetlock




#define MAX_EXEC_QUEUE (0x8u) //max anims in exec-queue
#define NUM_CAF_FILES (0x4000) //to avoid resizing in production mode
#define NUM_AIM_FILES (0x0400) //to avoid resizing in production mode


#define STORE_ANIMATION_NAMES


// Is CRYANIMATION_REPEAT_MOTION is set: When an animation gets started with
// CA_REPEAT_LAST_KEY and the end of the animation is reached, motion extraction
// will also keep repeating the final motion
#define CRYANIMATION_REPEAT_MOTION() 0

ILINE void g_LogToFile (const char* szFormat, ...) PRINTF_PARAMS(1, 2);


class CharacterManager;

struct ISystem;
struct IConsole;
struct ITimer;
struct ILog;
struct ICryPak;
struct IStreamEngine;

struct IRenderer;
struct IPhysicalWorld;
struct I3DEngine;
class CCamera;
struct SParametricSamplerInternal;

//////////////////////////////////////////////////////////////////////////
// There's only one ISystem in the process, just like there is one CharacterManager.
// So this ISystem is kept in the global pointer and is initialized upon creation
// of the CharacterManager and is valid until its destruction.
// Upon destruction, it's NULLed. In any case, there must be no object needing it
// after that since the animation system is only active when the Manager object is alive
//////////////////////////////////////////////////////////////////////////

//global interfaces
extern ISystem*                     g_pISystem;
extern IConsole*                    g_pIConsole;
extern ITimer*                      g_pITimer;
extern ILog*                            g_pILog;
extern ICryPak*                     g_pIPak;
extern IStreamEngine*     g_pIStreamEngine;

extern IRenderer*                   g_pIRenderer;
extern IRenderAuxGeom*      g_pAuxGeom;
extern IPhysicalWorld*      g_pIPhysicalWorld;
extern I3DEngine*                   g_pI3DEngine;


extern bool g_bProfilerOn;
extern f32  g_fCurrTime;
extern f32  g_AverageFrameTime;
extern CAnimation g_DefaultAnim;
extern CharacterManager* g_pCharacterManager;
extern QuatT g_IdentityQuatT;
extern AABB g_IdentityAABB;
extern int32    g_nRenderThreadUpdateID;
extern DynArray<string> g_DataMismatch;
extern SParametricSamplerInternal* g_parametricPool;
extern bool* g_usedParametrics;
extern int32 g_totalParametrics;

    #define g_AnimationManager g_pCharacterManager->GetAnimationManager()

// initializes the global values - just remembers the pointer to the system that will
// be kept valid until deinitialization of the class (that happens upon destruction of the
// CharacterManager instance). Also initializes the console variables
ILINE void g_InitInterfaces()
{
    assert (g_pISystem);
    g_pIConsole             = gEnv->pConsole;
    g_pITimer                   = gEnv->pTimer;
    g_pILog                     = g_pISystem->GetILog();
    g_pIPak                     =   gEnv->pCryPak;
    g_pIStreamEngine    =   g_pISystem->GetStreamEngine();

    //We initialize these pointers once here and then every frame in CharacterManager::Update()
    g_pIRenderer            = g_pISystem->GetIRenderer();
    g_pIPhysicalWorld   = g_pISystem->GetIPhysicalWorld();
    g_pI3DEngine            =   g_pISystem->GetI3DEngine();

    Console::GetInst().Init();
}


// de-initializes the Class - actually just NULLs the system pointer and deletes the variables
ILINE void g_DeleteInterfaces()
{
    g_pISystem              = NULL;
    g_pITimer                   = NULL;
    g_pILog                     = NULL;
    g_pIConsole             = NULL;
    g_pIPak                     = NULL;
    g_pIStreamEngine    = NULL;
    ;

    g_pIRenderer            = NULL;
    g_pIPhysicalWorld   = NULL;
    g_pI3DEngine            =   NULL;
}



// Common single structure for time\memory statistics
struct AnimStatisticsInfo
{
    int64 m_iDBASizes;
    int64 m_iCAFSizes;
    int64 m_iInstancesSizes;
    int64 m_iModelsSizes;

    AnimStatisticsInfo()
        : m_iDBASizes(0)
        , m_iCAFSizes(0)
        , m_iInstancesSizes(0)
        , m_iModelsSizes(0) {}

    void Clear()    { m_iDBASizes = 0; m_iCAFSizes = 0; m_iInstancesSizes = 0; m_iModelsSizes = 0;  }
    // int - for convenient output to log
    int GetInstancesSize() { return (int)(m_iInstancesSizes - GetModelsSize() - m_iDBASizes - m_iCAFSizes); }
    int GetModelsSize() { return (int)(m_iModelsSizes - m_iDBASizes - m_iCAFSizes); }
};
extern  AnimStatisticsInfo g_AnimStatisticsInfo;

namespace LegacyCryAnimation
{
    bool InitCharacterManager(const SSystemInitParams& initParams);
}

#define ENABLE_GET_MEMORY_USAGE 1


#ifndef AUTO_PROFILE_SECTION
#pragma message ("Warning: ITimer not included")
#else
#undef AUTO_PROFILE_SECTION
#endif

#define AUTO_PROFILE_SECTION(g_fTimer) CITimerAutoProfiler<double> __section_auto_profiler(g_pITimer, g_fTimer)

#define DEFINE_PROFILER_FUNCTION() FUNCTION_PROFILER_FAST(g_pISystem, PROFILE_ANIMATION, g_bProfilerOn)
#define DEFINE_PROFILER_SECTION(NAME) FRAME_PROFILER_FAST(NAME, g_pISystem, PROFILE_ANIMATION, g_bProfilerOn)

#endif // CRYINCLUDE_CRYANIMATION_ANIMATIONBASE_H
