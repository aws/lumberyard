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

#ifdef USE_ANDROIDINPUT

#include "AndroidInput.h"
#include "AndroidKeyboard.h"

#include "AndroidTouch.h"

#include <ILog.h>
#include <ISystem.h>

#include <SDL.h>

// ----
// CAndroidInput (public)
// ----

////////////////////////////////////////////////////////////////////////////////////////////////////
CAndroidInput::CAndroidInput(ISystem* pSystem)
    : CMobileInput(pSystem)
    , m_keyboardDevice(nullptr)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CAndroidInput::~CAndroidInput()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAndroidInput::Init()
{
    gEnv->pLog->Log("Initializing CAndroidInput");

    if (SDL_InitSubSystem(SDL_INIT_EVENTS) != 0)
    {
        gEnv->pLog->Log("Error: Initializing SDL Subsystems:%s", SDL_GetError());
        return false;
    }

    if (!CMobileInput::Init())
    {
        gEnv->pLog->Log("Error: CAndroidInput::Init failed");
        return false;
    }

    if (!AddInputDevice(new CAndroidTouch(*this)))
    {
        // CBaseInput::AddInputDevice calls delete on it's argument if it fails.
        gEnv->pLog->Log("Error: Initializing Android Touch");
        return false;
    }

    m_keyboardDevice = new CAndroidKeyboard(*this);
    if (!AddInputDevice(m_keyboardDevice))
    {
        // CBaseInput::AddInputDevice calls delete on it's argument if it fails.
        m_keyboardDevice = nullptr;
        gEnv->pLog->Log("Error: Initializing Android Keyboard");
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CAndroidInput::ShutDown()
{
    CMobileInput::ShutDown();

    // calling CMobileInput::ShutDown() will delete the devices
    m_keyboardDevice = nullptr;

    SDL_QuitSubSystem(SDL_INIT_EVENTS);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CAndroidInput::Update(bool bFocus)
{
    SDL_PumpEvents();

    CMobileInput::Update(bFocus);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CAndroidInput::StartTextInput(const Vec2& inputRectTopLeft, const Vec2& inputRectBottomRight)
{
    if (m_keyboardDevice)
    {
        m_keyboardDevice->StartTextInput(inputRectTopLeft, inputRectBottomRight);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CAndroidInput::StopTextInput()
{
    if (m_keyboardDevice)
    {
        m_keyboardDevice->StopTextInput();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAndroidInput::IsScreenKeyboardShowing() const
{
    return (m_keyboardDevice ? m_keyboardDevice->IsScreenKeyboardShowing() : false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAndroidInput::IsScreenKeyboardSupported() const
{
    return (m_keyboardDevice ? m_keyboardDevice->IsScreenKeyboardSupported() : false);
}

#endif // USE_MOBILEINPUT
