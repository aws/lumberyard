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

#include "StdAfx.h"

#include "SynergyMouse.h"

#include "IRenderer.h"

#define CONTROL_RELATIVE_INPUT_WITH_MIDDLE_BUTTON

#if defined(AZ_FRAMEWORK_INPUT_ENABLED)
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SynergyInput
{
    using namespace AzFramework;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouse::Implementation* InputDeviceMouseSynergy::Create(InputDeviceMouse& inputDevice)
    {
        return aznew InputDeviceMouseSynergy(inputDevice);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouseSynergy::InputDeviceMouseSynergy(InputDeviceMouse& inputDevice)
        : InputDeviceMouse::Implementation(inputDevice)
        , m_systemCursorState(SystemCursorState::Unknown)
        , m_systemCursorPositionNormalized(0.5f, 0.5f)
        , m_threadAwareRawButtonEventQueuesById()
        , m_threadAwareRawButtonEventQueuesByIdMutex()
        , m_threadAwareSystemCursorPositionNormalized(0.5f, 0.5f)
        , m_threadAwareSystemCursorPositionNormalizedMutex()
    {
        RawInputNotificationBusSynergy::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouseSynergy::~InputDeviceMouseSynergy()
    {
        RawInputNotificationBusSynergy::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMouseSynergy::IsConnected() const
    {
        // We could check the validity of the socket connection to the synergy server
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseSynergy::SetSystemCursorState(SystemCursorState systemCursorState)
    {
        // This doesn't apply when using synergy, but we'll store it so it can be queried
        m_systemCursorState = systemCursorState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SystemCursorState InputDeviceMouseSynergy::GetSystemCursorState() const
    {
        return m_systemCursorState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseSynergy::SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized)
    {
        m_systemCursorPositionNormalized = positionNormalized;

        // This will simply get overridden by the next call to OnRawMousePositionEvent, but there's
        // not much we can do about it, and Synergy mouse input is only for debug purposes anyway.
        AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareSystemCursorPositionNormalizedMutex);
        m_threadAwareSystemCursorPositionNormalized = positionNormalized;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputDeviceMouseSynergy::GetSystemCursorPositionNormalized() const
    {
        return m_systemCursorPositionNormalized;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseSynergy::TickInputDevice()
    {
        {
            // Queue all mouse button events that were received in the other thread
            AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawButtonEventQueuesByIdMutex);
            for (const auto& buttonEventQueuesById : m_threadAwareRawButtonEventQueuesById)
            {
                const InputChannelId& inputChannelId = buttonEventQueuesById.first;
                for (bool rawButtonState : buttonEventQueuesById.second)
                {
                    QueueRawButtonEvent(inputChannelId, rawButtonState);
                }
            }
            m_threadAwareRawButtonEventQueuesById.clear();
        }

        const AZ::Vector2 oldSystemCursorPositionNormalized = m_systemCursorPositionNormalized;
        {
            // Update the system cursor position
            AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareSystemCursorPositionNormalizedMutex);
            m_systemCursorPositionNormalized = m_threadAwareSystemCursorPositionNormalized;
        }

        // Ideally we would obtain and queue mouse movement events directly from synergy, but unlike
        // windows/mac mouse implementations (where mouse movement events are calculated before any
        // operating system ballistics are applied), synergy calculates 'relative' mouse movement as
        // the delta between the previous mouse position and the current one. So we could modify our
        // synergy context implementation to parse and forward these relative mouse 'DMRM' messages,
        // but that would give us the same result as calculating the delta ourselves as we do below.
        const AZ::Vector2 delta = m_systemCursorPositionNormalized - oldSystemCursorPositionNormalized;
#if defined(CONTROL_RELATIVE_INPUT_WITH_MIDDLE_BUTTON)
        const InputChannel* inputChannel = InputChannelRequests::FindInputChannel(InputDeviceMouse::Button::Middle);
        if (inputChannel && inputChannel->IsActive())
#endif
        {
            QueueRawMovementEvent(InputDeviceMouse::Movement::X, delta.GetX() * static_cast<float>(gEnv->pRenderer->GetWidth()));
            QueueRawMovementEvent(InputDeviceMouse::Movement::Y, delta.GetY() * static_cast<float>(gEnv->pRenderer->GetHeight()));
        }

        // Process raw event queues once each frame
        ProcessRawEventQueues();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseSynergy::OnRawMouseButtonDownEvent(uint32_t buttonIndex)
    {
        ThreadSafeQueueRawButtonEvent(buttonIndex, true);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseSynergy::OnRawMouseButtonUpEvent(uint32_t buttonIndex)
    {
        ThreadSafeQueueRawButtonEvent(buttonIndex, false);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseSynergy::OnRawMousePositionEvent(float normalizedMouseX,
                                                          float normalizedMouseY)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareSystemCursorPositionNormalizedMutex);
        m_threadAwareSystemCursorPositionNormalized = AZ::Vector2(normalizedMouseX, normalizedMouseY);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseSynergy::ThreadSafeQueueRawButtonEvent(uint32_t buttonIndex,
                                                                bool rawButtonState)
    {
        const InputChannelId* inputChannelId = nullptr;
        switch (buttonIndex)
        {
            case 1: { inputChannelId = &InputDeviceMouse::Button::Left; } break;
            case 2: { inputChannelId = &InputDeviceMouse::Button::Middle; } break;
            case 3: { inputChannelId = &InputDeviceMouse::Button::Right; } break;
        }

        if (inputChannelId)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawButtonEventQueuesByIdMutex);
            m_threadAwareRawButtonEventQueuesById[*inputChannelId].push_back(rawButtonState);
        }
    }
} // namespace AzFramework

#elif defined(USE_SYNERGY_INPUT)

#include "SynergyContext.h"
#include <IHardwareMouse.h>

SInputSymbol*   CSynergyMouse::Symbol[MAX_SYNERGY_MOUSE_SYMBOLS] = {0};

CSynergyMouse::CSynergyMouse(IInput& input, CSynergyContext* pContext)
    : CInputDevice(input, "mouse")
{
    m_deviceType = eIDT_Mouse;
    m_pContext = pContext;
}

bool CSynergyMouse::Init()
{
    m_lastX = 0;
    m_lastY = 0;
    m_lastZ = 0;
    m_lastButtonL = false;
    m_lastButtonM = false;
    m_lastButtonR = false;
    Symbol[SYNERGY_BUTTON_L] = MapSymbol(SYNERGY_BUTTON_L, eKI_Mouse1, "mouse1");
    Symbol[SYNERGY_BUTTON_R] = MapSymbol(SYNERGY_BUTTON_R, eKI_Mouse2, "mouse2");
    Symbol[SYNERGY_BUTTON_M] = MapSymbol(SYNERGY_BUTTON_M, eKI_Mouse3, "mouse3");
    Symbol[SYNERGY_MOUSE_X] = MapSymbol(SYNERGY_MOUSE_X, eKI_MouseX, "maxis_x", SInputSymbol::RawAxis);
    Symbol[SYNERGY_MOUSE_Y] = MapSymbol(SYNERGY_MOUSE_Y, eKI_MouseY, "maxis_y", SInputSymbol::RawAxis);
    Symbol[SYNERGY_MOUSE_Z] = MapSymbol(SYNERGY_MOUSE_Z, eKI_MouseZ, "maxis_z", SInputSymbol::RawAxis);
    return true;
}

///////////////////////////////////////////
void CSynergyMouse::Update(bool bFocus)
{
    SInputEvent event;
    SInputSymbol* pSymbol;
    int16 deltaX, deltaY, deltaZ;
    uint16 x, y, wheelX, wheelY;
    bool buttonL, buttonM, buttonR;

    while (m_pContext->GetMouse(x, y, wheelX, wheelY, buttonL, buttonM, buttonR))
    {
        deltaX = x - m_lastX;
        m_lastX = x;
        deltaY = y - m_lastY;
        m_lastY = y;
        deltaZ = wheelY - m_lastZ;
        m_lastZ = wheelY;

        if (deltaX)
        {
#ifdef CONTROL_RELATIVE_INPUT_WITH_MIDDLE_BUTTON
            if (buttonM)
#endif
            {
                pSymbol = Symbol[SYNERGY_MOUSE_X];
                pSymbol->state = eIS_Changed;
                pSymbol->value = deltaX;
                pSymbol->AssignTo(event);
                GetIInput().PostInputEvent(event);
            }
        }

        if (deltaY)
        {
#ifdef CONTROL_RELATIVE_INPUT_WITH_MIDDLE_BUTTON
            if (buttonM)
#endif
            {
                pSymbol = Symbol[SYNERGY_MOUSE_Y];
                pSymbol->state = eIS_Changed;
                pSymbol->value = deltaY;
                pSymbol->AssignTo(event);
                GetIInput().PostInputEvent(event);
            }
        }

        if (deltaZ)
        {
            pSymbol = Symbol[SYNERGY_MOUSE_Z];
            pSymbol->state = eIS_Changed;
            pSymbol->value = deltaZ;
            pSymbol->AssignTo(event);
            GetIInput().PostInputEvent(event);
        }

        if (buttonL != m_lastButtonL)
        {
            pSymbol = Symbol[SYNERGY_BUTTON_L];
            pSymbol->state = buttonL ? eIS_Pressed : eIS_Released;
            pSymbol->value = buttonL ? 1.0f : 0.0f;
            pSymbol->AssignTo(event);
            GetIInput().PostInputEvent(event);
            m_lastButtonL = buttonL;
        }

#ifndef CONTROL_RELATIVE_INPUT_WITH_MIDDLE_BUTTON
        if (buttonM != m_lastButtonM)
        {
            pSymbol = Symbol[SYNERGY_BUTTON_M];
            pSymbol->state = buttonM ? eIS_Pressed : eIS_Released;
            pSymbol->value = buttonM ? 1.0f : 0.0f;
            pSymbol->AssignTo(event);
            GetIInput().PostInputEvent(event);
            m_lastButtonM = buttonM;
        }
#endif

        if (buttonR != m_lastButtonR)
        {
            pSymbol = Symbol[SYNERGY_BUTTON_R];
            pSymbol->state = buttonR ? eIS_Pressed : eIS_Released;
            pSymbol->value = buttonR ? 1.0f : 0.0f;
            pSymbol->AssignTo(event);
            GetIInput().PostInputEvent(event);
            m_lastButtonR = buttonR;
        }
    }

    gEnv->pHardwareMouse->SetHardwareMousePosition(m_lastX, m_lastY);

    // Send one mouse event each frame to report it's last known position.
    SInputEvent mousePositionEvent;
    mousePositionEvent.deviceType = eIDT_Mouse;
    mousePositionEvent.keyId = eKI_MousePosition;
    mousePositionEvent.keyName = "mouse_pos";
    mousePositionEvent.screenPosition.x = m_lastX;
    mousePositionEvent.screenPosition.y = m_lastY;
    GetIInput().PostInputEvent(mousePositionEvent);
}

bool CSynergyMouse::SetExclusiveMode(bool value)
{
    return false;
}

#endif // USE_SYNERGY_INPUT
