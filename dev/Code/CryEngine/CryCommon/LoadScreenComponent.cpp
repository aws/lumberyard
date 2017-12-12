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
#include <ISystem.h>
#include <IRenderer.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <LyShine/ILyShine.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Animation/IUiAnimation.h>

#include "LoadScreenComponent.h"

#if defined(AZ_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>   // GetCurrentThreadId
#endif

#if AZ_LOADSCREENCOMPONENT_ENABLED

void LoadScreenComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<LoadScreenComponent, AZ::Component>()
            ->Version(1)
            ->SerializerForEmptyClass();

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            editContext->Class<LoadScreenComponent>(
                "Load screen manager", "Allows management of a load screen")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "Game")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
            ;
        }
    }
}

void LoadScreenComponent::Reset()
{
    m_fixedDeltaTimeInSeconds = -1.0f;
    m_maxDeltaTimeInSeconds = -1.0f;
    m_previousCallTimeForUpdateAndRender = CTimeValue();
    m_isPlaying = false;
    m_processingLoadScreen.store(false);

    if (gEnv && gEnv->pLyShine)
    {
        AZ::Entity* canvasEntity = nullptr;

        // Release the game canvas.
        if (m_gameCanvasEntityId.IsValid())
        {
            EBUS_EVENT_RESULT(canvasEntity, AZ::ComponentApplicationBus, FindEntity, m_gameCanvasEntityId);
            if (canvasEntity)
            {
                gEnv->pLyShine->ReleaseCanvas(m_gameCanvasEntityId, false);
            }
        }

        // Release the level canvas.
        if (m_levelCanvasEntityId.IsValid())
        {
            EBUS_EVENT_RESULT(canvasEntity, AZ::ComponentApplicationBus, FindEntity, m_levelCanvasEntityId);
            if (canvasEntity)
            {
                gEnv->pLyShine->ReleaseCanvas(m_levelCanvasEntityId, false);
            }
        }
    }

    m_gameCanvasEntityId.SetInvalid();
    m_levelCanvasEntityId.SetInvalid();

    // IMPORTANT: It's NECESSARY to clear ALL the CVars, otherwise their
    // values will be carried over into levels where they AREN'T set.
    ClearCVars({"level_load_screen_uicanvas_path",
                "level_load_screen_sequence_to_auto_play",
                "level_load_screen_sequence_fixed_fps",
                "level_load_screen_max_fps"});
}

void LoadScreenComponent::ClearCVars(const std::list<const char*>& varNames)
{
    if (!gEnv ||
        !gEnv->pConsole)
    {
        return;
    }

    for (auto name : varNames)
    {
        ICVar* var = gEnv->pConsole->GetCVar(name);
        if (var)
        {
            var->Set("");
        }
    }
}

AZ::EntityId LoadScreenComponent::loadFromCfg(const char* pathVarName, const char* autoPlayVarName, const char* fixedFpsVarName, const char* maxFpsVarName)
{
    ICVar* pathVar = gEnv->pConsole->GetCVar(pathVarName);
    string path = pathVar ? pathVar->GetString() : "";
    if (path.empty())
    {
        // No canvas specified.
        Reset();
        return AZ::EntityId();
    }

    AZ::EntityId canvasId = gEnv->pLyShine->LoadCanvas(path);
    AZ_Warning("LoadScreenComponent", canvasId.IsValid(), "Can't load canvas: %s", path.c_str());
    if (!canvasId.IsValid())
    {
        // Error loading canvas.
        Reset();
        return AZ::EntityId();
    }

    EBUS_EVENT_ID(canvasId, UiCanvasBus, SetKeepLoadedOnLevelUnload, true);

    ICVar* autoPlayVar = gEnv->pConsole->GetCVar(autoPlayVarName);
    string sequence = autoPlayVar ? autoPlayVar->GetString() : "";
    if (sequence.empty())
    {
        // Nothing to auto-play.
        return canvasId;
    }

    IUiAnimationSystem* animSystem = nullptr;
    EBUS_EVENT_ID_RESULT(animSystem, canvasId, UiCanvasBus, GetAnimationSystem);
    if (!animSystem)
    {
        // Nothing can be auto-played.
        return canvasId;
    }

    ICVar* fixedFpsVar = gEnv->pConsole->GetCVar(fixedFpsVarName);
    if (fixedFpsVar &&
        fixedFpsVar->GetFVal() > 0.0f)
    {
        m_fixedDeltaTimeInSeconds = (1.0f / fixedFpsVar->GetFVal());
    }
    else
    {
        m_fixedDeltaTimeInSeconds = -1.0f;
    }

    ICVar* maxFpsVar = gEnv->pConsole->GetCVar(maxFpsVarName);
    if (maxFpsVar &&
        maxFpsVar->GetFVal() > 0.0f)
    {
        m_maxDeltaTimeInSeconds = (1.0f / maxFpsVar->GetFVal());
    }
    else
    {
        m_maxDeltaTimeInSeconds = -1.0f;
    }

    animSystem->PlaySequence(sequence, nullptr, false, false);

    return canvasId;
}

void LoadScreenComponent::Init()
{
    Reset();
}

void LoadScreenComponent::Activate()
{
    CrySystemEventBus::Handler::BusConnect();
    LoadScreenBus::Handler::BusConnect(GetEntityId());
}

void LoadScreenComponent::Deactivate()
{
    CrySystemEventBus::Handler::BusDisconnect();
    LoadScreenBus::Handler::BusDisconnect(GetEntityId());
}

void LoadScreenComponent::OnCrySystemInitialized(ISystem&, const SSystemInitParams&)
{
    if (gEnv->pSystem->GetGlobalEnvironment()->IsEditor())
    {
        return;
    }

    GameStart();
}

void LoadScreenComponent::OnCrySystemShutdown(ISystem&)
{
}

void LoadScreenComponent::UpdateAndRender()
{
    // IMPORTANT: We DON'T want to early out on m_isPlaying.
    // Because it's possible for a canvas to be loaded in LyShine,
    // outside of the scope of this class. In which case, we want
    // to update LyShine unconditionally.
    if (!gEnv ||
        !gEnv->pRenderer ||
        !gEnv->pTimer ||
        !gEnv->pLyShine)
    {
        return;
    }

    if (GetCurrentThreadId() != gEnv->mMainThreadId)
    {
        return;
    }

    // Throttling.
    float deltaTimeInSeconds;
    CTimeValue callTimeForUpdateAndRender;
    {
        if (!m_previousCallTimeForUpdateAndRender.GetValue())
        {
            // This is the first call to UpdateAndRender().
            m_previousCallTimeForUpdateAndRender = gEnv->pTimer->GetAsyncTime();
        }

        callTimeForUpdateAndRender = gEnv->pTimer->GetAsyncTime();
        deltaTimeInSeconds = fabs((callTimeForUpdateAndRender - m_previousCallTimeForUpdateAndRender).GetSeconds());
        if ((m_maxDeltaTimeInSeconds > 0.0f) &&
            (deltaTimeInSeconds < m_maxDeltaTimeInSeconds))
        {
            // Early-out: We DON'T need to execute UpdateAndRender() at a higher frequency than 30 FPS.
            return;
        }
    }

    bool expectedValue = false;
    if (!m_processingLoadScreen.compare_exchange_strong(expectedValue, true))
    {
        // Don't do anything.
        return;
    }

    m_previousCallTimeForUpdateAndRender = callTimeForUpdateAndRender;

    // update the animation system
    gEnv->pLyShine->Update((m_fixedDeltaTimeInSeconds == -1.0f) ? deltaTimeInSeconds : m_fixedDeltaTimeInSeconds);

    // Render.
    if (m_isPlaying)
    {
        gEnv->pRenderer->SetViewport(0, 0, gEnv->pRenderer->GetOverlayWidth(), gEnv->pRenderer->GetOverlayHeight());

        gEnv->pRenderer->BeginFrame();
        gEnv->pLyShine->Render();
        gEnv->pRenderer->EndFrame();
    }

    m_processingLoadScreen.store(false);
}

void LoadScreenComponent::GameStart()
{
    if (!gEnv ||
        !gEnv->pRenderer ||
        !gEnv->pLyShine ||
        m_isPlaying)
    {
        return;
    }

    if (!m_gameCanvasEntityId.IsValid())
    {
        // Load canvas.
        m_gameCanvasEntityId = loadFromCfg("game_load_screen_uicanvas_path",
                "game_load_screen_sequence_to_auto_play",
                "game_load_screen_sequence_fixed_fps",
                "game_load_screen_max_fps");
    }

    if (m_gameCanvasEntityId.IsValid())
    {
        m_isPlaying = true;

        // Kick-start the first frame.
        UpdateAndRender();
    }
}

void LoadScreenComponent::LevelStart()
{
    if (!gEnv ||
        !gEnv->pRenderer ||
        !gEnv->pLyShine ||
        m_isPlaying ||
        m_gameCanvasEntityId.IsValid())
    {
        return;
    }

    if (!m_levelCanvasEntityId.IsValid())
    {
        // Load canvas.
        m_levelCanvasEntityId = loadFromCfg("level_load_screen_uicanvas_path",
                "level_load_screen_sequence_to_auto_play",
                "level_load_screen_sequence_fixed_fps",
                "level_load_screen_max_fps");
    }

    if (m_levelCanvasEntityId.IsValid())
    {
        m_isPlaying = true;

        // Kick-start the first frame.
        UpdateAndRender();
    }
}

void LoadScreenComponent::Pause()
{
    if (!gEnv ||
        !gEnv->pRenderer ||
        !gEnv->pLyShine ||
        !m_isPlaying ||
        !(m_gameCanvasEntityId.IsValid() || m_levelCanvasEntityId.IsValid()))
    {
        return;
    }

    m_isPlaying = false;
}

void LoadScreenComponent::Resume()
{
    if (!gEnv ||
        !gEnv->pRenderer ||
        !gEnv->pLyShine ||
        m_isPlaying ||
        !(m_gameCanvasEntityId.IsValid() || m_levelCanvasEntityId.IsValid()))
    {
        return;
    }

    m_isPlaying = true;
}

void LoadScreenComponent::Stop()
{
    Reset();
}

#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
