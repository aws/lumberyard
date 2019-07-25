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
#include <AzFramework/Input/Devices/InputDevice.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class that filters input events by channel name, device name, local user id, or
    //! any combination of the three.
    class InputChannelEventFilter
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Wildcard representing any input channel name
        static const AZ::Crc32 AnyChannelNameCrc32;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Wildcard representing any input device name
        static const AZ::Crc32 AnyDeviceNameCrc32;

        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_DEPRECATED(static const AZ::u32 AnyDeviceIndex, "Deprecated, please use LocalUserIdAny");

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        InputChannelEventFilter() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Default copying
        AZ_DEFAULT_COPY(InputChannelEventFilter);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputChannelEventFilter() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Check whether an input channel should pass through the filter
        //! \param[in] inputChannel The input channel to be filtered
        //! \return True if the input channel passes the filter, false otherwise
        virtual bool DoesPassFilter(const InputChannel& inputChannel) const = 0;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class that filters input channel events based on whitelisted input channels and devices.
    class InputChannelEventFilterWhitelist : public InputChannelEventFilter
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor. By default the filter will be constructed to whitelist all input events.
        //! \param[in] channelNameCrc32 The input channel name (Crc32) to whitelist (default any)
        //! \param[in] deviceNameCrc32 The input device name (Crc32) to whitelist (default any)
        //! \param[in] localUserId The local user id to whitelist (default any)
        explicit InputChannelEventFilterWhitelist(const AZ::Crc32& channelNameCrc32 = AnyChannelNameCrc32,
                                                  const AZ::Crc32& deviceNameCrc32 = AnyDeviceNameCrc32,
                                                  const LocalUserId& localUserId = LocalUserIdAny);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Default copying
        AZ_DEFAULT_COPY(InputChannelEventFilterWhitelist);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputChannelEventFilterWhitelist() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelEventFilter::DoesPassFilter
        bool DoesPassFilter(const InputChannel& inputChannel) const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add an input channel name to the whitelist
        //! \param[in] channelNameCrc32 The input channel name (Crc32) to add to the whitelist
        void WhitelistChannelName(const AZ::Crc32& channelNameCrc32);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add an input device name to the whitelist
        //! \param[in] channelNameCrc32 The input device name (Crc32) to add to the whitelist
        void WhitelistDeviceName(const AZ::Crc32& deviceNameCrc32);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add a local user id to the whitelist
        //! \param[in] localUserId The local user id to add to the whitelist
        void WhitelistLocalUserId(LocalUserId localUserId);

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::unordered_set<AZ::Crc32>   m_channelNameCrc32Whitelist; //!< Channel name whitelist
        AZStd::unordered_set<AZ::Crc32>   m_deviceNameCrc32Whitelist;  //!< Device name whitelist
        AZStd::unordered_set<LocalUserId> m_localUserIdWhitelist;      //!< Local user whitelist
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class that filters input channel events based on blacklisted input channels and devices.
    class InputChannelEventFilterBlacklist : public InputChannelEventFilter
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor. By default the filter will be constructed to blacklist no input events.
        InputChannelEventFilterBlacklist();

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Default copying
        AZ_DEFAULT_COPY(InputChannelEventFilterBlacklist);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputChannelEventFilterBlacklist() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelEventFilter::DoesPassFilter
        bool DoesPassFilter(const InputChannel& inputChannel) const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add an input channel name to the blacklist
        //! \param[in] channelNameCrc32 The input channel name (Crc32) to add to the blacklist
        void BlacklistChannelName(const AZ::Crc32& channelNameCrc32);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add an input device name to the blacklist
        //! \param[in] channelNameCrc32 The input device name (Crc32) to add to the blacklist
        void BlacklistDeviceName(const AZ::Crc32& deviceNameCrc32);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add a local user id to the blacklist
        //! \param[in] localUserId The local user id to to add to the blacklist
        void BlacklistLocalUserId(LocalUserId localUserId);

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::unordered_set<AZ::Crc32>   m_channelNameCrc32Blacklist; //!< Channel name blacklist
        AZStd::unordered_set<AZ::Crc32>   m_deviceNameCrc32Blacklist;  //!< Device name blacklist
        AZStd::unordered_set<LocalUserId> m_localUserIdBlacklist;      //!< Local user blacklist
    };
} // namespace AzFramework
