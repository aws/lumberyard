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


#ifndef CRYINCLUDE_CRYINPUT_LINUXINPUT_H
#define CRYINCLUDE_CRYINPUT_LINUXINPUT_H
#pragma once

#ifdef USE_LINUXINPUT

#define USE_SDLINPUT

#include "SDLInput.h"

class CSDLMouse;
class CSDLPadManager;

#define SDL_USE_HAPTIC_FEEDBACK

class CLinuxInput
    : public CSDLInput
{
public:
    CLinuxInput(ISystem* pSystem);

    bool Init() override;
    void ShutDown() override;
    void Update(bool focus) override;
    bool GrabInput(bool bGrab) override;

private:
    CSDLMouse* m_pMouse;
    CSDLPadManager* m_pPadManager;
};

#endif // USE_LINUXINPUT

#endif // CRYINCLUDE_CRYINPUT_LINUXINPUT_H
