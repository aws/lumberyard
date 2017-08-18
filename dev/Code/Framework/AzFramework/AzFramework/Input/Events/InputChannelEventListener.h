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

#include <AzFramework/Input/Buses/Notifications/InputChannelEventNotificationBus.h>
#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Input/Devices/InputDevice.h>

// Start: fix for windows defining max/min macros
#pragma push_macro("max")
#pragma push_macro("min")
#undef max
#undef min

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class that handles input notifications by priority, and that allows events to be filtered by
    //! their channel name, device name, device index (local player) or any combination of the three.
    class InputChannelEventListener : public InputChannelEventNotificationBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Predefined input event listener priority, used to sort handlers from highest to lowest
        inline static AZ::s32 GetPriorityFirst()    { return std::numeric_limits<AZ::s32>::max(); }
        inline static AZ::s32 GetPriorityDefault()  { return 0;                                   }
        inline static AZ::s32 GetPriorityLast()     { return std::numeric_limits<AZ::s32>::min(); }
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Events can be filtered by any combination of channel name, device name, and local player
        class Filter
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////
            //! Wildcard representing any input channel name
            static const AZ::Crc32 AnyChannelNameCrc32;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Wildcard representing any input device name
            static const AZ::Crc32 AnyDeviceNameCrc32;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Wildcard representing any input device index
            static const AZ::u32 AnyDeviceIndex;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            //! \param[in] channelName Crc32 of the input channel name to filter by
            //! \param[in] deviceName Crc32 of the input device name to filter by
            //! \param[in] localPlayer Crc32 of the local player to filter by
            explicit Filter(const AZ::Crc32& channelNameCrc32 = AnyChannelNameCrc32,
                            const AZ::Crc32& deviceNameCrc32 = AnyDeviceNameCrc32,
                            const AZ::u32& deviceIndex = AnyDeviceIndex);

            ////////////////////////////////////////////////////////////////////////////////////////
            // Default copying
            AZ_DEFAULT_COPY(Filter);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Default destructor
            virtual ~Filter() = default;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Check whether an input channel should pass through the filter
            //! \param[in] inputChannel The input channel to be filtered
            //! \return True if the input channel passes the filter, false otherwise
            virtual bool DoesPassFilter(const InputChannel& inputChannel) const;

        private:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Variables
            AZ::Crc32 m_channelNameCrc32; //!< Crc32 of the input channel name to filter by
            AZ::Crc32 m_deviceNameCrc32;  //!< Crc32 of the input device name to filter by
            AZ::u32   m_deviceIndex;      //!< Index of the input device to filter by
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        InputChannelEventListener();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] autoConnect Whether to connect to the input notification bus on construction
        explicit InputChannelEventListener(bool autoConnect);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] priority The priority used to sort relative to other input event listeners
        explicit InputChannelEventListener(AZ::s32 priority);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] priority The priority used to sort relative to other input event listeners
        //! \param[in] autoConnect Whether to connect to the input notification bus on construction
        explicit InputChannelEventListener(AZ::s32 priority, bool autoConnect);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] filter The filter used to determine whether an inut event should be handled
        explicit InputChannelEventListener(const Filter& filter);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] filter The filter used to determine whether an inut event should be handled
        //! \param[in] priority The priority used to sort relative to other input event listeners
        explicit InputChannelEventListener(const Filter& filter, AZ::s32 priority);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] filter The filter used to determine whether an inut event should be handled
        //! \param[in] priority The priority used to sort relative to other input event listeners
        //! \param[in] autoConnect Whether to connect to the input notification bus on construction
        explicit InputChannelEventListener(const Filter& filter, AZ::s32 priority, bool autoConnect);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Default copying
        AZ_DEFAULT_COPY(InputChannelEventListener);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputChannelEventListener() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelEventNotifications::GetPriority
        AZ::s32 GetPriority() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Allow the filter to be set as necessary even if already connected to the input event bus
        //! \param[in] filter The filter used to determine whether an inut event should be handled
        void SetFilter(const Filter& filter);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Connect to the input notification bus to start receiving input notifications
        void Connect();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Disconnect from the input notification bus to stop receiving input notifications
        void Disconnect();

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelEventNotifications::OnInputChannelEvent
        void OnInputChannelEvent(const InputChannel& inputChannel, bool& o_hasBeenConsumed) final;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when an input channel is active or its state or value is updated,
        //! unless the event was consumed by a higher priority listener, or did not pass the filter.
        //! \param[in] inputChannel The input channel that is active or whose state or value updated
        //! \return True if the input event has been consumed, false otherwise
        ////////////////////////////////////////////////////////////////////////////////////////////
        virtual bool OnInputChannelEventFiltered(const InputChannel& inputChannel) = 0;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        Filter  m_filter;   //!< Filter used to determine whether an inut event should be handled
        AZ::s32 m_priority; //!< The priority used to sort relative to other input event listeners
    };
} // namespace AzFramework

// End: fix for windows defining max/min macros
#pragma pop_macro("max")
#pragma pop_macro("min")
