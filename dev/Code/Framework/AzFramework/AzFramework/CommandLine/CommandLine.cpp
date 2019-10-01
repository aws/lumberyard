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

#include "CommandLine.h"
#include <AzCore/std/string/conversions.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/AzFramework_Traits_Platform.h>

namespace AzFramework
{
    namespace
    {
        // helper utility to return a lower version of the string without altering the original.
        // regular to_lower operates directly on the input.
        AZStd::string ToLower(AZStd::string_view inStr)
        {
            AZStd::string lowerStr = inStr;
            AZStd::to_lower(lowerStr.begin(), lowerStr.end());

            return lowerStr;
        }
    }

    CommandLine::CommandLine()
    {
    }

    void CommandLine::Parse(int argc, char** argv)
    {
        m_switches.clear();
        m_miscValues.clear();

        AZStd::string currentSwitch;
        for (int i = 1; i < argc; ++i)
        {
            if (argv[i])
            {
                AZStd::string currentArg = argv[i]; // this eats the / or -
                StringFunc::Strip(currentArg, " ", false, true, true);
                if (!currentArg.empty())
                {
                    if (AZStd::string_view(currentArg.begin(), currentArg.begin() + 1).find_first_of(AZ_TRAIT_AZFRAMEWORK_COMMAND_LINE_OPTION_PREFIX) != AZStd::string_view::npos)
                    {
                        // its possible that its a key-value-pair like /blah=whatever
                        // we support this too, for compatibilty.

                        currentArg = currentArg.substr(1);
                        if (currentArg[0] == '-') // for -- extra
                        {
                            currentArg = currentArg.substr(1);
                        }

                        AZStd::size_t foundPos = StringFunc::Find(currentArg.c_str(), '=');
                        if (foundPos != AZStd::string::npos)
                        {
                            // its in '=' format
                            AZStd::vector<AZStd::string> tokens;
                            StringFunc::Tokenize(currentArg.substr(foundPos + 1).c_str(), tokens, ',');
                            currentArg.resize(foundPos);
                            currentArg = ToLower(currentArg);
                            StringFunc::Strip(currentArg, " ", false, true, true);
                            m_switches.insert_key(currentArg); // returns pair<iter, bool>
                           // we do it this way because you are allowed to do odd things like
                           // /root=abc /root=hij,klm /root whee /root fun,days
                           // and roots value should be { abc, hij, klm, whee, fun, days }
                            for (AZStd::string& switchValue : tokens)
                            {
                                StringFunc::Strip(switchValue, " ", false, true, true);
                                m_switches[currentArg].push_back(switchValue);
                            }
                            currentSwitch.clear();
                        }
                        else
                        {
                            // its in this format /switchName switchvalue
                            // (no equals)
                            currentSwitch = currentArg;
                            currentSwitch = ToLower(currentSwitch);
                            m_switches.insert_key(currentSwitch);
                        }
                    }
                    else
                    {
                        if (currentSwitch.empty())
                        {
                            m_miscValues.push_back(currentArg);
                        }
                        else
                        {
                            AZStd::vector<AZStd::string> tokens;
                            StringFunc::Tokenize(currentArg.c_str(), tokens, ',');
                            for (AZStd::string& switchValue : tokens)
                            {
                                StringFunc::Strip(switchValue, " ", false, true, true);
                                m_switches[currentSwitch].push_back(switchValue);
                            }

                        }

                        currentSwitch.clear();
                    }
                }
            }
        }
    }

    bool CommandLine::HasSwitch(AZStd::string_view switchName) const
    {
        return m_switches.find(ToLower(switchName)) != m_switches.end();
    }

    AZStd::size_t CommandLine::GetNumSwitchValues(AZStd::string_view switchName) const
    {
        ParamMap::const_iterator switchFound = m_switches.find(ToLower(switchName));
        if (switchFound == m_switches.end())
        {
            return 0;
        }

        return switchFound->second.size();
    }

    const AZStd::string& CommandLine::GetSwitchValue(AZStd::string_view switchName, AZStd::size_t index) const
    {
        ParamMap::const_iterator switchFound = m_switches.find(ToLower(switchName));
        if (switchFound == m_switches.end())
        {
            return m_emptyValue;
        }

        AZ_Assert(index < switchFound->second.size(), "Invalid Command line switch lookup");
        if (index >= switchFound->second.size())
        {
            return m_emptyValue;
        }

        return switchFound->second.at(index);
    }

    AZStd::size_t CommandLine::GetNumMiscValues() const
    {
        return m_miscValues.size();
    }

    const AZStd::string& CommandLine::GetMiscValue(AZStd::size_t index) const
    {
        AZ_Assert(index < m_miscValues.size(), "Invalid Command line lookup");
        if (index >= m_miscValues.size())
        {
            return m_emptyValue;
        }
        return m_miscValues[index];
    }

} // namespace AzFramework
