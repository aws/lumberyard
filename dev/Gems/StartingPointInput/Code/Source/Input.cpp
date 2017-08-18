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
#include "StdAfx.h"
#include "Input.h"
#include "LyToAzInputNameConversions.h"
#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzFramework/Input/Buses/Requests/InputDeviceRequestBus.h>

// CryCommon includes
#include <PlayerProfileRequestBus.h>


namespace Input
{
#if defined(AZ_FRAMEWORK_INPUT_ENABLED)
    static const int s_inputVersion = 2;
#else
    static const int s_inputVersion = 1;
#endif // defined(AZ_FRAMEWORK_INPUT_ENABLED)

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

#if defined(AZ_FRAMEWORK_INPUT_ENABLED)
    bool Input::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
    {
        const bool isPressed = fabs(inputChannel.GetValue()) > m_deadZone;
        const float value = inputChannel.GetValue() * m_eventValueMultiplier;
        if (!m_wasPressed && isPressed)
        {
            AZ::InputEventNotificationBus::Event(m_outgoingBusId, &AZ::InputEventNotifications::OnPressed, value);
        }
        else if (m_wasPressed && isPressed)
        {
            AZ::InputEventNotificationBus::Event(m_outgoingBusId, &AZ::InputEventNotifications::OnHeld, value);
        }
        else if (m_wasPressed && !isPressed)
        {
            AZ::InputEventNotificationBus::Event(m_outgoingBusId, &AZ::InputEventNotifications::OnReleased, value);
        }
        m_wasPressed = isPressed;

        // Return false so we don't consume the event. This should perhaps be a configurable option?
        return false;
    }
#else
    void Input::OnNotifyInputEvent(const SInputEvent& completeInputEvent)
    {
        EInputState inputState = completeInputEvent.state;
        if (completeInputEvent.state == eIS_Changed)
        {
            bool isAnalogBeingHeld = fabs(completeInputEvent.value) > m_deadZone;
            bool wasAnalogBeingHeld = fabs(m_lastKnownEventValue) > m_deadZone;
            if (!wasAnalogBeingHeld && isAnalogBeingHeld)
            {
                inputState = eIS_Pressed;
            }
            else if (!isAnalogBeingHeld && wasAnalogBeingHeld)
            {
                inputState = eIS_Released;
            }
            else
            {
                inputState = eIS_Down;
            }
        }
        m_lastKnownEventValue = completeInputEvent.value;

        if (inputState == eIS_Pressed)
        {
            AZ::InputEventNotificationBus::Event(m_outgoingBusId, &AZ::InputEventNotifications::OnPressed, m_lastKnownEventValue * m_eventValueMultiplier);
            AZ::TickBus::Handler::BusConnect();
        }
        else if (inputState == eIS_Released)
        {
            AZ::TickBus::Handler::BusDisconnect();
            AZ::InputEventNotificationBus::Event(m_outgoingBusId, &AZ::InputEventNotifications::OnReleased, m_lastKnownEventValue * m_eventValueMultiplier);
        }
    }

    void Input::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        AZ::InputEventNotificationBus::Event(m_outgoingBusId, &AZ::InputEventNotifications::OnHeld, m_lastKnownEventValue * m_eventValueMultiplier);
    }
#endif // defined(AZ_FRAMEWORK_INPUT_ENABLED)

    void Input::Activate(const AZ::InputEventNotificationId& eventNotificationId)
    {
#if defined(AZ_FRAMEWORK_INPUT_ENABLED)
        const AZ::Crc32 channelNameFilter(m_inputName.c_str());
        const AZ::Crc32 deviceNameFilter(m_inputDeviceType.c_str());
        AZ::u32 deviceIndexFilter = Filter::AnyDeviceIndex;
        if (eventNotificationId.m_profileIdCrc != BroadcastProfile)
        {
            AZ::u8 mappedDeviceIndex = 0;
            AZ::InputRequestBus::BroadcastResult(mappedDeviceIndex,
                                                 &AZ::InputRequests::GetMappedDeviceIndex,
                                                 eventNotificationId.m_profileIdCrc);
            deviceIndexFilter = mappedDeviceIndex;
        }
        const Filter filter(channelNameFilter, deviceNameFilter, deviceIndexFilter);
        InputChannelEventListener::SetFilter(filter);
        InputChannelEventListener::Connect();
        m_wasPressed = false;
#else
        m_lastKnownEventValue = 0;
        const AZ::InputNotificationId incomingBusId(eventNotificationId.m_profileIdCrc, m_inputName.c_str());
        AZ::InputNotificationBus::Handler::BusConnect(incomingBusId);
#endif // defined(AZ_FRAMEWORK_INPUT_ENABLED)

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
#if defined(AZ_FRAMEWORK_INPUT_ENABLED)
        if (m_wasPressed)
        {
            AZ::InputEventNotificationBus::Event(m_outgoingBusId, &AZ::InputEventNotifications::OnReleased, 0.0f);
        }
        InputChannelEventListener::Disconnect();
#else
        if (AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusDisconnect();
            m_lastKnownEventValue = 0;
            AZ::InputEventNotificationBus::Event(m_outgoingBusId, &AZ::InputEventNotifications::OnHeld, m_lastKnownEventValue * m_eventValueMultiplier);
        }
        const AZ::InputNotificationId incomingBusId(eventNotificationId.m_profileIdCrc, m_inputName.c_str());
        AZ::InputNotificationBus::Handler::BusDisconnect(incomingBusId);
#endif // defined(AZ_FRAMEWORK_INPUT_ENABLED)
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
#if defined(AZ_FRAMEWORK_INPUT_ENABLED)
        AzFramework::InputDeviceRequests::InputDeviceIdSet availableInputDeviceIds;
        AzFramework::InputDeviceRequestBus::Broadcast(&AzFramework::InputDeviceRequests::GetInputDeviceIds,
                                                      availableInputDeviceIds);
        for (const AzFramework::InputDeviceId& inputDeviceId : availableInputDeviceIds)
        {
            retval.push_back(inputDeviceId.GetName());
        }
#else
        AZ::InputRequestBus::BroadcastResult(retval, &AZ::InputRequests::GetRegisteredDeviceList);
#endif // defined(AZ_FRAMEWORK_INPUT_ENABLED)
        return retval;
    }

    const AZStd::vector<AZStd::string> Input::GetInputNamesBySelectedDevice() const
    {
        AZStd::vector<AZStd::string> retval;
#if defined(AZ_FRAMEWORK_INPUT_ENABLED)
        AzFramework::InputDeviceId selectedDeviceId(m_inputDeviceType.c_str());
        AzFramework::InputDeviceRequests::InputChannelIdSet availableInputChannelIds;
        AzFramework::InputDeviceRequestBus::Event(selectedDeviceId,
                                                  &AzFramework::InputDeviceRequests::GetInputChannelIds,
                                                  availableInputChannelIds);
        for (const AzFramework::InputChannelId& inputChannelId : availableInputChannelIds)
        {
            retval.push_back(inputChannelId.GetName());
        }
#else
        AZ::InputRequestBus::BroadcastResult(retval, &AZ::InputRequests::GetInputListByDevice, m_inputDeviceType);
#endif // defined(AZ_FRAMEWORK_INPUT_ENABLED)
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
