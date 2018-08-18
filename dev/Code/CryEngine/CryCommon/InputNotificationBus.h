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
#include <InputTypes.h>
#include <AzCore/std/utils.h>
#include <AzCore/EBus/EBus.h>
#include "InputRequestBus.h"

namespace AZ
{
    // ToDo: This file should be moved to:
    // Gems/InputManagementFramework/Code/Include/InputManagementFramework/InputNotificationBus.h

    //////////////////////////////////////////////////////////////////////////
    /// This bus is for listening to the fluctuation of the current input context
    /// The InputRequestBus can be used to push and pop contexts from the stack
    /// "" is the default context that is used when explicitly pushed or when the stack is empty
    class InputContextNotifications
        : public AZ::EBusTraits
    {
    public:
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef AZ::Crc32 BusIdType;

        /// This is called whenever the top of the input context stack has changed (pushed or popped)
        virtual void OnInputContextActivated() {}

        /// This is called whenever the the top of the input context stack is popped
        virtual void OnInputContextDeactivated() {}
        virtual ~InputContextNotifications() = default;


        //////////////////////////////////////////////////////////////////////////
        /// This connection policy will cause OnInputContextActivated to be called if 
        /// it is connecting to the currently active context
        template<class Bus>
        struct ConnectionPolicy
            : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, const typename Bus::BusIdType& id = 0)
            {
                AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, id);

                if (AZ::InputRequestBus::FindFirstHandler())
                {
                    AZStd::string currentInputContext;
                    AZ::InputRequestBus::BroadcastResult(currentInputContext, &AZ::InputRequestBus::Events::GetCurrentContext);
                    if (AZ::Crc32(currentInputContext.c_str()) == id)
                    {
                        handler->OnInputContextActivated();
                    }
                }
            }
        };
    };
    using InputContextNotificationBus = AZ::EBus<InputContextNotifications>;
} // namespace AZ
