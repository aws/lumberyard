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

// Description : System "Hardware mouse" cursor with reference counter.
//               This is needed because Menus / HUD / Profiler / or whatever
//               can use the cursor not at the same time be successively
//               => We need to know when to enable/disable the cursor.


#include "StdAfx.h"

#include "HardwareMouse.h"
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

#if defined(AZ_FRAMEWORK_INPUT_ENABLED)
//-----------------------------------------------------------------------------------------------------
HardwareMouse::HardwareMouse(bool visibleByDefault, const char* defaultCursorPath)
    : m_cursorTexture(nullptr)
    , m_cursorPositionX(0.0f)
    , m_cursorPositionY(0.0f)
    , m_visibleCounter(visibleByDefault ? 1 : 0)
{
    SetCursor(defaultCursorPath);

    if (!gEnv->IsEditor())
    {
        // Hide and constrain the system cursor
        AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                        &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                                                        AzFramework::SystemCursorState::ConstrainedAndHidden);
    }
}

//-----------------------------------------------------------------------------------------------------
HardwareMouse::~HardwareMouse()
{
}

//-----------------------------------------------------------------------------------------------------
void HardwareMouse::Release()
{
    delete this;
}

//-----------------------------------------------------------------------------------------------------
bool HardwareMouse::SetCursor(const char* cursorPath)
{
    if (m_cursorTexture)
    {
        m_cursorTexture->Release();
        m_cursorTexture = nullptr;
    }

    if (!cursorPath || !gEnv || !gEnv->pRenderer)
    {
        return false;
    }

    m_cursorTexture = gEnv->pRenderer->EF_LoadTexture(cursorPath, FT_DONT_RELEASE | FT_DONT_STREAM);
    if (!m_cursorTexture)
    {
        return false;
    }

    m_cursorTexture->SetClamp(true);
    return true;
}

//-----------------------------------------------------------------------------------------------------
void HardwareMouse::SetGameMode(bool gameMode)
{
    const AzFramework::SystemCursorState desiredState = gameMode ?
                                                        AzFramework::SystemCursorState::ConstrainedAndHidden :
                                                        AzFramework::SystemCursorState::UnconstrainedAndVisible;
    AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                    &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                                                    desiredState);
}

//-----------------------------------------------------------------------------------------------------
void HardwareMouse::IncrementCounter()
{
    ++m_visibleCounter;
}

//-----------------------------------------------------------------------------------------------------
void HardwareMouse::DecrementCounter()
{
    --m_visibleCounter;
}

//-----------------------------------------------------------------------------------------------------
bool HardwareMouse::IsHidden()
{
    return m_visibleCounter <= 0;
}

//-----------------------------------------------------------------------------------------------------
void HardwareMouse::Update()
{
    AZ::Vector2 systemCursorPositionNormalized = AZ::Vector2::CreateZero();
    AzFramework::InputSystemCursorRequestBus::EventResult(systemCursorPositionNormalized,
                                                          AzFramework::InputDeviceMouse::Id,
                                                          &AzFramework::InputSystemCursorRequests::GetSystemCursorPositionNormalized);
    m_cursorPositionX = systemCursorPositionNormalized.GetX() * static_cast<float>(gEnv->pRenderer->GetWidth());
    m_cursorPositionY = systemCursorPositionNormalized.GetY() * static_cast<float>(gEnv->pRenderer->GetHeight());
}

//-----------------------------------------------------------------------------------------------------
void HardwareMouse::Render()
{
#if !defined(AZ_PLATFORM_APPLE_IOS) && !defined(AZ_PLATFORM_ANDROID)
    if (!gEnv || !gEnv->pRenderer || !m_cursorTexture || m_visibleCounter <= 0)
    {
        return;
    }

    const float sizeX = static_cast<float>(m_cursorTexture->GetWidth());
    const float sizeY = static_cast<float>(m_cursorTexture->GetHeight());
    const float scaleX = gEnv->pRenderer->ScaleCoordX(1.0f);
    const float scaleY = gEnv->pRenderer->ScaleCoordY(1.0f);

    gEnv->pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
    gEnv->pRenderer->Draw2dImage(m_cursorPositionX / scaleX, m_cursorPositionY / scaleY, sizeX / scaleX, sizeY / scaleY, m_cursorTexture->GetTextureID(), 0, 1, 1, 0, 0, 1, 1, 1, 1, 0);
#endif // !defined(AZ_PLATFORM_APPLE_IOS) && !defined(AZ_PLATFORM_ANDROID)
}
#else
#include "IConsole.h"
#include "IInput.h"
#include "ISystem.h"
#include "ITimer.h"
#include "WindowsUtils.h"

#if defined(WIN32)
#   include <IImage.h>
#endif

namespace
{
    const char* g_default_cursor_path   = "EngineAssets/Textures/Cursor_Green.tif";
    const int   g_cursorAlphaThreshold  = 200;

#if defined(WIN32)

    //-----------------------------------------------------------------------------------------------------

    void ReleaseCursor()
    {
        ::ClipCursor(NULL);
    }
#endif

    //-----------------------------------------------------------------------------------------------------
    void RefreshCursorVisibility(bool visible)
    {
    #if defined(AZ_FRAMEWORK_INPUT_ENABLED)
        AzFramework::SystemCursorState currentState = AzFramework::SystemCursorState::Unknown;
        AzFramework::InputSystemCursorRequestBus::EventResult(currentState,
                                                              AzFramework::InputDeviceMouse::Id,
                                                              &AzFramework::InputSystemCursorRequests::GetSystemCursorState);

        AzFramework::SystemCursorState desiredState = currentState;
        switch (currentState)
        {
            case AzFramework::SystemCursorState::ConstrainedAndHidden:
            {
                if (visible)
                {
                    desiredState = AzFramework::SystemCursorState::ConstrainedAndVisible;
                }
            }
            break;
            case AzFramework::SystemCursorState::ConstrainedAndVisible:
            {
                if (!visible)
                {
                    desiredState = AzFramework::SystemCursorState::ConstrainedAndHidden;
                }
            }
            break;
            case AzFramework::SystemCursorState::UnconstrainedAndHidden:
            {
                if (visible)
                {
                    desiredState = AzFramework::SystemCursorState::UnconstrainedAndVisible;
                }
            }
            break;
            case AzFramework::SystemCursorState::UnconstrainedAndVisible:
            {
                if (!visible)
                {
                    desiredState = AzFramework::SystemCursorState::UnconstrainedAndHidden;
                }
            }
            break;
            case AzFramework::SystemCursorState::Unknown:
            default:
            {
                if (visible)
                {
                    desiredState = AzFramework::SystemCursorState::UnconstrainedAndVisible;
                }
                else
                {
                    desiredState = AzFramework::SystemCursorState::ConstrainedAndHidden;
                }
            }
            break;
        }

        if (desiredState != currentState)
        {
            AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                            &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                                                            desiredState);
        }
    #elif defined(WIN32)
        ::ShowCursor(visible);
    #endif
    }
}

//-----------------------------------------------------------------------------------------------------

CHardwareMouse::CHardwareMouse(bool bVisibleByDefault)
    : m_debugHardwareMouse(0)
    , m_pCursorTexture(NULL)
#if !defined(_RELEASE)
    , m_allowConfine(GetISystem()->GetICmdLine()->FindArg(eCLAT_Pre, "nomouse") == NULL)
#else
    , m_allowConfine(true)
#endif // !defined(_RELEASE)
#ifdef WIN32
    , m_useSystemCursor(true)
    , m_shouldUseSystemCursor(true)
    , m_rememberLastPosition(true)
    , m_hCursor(NULL)
    , m_nCurIDCCursorId(~0)
#endif
{
#ifdef WIN32
    atexit(ReleaseCursor);
#endif

    SetCursor(g_default_cursor_path);

#ifndef WIN32
    if (gEnv->pRenderer)
    {
        SetHardwareMousePosition(gEnv->pRenderer->GetWidth() * 0.5f, gEnv->pRenderer->GetHeight() * 0.5f);
    }
    else
    {
        SetHardwareMousePosition(0.0f, 0.0f);
    }
#endif

    Reset(bVisibleByDefault);

    if (gEnv->pSystem)
    {
        gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);
    }

#if !defined(_RELEASE)
    if (gEnv->pConsole)
    {
        REGISTER_CVAR2("g_debugHardwareMouse", &m_debugHardwareMouse, 0, VF_CHEAT, "Enables debug mode for the hardware mouse.");
    }
#endif //!defined(_RELEASE)

    m_hide = false;
    m_calledShowHWMouse = false;

    if (IsFullscreen())
    {
        ConfineCursor(true);
    }

#ifdef WIN32
    CryLog ("Initialized hardware mouse (game is %s to confine mouse to window)", m_allowConfine ? "allowed" : "not allowed");
#endif
}

//-----------------------------------------------------------------------------------------------------

CHardwareMouse::~CHardwareMouse()
{
    if (gEnv)
    {
        if (gEnv->pRenderer)
        {
#if !defined(DEDICATED_SERVER)
            SAFE_RELEASE(m_pCursorTexture);         // On dedicated server this texture is actually a static returned by NULL renderer.. can't release that.
#endif
        }
        if (gEnv->pInput)
        {
            gEnv->pInput->RemoveEventListener(this);
        }

        if (gEnv->pSystem)
        {
            gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
        }

#if !defined(_RELEASE)
        if (gEnv->pConsole)
        {
            gEnv->pConsole->UnregisterVariable("g_debugHardwareMouse", true);
        }
#endif //!defined(_RELEASE)
    }

    DestroyCursor();
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::ShowHardwareMouse(bool bShow)
{
    if (m_debugHardwareMouse)
    {
        CryLogAlways("HM: ShowHardwareMouse = %d", bShow);
    }

    if (m_rememberLastPosition)
    {
        if (bShow)
        {
            SetHardwareMousePosition(m_fCursorX, m_fCursorY);
        }
        else
        {
            GetHardwareMousePosition(&m_fCursorX, &m_fCursorY);
        }
    }

    RefreshCursorVisibility((bShow && !m_hide) || (m_allowConfine == false));

    bool bConfine = !bShow;

    if (IsFullscreen())
    {
        bConfine = true;
    }

    ConfineCursor(bConfine);

    if (gEnv->pInput)
    {
        gEnv->pInput->SetExclusiveMode(eIDT_Mouse, false);
    }

    m_calledShowHWMouse = true;
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::ConfineCursor(bool confine)
{
    if (m_debugHardwareMouse)
    {
        CryLogAlways("HM: ConfineCursor = %d", confine);
    }

    if (!gEnv || gEnv->pRenderer == NULL || m_allowConfine == false)
    {
        return;
    }

#if defined(AZ_FRAMEWORK_INPUT_ENABLED)
    if (gEnv->IsEditing())
    {
        confine = false;
    }
    AzFramework::SystemCursorState currentState = AzFramework::SystemCursorState::Unknown;
    AzFramework::InputSystemCursorRequestBus::EventResult(currentState,
                                                          AzFramework::InputDeviceMouse::Id,
                                                          &AzFramework::InputSystemCursorRequests::GetSystemCursorState);

    AzFramework::SystemCursorState desiredState = currentState;
    switch (currentState)
    {
        case AzFramework::SystemCursorState::ConstrainedAndHidden:
        {
            if (!confine)
            {
                desiredState = AzFramework::SystemCursorState::UnconstrainedAndHidden;
            }
        }
        break;
        case AzFramework::SystemCursorState::ConstrainedAndVisible:
        {
            if (!confine)
            {
                desiredState = AzFramework::SystemCursorState::UnconstrainedAndVisible;
            }
        }
        break;
        case AzFramework::SystemCursorState::UnconstrainedAndHidden:
        {
            if (confine)
            {
                desiredState = AzFramework::SystemCursorState::ConstrainedAndHidden;
            }
        }
        break;
        case AzFramework::SystemCursorState::UnconstrainedAndVisible:
        {
            if (confine)
            {
                desiredState = AzFramework::SystemCursorState::ConstrainedAndHidden;
            }
        }
        break;
        case AzFramework::SystemCursorState::Unknown:
        default:
        {
            if (confine)
            {
                desiredState = AzFramework::SystemCursorState::ConstrainedAndHidden;
            }
            else
            {
                desiredState = AzFramework::SystemCursorState::UnconstrainedAndVisible;
            }
        }
        break;
    }

    if (desiredState != currentState)
    {
        AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                        &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                                                        desiredState);
    }
#elif defined(WIN32)
    HWND hWnd = 0;

    if (gEnv->IsEditor())
    {
        hWnd = (HWND) gEnv->pRenderer->GetCurrentContextHWND();
    }
    else
    {
        hWnd = (HWND) gEnv->pRenderer->GetHWND();
    }

    if (hWnd)
    {
        // It's necessary to call ClipCursor AFTER the calls to
        // CreateDevice/ResetDevice otherwise the clip area is reseted.
        if (confine && !gEnv->IsEditing())
        {
            if (m_debugHardwareMouse)
            {
                gEnv->pLog->Log("HM:   Confining cursor");
            }
            RECT rcClient;
            ::GetClientRect(hWnd, &rcClient);
            ::ClientToScreen(hWnd, (LPPOINT)&rcClient.left);
            ::ClientToScreen(hWnd, (LPPOINT)&rcClient.right);
            ::ClipCursor(&rcClient);
        }
        else
        {
            if (m_debugHardwareMouse)
            {
                gEnv->pLog->Log("HM:   Releasing cursor");
            }
            ::ClipCursor(NULL);
        }
    }
#elif (defined(LINUX) || defined(MAC)) && !defined(ANDROID)
    if (gEnv->pInput)
    {
        if (confine && !gEnv->IsEditing())
        {
            gEnv->pInput->GrabInput(true);
        }
        else
        {
            gEnv->pInput->GrabInput(false);
        }
    }
#endif
}

//-----------------------------------------------------------------------------------------------------

bool CHardwareMouse::OnInputEvent(const SInputEvent& rInputEvent)
{
    static float s_fAcceleration = 1.0f;

    const EKeyId thumbX = eKI_XI_ThumbLX;
    const EKeyId thumbY = eKI_XI_ThumbLY;
    const EKeyId XButton = eKI_XI_A; //A on 360

    if (0 == m_iReferenceCounter)
    {
        // Do not emulate if mouse is not present on screen

        // But need to reset movement values otherwise if using controller and mouse at same time, it can get into the situation where
        // the mouse activates before analog stick gets back to deadzone which will cause the cursor to be stuck moving from in below eKI_SYS_Commit event
        m_fIncX = 0.0f;
        m_fIncY = 0.0f;
    }
    else if (eIDT_Gamepad == rInputEvent.deviceType)
    {
        if (rInputEvent.keyId == thumbX)
        {
            m_fIncX = rInputEvent.value;
        }
        else if (rInputEvent.keyId == thumbY)
        {
            m_fIncY = -rInputEvent.value;
        }
        else if (rInputEvent.keyId == XButton)
        {
            // This emulation was just not right, A-s meaning is context sensitive
            /*if(eIS_Pressed == rInputEvent.state)
            {
                Event((int)m_fCursorX,(int)m_fCursorY,HARDWAREMOUSEEVENT_LBUTTONDOWN);
            }
            else if(eIS_Released == rInputEvent.state)
            {
                Event((int)m_fCursorX,(int)m_fCursorY,HARDWAREMOUSEEVENT_LBUTTONUP);
            }*/
            // TODO: do we simulate double-click?
        }
    }
    else if (rInputEvent.keyId == eKI_SYS_Commit)
    {
        const float fSensitivity = 100.0f;
        const float fDeadZone = 0.3f;

        if (m_fIncX < -fDeadZone || m_fIncX > +fDeadZone ||
            m_fIncY < -fDeadZone || m_fIncY > +fDeadZone)
        {
            float fFrameTime = gEnv->pTimer->GetFrameTime(ITimer::ETIMER_UI);
            if (s_fAcceleration < 10.0f)
            {
                s_fAcceleration += fFrameTime * 5.0f;
            }
            m_fCursorX += m_fIncX * fSensitivity * s_fAcceleration * fFrameTime;
            m_fCursorY += m_fIncY * fSensitivity * s_fAcceleration * fFrameTime;
            SetHardwareMousePosition(m_fCursorX, m_fCursorY);
        }
        else
        {
            GetHardwareMousePosition(&m_fCursorX, &m_fCursorY);
            s_fAcceleration = 1.0f;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------------------------------
void CHardwareMouse::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_ACTIVATE:
    case ESYSTEM_EVENT_CHANGE_FOCUS:
    {
        m_bFocus = wparam != 0;
        if (m_bFocus)
        {
            if (gEnv->IsEditorGameMode() && m_recapture)
            {
                m_recapture = false;
                DecrementCounter();
            }

            if (IsFullscreen() || gEnv->IsEditorGameMode() || m_iReferenceCounter == 0)
            {
                ConfineCursor(true);
            }
        }
        else
        {
            if (gEnv->IsEditorGameMode() && m_iReferenceCounter == 0)
            {
                m_recapture = true;
                IncrementCounter();
            }
        }

        ConfineCursor(m_bFocus);
        break;
    }

    case ESYSTEM_EVENT_MOVE:
    case ESYSTEM_EVENT_RESIZE:
    {
        if (IsFullscreen() || (m_iReferenceCounter == 0))
        {
            ConfineCursor(true);
        }
        break;
    }

    case ESYSTEM_EVENT_TOGGLE_FULLSCREEN:
    {
        if (wparam || (m_iReferenceCounter == 0))
        {
            if (IsFullscreen() || m_iReferenceCounter == 0)
            {
                ConfineCursor(true);
            }
        }
        break;
    }

    default:
        // do nothing
        break;
    }
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::Release()
{
    delete this;
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::OnPreInitRenderer()
{

}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::OnPostInitInput()
{
    CRY_ASSERT(gEnv->pInput);

    if (gEnv->pInput)
    {
        gEnv->pInput->AddEventListener(this);
    }
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::SetGameMode(bool bGameMode)
{
    if (bGameMode)
    {
        DecrementCounter();
    }
    else
    {
        IncrementCounter();
    }
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::IncrementCounter()
{
    m_iReferenceCounter++;

    if (m_debugHardwareMouse)
    {
        CryLogAlways("HM: IncrementCounter = %d", m_iReferenceCounter);
    }
    CRY_ASSERT(m_iReferenceCounter >= 0);

    if (1 == m_iReferenceCounter)
    {
        ShowHardwareMouse(true);
    }
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::DecrementCounter()
{
    m_iReferenceCounter--;

    if (m_debugHardwareMouse)
    {
        CryLogAlways("HM: DecrementCounter = %d", m_iReferenceCounter);
    }
    CRY_ASSERT(m_iReferenceCounter >= 0);

    if (0 == m_iReferenceCounter)
    {
        ShowHardwareMouse(false);
    }
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::GetHardwareMousePosition(float* pfX, float* pfY)
{
#ifdef WIN32
    POINT pointCursor;
    GetCursorPos(&pointCursor);
    *pfX = (float)pointCursor.x;
    *pfY = (float)pointCursor.y;
#else
    *pfX = m_fVirtualX;
    *pfY = m_fVirtualY;
#endif
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::SetHardwareMousePosition(float fX, float fY)
{
#ifdef WIN32
    HWND hWnd = (HWND) gEnv->pRenderer->GetCurrentContextHWND();
    if (hWnd == ::GetFocus() && m_allowConfine)
    {
        // Move cursor position only if our window is focused.
        SetCursorPos((int) fX, (int) fY);
    }
#else
    m_fVirtualX = fX;
    m_fVirtualY = fY;
    if (gEnv && gEnv->pRenderer)
    {
        float fWidth    = float(gEnv->pRenderer->GetWidth());
        float fHeight   = float(gEnv->pRenderer->GetHeight());

        if (m_fVirtualX < 0.0f)
        {
            m_fVirtualX = 0.0f;
        }
        else if (m_fVirtualX >= fWidth)
        {
            m_fVirtualX = fWidth - 1.0f;
        }

        if (m_fVirtualY < 0.0f)
        {
            m_fVirtualY = 0.0f;
        }
        else if (m_fVirtualY >= fHeight)
        {
            m_fVirtualY = fHeight - 1.0f;
        }
    }
#endif
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::GetHardwareMouseClientPosition(float* pfX, float* pfY)
{
#ifdef WIN32
    if (gEnv == NULL || gEnv->pRenderer == NULL)
    {
        return;
    }

    HWND hWnd = (HWND) gEnv->pRenderer->GetCurrentContextHWND();
    CRY_ASSERT_MESSAGE(hWnd, "Impossible to get client coordinates from a non existing window!");

    if (hWnd)
    {
        POINT pointCursor;
        GetCursorPos(&pointCursor);
        ScreenToClient(hWnd, &pointCursor);
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        const float normalizedX = (float)pointCursor.x / (float)(clientRect.right - clientRect.left);
        const float normalizedY = (float)pointCursor.y / (float)(clientRect.bottom - clientRect.top);
        *pfX = normalizedX * (float)gEnv->pRenderer->GetWidth();
        *pfY = normalizedY * (float)gEnv->pRenderer->GetHeight();
    }
    else
    {
        *pfX = 0.0f;
        *pfY = 0.0f;
    }
#else
    *pfX = m_fVirtualX;
    *pfY = m_fVirtualY;
#endif
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::SetHardwareMouseClientPosition(float fX, float fY)
{
#ifdef WIN32
    HWND hWnd = (HWND) gEnv->pRenderer->GetCurrentContextHWND();
    CRY_ASSERT_MESSAGE(hWnd, "Impossible to set position of the mouse relative to client coordinates from a non existing window!");

    if (hWnd)
    {
        POINT pointCursor;
        pointCursor.x = (int)fX;
        pointCursor.y = (int)fY;
        ClientToScreen(hWnd, &pointCursor);
        SetCursorPos(pointCursor.x, pointCursor.y);
    }
#else
    SetHardwareMousePosition(fX, fY);
#endif
}

//-----------------------------------------------------------------------------------------------------
bool CHardwareMouse::SetCursor(int idc_cursor_id)
{
#ifdef WIN32
    if (m_nCurIDCCursorId != idc_cursor_id || !m_curCursorPath.empty())
    {
        DestroyCursor();
    }

    if (m_useSystemCursor) // HW cursor
    {
        // Load cursor
        if (!m_hCursor)
        {
            m_hCursor = ::LoadCursor(GetModuleHandle(0), MAKEINTRESOURCE(idc_cursor_id));

            if (!m_hCursor)
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Unable to load cursor from windows resource id: %i", idc_cursor_id);
                return false;
            }
        }

        // Set cursor
        if (m_hCursor)
        {
            if (HCURSOR ret = ::SetCursor(m_hCursor))
            {
                m_nCurIDCCursorId = idc_cursor_id;
                m_curCursorPath.clear();
            }
        }
    }
    else // SW cursor
    {
        return SetCursor(g_default_cursor_path);
    }

    return m_hCursor ? true : false;
#else
    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Setting cursor via windows resource id is not supported on this platform. Using default cursor.");
    return SetCursor(g_default_cursor_path);
#endif
}

//-----------------------------------------------------------------------------------------------------
bool CHardwareMouse::SetCursor(const char* path)
{
#ifdef WIN32
    if (m_nCurIDCCursorId || m_curCursorPath.compare(path))
    {
        DestroyCursor();
    }
#else
    if (m_curCursorPath.compare(path))
    {
        DestroyCursor();
    }
#endif

#ifdef WIN32
    // Load cursor
    if (m_useSystemCursor) // HW cursor
    {
        if (!m_hCursor)
        {
            m_hCursor = CreateResourceFromTexture(gEnv->pRenderer, path, eResourceType_Cursor);
            if (!m_hCursor)
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Unable to load cursor from texture file. Path %s", path);
                return false;
            }

            //Example to load a cursor from file (.cur or .ani) (when not in pak)
            //m_hCursor = LoadCursorFromFile("<my_path>\\my_cursor.cur");
        }

        // Set cursor
        if (m_hCursor)
        {
            if (HCURSOR ret = ::SetCursor(m_hCursor))
            {
                m_nCurIDCCursorId = 0;
                m_curCursorPath.assign(path);
            }
        }
        return m_hCursor ? true : false;
    }
    else // SW cursor
#endif
    {
        if (gEnv->pRenderer && !m_pCursorTexture)
        {
            m_pCursorTexture = gEnv->pRenderer->EF_LoadTexture(path, FT_DONT_RELEASE | FT_DONT_STREAM);

            if (!m_pCursorTexture)
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Unable to open image for cursor from path: %s", path);
                return false;
            }

            m_pCursorTexture->SetClamp(true);
            m_curCursorPath.assign(path);
            return true;
        }
        return true;
    }
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::Reset(bool bVisibleByDefault)
{
    m_iReferenceCounter = bVisibleByDefault ? 1 : 0;
    //ShowHardwareMouse(bVisibleByDefault);
    GetHardwareMousePosition(&m_fCursorX, &m_fCursorY);

    if (!bVisibleByDefault)
    {
        ConfineCursor(true);
    }

    m_fIncX = 0.0f;
    m_fIncY = 0.0f;
    m_bFocus = true;
    m_recapture = false;
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::Update()
{
#ifdef WIN32
    // Note: Calling ShowCursor from a different thread does not work
    if (m_useSystemCursor != m_shouldUseSystemCursor)
    {
        ::ShowCursor(m_shouldUseSystemCursor);
        m_useSystemCursor = m_shouldUseSystemCursor;
    }
#endif
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::Render()
{
#ifdef WIN32
    if (m_useSystemCursor)
    {
        return;
    }
#endif

#if !defined(AZ_PLATFORM_APPLE_IOS) && !defined(AZ_PLATFORM_ANDROID)
    if (gEnv && gEnv->pRenderer && m_iReferenceCounter && m_pCursorTexture && !m_hide)
    {
        float fScalerX = gEnv->pRenderer->ScaleCoordX(1.f);
        float fScalerY = gEnv->pRenderer->ScaleCoordY(1.f);
        const float fSizeX = float(m_pCursorTexture->GetWidth());
        const float fSizeY = float(m_pCursorTexture->GetHeight());
        float fPosX, fPosY;
        GetHardwareMouseClientPosition(&fPosX, &fPosY);
        gEnv->pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
        gEnv->pRenderer->Draw2dImage(fPosX / fScalerX, fPosY / fScalerY, fSizeX / fScalerX, fSizeY / fScalerY, m_pCursorTexture->GetTextureID(), 0, 1, 1, 0, 0, 1, 1, 1, 1, 0);
    }
#endif // !defined(AZ_PLATFORM_APPLE_IOS) && !defined(AZ_PLATFORM_ANDROID)
}

//-----------------------------------------------------------------------------------------------------

bool CHardwareMouse::IsFullscreen()
{
    bool bFullScreen = false;
    if (gEnv && gEnv->pRenderer)
    {
        gEnv->pRenderer->EF_Query(EFQ_Fullscreen, bFullScreen);
    }
    return bFullScreen;
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::Hide(bool hide)
{
    m_hide = hide;
    if (m_calledShowHWMouse)
    {
        RefreshCursorVisibility((m_iReferenceCounter > 0 && !m_hide) || (m_allowConfine == false));
    }
}

//-----------------------------------------------------------------------------------------------------

bool CHardwareMouse::IsHidden()
{
#ifdef WIN32
    bool show = (m_iReferenceCounter > 0) && !m_hide;
    return !show;
#else
    return false;
#endif
}

//-----------------------------------------------------------------------------------------------------

#ifdef WIN32
void CHardwareMouse::UseSystemCursor(bool useSystemCursor)
{
    m_shouldUseSystemCursor = useSystemCursor;
}
#endif

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::RememberLastCursorPosition(bool remember)
{
    m_rememberLastPosition = remember;
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::DestroyCursor()
{
    if (m_pCursorTexture)
    {
        m_pCursorTexture->Release();
        m_pCursorTexture = 0;
    }
    m_curCursorPath.clear();

#if defined (WIN32)
    if (m_hCursor)
    {
        ::DestroyCursor(m_hCursor);
        m_hCursor = 0;
    }
    m_nCurIDCCursorId = 0;
#endif
}
#endif // defined(AZ_FRAMEWORK_INPUT_ENABLED)
