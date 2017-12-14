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

#pragma once

#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

#include <AzCore/std/parallel/mutex.h>

#include "RawInputNotificationBus_synergy.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SynergyInput
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Synergy specific implementation for mouse input devices. This should eventually be moved to
    //! a Gem, with InputDeviceKeyboardSynergy and RawInputNotificationsSynergy they both depend on.
    class InputDeviceMouseSynergy : public AzFramework::InputDeviceMouse::Implementation
                                  , public RawInputNotificationBusSynergy::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceMouseSynergy, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Custom factory create function
        //! \param[in] inputDevice Reference to the input device being implemented
        static Implementation* Create(AzFramework::InputDeviceMouse& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceMouseSynergy(AzFramework::InputDeviceMouse& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceMouseSynergy() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::SetSystemCursorState
        void SetSystemCursorState(AzFramework::SystemCursorState systemCursorState) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::GetSystemCursorState
        AzFramework::SystemCursorState GetSystemCursorState() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::SetSystemCursorPositionNormalized
        void SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::GetSystemCursorPositionNormalized
        AZ::Vector2 GetSystemCursorPositionNormalized() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref RawInputNotificationsSynergy::OnRawMouseButtonDownEvent
        void OnRawMouseButtonDownEvent(uint32_t buttonIndex) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref RawInputNotificationsSynergy::OnRawMouseButtonUpEvent
        void OnRawMouseButtonUpEvent(uint32_t buttonIndex) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref RawInputNotificationsSynergy::OnRawMousePositionEvent
        void OnRawMousePositionEvent(float normalizedMouseX, float normalizedMouseY) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Thread safe method to queue raw button events to be processed in the main thread update
        //! \param[in] buttonIndex The index of the button
        //! \param[in] rawButtonState The raw button state
        void ThreadSafeQueueRawButtonEvent(uint32_t buttonIndex, bool rawButtonState);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AzFramework::SystemCursorState m_systemCursorState;
        AZ::Vector2                    m_systemCursorPositionNormalized;

        RawButtonEventQueueByIdMap     m_threadAwareRawButtonEventQueuesById;
        AZStd::mutex                   m_threadAwareRawButtonEventQueuesByIdMutex;

        AZ::Vector2                    m_threadAwareSystemCursorPositionNormalized;
        AZStd::mutex                   m_threadAwareSystemCursorPositionNormalizedMutex;
    };
} // namespace SynergyInput
