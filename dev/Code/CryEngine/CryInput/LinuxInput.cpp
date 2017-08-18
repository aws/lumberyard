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

// Description : Input implementation for Linux using SDL

#include "StdAfx.h"

#ifdef USE_LINUXINPUT

#include "LinuxInput.h"

#include <math.h>

#include <IConsole.h>
#include <ILog.h>
#include <ISystem.h>
#include <IRenderer.h>

#include "SDLKeyboard.h"
#include "SDLMouse.h"
#include "SDLPad.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// CLinuxInput Functions
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
CLinuxInput::CLinuxInput(ISystem* pSystem)
    : CSDLInput(pSystem)
    , m_pMouse(nullptr)
    , m_pPadManager(nullptr)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CLinuxInput::Init()
{
    gEnv->pLog->Log("Initializing LinuxInput");

#ifndef KDAB_MAC_PORT
    return true;
#endif

    if (!CSDLInput::Init())
    {
        gEnv->pLog->Log("Error: CSDLInput::Init failed");
        return false;
    }

    CSDLMouse* pMouse = new CSDLMouse(*this);
    if (AddInputDevice(pMouse))
    {
        m_pMouse = pMouse;
    }
    else
    {
        // CBaseInput::AddInputDevice calls delete on it's argument if it fails.
        gEnv->pLog->Log("Error: Initializing SDL Mouse");
        return false;
    }

    if (!AddInputDevice(new CSDLKeyboard(*this)))
    {
        // CBaseInput::AddInputDevice calls delete on it's argument if it fails.
        gEnv->pLog->Log("Error: Initializing SDL Keyboard");
        return false;
    }

    m_pPadManager = new CSDLPadManager(*this);
    if (!m_pPadManager->Init())
    {
        delete m_pPadManager;
        m_pPadManager = nullptr;
        gEnv->pLog->Log("Error: Initializing SDL GamePad Manager");
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLinuxInput::ShutDown()
{
    gEnv->pLog->Log("LinuxInput Shutdown");
    if (m_pPadManager)
    {
        delete m_pPadManager;
    }
    CSDLInput::ShutDown();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLinuxInput::Update(bool bFocus)
{
    CSDLInput::Update(bFocus);
    m_pPadManager->Update(bFocus);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CLinuxInput::GrabInput(bool bGrab)
{
    if (!m_pMouse)
    {
        return false;
    }

    if (bGrab)
    {
        m_pMouse->GrabInput();
    }
    else
    {
        m_pMouse->UngrabInput();
    }
    return true;
}

#endif // USE_LINUXINPUT
