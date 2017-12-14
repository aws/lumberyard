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

#include "InputDevice.h"

#include <AzFramework/Input/Buses/Notifications/InputChannelEventNotificationBus.h>
#include <AzFramework/Input/Devices/InputDeviceId.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class AzToLyInputDevice : public CInputDevice,
                          public AzFramework::InputChannelEventNotificationBus::Handler
{
public:
    AzToLyInputDevice(IInput& input,
                      EInputDeviceType cryDeviceType,
                      const char* cryInputDeviceDisplayName,
                      const AzFramework::InputDeviceId& azFrameworkInputDeviceId);
    AzToLyInputDevice(IInput& input,
                      EInputDeviceType cryDeviceType,
                      const char* cryInputDeviceDisplayName,
                      const std::vector<SInputSymbol>& azToCryInputSymbols,
                      const AzFramework::InputDeviceId& azFrameworkInputDeviceId);
    ~AzToLyInputDevice() override;

protected:
    // IInputDevice
    void ClearKeyState() override;
    void Update(bool bFocus) override;
    bool SetForceFeedback(IFFParams params) override;
    int GetDeviceIndex() const override { return m_azFrameworkInputDeviceId.GetIndex(); }
    const char* GetDeviceCommonName() const override { return m_cryInputDeviceDisplayName; }

    // InputChannelEventNotificationBus::Handler
    void OnInputChannelEvent(const AzFramework::InputChannel& inputChannel,
                             bool& o_hasBeenConsumed) override;

    void PostCryInputEvent(const AzFramework::InputChannel& inputChannel, SInputSymbol& inputSymbol);

protected:
    const AzFramework::InputDeviceId m_azFrameworkInputDeviceId;
    const char* m_cryInputDeviceDisplayName;
    IFFParams m_timedForceFeedback;
};
