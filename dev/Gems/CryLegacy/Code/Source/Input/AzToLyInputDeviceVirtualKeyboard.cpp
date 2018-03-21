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
#include "CryLegacy_precompiled.h"

#include "AzToLyInputDeviceVirtualKeyboard.h"

#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>

using namespace AzFramework;

////////////////////////////////////////////////////////////////////////////////////////////////////
AzToLyInputDeviceVirtualKeyboard::AzToLyInputDeviceVirtualKeyboard(IInput& input)
    : AzToLyInputDevice(input, eIDT_Keyboard, "virtual_keyboard", InputDeviceVirtualKeyboard::Id)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzToLyInputDeviceVirtualKeyboard::~AzToLyInputDeviceVirtualKeyboard()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInputDeviceVirtualKeyboard::OnInputChannelEvent(const InputChannel& inputChannel, bool& /*o_hasBeenConsumed*/)
{
    if (inputChannel.GetInputDevice().GetInputDeviceId() != InputDeviceVirtualKeyboard::Id)
    {
        return;
    }

    // Post a regular (not unicode) keyboard event.
    SInputEvent event;
    event.deviceIndex = GetDeviceIndex();
    event.deviceType = eIDT_Keyboard;
    event.keyId = eKI_Enter;
    event.keyName = "enter";

    if (inputChannel.GetInputChannelId() == InputDeviceVirtualKeyboard::Command::EditEnter)
    {
        event.keyId = eKI_Enter;
        event.keyName = "enter";
    }
    else if (inputChannel.GetInputChannelId() == InputDeviceVirtualKeyboard::Command::NavigationBack)
    {
        event.keyId = eKI_Android_Back;
        event.keyName = "back";
    }
    else if (inputChannel.GetInputChannelId() == InputDeviceVirtualKeyboard::Command::EditClear)
    {
        // CryInput never supported this
        return;
    }

    event.state = eIS_Pressed;
    event.value = 1.0f;
    GetIInput().PostInputEvent(event);

    event.state = eIS_Released;
    event.value = 0.0f;
    GetIInput().PostInputEvent(event);
}
