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

// Description : Keyboard Input implementation for Linux using SDL


#ifndef CRYINCLUDE_CRYINPUT_SDLKEYBOARD_H
#define CRYINCLUDE_CRYINPUT_SDLKEYBOARD_H
#pragma once

#if defined(USE_SDLINPUT)

#include "SDLInput.h"

class CSDLMouse;

class CSDLKeyboard
    : public CSDLInputDevice
{
    friend class CSDLInputDevice;
public:

    CSDLKeyboard(IInput& input);

    virtual ~CSDLKeyboard();

    virtual int GetDeviceIndex() const { return 0; }    //Assume only one keyboard

    virtual bool Init();

    virtual void Update(bool focus);

    virtual char GetInputCharAscii(const SInputEvent& event);


protected:
    static int ConvertModifiers(unsigned);

private:
    unsigned char Event2ASCII(const SInputEvent& event);
    void SetupKeyNames();


private:
    unsigned m_lastKeySym;
    int m_lastMod;
    //unsigned m_lastUNICODE;
};

#endif // defined(USE_SDLINPUT)

#endif // CRYINCLUDE_CRYINPUT_SDLKEYBOARD_H
