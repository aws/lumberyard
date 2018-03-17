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

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Input/Buses/Requests/InputDeviceRequestBus.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GridMate
{
    struct PlayerId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Base class for all input devices. Input devices are responsible for processing raw input and
    //! using it to update their associated input channels. Input devices can also be queried at any
    //! time to determine their current connected state, or retrieve their associated input channels.
    //!
    //! While almost every concrete input device that inherits from this class will likely only send
    //! input events when running on a specific platform, we should always be able to access all the
    //! potentially available input devices and channels in order to display and set input bindings.
    //!
    //! Derived classes are responsible for:
    //! - Creating and maintaining a collection of associated input channels
    //! - Overriding GetInputChannelsById to return all associated input channels
    //! - Overriding TickInputDevice to process raw input and update input channel states
    //! - Overriding IsSupported to return whether the input device is supported by the platform
    //! - Overriding IsConnected to return whether the input device is currently connected
    //! - Broadcasting events when the input device gets connected or diconnected
    //!
    //! Ideally, an input device will use TickInputDevice (that gets called once every frame by the
    //! system) to process raw input, update all its input channels, and broadcast all input events.
    //! However, this may not always be possible, as the state of an input channel cannot always be
    //! queried at will, but will rather be sent by the system at a pre-determined time (eg. touch).
    //! In these cases, input devices must queue all raw input until TickInputDevice is called, when
    //! they can use it to update their input channels and broadcast events at the appropriate time.
    class InputDevice : public InputDeviceRequestBus::Handler
                      , public ApplicationLifecycleEvents::Bus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDevice, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputDevice, "{29F9FB6B-15CB-4DB4-9F36-DE7396B82F3D}");

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Reflection
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDeviceId Id of the input device
        explicit InputDevice(const InputDeviceId& inputDeviceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying (protected to workaround a VS2013 bug in std::is_copy_constructible)
        // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
    protected:
        AZ_DISABLE_COPY_MOVE(InputDevice);
    public:

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceRequests::GetInputDevice
        const InputDevice* GetInputDevice() const final;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the input device's id
        //! \return Id of the input device
        const InputDeviceId& GetInputDeviceId() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the input device's currently assigned local player id (if any)
        //! \return Id of the local player assigned to the input device (nullptr if none)
        virtual const GridMate::PlayerId* GetAssignedLocalPlayerId() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to all input channels associated with this input device
        //! \return Map of all input channels (keyed by their id) associated with this input device
        virtual const InputChannelByIdMap& GetInputChannelsById() const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Query whether the device is supported by the current platform. A device can be supported
        //! but not currently connected, while a currently connected device will always be supported.
        //! \return True if the input device is supported on the current platform, false otherwise
        virtual bool IsSupported() const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Query whether the device is currently connected to the system. A device can be supported
        //! but not currently connected, while a currently connected device will always be supported.
        //! \return True if the input device is currently connected, false otherwise
        virtual bool IsConnected() const = 0;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Broadcast an event when an associated input channel's state or value is updated
        //! \param[in] inputChannel The input channel whose state or value was updated
        void BroadcastInputChannelEvent(const InputChannel& inputChannel) const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Broadcast an event when unicode text input is generated by an input device
        //! \param[in] textUTF8 The text emitted by the input device (encoded using UTF-8)
        void BroadcastInputTextEvent(const AZStd::string& textUTF8) const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Broadcast an event when the input device connects to the system
        void BroadcastInputDeviceConnectedEvent() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Broadcast an event when the input device disconnects from the system
        void BroadcastInputDeviceDisconnectedEvent() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Reset the state of all this input device's associated input channels
        void ResetInputChannelStates();

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceRequests::GetInputDeviceIds
        void GetInputDeviceIds(InputDeviceIdSet& o_deviceIds) const final;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceRequests::GetInputDevicesById
        void GetInputDevicesById(InputDeviceByIdMap& o_devicesById) const final;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceRequests::GetInputChannelIds
        void GetInputChannelIds(InputChannelIdSet& o_channelIds) const final;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceRequests::GetInputChannelsById
        void GetInputChannelsById(InputChannelByIdMap& o_channelsById) const final;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::ApplicationLifecycleEvents::OnApplicationConstrained
        void OnApplicationConstrained(Event lastEvent) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        const InputDeviceId m_inputDeviceId; //!< Id of the input device
    };
} // namespace AzFramework
