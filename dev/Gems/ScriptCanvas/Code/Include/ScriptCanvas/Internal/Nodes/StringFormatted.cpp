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

#include "StringFormatted.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/Utils.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Internal
        {
            ////////////////////
            // StringFormatted
            ////////////////////            

            void StringFormatted::OnInit()
            {
                bool addedDisplayGroup = false;

                // DISPLAY_GROUP_VERSION_CONVERTER                
                for (auto slotPair : m_formatSlotMap)
                {
                    Slot* slot = GetSlot(slotPair.second);

                    if (slot)
                    {
                        // DYNAMIC_SLOT_VERSION_CONVERTER
                        if (!slot->IsDynamicSlot())
                        {
                            slot->SetDynamicDataType(DynamicDataType::Any);
                        }
                        ////

                        // DISPLAY_GROUP_VERSION_CONVERTER   
                        if (slot->GetDisplayGroup() != GetDisplayGroupId())
                        {
                            addedDisplayGroup = true;
                        }
                        ////
                    }
                }

                if (addedDisplayGroup)
                {
                    RelayoutNode();
                }

                m_stringInterface.SetPropertyReference(&m_format);
                m_stringInterface.RegisterListener(this);
            }

            void StringFormatted::OnConfigured()
            {
                // Parse the serialized format to make sure the utility containers are properly configured for lookup at runtime.
                ParseFormat();
            }

            void StringFormatted::ConfigureVisualExtensions()
            {
                {
                    VisualExtensionSlotConfiguration visualExtensions(VisualExtensionSlotConfiguration::VisualExtensionType::ExtenderSlot);

                    visualExtensions.m_name = "Add Input";
                    visualExtensions.m_tooltip = "Adds an input to the current string format";
                    visualExtensions.m_displayGroup = GetDisplayGroup();
                    visualExtensions.m_connectionType = ConnectionType::Input;

                    visualExtensions.m_identifier = GetExtensionId();

                    RegisterExtension(visualExtensions);
                }

                {
                    VisualExtensionSlotConfiguration visualExtensions(VisualExtensionSlotConfiguration::VisualExtensionType::PropertySlot);

                    visualExtensions.m_name = "";
                    visualExtensions.m_tooltip = "";
                    visualExtensions.m_displayGroup = GetDisplayGroup();
                    visualExtensions.m_connectionType = ConnectionType::Input;

                    visualExtensions.m_identifier = GetPropertyId();

                    RegisterExtension(visualExtensions);
                }
            }

            bool StringFormatted::CanDeleteSlot(const SlotId& slotId) const
            {
                Slot* slot = GetSlot(slotId);

                bool canDeleteSlot = false;

                if (slot && slot->IsData() && slot->IsInput())
                {
                    canDeleteSlot = (slot->GetDisplayGroup() == GetDisplayGroupId());
                }

                return canDeleteSlot;
            }

            SlotId StringFormatted::HandleExtension(AZ::Crc32 extensionId)
            {
                if (extensionId == GetExtensionId())
                {
                    int value = 0;
                    AZStd::string name = "Value";

                    while (m_formatSlotMap.find(name) != m_formatSlotMap.end())
                    {
                        ++value;
                        name = AZStd::string::format("Value_%i", value);                        
                    }

                    m_format.append("{");
                    m_format.append(name);
                    m_format.append("}");

                    m_stringInterface.SignalDataChanged();

                    auto formatMapIter = m_formatSlotMap.find(name);

                    if (formatMapIter != m_formatSlotMap.end())
                    {
                        return formatMapIter->second;
                    }
                }

                return SlotId();
            }

            NodePropertyInterface* StringFormatted::GetPropertyInterface(AZ::Crc32 propertyId)
            {
                if (propertyId == GetPropertyId())
                {
                    return &m_stringInterface;
                }

                return nullptr;
            }

            void StringFormatted::OnSlotRemoved(const SlotId& slotId)
            {
                if (m_parsingFormat)
                {
                    return;
                }

                for (auto slotPair : m_formatSlotMap)
                {
                    if (slotPair.second == slotId)
                    {
                        AZStd::string name = AZStd::string::format("{%s}", slotPair.first.c_str());

                        AZStd::size_t firstInstance = m_format.find(name);

                        if (firstInstance == AZStd::string::npos)
                        {
                            return;
                        }

                        m_format.erase(firstInstance, name.size());                        
                        m_formatSlotMap.erase(slotPair.first);
                        break;
                    }
                }

                m_stringInterface.SignalDataChanged();
            }

            AZStd::string StringFormatted::ProcessFormat()
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::StringFormatted::ProcessFormat");

                AZStd::string text;

                if (!m_format.empty())
                {
                    for (const auto& binding : m_arrayBindingMap)
                    {
                        SlotId slot = binding.second;
                        if (slot.IsValid())
                        {
                            const Datum* datum = FindDatum(slot);

                            // Now push the value into the formatted string at the right spot
                            AZStd::string result;
                            if (datum)
                            {
                                if (datum->GetType().IsValid() && datum->IS_A(Data::Type::Number()))
                                {
                                    m_unresolvedString[binding.first] = AZStd::string::format("%.*lf", m_numericPrecision, (*datum->GetAs<Data::NumberType>()));
                                }
                                else if (datum->GetType().IsValid() && datum->ToString(result))
                                {
                                    m_unresolvedString[binding.first] = result;
                                }
                            }
                        }
                    }

                    for (const auto& str : m_unresolvedString)
                    {
                        text.append(str);
                    }
                }

                return text;
            }

            void StringFormatted::OnPropertyChanged()
            {
                ParseFormat();
            }

            void StringFormatted::RelayoutNode()
            {
                // We have an in and an our slot. So we want to ensure we start off with 2 slots.
                int slotOrder = static_cast<int>(GetSlots().size() - m_formatSlotMap.size()) - 1;

                m_parsingFormat = true;
                for (const auto& entryPair : m_formatSlotMap)
                {
                    const bool removeConnections = false;
                    RemoveSlot(entryPair.second, removeConnections);
                }
                m_parsingFormat = false;

                AZStd::smatch match;
                
                AZStd::string format = m_format;
                while (AZStd::regex_search(format, match, GetRegex()))
                {
                    AZStd::string name = match[1].str();
                    AZStd::string tooltip = AZStd::string::format("Value which replaces instances of {%s} in the resulting string.", match[1].str().c_str());

                    auto formatSlotIter = m_formatSlotMap.find(name);

                    if (formatSlotIter == m_formatSlotMap.end())
                    {                        
                        continue;
                    }

                    DynamicDataSlotConfiguration dataSlotConfiguration;

                    dataSlotConfiguration.m_name = name;
                    dataSlotConfiguration.m_toolTip = tooltip;
                    dataSlotConfiguration.m_displayGroup = GetDisplayGroup();

                    dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
                    dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

                    dataSlotConfiguration.m_addUniqueSlotByNameAndType = true;

                    dataSlotConfiguration.m_slotId = formatSlotIter->second;

                    InsertSlot(slotOrder, dataSlotConfiguration);

                    format = match.suffix();

                    ++slotOrder;
                }
            }

            void StringFormatted::ParseFormat()
            {
                // Used to defer the removal of slots and only remove those slots that actually need to be removed at the end of the format parsing operation.
                AZStd::unordered_map<SlotId, AZStd::string> slotsToRemove;

                // Mark all existing slots as potential candidates for removal.
                for (const auto& entry : m_formatSlotMap)
                {
                    slotsToRemove.emplace(entry.second, entry.first);
                }

                // Going to move around some of the other slots here. But this should at least make it consistent no mater what
                // element was using it.
                int slotOrder = static_cast<int>(GetSlots().size() - m_formatSlotMap.size()) - 1;

                // Clear the utility containers.
                m_arrayBindingMap.clear();
                m_unresolvedString.clear();                

                static AZStd::regex formatRegex(R"(\{(\w+)\})");
                AZStd::smatch match;

                AZStd::string format = m_format;
                while (AZStd::regex_search(format, match, formatRegex))
                {
                    m_unresolvedString.push_back(match.prefix().str());

                    AZStd::string name = match[1].str();
                    AZStd::string tooltip = AZStd::string::format("Value which replaces instances of {%s} in the resulting string.", match[1].str().c_str());

                    SlotId slotId;
                    auto slotIdIter = m_formatSlotMap.find(name);

                    if (slotIdIter != m_formatSlotMap.end())
                    {
                        slotId = slotIdIter->second;
                    }
                    else
                    {
                        // If the slot has never existed, create it
                        DynamicDataSlotConfiguration dataSlotConfiguration;

                        dataSlotConfiguration.m_name = name;
                        dataSlotConfiguration.m_toolTip = tooltip;
                        dataSlotConfiguration.m_displayGroup = GetDisplayGroup();

                        dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
                        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

                        dataSlotConfiguration.m_addUniqueSlotByNameAndType = true;

                        auto pair = m_formatSlotMap.emplace(AZStd::make_pair(name, InsertSlot(slotOrder, dataSlotConfiguration)));

                        slotId = pair.first->second;
                    }

                    m_arrayBindingMap.emplace(AZStd::make_pair(m_unresolvedString.size(), slotId));
                    m_unresolvedString.push_back(""); // blank placeholder, will be filled when the data slot is resolved.

                    // Remove the slot from the list of slots that should be removed.
                    slotsToRemove.erase(slotId);

                    format = match.suffix();

                    ++slotOrder;                    
                }

                // If there's some left over in the match make sure it gets recorded.
                if (!format.empty())
                {
                    m_unresolvedString.push_back(format);
                }

                m_parsingFormat = true;

                // If there are still any items in slotsToRemove it means they really need to be removed.
                for (const auto& slot : slotsToRemove)
                {
                    m_formatSlotMap.erase(slot.second);
                    RemoveSlot(slot.first);

                    // Make sure we remove them from the arrayBinding as they should not be considered for 
                    // resolving the final text.
                    for (ArrayBindingMap::iterator it = m_arrayBindingMap.begin(); it != m_arrayBindingMap.end();)
                    {
                        if (it->second == slot.first)
                        {
                            it = m_arrayBindingMap.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }

                m_parsingFormat = false;
            }

            void StringFormatted::OnFormatChanged()
            {
                // Adding and removing slots within the ChangeNotify handler causes problems due to the property grid's
                // rows changing. To get around that problem we queue the parsing of the string format to happen
                // on the next system tick.
                AZ::SystemTickBus::QueueFunction([this]() {
                    m_stringInterface.SignalDataChanged();
                });
            }
        }
    }
}

#include <Include/ScriptCanvas/Internal/Nodes/StringFormatted.generated.cpp>
