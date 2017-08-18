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
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Crc.h>
#include "PlayerProfileRequestBus.h"

namespace AZ
{
    class InputEventNotificationId
    {
    public:
        AZ_TYPE_INFO(InputEventNotificationId, "{9E0F0801-348B-4FF9-AF9B-858D59404968}");
        InputEventNotificationId() = default;
        InputEventNotificationId(const Input::ProfileId& profileId, Input::ProcessedEventName actionNameCrc)
            : m_profileIdCrc(profileId)
            , m_actionNameCrc(actionNameCrc)
        { }
        InputEventNotificationId(const AZ::Crc32&  entityChannel, const char* actionName) : InputEventNotificationId(entityChannel, AZ::Crc32(actionName)) { }
        InputEventNotificationId(const Input::ProcessedEventName& actionName)
            : m_actionNameCrc(actionName)
        {
            const char* currentProfileName = nullptr;
            EBUS_EVENT_RESULT(currentProfileName, AZ::PlayerProfileRequestBus, GetCurrentProfileForCurrentUser);
            m_profileIdCrc = Input::ProfileId(currentProfileName);
        }
        InputEventNotificationId(const char* actionName) : InputEventNotificationId(AZ::Crc32(actionName)) {}
        bool operator==(const InputEventNotificationId& rhs) const { return m_profileIdCrc == rhs.m_profileIdCrc && m_actionNameCrc == rhs.m_actionNameCrc; }
        InputEventNotificationId Clone() const { return *this; }
        AZStd::string ToString() const { return AZStd::string::format("%lu, %lu", static_cast<AZ::u32>(m_profileIdCrc), static_cast<AZ::u32>(m_actionNameCrc)); }

        Input::ProfileId m_profileIdCrc;
        Input::ProcessedEventName m_actionNameCrc;
    };

    //////////////////////////////////////////////////////////////////////////
    /// The Input Event Notification bus is used to alert systems that an input event
    /// has been processed
    //////////////////////////////////////////////////////////////////////////
    class InputEventNotifications
        : public AZ::EBusTraits
    {
    public:
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef InputEventNotificationId BusIdType;
        virtual ~InputEventNotifications() = default;

        virtual void OnPressed(float /*Value*/) {}
        virtual void OnHeld(float /*Value*/) {}
        virtual void OnReleased(float /*Value*/) {}
    };
    using InputEventNotificationBus = AZ::EBus<InputEventNotifications>;

} // namespace AZ


namespace AZStd
{
    template <>
    struct hash <AZ::InputEventNotificationId>
    {
        inline size_t operator()(const AZ::InputEventNotificationId& actionId) const
        {
            size_t retVal = static_cast<size_t>(actionId.m_profileIdCrc);
            AZStd::hash_combine(retVal, actionId.m_actionNameCrc);
            return retVal;
        }
    };
}