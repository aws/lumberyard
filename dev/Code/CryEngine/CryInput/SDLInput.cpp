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
#include "StdAfx.h"

#if defined(USE_SDLINPUT)

#include "SDLInput.h"

#include <ILog.h>
#include <ISystem.h>

#include <SDL.h>

#include "SDLKeyboard.h"
#include "SDLMouse.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// CSDLInput Functions
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
CSDLInput::CSDLInput(ISystem* pSystem)
    : CBaseInput()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CSDLInput::~CSDLInput()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CSDLInput::Init()
{
    gEnv->pLog->Log("Initializing SDLInput");

    uint32 flags = SDL_INIT_EVENTS;
#if !defined(APPLETV)
    flags |= SDL_INIT_JOYSTICK;
#endif
#if defined(SDL_USE_HAPTIC_FEEDBACK)
    flags |= SDL_INIT_HAPTIC;
#endif

    if (SDL_InitSubSystem(flags) != 0)
    {
        gEnv->pLog->Log("Error: Initializing SDL Subsystems:%s", SDL_GetError());
        return false;
    }

    if (!CBaseInput::Init())
    {
        gEnv->pLog->Log("Error: CBaseInput::Init failed");
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSDLInput::ShutDown()
{
    gEnv->pLog->Log("SDLInput Shutdown");
    CBaseInput::ShutDown();

    uint32 flags = SDL_INIT_EVENTS;
#if !defined(APPLETV)
    flags |= SDL_INIT_JOYSTICK;
#endif
#if defined(SDL_USE_HAPTIC_FEEDBACK)
    flags |= SDL_INIT_HAPTIC;
#endif
    SDL_QuitSubSystem(flags);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSDLInput::Update(bool bFocus)
{
    SDL_PumpEvents();

    CBaseInput::Update(bFocus);

    SDL_Event eventList[32];
    const int nEvents = SDL_PeepEvents(eventList, 32, SDL_GETEVENT, SDL_QUIT, SDL_QUIT);
    if (nEvents == -1)
    {
        gEnv->pLog->LogError("SDL_GETEVENT error: %s", SDL_GetError());
        return;
    }

    for (int i = 0; i < nEvents; ++i)
    {
        if (eventList[i].type == SDL_QUIT)
        {
            gEnv->pSystem->Quit();
            return;
        }
        else
        {
            // Unexpected event type.
            abort();
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// CSDLInputDevice Functions
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
CSDLInputDevice::CSDLInputDevice(IInput& input, const char* deviceName)
    : CInputDevice(input, deviceName)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CSDLInputDevice::~CSDLInputDevice()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSDLInputDevice::PostEvent(SInputSymbol* pSymbol, unsigned keyMod)
{
    SInputEvent event;
    pSymbol->AssignTo(event, CSDLKeyboard::ConvertModifiers(keyMod));
    GetIInput().PostInputEvent(event);
}

#endif // defined(USE_SDLINPUT)
