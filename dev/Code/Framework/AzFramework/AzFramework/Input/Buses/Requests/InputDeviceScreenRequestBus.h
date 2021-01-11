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

#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Input/Devices/InputDeviceId.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/EBus/EBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    class InputDevice;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to query input devices for their associated input channels and state
    class InputDeviceScreenRequests : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputDeviceId so that they are only
        //! handled by one input device that has connected to the bus using that unique id, or they
        //! can be broadcast to all input devices that have connected to the bus, regardless of id.
        //! Connected input devices are ordered by their local player index from lowest to highest.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ByIdAndOrdered;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests should be handled by only one input device connected to each id
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputDeviceId
        using BusIdType = InputDeviceId;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests are handled by connected devices in the order of local player index
        using BusIdOrderCompare = AZStd::less<BusIdType>;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: InputDeviceScreenRequestBus can be accessed from multiple threads, but is safe to use with
        //! LocklessDispatch because connect/disconnect is handled only on engine startup/shutdown (InputSystemComponent).
        using MutexType = AZStd::recursive_mutex;
        static const bool LocklessDispatch = true;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Enables sleep timer for the input device that is uniquely identified by the InputDeviceId.
        //! If enabled is set to false, the device will not enter sleep from the idle timer.
        virtual void EnableIdleSleepTimer(bool enabled) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Gets the safe frame which is the bounding box of the screen not including the virtual
        //! keyboard. Dimensions of the bounding box are passed into the call and will hold
        //! the bounding box info after the call returns.
        virtual void GetSafeFrame(float& left, float& top, float& right, float& bottom) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputDeviceScreenRequests() = default;
    };
    using InputDeviceScreenRequestBus = AZ::EBus<InputDeviceScreenRequests>;

    
} // namespace AzFramework
