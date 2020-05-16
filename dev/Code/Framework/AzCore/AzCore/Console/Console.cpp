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

#include <AzCore/Console/Console.h>
#include <AzCore/Console/ConsoleNotificationBus.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/FileIO.h>


namespace AZ
{
    uint32_t CountMatchingPrefixes(const AZStd::string_view& string, const StringSet& stringSet)
    {
        uint32_t count = 0;

        for (AZStd::string_view iter : stringSet)
        {
            if (StringFunc::StartsWith(iter, string, false))
            {
                ++count;
            }
        }

        return count;
    }


    Console::Console()
        : m_head(nullptr)
    {
        AZ::Interface<IConsole>::Register(this);
    }


    Console::~Console()
    {
        // Unlink everything first
        for (ConsoleFunctorBase* curr = m_head; curr != nullptr; curr = curr->m_next)
        {
            curr->Unlink(m_head);
            curr->m_console = nullptr;
        }

        // If we are shutting down, make sure that we mark the head as nullptr so that we don't try to unlink functors after the fact
        // It appears that things to not necessarily cleanly unlink in the client standalone exe on shutdown, so to prevent accessing memory that has been freed, we do this
        m_head = nullptr;

        AZ::Interface<IConsole>::Unregister(this);
    }


    bool Console::PerformCommand(const char* command, ConsoleFunctorFlags requiredSet, ConsoleFunctorFlags requiredClear)
    {
        static const char* separators = " \t\n\r";

        AZStd::string trimmed = command;
        StringFunc::TrimWhiteSpace(trimmed, true, true);

        StringSet splitTokens;
        StringFunc::Tokenize(trimmed, splitTokens, " ");

        if (splitTokens.size() < 1)
        {
            return false;
        }

        const AZStd::string splitCommand = splitTokens.front();
        splitTokens.erase(splitTokens.begin());

        ConsoleFunctorFlags flags = ConsoleFunctorFlags::Null;
        if (DispatchCommand(splitCommand, splitTokens, requiredSet, requiredClear, flags))
        {
            ConsoleNotificationBus::Broadcast(&ConsoleNotificationBus::Events::OnPerformCommand, splitCommand, flags);
            return true;
        }

        return false;
    }


    void Console::ExecuteCommandLine(const char* commandLine)
    {
        // We consume a leading space so values like guids don't get split by accident
        const AZStd::vector<AZStd::string_view> switches = { " -", " +" };

        if (*commandLine == '\0')
        {
            return;
        }

        // Purposefully inject a space into the split string to match the switches above
        const AZStd::string commandline = AZStd::string(" ") + AZStd::string(commandLine);
        StringSet split;
        StringFunc::Tokenize(commandline, split, switches);

        for (AZStd::string& command : split)
        {
            const size_t delim = command.find_first_of('=');

            if (delim == AZStd::string::npos)
            {
                PerformCommand(command.c_str(), ConsoleFunctorFlags::Null, ConsoleFunctorFlags::Null);
                continue;
            }

            // Swap the '=' character for a space
            command[delim] = ' ';

            PerformCommand(command.c_str());
        }
    }


    bool Console::HasCommand(const char* command)
    {
        return FindCommand(command) != nullptr;
    }


    ConsoleFunctorBase* Console::FindCommand(const char* command)
    {
        const size_t commandLength = strlen(command);

        for (ConsoleFunctorBase* curr = m_head; curr != nullptr; curr = curr->m_next)
        {
            if ((curr->GetFlags() & ConsoleFunctorFlags::IsInvisible) == ConsoleFunctorFlags::IsInvisible)
            {
                // Filter functors marked as invisible
                continue;
            }

            if (StringFunc::Equal(curr->GetName(), command, false, commandLength + 1)) // + 1 to include the NULL terminator
            {
                return curr;
            }
        }

        return nullptr;
    }


    AZStd::string Console::AutoCompleteCommand(const char* command)
    {
        const size_t commandLength = strlen(command);

        if (commandLength <= 0)
        {
            return command;
        }

        StringSet commandSubset;

        for (ConsoleFunctorBase* curr = m_head; curr != nullptr; curr = curr->m_next)
        {
            if ((curr->GetFlags() & ConsoleFunctorFlags::IsInvisible) == ConsoleFunctorFlags::IsInvisible)
            {
                // Filter functors marked as invisible
                continue;
            }

            if (StringFunc::Equal(curr->m_name, command, false, commandLength))
            {
                AZ_TracePrintf("Az Console", "- %s : %s\n", curr->m_name, curr->m_desc);
                commandSubset.push_back(curr->m_name);
            }
        }

        AZStd::string largestSubstring = command;

        if ((!largestSubstring.empty()) && (!commandSubset.empty()))
        {
            const uint32_t totalCount = CountMatchingPrefixes(command, commandSubset);

            for (size_t i = largestSubstring.length(); i < commandSubset.front().length(); ++i)
            {
                const AZStd::string nextSubstring = largestSubstring + commandSubset.front()[i];
                const uint32_t count = CountMatchingPrefixes(nextSubstring, commandSubset);

                if (count < totalCount)
                {
                    break;
                }

                largestSubstring = nextSubstring;
            }
        }

        return largestSubstring;
    }


    void Console::VisitRegisteredFunctors(FunctorVisitorInterface& visitor)
    {
        for (ConsoleFunctorBase* curr = m_head; curr != nullptr; curr = curr->m_next)
        {
            visitor.Visit(curr);
        }
    }


    ConsoleFunctorBase*& Console::RetrieveHeadElement()
    {
        return m_head;
    }


    void Console::LinkDeferredFunctors(ConsoleFunctorBase*& deferredHead)
    {
        ConsoleFunctorBase* curr = deferredHead;

        while (curr != nullptr)
        {
            ConsoleFunctorBase* next = curr->m_next;

            curr->Unlink(deferredHead);
            curr->Link(m_head);
            curr->m_console = this;
            curr->m_isDeferred = false;

            curr = next;
        }

        deferredHead = nullptr;
    }


    bool Console::DispatchCommand(const AZStd::string& command, const StringSet& inputs, ConsoleFunctorFlags requiredSet, ConsoleFunctorFlags requiredClear, ConsoleFunctorFlags& outFlags)
    {
        const size_t commandLength = command.size();

        bool result = false;

        for (ConsoleFunctorBase* curr = m_head; curr != nullptr; curr = curr->m_next)
        {
            if (StringFunc::Equal(curr->m_name, command.c_str(), false, commandLength + 1)) // + 1 to include the NULL terminator
            {
                result = true;

                if ((curr->GetFlags() & requiredSet) != requiredSet)
                {
                    AZ_Warning("Az Console", false, "%s failed required set flag check\n", curr->m_name);
                    continue;
                }

                if ((curr->GetFlags() & requiredClear) != ConsoleFunctorFlags::Null)
                {
                    AZ_Warning("Az Console", false, "%s failed required clear flag check\n", curr->m_name);
                    continue;
                }

                if ((curr->GetFlags() & ConsoleFunctorFlags::IsCheat) != ConsoleFunctorFlags::Null)
                {
                    AZ_Warning("Az Console", false, "%s is marked as a cheat\n", curr->m_name);
                }

                if ((curr->GetFlags() & ConsoleFunctorFlags::IsDeprecated) != ConsoleFunctorFlags::Null)
                {
                    AZ_Warning("Az Console", false, "%s is marked as deprecated\n", curr->m_name);
                }

                if ((curr->GetFlags() & ConsoleFunctorFlags::NeedsReload) != ConsoleFunctorFlags::Null)
                {
                    AZ_Warning("Az Console", false, "Changes to %s will only take effect after level reload\n", curr->m_name);
                }

                // Letting this intentionally fall-through, since in editor we can register common variables multiple times
                (*curr)(inputs);
                outFlags = curr->GetFlags();
            }
        }

        if (!result)
        {
            AZ_Warning("Az Console", false, "Command %s not found", command.c_str());
        }

        return result;
    }
}
