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

#include <AzCore/EBus/EBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SynergyInput
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! State of the input channel (not all channels will go through all states)
    enum ModifierMask
    {
        ModifierMask_None       = 0x0000,
        ModifierMask_Shift      = 0x0001,
        ModifierMask_Ctrl       = 0x0002,
        ModifierMask_AltL       = 0x0004,
        ModifierMask_Windows    = 0x0010,
        ModifierMask_AltR       = 0x0020,
        ModifierMask_CapsLock   = 0x1000,
        ModifierMask_NumLock    = 0x2000,
        ModifierMask_ScrollLock = 0x4000,
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to listen for raw synergy input as broadcast by the system. Applications
    //! that want synergy events to be processed by the AzFramework input system must broadcast all
    //! synergy input events (received from a connection to a synergy server) using this interface.
    //!
    //! It's possible to receive multiple events per button/key per frame, and it's very likely that
    //! synergy input events will not be dispatched from the main thread, so care should be taken to
    //! ensure thread safety when implementing event handlers that connect to this synergy event bus.
    //!
    //! This EBus is intended primarily for the AzFramework input system to process raw input events.
    //! Most systems that need to process input should use the generic AzFramework input interfaces,
    //! but if necessary it is perfectly valid to connect directly to this EBus for synergy events.
    class RawInputNotificationsSynergy : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: raw input notifications are addressed to a single address
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: raw input notifications can be handled by multiple listeners
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~RawInputNotificationsSynergy() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw mouse button down events (assumed to be dispatched from any thread)
        //! \param[in] buttonIndex The index of the button that was pressed down
        virtual void OnRawMouseButtonDownEvent(uint32_t buttonIndex) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw mouse button up events (assumed to be dispatched from any thread)
        //! \param[in] buttonIndex The index of the button that was released up
        virtual void OnRawMouseButtonUpEvent(uint32_t buttonIndex) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw mouse position events (assumed to be dispatched from any thread)
        //! \param[in] mouseX The x position of the mouse normalized relative to the screen
        //! \param[in] mouseY The y position of the mouse normalized relative to the screen
        virtual void OnRawMousePositionEvent(float normalizedMouseX, float normalizedMouseY) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw keyboard key down events (assumed to be dispatched from any thread)
        //! \param[in] scanCode The scan code of the key that was pressed down
        //! \param[in] activeModifiers The bit mask of currently active modifier keys
        virtual void OnRawKeyboardKeyDownEvent(uint32_t scanCode, ModifierMask activeModifiers) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw keyboard key up events (assumed to be dispatched from any thread)
        //! \param[in] scanCode The scan code of the key that was released up
        //! \param[in] activeModifiers The bit mask of currently active modifier keys
        virtual void OnRawKeyboardKeyUpEvent(uint32_t scanCode, ModifierMask activeModifiers) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw keyboard key repeat events (assumed to be dispatched from any thread)
        //! \param[in] scanCode The scan code of the key that was repeatedly held down
        //! \param[in] activeModifiers The bit mask of currently active modifier keys
        virtual void OnRawKeyboardKeyRepeatEvent(uint32_t scanCode, ModifierMask activeModifiers) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw clipboard events (assumed to be dispatched from any thread)
        //! \param[in] clipboardContents The contents of the clipboard
        virtual void OnRawClipboardEvent(const char* clipboardContents) {}
    };
    using RawInputNotificationBusSynergy = AZ::EBus<RawInputNotificationsSynergy>;
} // namespace AzFramework
