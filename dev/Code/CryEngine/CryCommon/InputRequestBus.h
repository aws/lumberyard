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
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/utils.h>
#include <InputTypes.h>

namespace AZ
{
    /** 
     * This class is used both as the profile id for customizing a particular
     * input as well as the argument for what the new profile will be.
    */
    struct EditableInputRecord
    {
        AZ_TYPE_INFO(EditableInputRecord, "{86B216E5-D40D-474A-8EE7-629591EC75EE}");
        EditableInputRecord() = default;
        EditableInputRecord(const AzFramework::LocalUserId& localUserId,
                            const Input::ProcessedEventName& eventGroup,
                            const AZStd::string& deviceName,
                            const AZStd::string& inputName)
            : m_localUserId(localUserId)
            , m_eventGroup(eventGroup)
            , m_deviceName(deviceName)
            , m_inputName(inputName)
        {}

        bool operator==(const EditableInputRecord& rhs) const
        {
            return m_localUserId == rhs.m_localUserId
                && m_eventGroup == rhs.m_eventGroup
                && m_deviceName == rhs.m_deviceName
                && m_inputName == rhs.m_inputName;
        }

        AzFramework::LocalUserId m_localUserId;
        Input::ProcessedEventName m_eventGroup;
        AZStd::string m_deviceName;
        AZStd::string m_inputName;
    };
    using EditableInputRecords = AZStd::vector<EditableInputRecord>;

    /**
     * With this bus you can request a list of all editable inputs
    */
    class GlobalInputRecordRequests
        : public AZ::EBusTraits
    {
    public:
        virtual void GatherEditableInputRecords(EditableInputRecords& outResults) = 0;
    };
    using GlobalInputRecordRequestBus = AZ::EBus<GlobalInputRecordRequests>;

    /**
     * With this bus you can change an input binding at run time
     * This uses the EditableInputRecord as the bus id which will combine
     * all of the necessary information to uniquely identify the input you
     * are trying to Set
    */ 
    class InputRecordRequests
        : public AZ::EBusTraits
    {
    public:
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef EditableInputRecord BusIdType;
        virtual void SetInputRecord(const EditableInputRecord& newInputRecord) = 0;
    };
    using InputRecordRequestBus = AZ::EBus<InputRecordRequests>;

    /**
     * With this bus you can query for registered device names, as well as their
     * registered inputs
    */ 
    class InputRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~InputRequests() = default;

        // ToDo: This file should be moved to:
        // Gems/InputManagementFramework/Code/Include/InputManagementFramework/InputRequestBus.h
        
        /**
        * This will push the desired context onto the top of the input context stack making it active
        */
        virtual void PushContext(const AZStd::string&) = 0;

        /**
        * This will pop the top context from the input context stack, the new top will become the active context
        */
        virtual void PopContext() = 0;

        /**
        * This will pop all contexts from the input context stack.  The stack will be empty and the default "" context will be active
        */
        virtual void PopAllContexts() = 0;

        /**
        * This will return the name of the top of the input context stack
        */
        virtual AZStd::string GetCurrentContext() = 0;

        /**
        * This will return a list of the current context stack names.  
        * The first element in the list is the bottom of the stack and the last element is the top of the input context stack
        */
        virtual AZStd::vector<AZStd::string> GetContextStack() = 0;
    };

    using InputRequestBus = AZ::EBus<InputRequests>;

    /**
    * Use this bus to send messages to an input component on an entity
    */
    class InputComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~InputComponentRequests() = default;

        /**
        * Set the local user id that the input component should process input from
        */
        virtual void SetLocalUserId(AzFramework::LocalUserId localUserId) = 0;
    };
    using InputComponentRequestBus = AZ::EBus<InputComponentRequests>;
} // namespace AZ


namespace AZStd
{
    template <>
    struct hash <AZ::EditableInputRecord>
    {
        inline size_t operator()(const AZ::EditableInputRecord& record) const
        {
            size_t retVal = static_cast<size_t>(record.m_eventGroup);
            AZStd::hash_combine(retVal, record.m_deviceName);
            AZStd::hash_combine(retVal, record.m_inputName);
            return retVal;
        }
    };
}
