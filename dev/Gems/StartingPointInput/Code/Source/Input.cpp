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
#include "StartingPointInput_precompiled.h"
#include "Input.h"
#include "LyToAzInputNameConversions.h"
#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/sort.h>

#include <AzFramework/Input/Buses/Requests/InputDeviceRequestBus.h>

// CryCommon includes
#include <PlayerProfileRequestBus.h>
#include <InputTypes.h>


namespace Input
{
    static const int s_inputVersion = 2;

    bool ConvertInputVersion1To2(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        const int deviceTypeElementIndex = classElement.FindElement(AZ_CRC("Input Device Type"));
        if (deviceTypeElementIndex == -1)
        {
            AZ_Error("Input", false, "Could not find 'Input Device Type' element");
            return false;
        }

        AZ::SerializeContext::DataElementNode& deviceTypeElementNode = classElement.GetSubElement(deviceTypeElementIndex);
        AZStd::string deviceTypeElementValue;
        if (!deviceTypeElementNode.GetData(deviceTypeElementValue))
        {
            AZ_Error("Input", false, "Could not get 'Input Device Type' element as a string");
            return false;
        }

        const AZStd::string convertedDeviceType = ConvertInputDeviceName(deviceTypeElementValue);
        if (!deviceTypeElementNode.SetData(context, convertedDeviceType))
        {
            AZ_Error("Input", false, "Could not set 'Input Device Type' element as a string");
            return false;
        }

        const int eventNameElementIndex = classElement.FindElement(AZ_CRC("Input Name"));
        if (eventNameElementIndex == -1)
        {
            AZ_Error("Input", false, "Could not find 'Input Name' element");
            return false;
        }

        AZ::SerializeContext::DataElementNode& eventNameElementNode = classElement.GetSubElement(eventNameElementIndex);
        AZStd::string eventNameElementValue;
        if (!eventNameElementNode.GetData(eventNameElementValue))
        {
            AZ_Error("Input", false, "Could not get 'Input Name' element as a string");
            return false;
        }

        const AZStd::string convertedElementName = ConvertInputEventName(eventNameElementValue);
        if (!eventNameElementNode.SetData(context, convertedElementName))
        {
            AZ_Error("Input", false, "Could not set 'Input Name' element as a string");
            return false;
        }

        return true;
    }

    bool ConvertInputVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        int currentUpgradedVersion = classElement.GetVersion();
        while (currentUpgradedVersion < s_inputVersion)
        {
            switch (currentUpgradedVersion)
            {
                case 1:
                {
                    if (ConvertInputVersion1To2(context, classElement))
                    {
                        currentUpgradedVersion = 2;
                    }
                    else
                    {
                        AZ_Warning("Input", false, "Failed to convert Input from version 1 to 2, its data will be lost on save");
                        return false;
                    }
                }
                break;
                case 0:
                default:
                {
                    AZ_Warning("Input", false, "Unable to convert Input: unsupported version %i, its data will be lost on save", currentUpgradedVersion);
                    return false;
                }
            }
        }
        return true;
    }

    void Input::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<Input>()
                ->Version(s_inputVersion, &ConvertInputVersion)
                    ->Field("Input Device Type", &Input::m_inputDeviceType)
                    ->Field("Input Name", &Input::m_inputName)
                    ->Field("Event Value Multiplier", &Input::m_eventValueMultiplier)
                    ->Field("Dead Zone", &Input::m_deadZone);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<Input>("Input", "Hold an input to generate an event")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &Input::GetEditorText)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Input::m_inputDeviceType, "Input Device Type", "The type of input device, ex keyboard")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Input::OnDeviceSelected)
                        ->Attribute(AZ::Edit::Attributes::StringList, &Input::GetInputDeviceTypes)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Input::m_inputName, "Input Name", "The name of the input you want to hold ex. space")
                        ->Attribute(AZ::Edit::Attributes::StringList, &Input::GetInputNamesBySelectedDevice)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(0, &Input::m_eventValueMultiplier, "Event value multiplier", "When the event fires, the value will be scaled by this multiplier")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(0, &Input::m_deadZone, "Dead zone", "An event will only be sent out if the value is above this threshold")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.001f);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
            {
                behaviorContext->EBus<AZ::InputEventNotificationBus>("InputEventNotificationBus")
                    ->Event("OnPressed", &AZ::InputEventNotificationBus::Events::OnPressed)
                    ->Event("OnHeld", &AZ::InputEventNotificationBus::Events::OnHeld)
                    ->Event("OnReleased", &AZ::InputEventNotificationBus::Events::OnReleased);
            }
        }
    }

    Input::Input()
    {
        if (m_inputDeviceType.empty())
        {
            auto&& deviceTypes = GetInputDeviceTypes();
            if (!deviceTypes.empty())
            {
                m_inputDeviceType = deviceTypes[0];
                OnDeviceSelected();
            }
        }
    }

    bool Input::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
    {
        const bool isPressed = fabs(inputChannel.GetValue()) > m_deadZone;
        if (!m_wasPressed && isPressed)
        {
            SendEventsInternal(inputChannel, m_outgoingBusId, &AZ::InputEventNotificationBus::Events::OnPressed);
        }
        else if (m_wasPressed && isPressed)
        {
            SendEventsInternal(inputChannel, m_outgoingBusId, &AZ::InputEventNotificationBus::Events::OnHeld);
        }
        else if (m_wasPressed && !isPressed)
        {
            SendEventsInternal(inputChannel, m_outgoingBusId, &AZ::InputEventNotificationBus::Events::OnReleased);
        }
        m_wasPressed = isPressed;

        // Return false so we don't consume the event. This should perhaps be a configurable option?
        return false;
    }

    void Input::SendEventsInternal(const AzFramework::InputChannel& inputChannel, const AZ::InputEventNotificationId busId, InputEventType eventType)
    {
        const float value = inputChannel.GetValue() * m_eventValueMultiplier;
        AZ::InputEventNotificationBus::Event(busId, eventType, value);
        if (busId.m_profileIdCrc != ::Input::BroadcastProfile)
        {
            AZ::InputEventNotificationId wildCardBusId = AZ::InputEventNotificationId(::Input::BroadcastProfile, busId.m_actionNameCrc);
            AZ::InputEventNotificationBus::Event(wildCardBusId, eventType, value);
        }
    }

    void Input::Activate(const AZ::InputEventNotificationId& eventNotificationId)
    {
        const AZ::Crc32 channelNameFilter(m_inputName.c_str());
        const AZ::Crc32 deviceNameFilter(m_inputDeviceType.c_str());
        AZ::u32 deviceIndexFilter = InputChannelEventFilter::AnyDeviceIndex;
        if (eventNotificationId.m_profileIdCrc != BroadcastProfile)
        {
            AZ::u8 mappedDeviceIndex = 0;
            AZ::InputRequestBus::BroadcastResult(mappedDeviceIndex,
                                                 &AZ::InputRequests::GetMappedDeviceIndex,
                                                 eventNotificationId.m_profileIdCrc);
            deviceIndexFilter = mappedDeviceIndex;
        }
        AZStd::shared_ptr<InputChannelEventFilterWhitelist> filter = AZStd::make_shared<InputChannelEventFilterWhitelist>(channelNameFilter, deviceNameFilter, deviceIndexFilter);
        InputChannelEventListener::SetFilter(filter);
        InputChannelEventListener::Connect();
        m_wasPressed = false;

        m_outgoingBusId = eventNotificationId;
        AZ::GlobalInputRecordRequestBus::Handler::BusConnect();
        AZ::EditableInputRecord editableRecord;
        editableRecord.m_profile = eventNotificationId.m_profileIdCrc;
        editableRecord.m_deviceName = m_inputDeviceType;
        editableRecord.m_eventGroup = eventNotificationId.m_actionNameCrc;
        editableRecord.m_inputName = m_inputName;
        AZ::InputRecordRequestBus::Handler::BusConnect(editableRecord);
    }

    void Input::Deactivate(const AZ::InputEventNotificationId& eventNotificationId)
    {
        if (m_wasPressed)
        {
            AZ::InputEventNotificationBus::Event(m_outgoingBusId, &AZ::InputEventNotifications::OnReleased, 0.0f);
        }
        InputChannelEventListener::Disconnect();
        AZ::GlobalInputRecordRequestBus::Handler::BusDisconnect();
        AZ::InputRecordRequestBus::Handler::BusDisconnect();
    }

    AZStd::string Input::GetEditorText() const
    {
        return m_inputName.empty() ? "<Select input>" : m_inputName;
    }

    const AZStd::vector<AZStd::string> Input::GetInputDeviceTypes() const
    {
        AZStd::vector<AZStd::string> retval;
        AzFramework::InputDeviceRequests::InputDeviceIdSet availableInputDeviceIds;
        AzFramework::InputDeviceRequestBus::Broadcast(&AzFramework::InputDeviceRequests::GetInputDeviceIds,
                                                      availableInputDeviceIds);
        for (const AzFramework::InputDeviceId& inputDeviceId : availableInputDeviceIds)
        {
            retval.push_back(inputDeviceId.GetName());
        }
        return retval;
    }

    const AZStd::vector<AZStd::string> Input::GetInputNamesBySelectedDevice() const
    {
        AZStd::vector<AZStd::string> retval;
        AzFramework::InputDeviceId selectedDeviceId(m_inputDeviceType.c_str());
        AzFramework::InputDeviceRequests::InputChannelIdSet availableInputChannelIds;
        AzFramework::InputDeviceRequestBus::Event(selectedDeviceId,
                                                  &AzFramework::InputDeviceRequests::GetInputChannelIds,
                                                  availableInputChannelIds);
        for (const AzFramework::InputChannelId& inputChannelId : availableInputChannelIds)
        {
            retval.push_back(inputChannelId.GetName());
        }
        AZStd::sort(retval.begin(), retval.end());
        return retval;
    }

    AZ::Crc32 Input::OnDeviceSelected()
    {
        auto&& inputList = GetInputNamesBySelectedDevice();
        if (!inputList.empty())
        {
            m_inputName = inputList[0];
        }
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    void Input::GatherEditableInputRecords(AZ::EditableInputRecords& outResults)
    {
        outResults.push_back(AZ::EditableInputRecord());
    }

    void Input::SetInputRecord(const AZ::EditableInputRecord& newInputRecord)
    {
        Deactivate(m_outgoingBusId);
        m_inputName = newInputRecord.m_inputName;
        m_inputDeviceType = newInputRecord.m_deviceName;
        Activate(m_outgoingBusId);
    }
} // namespace Input
