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

#ifndef CRYINCLUDE_CRYSYSTEM_NULLIMPLEMENTATION_NULLINPUT_H
#define CRYINCLUDE_CRYSYSTEM_NULLIMPLEMENTATION_NULLINPUT_H
#pragma once

#include <IInput.h>

class CNullInput
    : public IInput
{
public:
    virtual void AddEventListener(IInputEventListener* pListener) {}
    virtual void RemoveEventListener(IInputEventListener* pListener) {}

    virtual void AddConsoleEventListener(IInputEventListener* pListener) {}
    virtual void RemoveConsoleEventListener(IInputEventListener* pListener) {}

    virtual bool IsMotionSensorDataAvailable(EMotionSensorFlags sensorFlags) const { return false; }
    virtual bool AddMotionSensorEventListener(IMotionSensorEventListener* pListener, EMotionSensorFlags sensorFlags = eMSF_AllAvailable) { return false; }
    virtual void RemoveMotionSensorEventListener(IMotionSensorEventListener* pListener, EMotionSensorFlags sensorFlags = eMSF_AllAvailable) {}

    virtual void SetExclusiveListener(IInputEventListener* pListener) {}
    virtual IInputEventListener* GetExclusiveListener()  { return NULL; }

    virtual void ActivateMotionSensors(EMotionSensorFlags sensorFlags = eMSF_AllAvailable) {}
    virtual void DeactivateMotionSensors(EMotionSensorFlags sensorFlags = eMSF_AllAvailable) {}

    virtual void PauseMotionSensorInput() {}
    virtual void UnpauseMotionSensorInput() {}

    virtual void SetMotionSensorUpdateInterval(float seconds, bool force = false) {}
    virtual void SetMotionSensorFilter(IMotionSensorFilter* filter) {}
    virtual const SMotionSensorData* GetMostRecentMotionSensorData() const { return nullptr; }

    virtual bool AddInputDevice(IInputDevice* pDevice) { return false; }
    virtual bool AddAzToLyInputDevice(EInputDeviceType, const char*, const std::vector<SInputSymbol>&, const AzFramework::InputDeviceId&) { return false; }

    virtual void EnableEventPosting (bool bEnable) {}
    virtual bool IsEventPostingEnabled() const { return false; }
    virtual void PostInputEvent(const SInputEvent& event, bool bForce = false) {}
    virtual void PostMotionSensorEvent(const SMotionSensorEvent& event, bool bForce = false) {}
    virtual void PostUnicodeEvent(const SUnicodeEvent& event, bool bForce = false) {}

    virtual void ForceFeedbackEvent(const SFFOutputEvent& event) {}
    virtual void ForceFeedbackSetDeviceIndex(int index) {};

    virtual bool    Init() { return true; }
    virtual void  PostInit() {}
    virtual void    Update(bool bFocus) {}
    virtual void    ShutDown() {}

    virtual void    SetExclusiveMode(EInputDeviceType deviceType, bool exclusive, void* hwnd = 0) {}

    virtual bool    InputState(const TKeyName& key, EInputState state) { return false; }

    virtual const char* GetKeyName(const SInputEvent& event) const { return NULL; }

    virtual const char* GetKeyName(EKeyId keyId) const { return NULL; }

    virtual char GetInputCharAscii(const SInputEvent& event) { return 0; };

    virtual SInputSymbol* LookupSymbol(EInputDeviceType deviceType, int deviceIndex, EKeyId keyId)  { return NULL; }

    virtual const SInputSymbol* GetSymbolByName(const char* name) const { return NULL; }

    virtual const char* GetOSKeyName(const SInputEvent& event) { return NULL; }

    virtual void ClearKeyState() {}

    virtual void ClearAnalogKeyState() {}

    virtual void RetriggerKeyState() {}

    virtual bool Retriggering() { return false; }

    virtual bool HasInputDeviceOfType(EInputDeviceType type) { return false; }

    virtual int GetModifiers() const { return 0; }

    virtual void EnableDevice(EInputDeviceType deviceType, bool enable) {}

    virtual void SetDeadZone(float fThreshold) {};

    virtual void RestoreDefaultDeadZone() {};

    virtual IInputDevice* GetDevice(uint16 id, EInputDeviceType deviceType) { return NULL; };

    virtual uint32 GetPlatformFlags() const { return 0; }

    virtual INaturalPointInput* GetNaturalPointInput() {return NULL; }
    // Input blocking functionality
    virtual bool SetBlockingInput(const SInputBlockData& inputBlockData) { return false; }
    virtual bool RemoveBlockingInput(const SInputBlockData& inputBlockData) { return false; }
    virtual bool HasBlockingInput(const SInputBlockData& inputBlockData) const { return false; }
    virtual int GetNumBlockingInputs() const { return 0; }
    virtual void ClearBlockingInputs() {}
    virtual bool ShouldBlockInputEventPosting(const EKeyId keyId,
        const EInputDeviceType  deviceType,
        const uint8 deviceIndex) const { return false; }

    virtual void ProcessKey(uint32 key, bool pressed, WCHAR unicode, bool repeat) {};
    virtual bool GrabInput(bool bGrab) { return false; }

    virtual void StartTextInput(const Vec2& inputRectTopLeft = ZERO, const Vec2& inputRectBottomRight = ZERO) {}
    virtual void StopTextInput() {}
    virtual bool IsScreenKeyboardShowing() const { return false; }
    virtual bool IsScreenKeyboardSupported() const { return false; }
    virtual const TInputDevices& GetInputDevices() const { static TInputDevices s_emptyDevices; return s_emptyDevices; };
};

#endif // CRYINCLUDE_CRYSYSTEM_NULLIMPLEMENTATION_NULLINPUT_H
