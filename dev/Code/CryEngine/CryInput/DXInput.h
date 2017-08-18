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

// Description : CDXInput is now an IInput implementation which is DirectInput
//               specific  This removes all these ugly  ifdefs from this code
//               which is a good thing


#ifndef CRYINCLUDE_CRYINPUT_DXINPUT_H
#define CRYINCLUDE_CRYINPUT_DXINPUT_H
#pragma once

#include "BaseInput.h"
#include <IWindowMessageHandler.h>
#include <map>
#include <queue>

#ifdef USE_DXINPUT

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

struct  ILog;
struct  ISystem;

class CDXInput
    : public CBaseInput
    , public IWindowMessageHandler
{
public:
    CDXInput(ISystem* pSystem, HWND hwnd);
    virtual ~CDXInput();

    // IInput overrides
    virtual bool    Init();
    virtual void    Update(bool bFocus);
    virtual void    ShutDown();
    virtual void    ClearKeyState();
    virtual void SetExclusiveMode(EInputDeviceType deviceType, bool exclusive, void* pUser);
    // ~IInput

    HWND                        GetHWnd() const {   return m_hwnd;  }
    LPDIRECTINPUT8  GetDirectInput() const  {   return m_pDI;   }

private:
    // IWindowMessageHandler
    bool HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);

    // very platform specific params
    HWND                            m_hwnd;
    LPDIRECTINPUT8      m_pDI;
    uint16 m_highSurrogate;
    static CDXInput*    This;
};

#endif //USE_DXINPUT

#endif // CRYINCLUDE_CRYINPUT_DXINPUT_H
