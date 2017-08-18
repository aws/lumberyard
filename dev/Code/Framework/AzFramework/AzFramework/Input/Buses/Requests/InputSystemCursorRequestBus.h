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

#include <AzFramework/Input/Devices/InputDeviceId.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! State of the system cursor
    enum class SystemCursorState
    {
        Unknown,                //!< The state of the system cursor is not known
        ConstrainedAndHidden,   //!< Constrained to the active window and hidden
        ConstrainedAndVisible,  //!< Constrained to the active window and visible
        UnconstrainedAndHidden, //!< Free to move outside the active window but hidden while inside
        UnconstrainedAndVisible //!< Free to move outside the active window but visible while inside
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to query/change the state, position, or appearance of the system cursor
    class InputSystemCursorRequests : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputDeviceId using EBus<>::Event,
        //! which should be handled by only one device that has connected to the bus using that id.
        //! Input requests can also be sent using EBus<>::Broadcast, in which case they'll be sent
        //! to all input devices that have connected to the input event bus regardless of their id.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests should be handled by only one input device connected to each id
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputDeviceId
        using BusIdType = InputDeviceId;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Inform input devices that the system cursor state should be changed. Calls to the method
        //! should usually be addressed to a mouse input device, but it may be possible for multiple
        //! different device instances to each be associated with a system cursor (eg. Wii remotes).
        //!
        //! Called using either:
        //! - EBus<>::Broadcast (any input device can respond to the request)
        //! - EBus<>::Event(id) (the given device can respond to the request)
        //!
        //! \param[in] systemCursorState The desired system cursor state
        virtual void SetSystemCursorState(SystemCursorState /*systemCursorState*/) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Query input devices for the current state of the system cursor. All calls to this method
        //! should usually be addressed to a mouse input device, but it may be possible for multiple
        //! different device instances to each be associated with a system cursor (eg. Wii remotes).
        //!
        //! Called using either:
        //! - EBus<>::Broadcast (any input device can respond to the request)
        //! - EBus<>::Event(id) (the given device can respond to the request)
        //!
        //! \return The current state of the system cursor
        virtual SystemCursorState GetSystemCursorState() const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Inform input devices that the system cursor position should be set. Calls to the method
        //! should usually be addressed to a mouse input device, but it may be possible for multiple
        //! different device instances to each be associated with a system cursor (eg. Wii remotes).
        //!
        //! Called using either:
        //! - EBus<>::Broadcast (any input device can respond to the request)
        //! - EBus<>::Event(id) (the given device can respond to the request)
        //!
        //! \param[in] positionNormalized The desired system cursor position normalized
        virtual void SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the current system cursor position normalized relative to the active window. The
        //! position obtained has had os ballistics applied, and is valid regardless of whether
        //! the system cursor is hidden or visible. When the cursor has been constrained to the
        //! active window the values will be in the [0.0, 1.0] range, but not when unconstrained.
        //! See also InputSystemCursorRequests::SetSystemCursorState and GetSystemCursorState.
        //!
        //! Called using either:
        //! - EBus<>::Broadcast (any input device can respond to the request)
        //! - EBus<>::Event(id) (the given device can respond to the request)
        //!
        //! \return The current system cursor position normalized relative to the active window
        virtual AZ::Vector2 GetSystemCursorPositionNormalized() const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputSystemCursorRequests() = default;
    };
    using InputSystemCursorRequestBus = AZ::EBus<InputSystemCursorRequests>;
} // namespace AzFramework
