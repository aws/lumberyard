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

#ifndef CRYINCLUDE_CRYINPUT_XINPUTDEVICE_H
#define CRYINCLUDE_CRYINPUT_XINPUTDEVICE_H
#pragma once

#include "InputDevice.h"

#if defined(USE_DXINPUT)

#if defined(USE_DXINPUT)
#include <xinput.h>
#endif

struct ICVar;

/*
Device: 'XBOX 360 For Windows (Controller)' - 'XBOX 360 For Windows (Controller)'
Product GUID:  {028E045E-0000-0000-0000-504944564944}
Instance GUID: {65429170-3725-11DA-8001-444553540000}
*/

class CXInputDevice
    : public CInputDevice
{
public:
    CXInputDevice(IInput& pInput, int deviceNo);
    virtual ~CXInputDevice();

    // IInputDevice overrides
    const char* GetDeviceCommonName() const override { return "xbox 360 controller"; }
    virtual int GetDeviceIndex() const { return m_deviceNo; }
    virtual bool Init();
    virtual void PostInit();
    virtual void Update(bool bFocus);
    virtual void ClearKeyState();
    virtual void ClearAnalogKeyState(TInputSymbols& clearedSymbols);
    virtual bool SetForceFeedback(IFFParams params);
    virtual bool IsOfDeviceType(EInputDeviceType type) const { return type == eIDT_Gamepad && m_connected; }
    virtual void SetDeadZone(float fThreshold);
    virtual void RestoreDefaultDeadZone();
    // ~IInputDevice overrides

private:
    void UpdateConnectedState(bool isConnected);
    //void AddInputItem(const IInput::InputItem& item);

    //triggers the speed of the vibration motors -> leftMotor is for low frequencies, the right one is for high frequencies
    bool SetVibration(USHORT leftMotor = 0, USHORT rightMotor = 0, float timing = 0, EFFEffectId effectId = eFF_Rumble_Basic);
    void ProcessAnalogStick(SInputSymbol* pSymbol, SHORT prev, SHORT current, SHORT threshold);

    ILINE float GetClampedLeftMotorAccumulatedVibration() const { return clamp_tpl(m_basicLeftMotorRumble + m_frameLeftMotorRumble, 0.0f, 65535.0f); }
    ILINE float GetClampedRightMotorAccumulatedVibration() const { return clamp_tpl(m_basicRightMotorRumble + m_frameRightMotorRumble, 0.0f, 65535.0f); }

    int                         m_deviceNo; //!< device number (from 0 to 3) for this XInput device
    bool                        m_connected;
    bool                        m_forceResendSticks;
    XINPUT_STATE                m_state;
    XINPUT_VIBRATION            m_currentVibrationSettings;
    float                       m_basicLeftMotorRumble, m_basicRightMotorRumble;
    float                       m_frameLeftMotorRumble, m_frameRightMotorRumble;
    float                       m_fVibrationTimer;
    float                       m_fDeadZone;
    //std::vector<IInput::InputItem>    mInputQueue;    //!< queued inputs

public:
    static GUID     ProductGUID;
};

#endif //defined(USE_DXINPUT)

#endif // CRYINCLUDE_CRYINPUT_XINPUTDEVICE_H
