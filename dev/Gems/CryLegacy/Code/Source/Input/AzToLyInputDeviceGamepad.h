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

#include "AzToLyInputDevice.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
class AzToLyInputDeviceGamepad : public AzToLyInputDevice
{
public:
    AzToLyInputDeviceGamepad(IInput& input, AZ::u32 index);
    ~AzToLyInputDeviceGamepad() override;

protected:
    // InputChannelNotificationBus::Handler
    void OnInputChannelEvent(const AzFramework::InputChannel& inputChannel,
                             bool& o_hasBeenConsumed) override;
};

#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(AzToLyInputDeviceGamepad_h, AZ_RESTRICTED_PLATFORM)
#elif defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#if defined(TOOLS_SUPPORT_XBONE)
#include AZ_RESTRICTED_FILE(AzToLyInputDeviceGamepad_h, TOOLS_SUPPORT_XBONE)
#endif
#if defined(TOOLS_SUPPORT_PS4)
#include AZ_RESTRICTED_FILE(AzToLyInputDeviceGamepad_h, TOOLS_SUPPORT_PS4)
#endif
#endif
