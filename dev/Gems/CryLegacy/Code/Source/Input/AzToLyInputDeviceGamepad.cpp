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

#include "AzToLyInputDeviceGamepad.h"

#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>

#include "InputCVars.h"

using namespace AzFramework;

////////////////////////////////////////////////////////////////////////////////////////////////////
AzToLyInputDeviceGamepad::AzToLyInputDeviceGamepad(IInput& input, AZ::u32 index)
    : AzToLyInputDevice(input, eIDT_Gamepad, "xbox 360 controller", InputDeviceGamepad::IdForIndexN(index))
{
    MapSymbol(InputDeviceGamepad::Button::A.GetNameCrc32(), eKI_XI_A, "xi_a");
    MapSymbol(InputDeviceGamepad::Button::B.GetNameCrc32(), eKI_XI_B, "xi_b");
    MapSymbol(InputDeviceGamepad::Button::X.GetNameCrc32(), eKI_XI_X, "xi_x");
    MapSymbol(InputDeviceGamepad::Button::Y.GetNameCrc32(), eKI_XI_Y, "xi_y");
    MapSymbol(InputDeviceGamepad::Button::L1.GetNameCrc32(), eKI_XI_ShoulderL, "xi_shoulderl");
    MapSymbol(InputDeviceGamepad::Button::R1.GetNameCrc32(), eKI_XI_ShoulderR, "xi_shoulderr");
    MapSymbol(InputDeviceGamepad::Button::L3.GetNameCrc32(), eKI_XI_ThumbL, "xi_thumbl");
    MapSymbol(InputDeviceGamepad::Button::R3.GetNameCrc32(), eKI_XI_ThumbR, "xi_thumbr");
    MapSymbol(InputDeviceGamepad::Button::DU.GetNameCrc32(), eKI_XI_DPadUp, "xi_dpad_up");
    MapSymbol(InputDeviceGamepad::Button::DD.GetNameCrc32(), eKI_XI_DPadDown, "xi_dpad_down");
    MapSymbol(InputDeviceGamepad::Button::DL.GetNameCrc32(), eKI_XI_DPadLeft, "xi_dpad_left");
    MapSymbol(InputDeviceGamepad::Button::DR.GetNameCrc32(), eKI_XI_DPadRight, "xi_dpad_right");
    MapSymbol(InputDeviceGamepad::Button::Start.GetNameCrc32(), eKI_XI_Start, "xi_start");
    MapSymbol(InputDeviceGamepad::Button::Select.GetNameCrc32(), eKI_XI_Back, "xi_back");
    MapSymbol(InputDeviceGamepad::Trigger::L2.GetNameCrc32(), eKI_XI_TriggerL, "xi_triggerl", SInputSymbol::Trigger);
    MapSymbol(InputDeviceGamepad::Trigger::R2.GetNameCrc32(), eKI_XI_TriggerR, "xi_triggerr", SInputSymbol::Trigger);
    MapSymbol(InputDeviceGamepad::ThumbStickAxis1D::LX.GetNameCrc32(), eKI_XI_ThumbLX, "xi_thumblx", SInputSymbol::Axis);
    MapSymbol(InputDeviceGamepad::ThumbStickAxis1D::LY.GetNameCrc32(), eKI_XI_ThumbLY, "xi_thumbly", SInputSymbol::Axis);
    MapSymbol(InputDeviceGamepad::ThumbStickDirection::LU.GetNameCrc32(), eKI_XI_ThumbLUp, "xi_thumbl_up");
    MapSymbol(InputDeviceGamepad::ThumbStickDirection::LD.GetNameCrc32(), eKI_XI_ThumbLDown, "xi_thumbl_down");
    MapSymbol(InputDeviceGamepad::ThumbStickDirection::LL.GetNameCrc32(), eKI_XI_ThumbLLeft, "xi_thumbl_left");
    MapSymbol(InputDeviceGamepad::ThumbStickDirection::LR.GetNameCrc32(), eKI_XI_ThumbLRight, "xi_thumbl_right");
    MapSymbol(InputDeviceGamepad::ThumbStickAxis1D::RX.GetNameCrc32(), eKI_XI_ThumbRX, "xi_thumbrx", SInputSymbol::Axis);
    MapSymbol(InputDeviceGamepad::ThumbStickAxis1D::RY.GetNameCrc32(), eKI_XI_ThumbRY, "xi_thumbry", SInputSymbol::Axis);
    MapSymbol(InputDeviceGamepad::ThumbStickDirection::RU.GetNameCrc32(), eKI_XI_ThumbRUp, "xi_thumbr_up");
    MapSymbol(InputDeviceGamepad::ThumbStickDirection::RD.GetNameCrc32(), eKI_XI_ThumbRDown, "xi_thumbr_down");
    MapSymbol(InputDeviceGamepad::ThumbStickDirection::RL.GetNameCrc32(), eKI_XI_ThumbRLeft, "xi_thumbr_left");
    MapSymbol(InputDeviceGamepad::ThumbStickDirection::RR.GetNameCrc32(), eKI_XI_ThumbRRight, "xi_thumbr_right");
    MapSymbol(AZ::Crc32("gamepad_button_l2_digital"), eKI_XI_TriggerLBtn, "xi_triggerl_btn");
    MapSymbol(AZ::Crc32("gamepad_button_r2_digital"), eKI_XI_TriggerRBtn, "xi_triggerr_btn");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzToLyInputDeviceGamepad::~AzToLyInputDeviceGamepad()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInputDeviceGamepad::OnInputChannelEvent(const InputChannel& inputChannel, bool& o_hasBeenConsumed)
{
    AzToLyInputDevice::OnInputChannelEvent(inputChannel, o_hasBeenConsumed);

    if (inputChannel.GetInputChannelId() == InputDeviceGamepad::Trigger::L2)
    {
        SInputSymbol* inputSymbol = DevSpecIdToSymbol(AZ::Crc32("gamepad_button_l2_digital"));
        if (inputSymbol)
        {
            PostCryInputEvent(inputChannel, *inputSymbol);
        }
    }
    else if (inputChannel.GetInputChannelId() == InputDeviceGamepad::Trigger::R2)
    {
        SInputSymbol* inputSymbol = DevSpecIdToSymbol(AZ::Crc32("gamepad_button_r2_digital"));
        if (inputSymbol)
        {
            PostCryInputEvent(inputChannel, *inputSymbol);
        }
    }
}

#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/AzToLyInputDeviceGamepad_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/AzToLyInputDeviceGamepad_cpp_provo.inl"
    #endif
#elif defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#if defined(TOOLS_SUPPORT_XENIA)
    #include "Xenia/AzToLyInputDeviceGamepad_cpp_xenia.inl"
#endif
#if defined(TOOLS_SUPPORT_PROVO)
    #include "Provo/AzToLyInputDeviceGamepad_cpp_provo.inl"
#endif
#endif
