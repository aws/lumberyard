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

#include "AzToLyInputDeviceMouse.h"

#include <IRenderer.h>

#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

using namespace AzFramework;

////////////////////////////////////////////////////////////////////////////////////////////////////
AzToLyInputDeviceMouse::AzToLyInputDeviceMouse(IInput& input)
    : AzToLyInputDevice(input, eIDT_Mouse, "mouse", InputDeviceMouse::Id)
{
    MapSymbol(InputDeviceMouse::Button::Left.GetNameCrc32(), eKI_Mouse1, "mouse1");
    MapSymbol(InputDeviceMouse::Button::Right.GetNameCrc32(), eKI_Mouse2, "mouse2");
    MapSymbol(InputDeviceMouse::Button::Middle.GetNameCrc32(), eKI_Mouse3, "mouse3");
    MapSymbol(InputDeviceMouse::Button::Other1.GetNameCrc32(), eKI_Mouse4, "mouse4");
    MapSymbol(InputDeviceMouse::Button::Other2.GetNameCrc32(), eKI_Mouse5, "mouse5");

    MapSymbol(AZ::Crc32("mouse_delta_z_up"), eKI_MouseWheelUp, "mwheel_up");
    MapSymbol(AZ::Crc32("mouse_delta_z_down"), eKI_MouseWheelDown, "mwheel_down");
    MapSymbol(InputDeviceMouse::Movement::X.GetNameCrc32(), eKI_MouseX, "maxis_x", SInputSymbol::RawAxis);
    MapSymbol(InputDeviceMouse::Movement::Y.GetNameCrc32(), eKI_MouseY, "maxis_y", SInputSymbol::RawAxis);
    MapSymbol(InputDeviceMouse::Movement::Z.GetNameCrc32(), eKI_MouseZ, "maxis_z", SInputSymbol::RawAxis);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzToLyInputDeviceMouse::~AzToLyInputDeviceMouse()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInputDeviceMouse::OnInputChannelEvent(const InputChannel& inputChannel, bool& o_hasBeenConsumed)
{
    AzToLyInputDevice::OnInputChannelEvent(inputChannel, o_hasBeenConsumed);

    if (inputChannel.GetInputChannelId() == InputDeviceMouse::Movement::Z)
    {
        SInputSymbol* inputSymbol = nullptr;
        if (inputChannel.GetValue() > 0.0f)
        {
            inputSymbol = DevSpecIdToSymbol(AZ::Crc32("mouse_delta_z_up"));
        }
        else if (inputChannel.GetValue() < 0.0f)
        {
            inputSymbol = DevSpecIdToSymbol(AZ::Crc32("mouse_delta_z_down"));
        }

        if (inputSymbol)
        {
            PostCryInputEvent(inputChannel, *inputSymbol);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInputDeviceMouse::Update(bool focus)
{
    AzToLyInputDevice::Update(focus);

    const InputDevice* mouseDevice = InputDeviceRequests::FindInputDevice(InputDeviceMouse::Id);
    if (mouseDevice && mouseDevice->IsSupported())
    {
        // Send one 'mouse_pos' event each frame to report it's last known position. This is just
        // to maintain the existing CryInput behavior, and when systems that rely on this are
        // converted to use AzFramework input directly they will have to be modified to assume
        // mouse position events are only sent when the mouse position has actually been moved.
        AZ::Vector2 systemCursorPositionNormalized = AZ::Vector2::CreateZero();
        InputSystemCursorRequestBus::EventResult(systemCursorPositionNormalized,
                                                 InputDeviceMouse::Id,
                                                 &InputSystemCursorRequests::GetSystemCursorPositionNormalized);
        SInputEvent inputEvent;
        inputEvent.deviceType = eIDT_Mouse;
        inputEvent.keyId = eKI_MousePosition;
        inputEvent.keyName = "mouse_pos";
        inputEvent.screenPosition.x = systemCursorPositionNormalized.GetX() * gEnv->pRenderer->GetWidth();
        inputEvent.screenPosition.y = systemCursorPositionNormalized.GetY() * gEnv->pRenderer->GetHeight();
        GetIInput().PostInputEvent(inputEvent);
    }
}
