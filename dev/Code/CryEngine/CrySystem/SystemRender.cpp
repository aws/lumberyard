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

// Description : CryENGINE system core


#include "StdAfx.h"
#include "System.h"
#include "IEntity.h"
#include "IStatoscope.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#include <IRenderer.h>
#include <IRenderAuxGeom.h>
#include <IProcess.h>
#include "Log.h"
#include "XConsole.h"
#include <I3DEngine.h>
#include <IAISystem.h>
#include <CryLibrary.h>
#include <IBudgetingSystem.h>
#include "PhysRenderer.h"
#include <IScriptSystem.h>
#include <IEntitySystem.h>
#include <IParticles.h>
#include <IMovieSystem.h>
#include <IPlatformOS.h>

#include "CrySizerStats.h"
#include "CrySizerImpl.h"
#include "ITestSystem.h"                            // ITestSystem
#include "VisRegTest.h"
#include "ThreadProfiler.h"
#include "IDiskProfiler.h"
#include "ITextModeConsole.h"
#include <IEntitySystem.h> // <> required for Interfuscator
#include <IGame.h>
#include <ILevelSystem.h>
#include <LyShine/ILyShine.h>

#include "MiniGUI/MiniGUI.h"
#include "PerfHUD.h"
#include "ThreadInfo.h"

#include <LoadScreenBus.h>

extern CMTSafeHeap* g_pPakHeap;
#if defined(AZ_PLATFORM_ANDROID)
#include <AzCore/Android/Utils.h>
#elif defined(AZ_PLATFORM_APPLE_IOS) || defined(AZ_PLATFORM_APPLE_TV)
extern bool UIKitGetPrimaryPhysicalDisplayDimensions(int& o_widthPixels, int& o_heightPixels);
#endif

extern int CryMemoryGetAllocatedSize();

/////////////////////////////////////////////////////////////////////////////////
static void VerifySizeRenderVar(ICVar* pVar)
{
    const int size = pVar->GetIVal();
    if (size <= 0)
    {
        AZ_Error("Console Variable", false, "'%s' set to invalid value: %i. Setting to nearest safe value: 1.", pVar->GetName(), size);
        pVar->Set(1);
    }
}

/////////////////////////////////////////////////////////////////////////////////
bool CSystem::GetPrimaryPhysicalDisplayDimensions(int& o_widthPixels, int& o_heightPixels)
{
#if defined(AZ_PLATFORM_WINDOWS)
    o_widthPixels = GetSystemMetrics(SM_CXSCREEN);
    o_heightPixels = GetSystemMetrics(SM_CYSCREEN);
    return true;
#elif defined(AZ_PLATFORM_ANDROID)
    return AZ::Android::Utils::GetWindowSize(o_widthPixels, o_heightPixels);
#elif defined(AZ_PLATFORM_APPLE_IOS) || defined(AZ_PLATFORM_APPLE_TV)
    return UIKitGetPrimaryPhysicalDisplayDimensions(o_widthPixels, o_heightPixels);
#else
    return false;
#endif
}

/////////////////////////////////////////////////////////////////////////////////
void CSystem::CreateRendererVars(const SSystemInitParams& startupParams)
{
    int iFullScreenDefault = 1;
    int iDisplayInfoDefault = 0;
    int iWidthDefault = 1280;
    int iHeightDefault = 720;
#if defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE_IOS) || defined(AZ_PLATFORM_APPLE_TV)
    GetPrimaryPhysicalDisplayDimensions(iWidthDefault, iHeightDefault);
#elif defined(WIN32) || defined(WIN64)
    iFullScreenDefault = 0;
    iWidthDefault = GetSystemMetrics(SM_CXFULLSCREEN) * 2 / 3;
    iHeightDefault = GetSystemMetrics(SM_CYFULLSCREEN) * 2 / 3;
#endif

    if (IsDevMode())
    {
        iFullScreenDefault = 0;
        iDisplayInfoDefault = 1;
    }

    // load renderer settings from engine.ini
    m_rWidth = REGISTER_INT_CB("r_Width", iWidthDefault, VF_DUMPTODISK,
            "Sets the display width, in pixels. Default is 1280.\n"
            "Usage: r_Width [800/1024/..]", VerifySizeRenderVar);
    m_rHeight = REGISTER_INT_CB("r_Height", iHeightDefault, VF_DUMPTODISK,
            "Sets the display height, in pixels. Default is 720.\n"
            "Usage: r_Height [600/768/..]", VerifySizeRenderVar);
    m_rWidthAndHeightAsFractionOfScreenSize = REGISTER_FLOAT("r_WidthAndHeightAsFractionOfScreenSize", 1.0f, VF_DUMPTODISK,
            "(iOS/Android only) Sets the display width and height as a fraction of the physical screen size. Default is 1.0.\n"
            "Usage: rWidthAndHeightAsFractionOfScreenSize [0.1 - 1.0]");
    m_rMaxWidth = REGISTER_INT("r_MaxWidth", 0, VF_DUMPTODISK,
            "(iOS/Android only) Sets the maximum display width while maintaining the device aspect ratio.\n"
            "Usage: r_MaxWidth [1024/1920/..] (0 for no max), combined with r_WidthAndHeightAsFractionOfScreenSize [0.1 - 1.0]");
    m_rMaxHeight = REGISTER_INT("r_MaxHeight", 0, VF_DUMPTODISK,
            "(iOS/Android only) Sets the maximum display height while maintaining the device aspect ratio.\n"
            "Usage: r_MaxHeight [768/1080/..] (0 for no max), combined with r_WidthAndHeightAsFractionOfScreenSize [0.1 - 1.0]");
    m_rColorBits = REGISTER_INT("r_ColorBits", 32, VF_DUMPTODISK,
            "Sets the color resolution, in bits per pixel. Default is 32.\n"
            "Usage: r_ColorBits [32/24/16/8]");
    m_rDepthBits = REGISTER_INT("r_DepthBits", 24, VF_DUMPTODISK | VF_REQUIRE_APP_RESTART,
            "Sets the depth precision, in bits per pixel. Default is 24.\n"
            "Usage: r_DepthBits [32/24]");
    m_rStencilBits = REGISTER_INT("r_StencilBits", 8, VF_DUMPTODISK,
            "Sets the stencil precision, in bits per pixel. Default is 8.\n");

    // Needs to be initialized as soon as possible due to swap chain creation modifications..
    m_rHDRDolby = REGISTER_INT_CB("r_HDRDolby", 0, VF_DUMPTODISK,
            "HDR dolby output mode\n"
            "Usage: r_HDRDolby [Value]\n"
            "0: Off (default)\n"
            "1: Dolby maui output\n"
            "2: Dolby vision output\n",
            [] (ICVar* cvar)
            {
                eDolbyVisionMode mode = static_cast<eDolbyVisionMode>(cvar->GetIVal());

                if (mode == eDVM_Vision && gEnv->IsEditor())
                {
                    cvar->Set(static_cast<int>(eDVM_Disabled));
                }
            });
    // Restrict the limits of this cvar to the eDolbyVisionMode values
    m_rHDRDolby->SetLimits(static_cast<float>(eDVM_Disabled), static_cast<float>(eDVM_Vision));

#if defined(WIN32) || defined(WIN64)
    REGISTER_INT("r_overrideDXGIAdapter", -1, VF_REQUIRE_APP_RESTART,
        "Specifies index of the preferred video adapter to be used for rendering (-1=off, loops until first suitable adapter is found).\n"
        "Use this to resolve which video card to use if more than one DX11 capable GPU is available in the system.");
#endif

#if defined(WIN32) || defined(WIN64)
    const char* p_r_DriverDef = "Auto";
#elif defined(APPLE)
    const char* p_r_DriverDef = "METAL";
#elif defined(ANDROID)
    const char* p_r_DriverDef = "GL";
#elif defined(LINUX)
    const char* p_r_DriverDef = "NULL";
#else
    const char* p_r_DriverDef = "DX9";                          // required to be deactivated for final release
#endif
    if (startupParams.pCvarsDefault)
    { // hack to customize the default value of r_Driver
        SCvarsDefault* pCvarsDefault = startupParams.pCvarsDefault;
        if (pCvarsDefault->sz_r_DriverDef && pCvarsDefault->sz_r_DriverDef[0])
        {
            p_r_DriverDef = startupParams.pCvarsDefault->sz_r_DriverDef;
        }
    }

    m_rDriver = REGISTER_STRING("r_Driver", p_r_DriverDef, VF_DUMPTODISK | VF_INVISIBLE,
            "Sets the renderer driver ( DX11/AUTO/NULL ).\n"
            "Specify in bootstrap.cfg like this: r_Driver = \"DX11\"");

    m_rFullscreen = REGISTER_INT("r_Fullscreen", iFullScreenDefault, VF_DUMPTODISK,
            "Toggles fullscreen mode. Default is 1 in normal game and 0 in DevMode.\n"
            "Usage: r_Fullscreen [0=window/1=fullscreen]");

    m_rFullscreenWindow = REGISTER_INT("r_FullscreenWindow", 0, VF_DUMPTODISK,
            "Toggles fullscreen-as-window mode. Fills screen but allows seamless switching. Default is 0.\n"
            "Usage: r_FullscreenWindow [0=locked fullscreen/1=fullscreen as window]");

    m_rFullscreenNativeRes = REGISTER_INT("r_FullscreenNativeRes", 0, VF_DUMPTODISK, "");

    m_rDisplayInfo = REGISTER_INT("r_DisplayInfo", iDisplayInfoDefault, VF_RESTRICTEDMODE | VF_DUMPTODISK,
            "Toggles debugging information display.\n"
            "Usage: r_DisplayInfo [0=off/1=show/2=enhanced/3=compact]");

    m_rOverscanBordersDrawDebugView = REGISTER_INT("r_OverscanBordersDrawDebugView", 0, VF_RESTRICTEDMODE | VF_DUMPTODISK,
            "Toggles drawing overscan borders.\n"
            "Usage: r_OverscanBordersDrawDebugView [0=off/1=show]");
}

//////////////////////////////////////////////////////////////////////////
void CSystem::RenderBegin()
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);

    if (m_bIgnoreUpdates)
    {
        return;
    }

    bool rndAvail = m_env.pRenderer != 0;

    //////////////////////////////////////////////////////////////////////
    //start the rendering pipeline
    if (rndAvail)
    {
        m_env.pRenderer->BeginFrame();
    }

    gEnv->nMainFrameID = (rndAvail) ? m_env.pRenderer->GetFrameID(false) : 0;
}

char* PhysHelpersToStr(int iHelpers, char* strHelpers);
int StrToPhysHelpers(const char* strHelpers);

//////////////////////////////////////////////////////////////////////////
void CSystem::RenderEnd(bool bRenderStats, bool bMainWindow)
{
    {
        FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);

        if (m_bIgnoreUpdates)
        {
            return;
        }

        if (!m_env.pRenderer)
        {
            return;
        }

        /*
                if(m_env.pMovieSystem)
                    m_env.pMovieSystem->Render();
        */

        GetPlatformOS()->RenderEnd();

        if (m_env.pGame && bMainWindow)
        {
            m_env.pGame->RenderGameWarnings();
        }

        if (bMainWindow)    // we don't do this in UI Editor window for example
        {
            // keep debug allocations out of level heap
            ScopedSwitchToGlobalHeap globalHeap;

#if !defined (_RELEASE)
            // Flush render data and swap buffers.
            m_env.pRenderer->RenderDebug(bRenderStats);
#endif

#if defined(USE_PERFHUD)
            if (m_pPerfHUD)
            {
                m_pPerfHUD->Draw();
            }
            if (m_pMiniGUI)
            {
                m_pMiniGUI->Draw();
            }
#endif
            //If this is called from the launcher / game mode, render the statistic here.
            //If it is editor mode then we want the editor RenderViewport to handle drawing statistics so that
            //other renderable windows won't steal the text render events.
            if (bRenderStats && (!gEnv->IsEditor() || gEnv->IsEditorGameMode()))
            {
                RenderStatistics();
            }

            if (!gEnv->pSystem->GetILevelSystem() || !gEnv->pSystem->GetILevelSystem()->IsLevelLoaded())
            {
                IConsole* console = GetIConsole();
                ILyShine* lyShine = gEnv->pLyShine;

                //Normally the UI is drawn as part of the scene so it can properly render once per eye in VR
                //We only want to draw here if there is no level loaded. This way the user can see loading
                // UI and other information before the level is loaded.
                if (lyShine != nullptr)
                {
                    lyShine->Render();
                }

                //Same goes for the console. When no level is loaded, it's okay to render it outside of the renderer
                //so that users can load maps or change settings.
                if (console != nullptr)
                {
                    console->Draw();
                }
            }
        }

        m_env.pRenderer->ForceGC(); // XXX Rename this
        m_env.pRenderer->EndFrame();


        if (IConsole* pConsole = GetIConsole())
        {
            // if we have pending cvar calculation, execute it here
            // since we know cvars will be correct here after ->EndFrame().
            if (!pConsole->IsHashCalculated())
            {
                pConsole->CalcCheatVarHash();
            }
        }
    }

#if defined(USE_FRAME_PROFILER)
    gEnv->bProfilerEnabled = m_sys_profile->GetIVal() != 0;
#endif
}

void CSystem::OnScene3DEnd()
{
    // Render UI Canvas
    if (gEnv->pLyShine)
    {
        gEnv->pLyShine->Render();
    }

    //Render Console
    if (IConsole* pConsole = gEnv->pConsole)
    {
        pConsole->Draw();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::RenderPhysicsHelpers()
{
#if !defined (_RELEASE)
    if (gEnv->pPhysicalWorld)
    {
        char str[128];
        if (StrToPhysHelpers(m_p_draw_helpers_str->GetString()) != m_env.pPhysicalWorld->GetPhysVars()->iDrawHelpers)
        {
            m_p_draw_helpers_str->Set(PhysHelpersToStr(m_env.pPhysicalWorld->GetPhysVars()->iDrawHelpers, str));
        }

        m_pPhysRenderer->UpdateCamera(GetViewCamera());
        m_pPhysRenderer->DrawAllHelpers(m_env.pPhysicalWorld);
        m_pPhysRenderer->Flush(m_Time.GetFrameTime());

        RenderPhysicsStatistics(gEnv->pPhysicalWorld);
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
void CSystem::RenderPhysicsStatistics(IPhysicalWorld* pWorld)
{
#if !defined(_RELEASE)
    //////////////////////////////////////////////////////////////////////
    // draw physics helpers
    if (pWorld)
    {
        ITextModeConsole* pTMC = GetITextModeConsole();
        phys_profile_info* pInfos;
        phys_job_info* pJobInfos;
        PhysicsVars* pVars;
        int i = -2;
        char msgbuf[512];
        if ((pVars = pWorld->GetPhysVars())->bProfileEntities == 1)
        {
            pe_status_pos sp;
            int j, mask, nEnts = pWorld->GetEntityProfileInfo(pInfos);
            float fColor[4] = { 0.3f, 0.6f, 1.0f, 1.0f }, dt;
            if (!pVars->bSingleStepMode)
            {
                for (i = 0; i < nEnts; i++)
                {
                    pInfos[i].nTicksAvg = (int)(((int64)pInfos[i].nTicksAvg * 15 + pInfos[i].nTicksLast) >> 4);
                    pInfos[i].nCallsAvg = pInfos[i].nCallsAvg * (15.0f / 16) + pInfos[i].nCallsLast * (1.0f / 16);
                }
                phys_profile_info ppi;
                for (i = 0; i < nEnts - 1; i++)
                {
                    for (j = nEnts - 1; j > i; j--)
                    {
                        if (pInfos[j - 1].nTicksAvg < pInfos[j].nTicksAvg)
                        {
                            ppi = pInfos[j - 1];
                            pInfos[j - 1] = pInfos[j];
                            pInfos[j] = ppi;
                        }
                    }
                }
            }
            for (i = 0; i < nEnts; i++)
            {
                mask = (pInfos[i].nTicksPeak - pInfos[i].nTicksLast) >> 31;
                mask |= (70 - pInfos[i].peakAge) >> 31;
                mask &= (pVars->bSingleStepMode - 1) >> 31;
                pInfos[i].nTicksPeak += pInfos[i].nTicksLast - pInfos[i].nTicksPeak & mask;
                pInfos[i].nCallsPeak += pInfos[i].nCallsLast - pInfos[i].nCallsPeak & mask;
                azsprintf(msgbuf, "%.2fms/%.1f (peak %.2fms/%d) %s (id %d)",
                    dt = gEnv->pTimer->TicksToSeconds(pInfos[i].nTicksAvg) * 1000.0f, pInfos[i].nCallsAvg,
                    gEnv->pTimer->TicksToSeconds(pInfos[i].nTicksPeak) * 1000.0f, pInfos[i].nCallsPeak,
                    pInfos[i].pName, pInfos[i].id);
                GetIRenderer()->Draw2dLabel(10.0f, 60.0f + i * 12.0f, 1.3f, fColor, false, "%s", msgbuf);
                if (pTMC)
                {
                    pTMC->PutText(0, i, msgbuf);
                }
                IPhysicalEntity* pent = pWorld->GetPhysicalEntityById(pInfos[i].id);
                if (dt > 0.1f && pent && pent->GetStatus(&sp))
                {
                    GetIRenderer()->DrawLabelEx(sp.pos + Vec3(0, 0, sp.BBox[1].z), 1.4f, fColor, true, true, "%s %.2fms", pInfos[i].pName, dt);
                }
                pInfos[i].peakAge = pInfos[i].peakAge + 1 & ~mask;
                //pInfos[i].nCalls=pInfos[i].nTicks = 0;
            }

            if (inrange(m_iJumpToPhysProfileEnt, 0, nEnts + 1))
            {
                ScriptHandle hdl;
                hdl.n = ~0;
                if (m_env.pScriptSystem)
                {
                    m_env.pScriptSystem->GetGlobalValue("g_localActorId", hdl);
                }
                IEntity* pPlayerEnt = m_env.pEntitySystem ? m_env.pEntitySystem->GetEntity((EntityId)hdl.n) : nullptr;
                IPhysicalEntity* pent = pWorld->GetPhysicalEntityById(pInfos[m_iJumpToPhysProfileEnt - 1].id);
                if (pPlayerEnt && pent)
                {
                    pe_params_bbox pbb;
                    pent->GetParams(&pbb);
                    pPlayerEnt->SetPos((pbb.BBox[0] + pbb.BBox[1]) * 0.5f + Vec3(0, -3.0f, 1.5f));
                    pPlayerEnt->SetRotation(Quat(IDENTITY));
                }
                m_iJumpToPhysProfileEnt = 0;
            }
        }
        if (pVars->bProfileFunx)
        {
            int j, mask, nFunx = pWorld->GetFuncProfileInfo(pInfos);
            float fColor[4] = { 0.75f, 0.08f, 0.85f, 1.0f };
            for (j = 0, ++i; j < nFunx; j++, i++)
            {
                mask = (pInfos[j].nTicksPeak - pInfos[j].nTicks) >> 31;
                mask |= (70 - pInfos[j].peakAge) >> 31;
                pInfos[j].nTicksPeak += pInfos[j].nTicks - pInfos[j].nTicksPeak & mask;
                pInfos[j].nCallsPeak += pInfos[j].nCalls - pInfos[j].nCallsPeak & mask;
                GetIRenderer()->Draw2dLabel(10.0f, 60.0f + i * 12.0f, 1.3f, fColor, false,
                    "%s %.2fms/%d (peak %.2fms/%d)", pInfos[j].pName, gEnv->pTimer->TicksToSeconds(pInfos[j].nTicks) * 1000.0f, pInfos[j].nCalls,
                    gEnv->pTimer->TicksToSeconds(pInfos[j].nTicksPeak) * 1000.0f, pInfos[j].nCallsPeak);
                pInfos[j].peakAge = pInfos[j].peakAge + 1 & ~mask;
                pInfos[j].nCalls = pInfos[j].nTicks = 0;
            }
        }
        if (pVars->bProfileGroups)
        {
            int j = 0, mask, nGroups = pWorld->GetGroupProfileInfo(pInfos), nJobs = pWorld->GetJobProfileInfo(pJobInfos);
            float fColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            if (!pVars->bProfileEntities)
            {
                j = 12;
            }

            for (++i; j < nGroups; j++, i++)
            {
                pInfos[j].nTicksAvg = (int)(((int64)pInfos[j].nTicksAvg * 15 + pInfos[j].nTicksLast) >> 4);
                mask = (pInfos[j].nTicksPeak - pInfos[j].nTicksLast) >> 31;
                mask |= (70 - pInfos[j].peakAge) >> 31;
                pInfos[j].nTicksPeak += pInfos[j].nTicksLast - pInfos[j].nTicksPeak & mask;
                pInfos[j].nCallsPeak += pInfos[j].nCallsLast - pInfos[j].nCallsPeak & mask;
                float time = gEnv->pTimer->TicksToSeconds(pInfos[j].nTicksAvg) * 1000.0f;
                float timeNorm = time * (1.0f / 32);
                fColor[1] = fColor[2] = 1.0f - (max(0.7f, min(1.0f, timeNorm)) - 0.7f) * (1.0f / 0.3f);
                GetIRenderer()->Draw2dLabel(10.0f, 60.0f + i * 12.0f, 1.3f, fColor, false,
                    "%s %.2fms/%d (peak %.2fms/%d)", pInfos[j].pName, time, pInfos[j].nCallsLast,
                    gEnv->pTimer->TicksToSeconds(pInfos[j].nTicksPeak) * 1000.0f, pInfos[j].nCallsPeak);
                pInfos[j].peakAge = pInfos[j].peakAge + 1 & ~mask;
                if (j == nGroups - 4)
                {
                    ++i;
                }
            }
        }
        if (pVars->bProfileEntities == 2)
        {
            int nEnts = m_env.pPhysicalWorld->GetEntityProfileInfo(pInfos);

            for (int j = 0; j < nEnts; j++)
            {
                gEnv->pStatoscope->AddPhysEntity(&pInfos[j]);
            }
        }
    }
#endif
}

//! Update screen and call some important tick functions during loading.
void CSystem::SynchronousLoadingTick(const char* pFunc, int line)
{
    LOADING_TIME_PROFILE_SECTION;
    if (gEnv && gEnv->bMultiplayer && !gEnv->IsEditor())
    {
        //UpdateLoadingScreen currently contains a couple of tick functions that need to be called regularly during the synchronous level loading,
        //when the usual engine and game ticks are suspended.
        UpdateLoadingScreen();

#if defined(MAP_LOADING_SLICING)
        GetISystemScheduler()->SliceAndSleep(pFunc, line);
#endif
    }
}


//////////////////////////////////////////////////////////////////////////
#define CHECK_UPDATE_TIMES
void CSystem::UpdateLoadingScreen()
{
    // Do not update the network thread from here - it will cause context corruption - use the NetworkStallTicker thread system

    if (GetCurrentThreadId() != gEnv->mMainThreadId)
    {
        return;
    }

#if defined(CHECK_UPDATE_TIMES)
#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(SystemRender_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#endif // CHECK_UPDATE_TIMES

#if AZ_LOADSCREENCOMPONENT_ENABLED
    EBUS_EVENT(LoadScreenBus, UpdateAndRender);
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED

    if (!m_bEditor && !m_bQuit)
    {
        if (m_pProgressListener)
        {
            m_pProgressListener->OnLoadingProgress(0);
        }
    }
}

//////////////////////////////////////////////////////////////////////////

void CSystem::DisplayErrorMessage(const char* acMessage,
    float fTime,
    const float* pfColor,
    bool bHardError)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;

    SErrorMessage message;
    message.m_Message = acMessage;
    if (pfColor)
    {
        memcpy(message.m_Color, pfColor, 4 * sizeof(float));
    }
    else
    {
        message.m_Color[0] = 1.0f;
        message.m_Color[1] = 0.0f;
        message.m_Color[2] = 0.0f;
        message.m_Color[3] = 1.0f;
    }
    message.m_HardFailure = bHardError;
#ifdef _RELEASE
    message.m_fTimeToShow = fTime;
#else
    message.m_fTimeToShow = 1.0f;
#endif
    m_ErrorMessages.push_back(message);
}

//! Renders the statistics; this is called from RenderEnd, but if the
//! Host application (Editor) doesn't employ the Render cycle in ISystem,
//! it may call this method to render the essential statistics
//////////////////////////////////////////////////////////////////////////
void CSystem::RenderStatistics()
{
    RenderStats();
#if defined(USE_FRAME_PROFILER)
    // Render profile info.
    m_FrameProfileSystem.Render();
    if (m_pThreadProfiler)
    {
        m_pThreadProfiler->Render();
    }

#if defined(USE_DISK_PROFILER)
    if (m_pDiskProfiler)
    {
        m_pDiskProfiler->Update();
    }
#endif

    RenderMemStats();

    if (m_sys_profile_sampler->GetIVal() > 0)
    {
        m_sys_profile_sampler->Set(0);
        m_FrameProfileSystem.StartSampling(m_sys_profile_sampler_max_samples->GetIVal());
    }

    // Update frame profiler from sys variable: 1 = enable and display, -1 = just enable
    int profValue = m_sys_profile->GetIVal();
    static int prevProfValue = -100;
    bool bEnable = profValue != 0,
         bDisplay = profValue > 0;
    if (prevProfValue != profValue)
    {
        prevProfValue = profValue;
        int dispNum = abs(profValue);
        m_FrameProfileSystem.SetDisplayQuantity((CFrameProfileSystem::EDisplayQuantity)(dispNum - 1));
    }
    if (bEnable != m_FrameProfileSystem.IsEnabled() || bDisplay != m_FrameProfileSystem.IsVisible())
    {
        m_FrameProfileSystem.Enable(bEnable, bDisplay);
    }
    if (m_FrameProfileSystem.IsEnabled())
    {
        static string sSysProfileFilter;
        if (azstricmp(m_sys_profile_filter->GetString(), sSysProfileFilter.c_str()) != 0)
        {
            sSysProfileFilter = m_sys_profile_filter->GetString();
            m_FrameProfileSystem.SetSubsystemFilter(sSysProfileFilter.c_str());
        }
        static string sSysProfileFilterThread;
        if (0 != sSysProfileFilterThread.compare(m_sys_profile_filter_thread->GetString()))
        {
            sSysProfileFilterThread = m_sys_profile_filter_thread->GetString();
            m_FrameProfileSystem.SetSubsystemFilterThread(sSysProfileFilterThread.c_str());
            m_sys_profile_allThreads->Set(1);
        }
        m_FrameProfileSystem.SetHistogramScale(m_sys_profile_graphScale->GetFVal());
        m_FrameProfileSystem.SetDrawGraph(m_sys_profile_graph->GetIVal() != 0);
        m_FrameProfileSystem.SetThreadSupport(m_sys_profile_allThreads->GetIVal());
        m_FrameProfileSystem.SetNetworkProfiler(m_sys_profile_network->GetIVal() != 0);
        m_FrameProfileSystem.SetPeakTolerance(m_sys_profile_peak->GetFVal());
        m_FrameProfileSystem.SetPageFaultsGraph(m_sys_profile_pagefaultsgraph->GetIVal() != 0);
        m_FrameProfileSystem.SetPeakDisplayDuration(m_sys_profile_peak_time->GetFVal());
        m_FrameProfileSystem.SetAdditionalSubsystems(m_sys_profile_additionalsub->GetIVal() != 0);
    }
    static int memProfileValueOld = 0;
    int memProfileValue = m_sys_profile_memory->GetIVal();
    if (memProfileValue != memProfileValueOld)
    {
        memProfileValueOld = memProfileValue;
        m_FrameProfileSystem.EnableMemoryProfile(memProfileValue != 0);
    }
#endif

    RenderThreadInfo();
#if !defined(_RELEASE)
    if (m_sys_enable_budgetmonitoring->GetIVal())
    {
        m_pIBudgetingSystem->MonitorBudget();
    }
#endif
}

//////////////////////////////////////////////////////////////////////
void CSystem::Render()
{
    if (m_bIgnoreUpdates)
    {
        return;
    }

    //check what is the current process
    if (!m_pProcess)
    {
        return; //should never happen
    }
    //check if the game is in pause or
    //in menu mode
    //bool bPause=false;
    //if (m_pProcess->GetFlags() & PROC_MENU)
    //  bPause=true;

    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);

    //////////////////////////////////////////////////////////////////////
    //draw
    m_env.p3DEngine->PreWorldStreamUpdate(m_ViewCamera);

    if (m_pProcess)
    {
        if (m_pProcess->GetFlags() & PROC_3DENGINE)
        {
            if (!gEnv->IsEditing())  // Editor calls it's own rendering update
            {
#if !defined(_RELEASE)
                if (m_pTestSystem)
                {
                    m_pTestSystem->BeforeRender();
                }
#endif

                if (m_env.p3DEngine && !m_env.IsFMVPlaying())
                {
                    if (!IsEquivalent(m_ViewCamera.GetPosition(), Vec3(0, 0, 0), VEC_EPSILON) || // never pass undefined camera to p3DEngine->RenderWorld()
                        gEnv->IsDedicated() || (gEnv->pRenderer && gEnv->pRenderer->IsPost3DRendererEnabled()))
                    {
                        GetIRenderer()->SetViewport(0, 0, GetIRenderer()->GetWidth(), GetIRenderer()->GetHeight());
                        m_env.p3DEngine->RenderWorld(SHDF_ALLOW_WATER | SHDF_ALLOWPOSTPROCESS | SHDF_ALLOWHDR | SHDF_ZPASS | SHDF_ALLOW_AO, SRenderingPassInfo::CreateGeneralPassRenderingInfo(m_ViewCamera), __FUNCTION__);
                    }
                    else
                    {
                        if (gEnv->pRenderer)
                        {
                            // force rendering of black screen to be sure we don't only render the clear color (which is the fog color by default)
                            gEnv->pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
                            gEnv->pRenderer->Draw2dImage(0, 0, 800, 600, -1, 0.0f, 0.0f, 1.0f, 1.0f,   0.f,
                                0.0f, 0.0f, 0.0f, 1.0f, 0.f);
                        }
                    }
                }
#if !defined(_RELEASE)
                if (m_pVisRegTest)
                {
                    m_pVisRegTest->AfterRender();
                }
                if (m_pTestSystem)
                {
                    m_pTestSystem->AfterRender();
                }

                //          m_pProcess->Draw();

                if (m_env.pEntitySystem)
                {
                    m_env.pEntitySystem->DebugDraw();
                }
#endif

                if (m_env.pMovieSystem)
                {
                    m_env.pMovieSystem->Render();
                }
            }
        }
        else
        {
            GetIRenderer()->SetViewport(0, 0, GetIRenderer()->GetWidth(), GetIRenderer()->GetHeight());
            m_pProcess->RenderWorld(SHDF_ALLOW_WATER | SHDF_ALLOWPOSTPROCESS | SHDF_ALLOWHDR | SHDF_ZPASS | SHDF_ALLOW_AO, SRenderingPassInfo::CreateGeneralPassRenderingInfo(m_ViewCamera), __FUNCTION__);
        }
    }

    m_env.p3DEngine->WorldStreamUpdate();

    gEnv->pRenderer->SwitchToNativeResolutionBackbuffer();
}

//////////////////////////////////////////////////////////////////////////
void CSystem::RenderMemStats()
{
#if !defined(_RELEASE)
    // check for the presence of the system
    if (!m_env.pConsole || !m_env.pRenderer || !m_cvMemStats->GetIVal())
    {
        SAFE_DELETE(m_pMemStats);
        return;
    }

    TickMemStats();

    assert (m_pMemStats);
    m_pMemStats->updateKeys();
    // render the statistics
    {
        CrySizerStatsRenderer StatsRenderer (this, m_pMemStats, m_cvMemStatsMaxDepth->GetIVal(), m_cvMemStatsThreshold->GetIVal());
        StatsRenderer.render((m_env.pRenderer->GetFrameID(false) + 2) % m_cvMemStats->GetIVal() <= 1);
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
void CSystem::RenderStats()
{
#if defined(ENABLE_PROFILING_CODE)
#ifndef _RELEASE
    // if we rendered an error message on screen during the last frame, then sleep now
    // to force hard stall for 3sec
    if (m_bHasRenderedErrorMessage && !gEnv->IsEditor() && !IsLoading())
    {
        // DO NOT REMOVE OR COMMENT THIS OUT!
        // If you hit this, then you most likely have invalid (synchronous) file accesses
        // which must be fixed in order to not stall the entire game.
        Sleep(3000);
        m_bHasRenderedErrorMessage = false;
    }
#endif

    // render info messages on screen
    float fTextPosX = 5.0f;
    float fTextPosY = -10;
    float fTextStepY = 13;

    float fFrameTime = gEnv->pTimer->GetRealFrameTime();
    TErrorMessages::iterator itnext;
    for (TErrorMessages::iterator it = m_ErrorMessages.begin(); it != m_ErrorMessages.end(); it = itnext)
    {
        itnext = it;
        ++itnext;
        SErrorMessage& message = *it;

        SDrawTextInfo ti;
        ti.flags = eDrawText_FixedSize | eDrawText_2D | eDrawText_Monospace;
        memcpy(ti.color, message.m_Color, 4 * sizeof(float));
        ti.xscale = ti.yscale = 1.4f;
        m_env.pRenderer->DrawTextQueued(Vec3(fTextPosX, fTextPosY += fTextStepY, 1.0f), ti, message.m_Message.c_str());

        if (!IsLoading())
        {
            message.m_fTimeToShow -= fFrameTime;
        }

        if (message.m_HardFailure)
        {
            m_bHasRenderedErrorMessage = true;
        }

        if (message.m_fTimeToShow < 0.0f)
        {
            m_ErrorMessages.erase(it);
        }
    }
#endif

    if (!m_env.pConsole)
    {
        return;
    }

#ifndef _RELEASE
    if (m_rOverscanBordersDrawDebugView)
    {
        RenderOverscanBorders();
    }
#endif

    int iDisplayInfo = m_rDisplayInfo->GetIVal();
    if (iDisplayInfo == 0)
    {
        return;
    }

    // Draw engine stats
    if (m_env.p3DEngine)
    {
        // Draw 3dengine stats and get last text cursor position
        float nTextPosX = 101 - 20, nTextPosY = -2, nTextStepY = 3;
        m_env.p3DEngine->DisplayInfo(nTextPosX, nTextPosY, nTextStepY, iDisplayInfo != 1);

    #if defined(ENABLE_LW_PROFILERS)
        if (m_rDisplayInfo->GetIVal() == 2)
        {
            m_env.pRenderer->TextToScreen(nTextPosX, nTextPosY += nTextStepY, "SysMem %.1f mb",
                float(DumpMMStats(false)) / 1024.f);
        }
    #endif

    #if 0
        for (int i = 0; i < NUM_POOLS; ++i)
        {
            int used = (g_pPakHeap->m_iBigPoolUsed[i] ? (int)g_pPakHeap->m_iBigPoolSize[i] : 0);
            int size = (int)g_pPakHeap->m_iBigPoolSize[i];
            float fC1[4] = {1, 1, 0, 1};
            m_env.pRenderer->Draw2dLabel(10, 100.0f + i * 16, 2.1f, fC1, false,  "BigPool %d: %d bytes of %d bytes used", i, used, size);
        }
    #endif
    }
}

void CSystem::RenderOverscanBorders()
{
#ifndef _RELEASE

    if (m_env.pRenderer && m_rOverscanBordersDrawDebugView)
    {
        int iOverscanBordersDrawDebugView = m_rOverscanBordersDrawDebugView->GetIVal();
        if (iOverscanBordersDrawDebugView)
        {
            const int texId = -1;
            const float uv = 0.0f;
            const float rot = 0.0f;
            const int whiteTextureId = m_env.pRenderer->GetWhiteTextureId();

            const float r = 1.0f;
            const float g = 1.0f;
            const float b = 1.0f;
            const float a = 0.2f;

            Vec2 overscanBorders = Vec2(0.0f, 0.0f);
            m_env.pRenderer->EF_Query(EFQ_OverscanBorders, overscanBorders);

            const float overscanBorderWidth = overscanBorders.x * VIRTUAL_SCREEN_WIDTH;
            const float overscanBorderHeight = overscanBorders.y * VIRTUAL_SCREEN_HEIGHT;

            const float xPos = overscanBorderWidth;
            const float yPos = overscanBorderHeight;
            const float width = VIRTUAL_SCREEN_WIDTH - (2.0f * overscanBorderWidth);
            const float height = VIRTUAL_SCREEN_HEIGHT - (2.0f * overscanBorderHeight);

            m_env.pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
            m_env.pRenderer->Draw2dImage(xPos, yPos,
                width, height,
                whiteTextureId,
                uv, uv, uv, uv,
                rot,
                r, g, b, a);
        }
    }
#endif
}

void CSystem::RenderThreadInfo()
{
#if !defined(RELEASE) && AZ_LEGACY_CRYSYSTEM_TRAIT_RENDERTHREADINFO
    if (g_cvars.sys_display_threads)
    {
        static int maxCPU = 0;
        const int maxAvailCPU = 6;
        static unsigned int maxMask = 0;

        if (maxCPU == 0)
        {
            SYSTEM_INFO sysinfo;
            GetSystemInfo(&sysinfo);
            maxCPU = sysinfo.dwNumberOfProcessors;
            maxMask = (1 << maxCPU) - 1;
        }

        struct SThreadProcessorInfo
        {
            SThreadProcessorInfo(const string& _name, uint32 _id)
                : name(_name)
                , id(_id) {}
            string name;
            uint32 id;
        };

        static std::multimap<DWORD, SThreadProcessorInfo> sortetThreads;
        static SThreadInfo::TThreads threads;
        static SThreadInfo::TThreadInfo threadInfo;
        static int frame = 0;

        if ((frame++ % 100) == 0) // update thread info every 100 frame
        {
            threads.clear();
            threadInfo.clear();
            sortetThreads.clear();
            SThreadInfo::OpenThreadHandles(threads, SThreadInfo::TThreadIds(), false);
            SThreadInfo::GetCurrentThreads(threadInfo);
            for (SThreadInfo::TThreads::const_iterator it = threads.begin(); it != threads.end(); ++it)
            {
                DWORD mask = (DWORD)SetThreadAffinityMask(it->Handle, maxMask);
                SetThreadAffinityMask(it->Handle, mask);
                sortetThreads.insert(std::make_pair(mask, SThreadProcessorInfo(threadInfo[it->Id], it->Id)));
            }
            SThreadInfo::CloseThreadHandles(threads);
        }

        float nX = 5, nY = 10, dY = 12, dX = 10;
        float fFSize = 1.2f;
        ColorF col1 = Col_Yellow;
        ColorF col2 = Col_Red;

        for (int i = 0; i < maxCPU; ++i)
        {
            gEnv->pRenderer->Draw2dLabel(nX + i * dX, nY, fFSize, i < maxAvailCPU ? &col1.r : &col2.r, false, "%i", i + 1);
        }

        nY += dY;
        for (std::multimap<DWORD, SThreadProcessorInfo>::const_iterator it = sortetThreads.begin(); it != sortetThreads.end(); ++it)
        {
            for (int i = 0; i < maxCPU; ++i)
            {
                gEnv->pRenderer->Draw2dLabel(nX + i * dX, nY, fFSize, i < maxAvailCPU ? &col1.r : &col2.r, false, "%s", it->first & BIT(i) ? "X" : "-");
            }

            gEnv->pRenderer->Draw2dLabel(nX + dX * maxCPU, nY, fFSize, &col1.r, false, "Thread: %s (0x%X)", it->second.name.c_str(), it->second.id);
            nY += dY;
        }
    }
#endif
}
