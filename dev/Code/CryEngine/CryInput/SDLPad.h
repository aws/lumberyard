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

// Description : GamePad Input implementation for Linux using SDL


#ifndef CRYINCLUDE_CRYINPUT_SDLPAD_H
#define CRYINCLUDE_CRYINPUT_SDLPAD_H
#pragma once


#if defined(USE_SDLINPUT)
#include <SDL.h>
#include "SDLInput.h"
// We need a manager, since all the input for each Game Pad is collected
// in the same queue. If we were to update the game pads separately
// they would consume each other's events.




class CSDLPad
    : public CSDLInputDevice
{
public:

    CSDLPad(IInput& input, int device);

    virtual ~CSDLPad();

    // IInputDevice overrides
    virtual int GetDeviceIndex() const { return m_deviceNo; }
    virtual bool Init();
    virtual void Update(bool bFocus);
    virtual void ClearAnalogKeyState(TInputSymbols& clearedSymbols);
    virtual void ClearKeyState();
    virtual bool    SetForceFeedback(IFFParams params);
    // ~IInputDevice

    int GetInstanceId() const;

    void HandleAxisEvent(const SDL_JoyAxisEvent& evt);

    void HandleHatEvent(const SDL_JoyHatEvent& evt);

    void HandleButtonEvent(const SDL_JoyButtonEvent& evt);

    void HandleConnectionState(const bool connected);
private:
    static float DeadZoneFilter(int input);

    bool OpenDevice();

    void CloseDevice();

private:
    SDL_Joystick*               m_pSDLDevice;
    SDL_Haptic*                 m_pHapticDevice;
    int                         m_curHapticEffect;
    int                         m_deviceNo;
    int                         m_handle;
    bool                        m_connected;
    bool                        m_supportsFeedback;
    float                       m_vibrateTime;
};

class CSDLPadManager
{
public:

    CSDLPadManager(IInput& input);

    ~CSDLPadManager();

    bool Init();

    void Update(bool bFocus);

private:
    bool AddGamePad(int deviceIndex);

    bool RemovGamePad(int instanceId);

    CSDLPad* FindPadByInstanceId(int instanceId);
    CSDLPad* FindPadByDeviceIndex(int deviceIndex);
private:

    typedef std::vector<CSDLPad*> GamePadVector;

    IInput& m_input;
    GamePadVector m_gamePads;
};


#endif // USE_LINUX_INPUT
#endif // CRYINCLUDE_CRYINPUT_SDLPAD_H
