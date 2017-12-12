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

#include <StdAfx.h>
#include <time.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>
#include <SceneAPI/SceneUI/CommonWidgets/ReportWidget.h>
#include <SceneAPI/SceneUI/CommonWidgets/ReportCard.h>
#include <TraceMessageAggregator.h>

TraceMessageAggregator::Entry::Entry(Entry&& rhs)
    : m_stack(AZStd::move(rhs.m_stack))
    , m_message(AZStd::move(rhs.m_message))
    , m_timeStamp(rhs.m_timeStamp)
    , m_type(rhs.m_type)
{
}

TraceMessageAggregator::Entry& TraceMessageAggregator::Entry::operator=(Entry&& rhs)
{
    if (this != &rhs)
    {
        m_stack = AZStd::move(rhs.m_stack);
        m_message = AZStd::move(rhs.m_message);
        m_timeStamp = rhs.m_timeStamp;
        m_type = rhs.m_type;
    }
    return *this;
}

TraceMessageAggregator::TraceMessageAggregator(const AZ::Uuid& filterTag)
    : m_filterTag(filterTag)
{
    BusConnect();
}

TraceMessageAggregator::~TraceMessageAggregator()
{
    BusDisconnect();
}

bool TraceMessageAggregator::OnPrintf(const char* window, const char* message)
{
    if (!IsAcceptedMessage())
    {
        return false;
    }

    Entry entry;
    entry.m_timeStamp = 0;
    time(&entry.m_timeStamp);
    entry.m_stack = m_stack.GetStack();
    entry.m_message = message;
    if (entry.m_message[entry.m_message.length() - 1] != '\n')
    {
        entry.m_message += '\n';
    }

    if (AzFramework::StringFunc::Equal(window, AZ::SceneAPI::Utilities::ErrorWindow))
    {
        entry.m_type = Type::Error;
    }
    else if (AzFramework::StringFunc::Equal(window, AZ::SceneAPI::Utilities::WarningWindow))
    {
        entry.m_type = Type::Warning;
    }
    else
    {
        entry.m_type = Type::Log;
    }

    m_entries.push_back(AZStd::move(entry));

    return true;
}

bool TraceMessageAggregator::OnAssert(const char* message)
{
    if (!IsAcceptedMessage())
    {
        return false;
    }

    Entry entry;
    entry.m_timeStamp = 0;
    time(&entry.m_timeStamp);
    entry.m_stack = m_stack.GetStack();
    entry.m_message = message;
    if (entry.m_message[entry.m_message.length() - 1] != '\n')
    {
        entry.m_message += '\n';
    }
    entry.m_type = Type::Assert;
    m_entries.push_back(AZStd::move(entry));

    // Return false so the normal assert message box still appears.
    return false;
}

bool TraceMessageAggregator::HasEntries() const
{
    return !m_entries.empty();
}

bool TraceMessageAggregator::HasWarnings() const
{
    for (const Entry& entry : m_entries)
    {
        if (entry.m_type == Type::Warning)
        {
            return true;
        }
    }

    return false;
}

bool TraceMessageAggregator::HasErrorsOrAsserts() const
{
    for (const Entry& entry : m_entries)
    {
        if (entry.m_type == Type::Assert || entry.m_type == Type::Error)
        {
            return true;
        }
    }

    return false;
}

void TraceMessageAggregator::Report(AZ::SceneAPI::UI::ReportWidget* report)
{
    if (m_entries.empty())
    {
        return;
    }

    for (const TraceMessageAggregator::Entry& entry : m_entries)
    {
        if (entry.m_type != TraceMessageAggregator::Type::Log)
        {
            AZ::SceneAPI::UI::ReportCard::Type type = AZ::SceneAPI::UI::ReportCard::Type::Log;
            if (entry.m_type == TraceMessageAggregator::Type::Warning)
            {
                type = AZ::SceneAPI::UI::ReportCard::Type::Warning;
            }
            else if (entry.m_type == TraceMessageAggregator::Type::Error)
            {
                type = AZ::SceneAPI::UI::ReportCard::Type::Error;
            }
            else if (entry.m_type == TraceMessageAggregator::Type::Assert)
            {
                type = AZ::SceneAPI::UI::ReportCard::Type::Assert;
            }

            AZ::SceneAPI::UI::ReportCard* card = new AZ::SceneAPI::UI::ReportCard(type);
            card->SetMessage(entry.m_message);
            card->SetContext(entry.m_stack, false);
            card->SetTimeStamp(entry.m_timeStamp);

            report->AddReportCard(card);
        }
    }
}

void TraceMessageAggregator::Report(const AZStd::string& taskDescription, AZ::SceneAPI::UI::OverlayWidget* overlay)
{
    if (m_entries.empty())
    {
        return;
    }

    AZ::SceneAPI::UI::ReportWidget* report = aznew AZ::SceneAPI::UI::ReportWidget(nullptr);
    report->SetTaskDescription(taskDescription);
    Report(report);
    report->ReportToOverlay(overlay, AZ::SceneAPI::UI::ReportWidget::Host::BreakoutWindow);
}

void TraceMessageAggregator::Report(const AZStd::string& taskDescription)
{
    Report(taskDescription, nullptr);
}

TraceMessageAggregator::EntryContainerView TraceMessageAggregator::GetEntries() const
{
    return AZ::SceneAPI::Containers::Views::MakeView(m_entries.cbegin(), m_entries.cend());
}

bool TraceMessageAggregator::IsAcceptedMessage() const
{
    const AzToolsFramework::Debug::TraceContextStackInterface& stack = m_stack.GetStack();
    size_t stackSize = stack.GetStackCount();
    for (size_t i = 0; i < stackSize; ++i)
    {
        if (stack.GetType(i) == AzToolsFramework::Debug::TraceContextStackInterface::ContentType::UuidType)
        {
            if (stack.GetUuidValue(i) == m_filterTag)
            {
                return true;
            }
        }
    }
    return false;
}