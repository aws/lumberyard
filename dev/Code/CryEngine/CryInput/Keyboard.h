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

// Description : Keyboard for Windows/DirectX


#ifndef CRYINCLUDE_CRYINPUT_KEYBOARD_H
#define CRYINCLUDE_CRYINPUT_KEYBOARD_H
#pragma once

#ifdef USE_DXINPUT

#include "DXInputDevice.h"

class CDXInput;
struct  SInputSymbol;

class CKeyboard
    : public CDXInputDevice
{
    struct SScanCode
    {
        char        lc; // lowercase
        char        uc; // uppercase
        char        ac; // alt gr
        char        cl; // caps lock (differs slightly from uppercase)
    };
public:
    CKeyboard(CDXInput& input);

    // IInputDevice overrides
    virtual int GetDeviceIndex() const { return 0; }    //Assume only one keyboard
    virtual bool Init();
    virtual void Update(bool bFocus);
    virtual bool SetExclusiveMode(bool value);
    virtual void ClearKeyState();
    virtual char GetInputCharAscii(const SInputEvent& event);
    virtual const char* GetOSKeyName(const SInputEvent& event);
    virtual void OnLanguageChange();
    // ~IInputDevice

public:
    unsigned char Event2ASCII(const SInputEvent& event);
    unsigned char ToAscii(unsigned int vKeyCode, unsigned int k, unsigned char sKState[256]) const;
    wchar_t ToUnicode(unsigned int vKeyCode, unsigned int k, unsigned char sKState[256]) const;

protected:
    void    SetupKeyNames();
    void    ProcessKey(uint32 devSpecId, bool pressed);

private:
    static SScanCode            m_scanCodes[256];
    static SInputSymbol*    Symbol[256];
};

#endif // USE_DXINPUT

#endif // CRYINCLUDE_CRYINPUT_KEYBOARD_H
