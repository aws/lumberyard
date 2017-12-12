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

#include <AzFramework/Input/Events/InputChannelEventListener.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    const AZ::Crc32 InputChannelEventListener::Filter::AnyChannelNameCrc32("wildcard_any_input_channel_name");
    const AZ::Crc32 InputChannelEventListener::Filter::AnyDeviceNameCrc32("wildcard_any_input_device_name");
    const AZ::u32 InputChannelEventListener::Filter::AnyDeviceIndex(std::numeric_limits<AZ::u32>::max());

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::Filter::Filter(const AZ::Crc32& channelNameCrc32,
                                              const AZ::Crc32& deviceNameCrc32,
                                              const AZ::u32& deviceIndex)
        : m_channelNameCrc32(channelNameCrc32)
        , m_deviceNameCrc32(deviceNameCrc32)
        , m_deviceIndex(deviceIndex)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannelEventListener::Filter::DoesPassFilter(const InputChannel& inputChannel) const
    {
        if (m_channelNameCrc32 != AnyChannelNameCrc32 &&
            m_channelNameCrc32 != inputChannel.GetInputChannelId().GetNameCrc32())
        {
            return false;
        }

        if (m_deviceNameCrc32 != AnyDeviceNameCrc32 &&
            m_deviceNameCrc32 != inputChannel.GetInputDevice().GetInputDeviceId().GetNameCrc32())
        {
            return false;
        }

        if (m_deviceIndex != AnyDeviceIndex &&
            m_deviceIndex != inputChannel.GetInputDevice().GetInputDeviceId().GetIndex())
        {
            return false;
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener()
        : m_filter()
        , m_priority(GetPriorityDefault())
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(bool autoConnect)
        : m_filter()
        , m_priority(GetPriorityDefault())
    {
        if (autoConnect)
        {
            Connect();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(AZ::s32 priority)
        : m_filter()
        , m_priority(priority)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(AZ::s32 priority, bool autoConnect)
        : m_filter()
        , m_priority(priority)
    {
        if (autoConnect)
        {
            Connect();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(const Filter& filter)
        : m_filter(filter)
        , m_priority(GetPriorityDefault())
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(const Filter& filter, AZ::s32 priority)
        : m_filter(filter)
        , m_priority(priority)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(const Filter& filter, AZ::s32 priority, bool autoConnect)
        : m_filter(filter)
        , m_priority(priority)
    {
        if (autoConnect)
        {
            Connect();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::s32 InputChannelEventListener::GetPriority() const
    {
        return m_priority;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventListener::SetFilter(const Filter& filter)
    {
        m_filter = filter;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventListener::Connect()
    {
        InputChannelEventNotificationBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventListener::Disconnect()
    {
        InputChannelEventNotificationBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventListener::OnInputChannelEvent(const InputChannel& inputChannel,
                                                        bool& o_hasBeenConsumed)
    {
        if (o_hasBeenConsumed)
        {
            return;
        }

        const bool doesPassFilter = m_filter.DoesPassFilter(inputChannel);
        if (!doesPassFilter)
        {
            return;
        }

        o_hasBeenConsumed = OnInputChannelEventFiltered(inputChannel);
    }
} // namespace AzFramework
