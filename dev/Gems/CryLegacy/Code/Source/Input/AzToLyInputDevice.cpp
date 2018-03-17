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

#include "AzToLyInputDevice.h"

#include <AzFramework/Input/Buses/Requests/InputHapticFeedbackRequestBus.h>
#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Input/Devices/InputDevice.h>

#include <IRenderer.h>

using namespace AzFramework;

////////////////////////////////////////////////////////////////////////////////////////////////////
AzToLyInputDevice::AzToLyInputDevice(IInput& input,
                                     EInputDeviceType cryDeviceType,
                                     const char* cryInputDeviceDisplayName,
                                     const InputDeviceId& azFrameworkInputDeviceId)
    : CInputDevice(input, azFrameworkInputDeviceId.GetName())
    , m_azFrameworkInputDeviceId(azFrameworkInputDeviceId)
    , m_cryInputDeviceDisplayName(cryInputDeviceDisplayName)
{
    m_deviceType = cryDeviceType;

    InputChannelNotificationBus::Handler::BusConnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzToLyInputDevice::AzToLyInputDevice(IInput& input,
                                     EInputDeviceType cryDeviceType,
                                     const char* cryInputDeviceDisplayName,
                                     const std::vector<SInputSymbol>& azToCryInputSymbols,
                                     const AzFramework::InputDeviceId& azFrameworkInputDeviceId)
    : AzToLyInputDevice(input, cryDeviceType, cryInputDeviceDisplayName, azFrameworkInputDeviceId) // Delegated constructor
{
    for (const SInputSymbol& symbol : azToCryInputSymbols)
    {
        MapSymbol(symbol.devSpecId, symbol.keyId, symbol.name, symbol.type);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzToLyInputDevice::~AzToLyInputDevice()
{
    InputChannelNotificationBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInputDevice::ClearKeyState()
{
    CInputDevice::ClearKeyState();
    SetForceFeedback(IFFParams());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInputDevice::Update(bool focus)
{
    CInputDevice::Update(focus);

    if (m_timedForceFeedback.timeInSeconds > 0.0f)
    {
        m_timedForceFeedback.timeInSeconds -= gEnv->pTimer->GetFrameTime();
        if (m_timedForceFeedback.timeInSeconds <= 0.0f)
        {
            m_timedForceFeedback = IFFParams();
            SetForceFeedback(IFFParams());
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool AzToLyInputDevice::SetForceFeedback(IFFParams params)
{
    if (params.effectId == eFF_Rumble_Basic && params.timeInSeconds > 0.0f)
    {
        m_timedForceFeedback = params;
    }

    InputHapticFeedbackRequestBus::Event(m_azFrameworkInputDeviceId,
                                         &InputHapticFeedbackRequests::SetVibration,
                                         params.strengthA,
                                         params.strengthB);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInputDevice::OnInputChannelEvent(const InputChannel& inputChannel, bool& o_hasBeenConsumed)
{
    if (o_hasBeenConsumed)
    {
        return;
    }

    if (inputChannel.GetInputDevice().GetInputDeviceId() != m_azFrameworkInputDeviceId)
    {
        return;
    }

    SInputSymbol* inputSymbol = DevSpecIdToSymbol(inputChannel.GetInputChannelId().GetNameCrc32());
    if (inputSymbol)
    {
        PostCryInputEvent(inputChannel, *inputSymbol);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInputDevice::PostCryInputEvent(const InputChannel& inputChannel, SInputSymbol& inputSymbol)
{
    // Value
    inputSymbol.value = inputChannel.GetValue();

    // State
    if (inputSymbol.type == SInputSymbol::Axis ||
        inputSymbol.type == SInputSymbol::RawAxis ||
        inputSymbol.type == SInputSymbol::Trigger)
    {
        // CryInput only dispatched 'changed' events for analog inputs (with the execption of touch)
        inputSymbol.state = eIS_Changed;
    }
    else
    {
        switch (inputChannel.GetState())
        {
            case AzFramework::InputChannel::State::Began:
            {
                inputSymbol.state = eIS_Pressed;
            }
            break;
            case AzFramework::InputChannel::State::Updated:
            {
                inputSymbol.state = eIS_Down;
            }
            break;
            case AzFramework::InputChannel::State::Ended:
            {
                inputSymbol.state = eIS_Released;
            }
            break;
            case AzFramework::InputChannel::State::Idle:
            {
                AZ_Assert(false, "Received input event in the idle state");
                inputSymbol.state = eIS_Unknown;
            }
            break;
        }
    }

    // Screen Position
    const InputChannel::PositionData2D* positionData2D = inputChannel.GetCustomData<InputChannel::PositionData2D>();
    if (positionData2D)
    {
        inputSymbol.screenPosition.x = positionData2D->m_normalizedPosition.GetX() * gEnv->pRenderer->GetWidth();
        inputSymbol.screenPosition.y = positionData2D->m_normalizedPosition.GetY() * gEnv->pRenderer->GetHeight();
    }

    // Post Event
    SInputEvent inputEvent;
    inputSymbol.AssignTo(inputEvent, GetIInput().GetModifiers());
    GetIInput().PostInputEvent(inputEvent);

    // CryInput dispatched 'changed' events for touch inputs, in addition to the regular pressed/held/released
    if (inputSymbol.deviceType == eIDT_TouchScreen &&
        inputSymbol.state == eIS_Down)
    {
        inputEvent.state = eIS_Changed;
        GetIInput().PostInputEvent(inputEvent);
    }
}
