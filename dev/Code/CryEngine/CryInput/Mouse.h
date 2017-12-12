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

// Description : Mouse for Windows/DirectX


#ifndef CRYINCLUDE_CRYINPUT_MOUSE_H
#define CRYINCLUDE_CRYINPUT_MOUSE_H
#pragma once

#ifdef USE_DXINPUT

#include "DXInputDevice.h"

struct  ITimer;
struct  ILog;
struct  ICVar;
class CDXInput;

class CMouse
    : public CDXInputDevice
{
public:
    CMouse(CDXInput& input);

    // IInputDevice overrides
    virtual int GetDeviceIndex() const { return 0; }    //Assume only one device of this type
    virtual bool Init();
    virtual void Update(bool bFocus);
    virtual bool SetExclusiveMode(bool value);
    // ~IInputDevice

private:
    void PostEvent(SInputSymbol* pSymbol);
    void PostOnlyIfChanged(SInputSymbol* pSymbol, EInputState newState);

    //smooth movement & mouse accel
    void CapDeltas(float cap);
    void SmoothDeltas(float accel, float decel = 0.0f);

    Vec2                        m_deltas;
    Vec2                        m_oldDeltas;
    Vec2                        m_deltasInertia;
    float                       m_mouseWheel;

    const static int MAX_MOUSE_SYMBOLS = eKI_MouseLast - KI_MOUSE_BASE;
    static SInputSymbol*    Symbol[MAX_MOUSE_SYMBOLS];
};

#endif //USE_DXINPUT

#endif // CRYINCLUDE_CRYINPUT_MOUSE_H
