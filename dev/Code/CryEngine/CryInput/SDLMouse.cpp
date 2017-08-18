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

// Description : Mouse Input implementation for Linux using SDL


#include "StdAfx.h"

#if defined(USE_SDLINPUT)

#include "SDLMouse.h"
#include <IRenderer.h>
#include <IHardwareMouse.h>

#include <SDL.h>

#define MOUSE_SYM_BASE (1024)
#define MOUSE_SYM(X) (MOUSE_SYM_BASE + (X))
#define MOUSE_AXIS_X (MOUSE_SYM(10))
#define MOUSE_AXIS_Y (MOUSE_SYM(11))
#define MOUSE_AXIS_Z (MOUSE_SYM(12))
// We need to define custom macros for mouse wheel symbols
// since in SDL2 the mouse wheel event is handles as a single
// occurance
#define MOUSE_WHEEL_UP (MOUSE_SYM(13))
#define MOUSE_WHEEL_DOWN (MOUSE_SYM(14))
#define MOUSE_MAX_PEEP 64

// Define to enable automatic input grabbing support.  We don't want this
// during development.  If automatic input grabbing support is off, then input
// grabbing can be enforced by typing CTRL+ALT+G.

#if defined(USE_LINUXINPUT)
#define LINUXINPUT_AUTOGRAB 1
#else
#undef LINUXINPUT_AUTOGRAB
#endif

CSDLMouse::CSDLMouse(IInput& input)
    : CSDLInputDevice(input, "SDL Mouse")
{
    m_deviceType = eIDT_Mouse;
    m_pRenderer = gEnv->pRenderer;
    m_bGrabInput = false;
}

CSDLMouse::~CSDLMouse()
{
}

bool CSDLMouse::Init()
{
    m_deltas.zero();
    m_oldDeltas.zero();
    m_deltasInertia.zero();

    MapSymbol(MOUSE_SYM(SDL_BUTTON_LEFT), eKI_Mouse1, "mouse1");
    MapSymbol(MOUSE_SYM(SDL_BUTTON_MIDDLE), eKI_Mouse3, "mouse3");
    MapSymbol(MOUSE_SYM(SDL_BUTTON_RIGHT), eKI_Mouse2, "mouse2");
    MapSymbol(MOUSE_WHEEL_UP, eKI_MouseWheelUp, "mwheel_up");
    MapSymbol(MOUSE_WHEEL_DOWN, eKI_MouseWheelDown, "mwheel_down");
    MapSymbol(MOUSE_AXIS_X, eKI_MouseX, "maxis_x", SInputSymbol::RawAxis);
    MapSymbol(MOUSE_AXIS_Y, eKI_MouseY, "maxis_y", SInputSymbol::RawAxis);
    MapSymbol(MOUSE_AXIS_Z, eKI_MouseZ, "maxis_z", SInputSymbol::RawAxis);

    SDL_ShowCursor(SDL_DISABLE);

#if defined(LINUXINPUT_AUTOGRAB)
    GrabInput();
#endif
    return true;
}

void CSDLMouse::Update(bool focus)
{
    SDL_Event eventList[MOUSE_MAX_PEEP];

    int nEvents;
    unsigned type = 0;

    m_deltas.zero();
    float mouseWheel = 0.0f;

    SInputSymbol* pSymbol = NULL;

    // Init the mousePosition to the last known in case we don't process any events below.
    Vec2 mousePosition(ZERO);
    gEnv->pHardwareMouse->GetHardwareMouseClientPosition(&(mousePosition.x), &(mousePosition.y));

    // Assuming that SDL_PumpEvents is called from CSDLInput

    nEvents = SDL_PeepEvents(eventList, MOUSE_MAX_PEEP, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEWHEEL);

    if (nEvents == -1)
    {
        gEnv->pLog->LogError("SDL_GETEVENT error: %s", SDL_GetError());
        return;
    }
    for (int i = 0; i < nEvents; ++i)
    {
        type = eventList[i].type;
        if (type == SDL_MOUSEBUTTONDOWN || type == SDL_MOUSEBUTTONUP)
        {
            SDL_MouseButtonEvent* const buttonEvent = &eventList[i].button;
            pSymbol = DevSpecIdToSymbol((uint32)MOUSE_SYM(buttonEvent->button));
            if (pSymbol == NULL)
            {
                continue;
            }
            pSymbol->screenPosition.x = buttonEvent->x;
            pSymbol->screenPosition.y = buttonEvent->y;
            if (type == SDL_MOUSEBUTTONDOWN)
            {
                pSymbol->state = eIS_Pressed;
                pSymbol->value = 1.f;
                CSDLInputDevice::PostEvent(pSymbol);
            }
            else
            {
                pSymbol->state = eIS_Released;
                pSymbol->value = 0.f;
                CSDLInputDevice::PostEvent(pSymbol);
            }
        }
        else if (type == SDL_MOUSEMOTION)
        {
            SDL_MouseMotionEvent* const motionEvent = &eventList[i].motion;
            if (motionEvent->xrel != 0)
            {
                m_deltas.x = (float)motionEvent->xrel;
            }
            if (motionEvent->yrel != 0)
            {
                m_deltas.y = (float)motionEvent->yrel;
            }
            mousePosition.x = motionEvent->x;
            mousePosition.y = motionEvent->y;
        }
        else if (type == SDL_MOUSEWHEEL)
        {
            SDL_MouseWheelEvent* pWheelEvent = &eventList[i].wheel;

            if (pWheelEvent->y > 0)
            {
                pSymbol = DevSpecIdToSymbol(MOUSE_WHEEL_UP);
                assert(pSymbol);
                pSymbol->state = eIS_Changed;
                mouseWheel = (float)pWheelEvent->y;
                pSymbol->value = mouseWheel;
                pSymbol->screenPosition.x = pWheelEvent->x;
                pSymbol->screenPosition.y = pWheelEvent->y;
                CSDLInputDevice::PostEvent(pSymbol);
            }
            else if (pWheelEvent->y < 0)
            {
                pSymbol = DevSpecIdToSymbol(MOUSE_WHEEL_DOWN);
                assert(pSymbol);
                pSymbol->state = eIS_Changed;
                mouseWheel = (float)pWheelEvent->y * -1.0f;
                pSymbol->value = mouseWheel;
                pSymbol->screenPosition.x = pWheelEvent->x;
                pSymbol->screenPosition.y = pWheelEvent->y;
                CSDLInputDevice::PostEvent(pSymbol);
            }
        }
    }

    m_deltas *= g_pInputCVars->i_mouse_sensitivity;
    float mouseaccel = g_pInputCVars->i_mouse_accel;
    if (mouseaccel > 0.0f)
    {
        m_deltas.x = m_deltas.x * (float)fabs(m_deltas.x * mouseaccel);
        m_deltas.y = m_deltas.y * (float)fabs(m_deltas.y * mouseaccel);

        CapDeltas(g_pInputCVars->i_mouse_accel_max);
    }

    SmoothDeltas(g_pInputCVars->i_mouse_smooth);

    const bool hasDeltaChanged = !(fcmp(m_deltas.x, m_oldDeltas.x) && fcmp(m_deltas.y, m_oldDeltas.y));
    m_oldDeltas = m_deltas; //this needs to happen always. We want to keep the attribute always valid

    // mouse movements
    if (m_deltas.GetLength2() > 0.0f || hasDeltaChanged || mouseWheel)
    {
        pSymbol = DevSpecIdToSymbol(MOUSE_AXIS_X);
        assert(pSymbol);
        pSymbol->state = eIS_Changed;
        pSymbol->value = m_deltas.x;
        pSymbol->screenPosition = mousePosition;
        CSDLInputDevice::PostEvent(pSymbol);

        pSymbol = DevSpecIdToSymbol(MOUSE_AXIS_Y);
        assert(pSymbol);
        pSymbol->state = eIS_Changed;
        pSymbol->value = m_deltas.y;
        pSymbol->screenPosition = mousePosition;
        CSDLInputDevice::PostEvent(pSymbol);

        pSymbol = DevSpecIdToSymbol(MOUSE_AXIS_Z);
        assert(pSymbol);
        pSymbol->state = eIS_Changed;
        pSymbol->value = mouseWheel;
        pSymbol->screenPosition = mousePosition;
        CSDLInputDevice::PostEvent(pSymbol);
    }

    gEnv->pHardwareMouse->SetHardwareMousePosition(mousePosition.x, mousePosition.y);

    // Send one mouse event each frame to report it's last known position.
    SInputEvent event;
    event.deviceType = eIDT_Mouse;
    event.keyId = eKI_MousePosition;
    event.keyName = "mouse_pos";
    event.screenPosition = mousePosition;
    GetIInput().PostInputEvent(event);

    float inertia = g_pInputCVars->i_mouse_inertia;
    if (inertia > 0.0f)
    {
        float dt = gEnv->pTimer->GetFrameTime();
        if (dt > 0.1f)
        {
            dt = 0.1f;
        }
        m_deltas = (m_deltasInertia += (m_deltas - m_deltasInertia) * inertia * dt);
    }
}


void CSDLMouse::GrabInput()
{
    if (m_bGrabInput) // SDL_TRUE == SDL_GetWindowGrab(static_cast<SDL_Window*>(gEnv->pRenderer->GetCurrentContextHWND())))
    {
        return;
    }
    SDL_Window* pWindow = *static_cast<SDL_Window**>(m_pRenderer->GetCurrentContextHWND());
    if (!pWindow)
    {
        return;
    }
	
	//SDL window is invalid for OSX as its all driven through OpenGl or Metal
#if !defined(AZ_PLATFORM_APPLE_OSX)
    SDL_SetWindowGrab(pWindow, SDL_TRUE);
#endif

    if (SDL_SetRelativeMouseMode(SDL_TRUE) != 0)
    {
        CryLogAlways("SDL: Could not set relative mouse mode: %s", SDL_GetError());
    }

    m_bGrabInput = true;
}

void  CSDLMouse::UngrabInput()
{
    if (!m_bGrabInput)
    {
        return;
    }
    m_bGrabInput = false;

    SDL_SetWindowGrab(static_cast<SDL_Window*>(m_pRenderer->GetCurrentContextHWND()), SDL_FALSE);
    if (SDL_SetRelativeMouseMode(SDL_FALSE) != 0)
    {
        CryLogAlways("SDL: Could not unset relative mouse mode: %s", SDL_GetError());
    }
}

void CSDLMouse::CapDeltas(float cap)
{
    float temp;

    temp = fabs(m_deltas.x) / cap;
    if (temp > 1.0f)
    {
        m_deltas.x /= temp;
    }

    temp = fabs(m_deltas.y) / cap;
    if (temp > 1.0f)
    {
        m_deltas.y /= temp;
    }
}

void CSDLMouse::SmoothDeltas(float accel, float decel)
{
    if (accel < 0.0001f)
    {
        //do nothing ,just like it was before.
        return;
    }
    else if (accel < 0.9999f)//mouse smooth, average the old and the actual delta by the delta ammount, less delta = more smooth speed.
    {
        Vec2 delta = m_deltas - m_oldDeltas;

        float len = delta.GetLength();

        float amt = 1.0f - (min(10.0f, len) / 10.0f * min(accel, 0.9f));

        m_deltas = m_oldDeltas + delta * amt;
    }
    else if (accel < 1.0001f)//mouse smooth, just average the old and the actual delta.
    {
        m_deltas = (m_deltas + m_oldDeltas) * 0.5f;
    }
    else//mouse smooth with acceleration
    {
        float dt = min(gEnv->pTimer->GetFrameTime(), 0.1f);

        Vec2 delta;

        float amt = 0.0;

        //if the input want to stop use twice of the acceleration.
        if (m_deltas.GetLength2() < 0.0001f)
        {
            if (decel > 0.0001f)//there is a custom deceleration value? use it.
            {
                amt = min(1.0f, dt * decel);
            }
            else//otherwise acceleration * 2 is the default.
            {
                amt = min(1.0f, dt * accel * 2.0f);
            }
        }
        else
        {
            amt = min(1.0f, dt * accel);
        }

        delta = m_deltas - m_oldDeltas;
        m_deltas = m_oldDeltas + delta * amt;
    }

    m_oldDeltas = m_deltas;
}
#endif
