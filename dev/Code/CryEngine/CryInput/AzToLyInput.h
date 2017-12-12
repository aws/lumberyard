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
#pragma once

#include "BaseInput.h"

#include <AzFramework/Input/Buses/Notifications/InputTextEventNotificationBus.h>

#include <unordered_map>

////////////////////////////////////////////////////////////////////////////////////////////////////
// This class (along with other CryInput/AzToLyInput* classes) is just a temporary bridge that we must
// use to translate AzFramework Input events back to CryInput events, until such time as we are able
// to deprecate and then completely remove CryInput. This is because on almost all platforms it is
// not possible for two different systems to process the raw input obtained from the various system
// APIs, so although we need to maintain the CryInput interface for a time we must make it depend
// on AzFramework Input in the interim.
class AzToLyInput : public CBaseInput, public AzFramework::InputTextEventNotificationBus::Handler
{
public:
    AzToLyInput();
    ~AzToLyInput() override;

protected:
    // CBaseInput
    bool Init() override;
    void ShutDown() override;
    void ClearKeyState() override;
    void PostHoldEvents() override;
    int GetModifiers() const override;
    bool AddAzToLyInputDevice(EInputDeviceType cryInputDeviceType,
                              const char* cryInputDeviceDisplayName,
                              const std::vector<SInputSymbol>& azToCryInputSymbols,
                              const AzFramework::InputDeviceId& azFrameworkInputDeviceId) override;

    // Update Related
    void Update(bool bFocus) override;
    void UpdateKeyboardModifiers();
    void UpdateMotionSensorInput();

    // Motion Sensor Related
    bool IsMotionSensorDataAvailable(EMotionSensorFlags sensorFlags) const override;
    bool AddMotionSensorEventListener(IMotionSensorEventListener* pListener,
        EMotionSensorFlags sensorFlags = eMSF_AllAvailable) override;
    void RemoveMotionSensorEventListener(IMotionSensorEventListener* pListener,
        EMotionSensorFlags sensorFlags = eMSF_AllAvailable) override;
    void PostMotionSensorEvent(const SMotionSensorEvent& event, bool bForce = false) override;
    void ActivateMotionSensors(EMotionSensorFlags sensorFlags = eMSF_AllAvailable) override;
    void DeactivateMotionSensors(EMotionSensorFlags sensorFlags = eMSF_AllAvailable) override;
    void PauseMotionSensorInput() override;
    void UnpauseMotionSensorInput() override;
    void RefreshCurrentlyActivatedMotionSensors();
    void SetMotionSensorFilter(IMotionSensorFilter* filter) override;
    const SMotionSensorData* GetMostRecentMotionSensorData() const override;

    // Text Input Related
    void StartTextInput(const Vec2& topLeft = ZERO, const Vec2& bottomRight = ZERO) override;
    void StopTextInput() override;
    bool IsScreenKeyboardShowing() const override;
    bool IsScreenKeyboardSupported() const override;

    // InputTextEventNotificationBus::Handler
    void OnInputTextEvent(const AZStd::string& textUTF8, bool& o_hasBeenConsumed) override;

private:
    std::unordered_map<IMotionSensorEventListener*, int> m_motionSensorListeners;
    SMotionSensorData m_mostRecentMotionSensorData;
    IMotionSensorFilter* m_motionSensorFilter;
    int m_currentlyActivatedMotionSensorFlags;
    int m_manuallyActivatedMotionSensorFlags;
    int m_motionSensorPausedCount;
    int m_activeKeyboardModifiers;
};
