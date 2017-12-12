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

#ifndef CRYINCLUDE_CRYINPUT_SDLMOUSE_H
#define CRYINCLUDE_CRYINPUT_SDLMOUSE_H
#pragma once

#if defined(USE_SDLINPUT)

#include "SDLInput.h"

struct IRenderer;
class CSDLMouse
    : public CSDLInputDevice
{
public:
    CSDLMouse(IInput& input);

    virtual ~CSDLMouse();

    virtual int GetDeviceIndex() const { return 0; }    //Assume only one mouse

    virtual bool Init();

    virtual void Update(bool focus);

    void GrabInput();

    void UngrabInput();

protected:

    void CapDeltas(float cap);

    void SmoothDeltas(float accel, float decel = 0.0f);

private:
    IRenderer* m_pRenderer;
    Vec2 m_deltas;
    Vec2 m_oldDeltas;
    Vec2 m_deltasInertia;
    bool m_bGrabInput;
};

#endif // defined(USE_SDLINPUT)

#endif // CRYINCLUDE_CRYINPUT_SDLMOUSE_H
