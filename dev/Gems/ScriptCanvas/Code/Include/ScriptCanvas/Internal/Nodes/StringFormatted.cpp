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

#include <AzCore/std/string/regex.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/Utils.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Internal
        {
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
                            const Datum* datum = GetInput(slot);

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

            void StringFormatted::ParseFormat()
            {
                // Used to defer the removal of slots and only remove those slots that actually need to be removed at the end of the format parsing operation.
                AZStd::unordered_set<SlotId> slotsToRemove;

                // Mark all existing slots as potential candidates for removal.
                for (const auto& entry : m_formatSlotMap)
                {
                    slotsToRemove.insert(entry.second);
                }

                // Clear the utility containers.
                m_arrayBindingMap.clear();
                m_unresolvedString.clear();
                m_formatSlotMap.clear();

                static AZStd::regex formatRegex(R"(\{(\w+)\})");
                AZStd::smatch match;

                AZStd::string format = m_format;
                while (AZStd::regex_search(format, match, formatRegex))
                {
                    m_unresolvedString.push_back(match.prefix().str());

                    AZStd::string name = match[1].str();
                    AZStd::string tooltip = AZStd::string::format("Value which replaces instances of {%s} in the resulting string.", match[1].str().c_str());

                    // If the slot has never existed, create it
                    auto pair = m_formatSlotMap.emplace(AZStd::make_pair(name, AddInputDatumOverloadedSlot(name, tooltip)));

                    const auto& slotId = pair.first->second;

                    m_arrayBindingMap.emplace(AZStd::make_pair(m_unresolvedString.size(), slotId));
                    m_unresolvedString.push_back(""); // blank placeholder, will be filled when the data slot is resolved.

                    // Remove the slot from the list of slots that should be removed.
                    slotsToRemove.erase(slotId);

                    format = match.suffix();
                }

                // If there's some left over in the match make sure it gets recorded.
                if (!format.empty())
                {
                    m_unresolvedString.push_back(format);
                }

                // If there are still any items in slotsToRemove it means they really need to be removed.
                for (const auto& slot : slotsToRemove)
                {
                    RemoveSlot(slot);

                    // Make sure we remove them from the arrayBinding as they should not be considered for 
                    // resolving the final text.
                    for (ArrayBindingMap::iterator it = m_arrayBindingMap.begin(); it != m_arrayBindingMap.end();)
                    {
                        if (it->second == slot)
                        {
                            it = m_arrayBindingMap.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }
            }

            AZ::u32 StringFormatted::OnFormatChanged()
            {
                // Adding and removing slots within the ChangeNotify handler causes problems due to the property grid's
                // rows changing. To get around that problem we queue the parsing of the string format to happen
                // on the next system tick.
                AZ::SystemTickBus::QueueFunction([this]() { ParseFormat(); });

                return AZ::Edit::PropertyRefreshLevels::None;
            }

            void StringFormatted::OnInit()
            {
                // Parse the serialized format to make sure the utility containers are properly configured for lookup at runtime.
                ParseFormat();

                // DYNAMIC_SLOT_VERSION_CONVERTER
                for (auto slotPair : m_formatSlotMap)
                {
                    Slot* slot = GetSlot(slotPair.second);

                    if (slot && !slot->IsDynamicSlot())
                    {
                        slot->SetDynamicDataType(DynamicDataType::Any);
                    }
                }
                ////
            }
        }
    }
}

#include <Include/ScriptCanvas/Internal/Nodes/StringFormatted.generated.cpp>