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
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/AzToLyInputDeviceGamepad_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/AzToLyInputDeviceGamepad_h_provo.inl"
    #endif
#elif defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#if defined(TOOLS_SUPPORT_XENIA)
    #include "Xenia/AzToLyInputDeviceGamepad_h_xenia.inl"
#endif
#if defined(TOOLS_SUPPORT_PROVO)
    #include "Provo/AzToLyInputDeviceGamepad_h_provo.inl"
#endif
#endif
