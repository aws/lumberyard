#pragma once

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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <SceneAPI/SceneCore/Containers/Views/View.h>
#include <AzToolsFramework/Debug/TraceContextSingleStackHandler.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            class ReportWidget;
            class OverlayWidget;
        }
    }
}

class TraceMessageAggregator
    : public AZ::Debug::TraceMessageBus::Handler
{
public:
    enum class Type
    {
        Assert,
        Error,
        Warning,
        Log
    };

    struct Entry
    {
        Entry() = default;
        Entry(Entry&& rhs);
        Entry& operator=(Entry&& rhs);

        AzToolsFramework::Debug::TraceContextStack m_stack;
        AZStd::string m_message;
        time_t m_timeStamp;
        Type m_type;
    };

    using EntryContainer = AZStd::vector<Entry>;
    using EntryContainerView = AZ::SceneAPI::Containers::Views::View<EntryContainer::const_iterator>;

    explicit TraceMessageAggregator(const AZ::Uuid& filterTag);
    ~TraceMessageAggregator() override;

    virtual AZ::Debug::Result OnPrintf(const AZ::Debug::TraceMessageParameters& parameters) override;
    virtual AZ::Debug::Result OnAssert(const AZ::Debug::TraceMessageParameters& parameters) override;

    bool HasEntries() const;
    bool HasWarnings() const;
    bool HasErrorsOrAsserts() const;
    EntryContainerView GetEntries() const;

    // Adds the recorded entries to the given report widget.
    void Report(AZ::SceneAPI::UI::ReportWidget* report);
    // Prints the recorded entries to the given overlay using a ReportWidget. If the overlay is null
    //      the nearest overlay will be used.
    void Report(const AZStd::string& taskDescription, AZ::SceneAPI::UI::OverlayWidget* overlay);
    // Prints the recorded entries to the nearest overlay using a ReportWidget.
    void Report(const AZStd::string& taskDescription);

protected:
    bool IsAcceptedMessage() const;

    AzToolsFramework::Debug::TraceContextSingleStackHandler m_stack;
    EntryContainer m_entries;
    AZ::Uuid m_filterTag;
};