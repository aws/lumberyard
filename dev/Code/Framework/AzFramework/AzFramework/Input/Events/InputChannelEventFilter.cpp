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

#include <AzFramework/Input/Events/InputChannelEventFilter.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    const AZ::Crc32 InputChannelEventFilter::AnyChannelNameCrc32("wildcard_any_input_channel_name");
    const AZ::Crc32 InputChannelEventFilter::AnyDeviceNameCrc32("wildcard_any_input_device_name");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventFilterWhitelist::InputChannelEventFilterWhitelist(
        const AZ::Crc32& channelNameCrc32, // AnyChannelNameCrc32
        const AZ::Crc32& deviceNameCrc32,  // AnyDeviceNameCrc32
        const LocalUserId& localUserId)    // LocalUserIdAny
    : m_channelNameCrc32Whitelist()
    , m_deviceNameCrc32Whitelist()
    , m_localUserIdWhitelist()
    {
        WhitelistChannelName(channelNameCrc32);
        WhitelistDeviceName(deviceNameCrc32);
        WhitelistLocalUserId(localUserId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannelEventFilterWhitelist::DoesPassFilter(const InputChannel& inputChannel) const
    {
        if (!m_channelNameCrc32Whitelist.empty())
        {
            const AZ::Crc32& channelName = inputChannel.GetInputChannelId().GetNameCrc32();
            if (m_channelNameCrc32Whitelist.find(channelName) == m_channelNameCrc32Whitelist.end())
            {
                return false;
            }
        }

        if (!m_deviceNameCrc32Whitelist.empty())
        {
            const AZ::Crc32& deviceName = inputChannel.GetInputDevice().GetInputDeviceId().GetNameCrc32();
            if (m_deviceNameCrc32Whitelist.find(deviceName) == m_deviceNameCrc32Whitelist.end())
            {
                return false;
            }
        }

        if (!m_localUserIdWhitelist.empty())
        {
            const LocalUserId localUserId = inputChannel.GetInputDevice().GetAssignedLocalUserId();
            if (m_localUserIdWhitelist.find(localUserId) == m_localUserIdWhitelist.end())
            {
                return false;
            }
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventFilterWhitelist::WhitelistChannelName(const AZ::Crc32& channelNameCrc32)
    {
        if (channelNameCrc32 == AnyChannelNameCrc32)
        {
            m_channelNameCrc32Whitelist.clear();
        }
        else
        {
            m_channelNameCrc32Whitelist.insert(channelNameCrc32);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventFilterWhitelist::WhitelistDeviceName(const AZ::Crc32& deviceNameCrc32)
    {
        if (deviceNameCrc32 == AnyDeviceNameCrc32)
        {
            m_deviceNameCrc32Whitelist.clear();
        }
        else
        {
            m_deviceNameCrc32Whitelist.insert(deviceNameCrc32);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventFilterWhitelist::WhitelistLocalUserId(LocalUserId localUserId)
    {
        if (localUserId == LocalUserIdAny)
        {
            m_localUserIdWhitelist.clear();
        }
        else
        {
            m_localUserIdWhitelist.insert(localUserId);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventFilterBlacklist::InputChannelEventFilterBlacklist()
        : m_channelNameCrc32Blacklist()
        , m_deviceNameCrc32Blacklist()
        , m_localUserIdBlacklist()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannelEventFilterBlacklist::DoesPassFilter(const InputChannel& inputChannel) const
    {
        const AZ::Crc32& channelName = inputChannel.GetInputChannelId().GetNameCrc32();
        if (m_channelNameCrc32Blacklist.find(channelName) != m_channelNameCrc32Blacklist.end())
        {
            return false;
        }

        const AZ::Crc32& deviceName = inputChannel.GetInputDevice().GetInputDeviceId().GetNameCrc32();
        if (m_deviceNameCrc32Blacklist.find(deviceName) != m_deviceNameCrc32Blacklist.end())
        {
            return false;
        }

        const LocalUserId localUserId = inputChannel.GetInputDevice().GetAssignedLocalUserId();
        if (m_localUserIdBlacklist.find(localUserId) != m_localUserIdBlacklist.end())
        {
            return false;
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventFilterBlacklist::BlacklistChannelName(const AZ::Crc32& channelNameCrc32)
    {
        m_channelNameCrc32Blacklist.insert(channelNameCrc32);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventFilterBlacklist::BlacklistDeviceName(const AZ::Crc32& deviceNameCrc32)
    {
        m_deviceNameCrc32Blacklist.insert(deviceNameCrc32);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventFilterBlacklist::BlacklistLocalUserId(LocalUserId localUserId)
    {
        m_localUserIdBlacklist.insert(localUserId);
    }
} // namespace AzFramework
