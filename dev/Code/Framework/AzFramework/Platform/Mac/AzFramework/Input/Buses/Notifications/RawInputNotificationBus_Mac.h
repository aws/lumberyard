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

@class NSEvent;

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to listen for raw osx input events broadcast by the system. Applications
    //! that want raw osx events to be processed by the AzFramework input system must broadcast all
    //! events received when pumping the NSEvent loop, which is the lowest level we can access input.
    //!
    //! It's possible to receive multiple events per button/key per frame and (depending on how the
    //! NSEvent event loop is pumped) it is also possible that events could be sent from any thread,
    //! however it is assumed they'll always be dispatched on the main thread which is the standard.
    //!
    //! This EBus is intended primarily for the AzFramework input system to process osx input events.
    //! Most systems that need to process input should use the generic AzFramework input interfaces,
    //! but if necessary it is perfectly valid to connect directly to this EBus for raw osx events.
    class RawInputNotificationsOsx : public AZ::EBusTraits
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
        virtual ~RawInputNotificationsOsx() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process raw input events (assumed to be dispatched on the main thread)
        //! \param[in] nsEvent The raw event data
        virtual void OnRawInputEvent(const NSEvent* /*nsEvent*/) = 0;
    };
    using RawInputNotificationBusOsx = AZ::EBus<RawInputNotificationsOsx>;
} // namespace AzFramework
