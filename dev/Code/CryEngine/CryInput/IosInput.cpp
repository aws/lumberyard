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

#ifdef USE_IOSINPUT

#include "IosInput.h"
#include "IosKeyboard.h"
#include "IosTouch.h"

#include <ILog.h>
#include <ISystem.h>

#include <UIKit/UIKit.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// CIosInput Functions
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
CIosInput::CIosInput(ISystem* pSystem)
    : CMobileInput(pSystem)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CIosInput::~CIosInput()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CIosInput::Init()
{
    gEnv->pLog->Log("Initializing IosInput");

    if (!CMobileInput::Init())
    {
        gEnv->pLog->Log("Error: CMobileInput::Init failed");
        return false;
    }

    if (!AddInputDevice(new CIosTouch(*this)))
    {
        // CBaseInput::AddInputDevice calls delete on it's argument if it fails.
        gEnv->pLog->Log("Error: Initializing iOS Touch");
        return false;
    }

    m_keyboardDevice = new CIosKeyboard(*this);
    if (!AddInputDevice(m_keyboardDevice))
    {
        // CBaseInput::AddInputDevice calls delete on it's argument if it fails.
        m_keyboardDevice = nullptr;
        gEnv->pLog->Log("Error: Initializing iOS Touch");
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CIosInput::ShutDown()
{
    CMobileInput::ShutDown();
    m_keyboardDevice = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CIosInput::Update(bool focus)
{
    // Pump the system event loop to ensure that it has dispatched all input events.
    // This is also being done in CDevice::Present (METALDevice.cpp) to present the
    // metal view, so some touch events will be sent then, but this is fine because
    // we're caching all touch events in m_filteredTouchEvents which gets cleared at
    // the end of the parent Update function regardless.
    SInt32 result;
    do
    {
        result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, TRUE);
    }
    while (result == kCFRunLoopRunHandledSource);

    // Call the parent update (which calls update on the individual input devices)
    // so the input events dispatched since last frame are all actually processed.
    CMobileInput::Update(focus);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CIosInput::StartTextInput(const Vec2& inputRectTopLeft, const Vec2& inputRectBottomRight)
{
    if (m_keyboardDevice)
    {
        m_keyboardDevice->StartTextInput(inputRectTopLeft, inputRectBottomRight);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CIosInput::StopTextInput()
{
    if (m_keyboardDevice)
    {
        m_keyboardDevice->StopTextInput();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CIosInput::IsScreenKeyboardShowing() const
{
    return m_keyboardDevice ? m_keyboardDevice->IsScreenKeyboardShowing() : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CIosInput::IsScreenKeyboardSupported() const
{
    return m_keyboardDevice ? m_keyboardDevice->IsScreenKeyboardSupported() : false;
}

#endif // USE_IOSINPUT
