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

#include "stdafx.h"

#include <ICryAnimation.h>
#include <IFacialAnimation.h>
#include "CharacterManager.h"
#include "AnimEventLoader.h"

#include "SkeletonAnim.h"
#include "AttachmentSkin.h"

// Must be included once in the module.
#include <platform_impl.h>

#include <IEngineModule.h>

#include "AnimationThreadTask.h"

#undef GetClassName

//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_Animation
    : public ISystemEventListener
{
public:
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
    {
        switch (event)
        {
        case ESYSTEM_EVENT_RANDOM_SEED:
            cry_random_seed(gEnv->bNoRandomSeed ? 0 : (uint32)wparam);
            break;
        case ESYSTEM_EVENT_LEVEL_UNLOAD:
            break;
        case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
        {
            STLALLOCATOR_CLEANUP;

            if (Memory::CPoolFrameLocal* pMemoryPool = CSkeletonAnimTask::GetMemoryPool())
            {
                pMemoryPool->ReleaseBuckets();
            }

            if (!gEnv->IsEditor())
            {
                delete g_pCharacterManager;
                g_pCharacterManager = NULL;
                gEnv->pCharacterManager = NULL;
            }

            break;
        }
        case ESYSTEM_EVENT_3D_POST_RENDERING_START:
        {
            if (!g_pCharacterManager)
            {
                g_pCharacterManager = new CharacterManager;
                gEnv->pCharacterManager = g_pCharacterManager;
            }
            AnimEventLoader::SetPreLoadParticleEffects(false);
            break;
        }
        case ESYSTEM_EVENT_3D_POST_RENDERING_END:
        {
            if (Memory::CPoolFrameLocal* pMemoryPool = CSkeletonAnimTask::GetMemoryPool())
            {
                pMemoryPool->ReleaseBuckets();
            }

            SAFE_DELETE(g_pCharacterManager);
            gEnv->pCharacterManager = NULL;
            AnimEventLoader::SetPreLoadParticleEffects(true);
            break;
        }
        case ESYSTEM_EVENT_LEVEL_LOAD_START:
        {
            if (!g_pCharacterManager)
            {
                g_pCharacterManager = new CharacterManager;
                gEnv->pCharacterManager = g_pCharacterManager;
                gEnv->pCharacterManager->PostInit();
            }
            break;
        }
        }
    }
};
static CSystemEventListner_Animation g_system_event_listener_anim;

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryAnimation
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule)
    CRYGENERATE_SINGLETONCLASS(CEngineModule_CryAnimation, "EngineModule_CryAnimation", 0x9c73d2cd142c4256, 0xa8f0706d80cd7ad2)

    //////////////////////////////////////////////////////////////////////////
    virtual const char* GetName() const {
        return "CryAnimation";
    };
    virtual const char* GetCategory() const { return "CryEngine"; };

    //////////////////////////////////////////////////////////////////////////
    virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
    {
        ISystem* pSystem = env.pSystem;

        if (!CSkeletonAnimTask::Initialize())
        {
            return false;
        }

        g_pISystem = pSystem;
        g_InitInterfaces();

        if (!g_controllerHeap.IsInitialised())
        {
            g_controllerHeap.Init(Console::GetInst().ca_MemoryDefragPoolSize);
        }

        pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_anim);

        g_pCharacterManager = NULL;
        env.pCharacterManager = NULL;
        if (gEnv->IsEditor())
        {
            g_pCharacterManager = new CharacterManager;
            gEnv->pCharacterManager = g_pCharacterManager;
        }

        return true;
    }
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryAnimation)

CEngineModule_CryAnimation::CEngineModule_CryAnimation()
{
};

CEngineModule_CryAnimation::~CEngineModule_CryAnimation()
{
};


// cached interfaces - valid during the whole session, when the character manager is alive; then get erased
ISystem*                        g_pISystem              = NULL;
ITimer*                         g_pITimer                   = NULL; //module implemented in CrySystem
ILog*                               g_pILog                     = NULL; //module implemented in CrySystem
IConsole*                       g_pIConsole             = NULL; //module implemented in CrySystem
ICryPak*                        g_pIPak                     = NULL; //module implemented in CrySystem
IStreamEngine*      g_pIStreamEngine    = NULL; //module implemented in CrySystem

IRenderer*                  g_pIRenderer            = NULL;
IRenderAuxGeom*         g_pAuxGeom              = NULL;
IPhysicalWorld*         g_pIPhysicalWorld   = NULL;
I3DEngine*                  g_pI3DEngine            = NULL; //Need just for loading of chunks. Should be part of CrySystem


f32 g_AverageFrameTime = 0;
CAnimation g_DefaultAnim;
CharacterManager* g_pCharacterManager;
QuatT g_IdentityQuatT = QuatT(IDENTITY);
int32 g_nRenderThreadUpdateID = 0;
DynArray<string> g_DataMismatch;
SParametricSamplerInternal* g_parametricPool  = NULL;
bool* g_usedParametrics = NULL;
int32 g_totalParametrics = 0;
AABB g_IdentityAABB = AABB(ZERO, ZERO);

CControllerDefragHeap g_controllerHeap;




ILINE void g_LogToFile (const char* szFormat, ...)
{
    char szBuffer[0x800];
    va_list args;
    va_start(args, szFormat);
    vsnprintf_s (szBuffer, sizeof(szBuffer), sizeof(szBuffer) - 1, szFormat, args);
    va_end(args);
    g_pILog->LogToFile ("%s", szBuffer);
}









f32 g_fCurrTime = 0;
int g_CpuFlags = 0;
bool g_bProfilerOn = false;

AnimStatisticsInfo g_AnimStatisticsInfo;

// TypeInfo implementations for CryAnimation
#ifndef AZ_MONOLITHIC_BUILD
    #include "Common_TypeInfo.h"
    #include "CGFContent_info.h"
#endif // AZ_MONOLITHIC_BUILD
