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
#include "IInput.h"
#include <InputTypes.h>
#include <AzCore/std/utils.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzFramework/Input/System/InputSystemComponent.h>
#include "InputRequestBus.h"


namespace AZ
{
    // ToDo: When CryInput and the AZ_FRAMEWORK_INPUT_ENABLED #define are removed,
    // the following functions should be deleted and the rest of the file moved to:
    // Gems/InputManagementFramework/Code/Include/InputManagementFramework/InputRequestBus.h
#if !defined(AZ_FRAMEWORK_INPUT_ENABLED)

    /// Profile Owner, Input Key. Ex: 151783871389501, "space"
    /// The invalid EntityId is a wild card for all Entities
    /// The empty event name or 0 crc is the wild card for input event names
    /// when the profileId is the BroadcastProfile you will receive "eventName" events for all entities.
    ///   Use the InputRequestBus if you need the EntityId the message was routed to
    /// when the "inputKey" is "" or 0 then you will get all events for the specified profile
    ///   The event parameter holds the "eventName" that was fired
    /// when both invalid AZ::EnityId and "eventName" of 0 are used you will get all raw input
    struct InputNotificationId
    {
        AZ_TYPE_INFO(InputNotificationId, "{B2CA87F6-F3A0-4C1C-96E8-F8D27DFE53EF}");
        InputNotificationId() = default;
        InputNotificationId(const Input::ProfileId& profileId, Input::RawEventName eventNameCrc)
            : m_profileIdCrc(profileId)
            , m_eventNameCrc(eventNameCrc)
        { }
        InputNotificationId(const Input::ProfileId& profileId, const char* eventName)
            : InputNotificationId(profileId, AZ_CRC(eventName)){}
        bool operator==(const InputNotificationId& rhs) const
        {
            return m_profileIdCrc == rhs.m_profileIdCrc && m_eventNameCrc == rhs.m_eventNameCrc;
        }
        Input::ProfileId m_profileIdCrc;
        Input::RawEventName m_eventNameCrc;
    };

    /// This bus is for listening for particular raw input events routed to an entity.  
    /// 
    /// You will connect to it like this:
    /// AZ::InputNotificationId inputBusId(profileId, "inputKey");
    /// AZ::InputNotificationBus::Handler::BusConnect(inputBusId);
    /// when that "inputKey" is activated on the corresponding profile you will get an OnNotifyInputEvent
    class InputNotification
        : public AZ::EBusTraits
    {
    public:
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef InputNotificationId BusIdType;
        virtual void OnNotifyInputEvent(const SInputEvent& completeInputEvent) = 0;
        virtual ~InputNotification() = default;
    };

    using InputNotificationBus = AZ::EBus<InputNotification>;
    
    /// Use this helper to subscribe to all input events
    inline const InputNotificationId& CreateBroadcastNotificationId() 
    {
        static const InputNotificationId s_broadcastNotificationId = InputNotificationId(Input::BroadcastProfile, Input::AllEvents);
        return s_broadcastNotificationId;
    }

    /// Use this helper to subscribe to specific input events from all profiles. Ex. Press "A" to sign in
    inline InputNotificationId CreateSpecificEventNotificationId(const char* eventName) { return InputNotificationId(Input::BroadcastProfile, eventName); }
    inline InputNotificationId CreateSpecificEventNotificationId(Input::RawEventName eventNameCrc) { return InputNotificationId(Input::BroadcastProfile, eventNameCrc); }

    /// Use this helper to subscribe to all events sent to a specific profile
    inline InputNotificationId CreateAllEventsOnProfileNotificationId(const Input::ProfileId& profileId) { return InputNotificationId(profileId, Input::AllEvents); }

#endif // !defined(AZ_FRAMEWORK_INPUT_ENABLED)

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
                        typename Bus::template CallstackEntryIterator<typename Bus::InterfaceType**> callstack(nullptr, &id); // Workaround for GetCurrentBusId in callee
                        handler->OnInputContextActivated();
                    }
                }
            }
        };
    };
    using InputContextNotificationBus = AZ::EBus<InputContextNotifications>;
} // namespace AZ

#if !defined(AZ_FRAMEWORK_INPUT_ENABLED)
namespace AZStd
{
    template <>
    struct hash <AZ::InputNotificationId>
    {
        inline size_t operator()(const AZ::InputNotificationId& actionId) const
        {
            AZStd::hash<Input::ProfileId> profileHasher;
            size_t retVal = profileHasher(actionId.m_profileIdCrc);
            AZStd::hash_combine(retVal, actionId.m_eventNameCrc);
            return retVal;
        }
    };
} // namespace AZStd
#endif // !defined(AZ_FRAMEWORK_INPUT_ENABLED)