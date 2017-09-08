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

#include "LogPanel.h"
#include "native/assetprocessor.h"
#include <AzToolsFramework/UI/Logging/LogLine.h>

namespace AssetProcessor
{

    LogPanel::LogPanel(QWidget* pParent /* = nullptr */)
        : AzToolsFramework::LogPanel::TracePrintFLogPanel(pParent)
    {
    }

    QWidget* LogPanel::CreateTab(const AzToolsFramework::LogPanel::TabSettings& settings)
    {
        LogTab* logTab = aznew LogTab(this, settings);
        logTab->AddInitialLogMessage();
        return logTab;
    }

    LogTab::LogTab(QWidget* pParent, const AzToolsFramework::LogPanel::TabSettings& in_settings)
        : AzToolsFramework::LogPanel::AZTracePrintFLogTab(pParent, in_settings)
    {
    }

    void LogTab::AddInitialLogMessage()
    {
        LogTraceMessage(AzToolsFramework::Logging::LogLine::TYPE_MESSAGE, "AssetProcessor", "Started recording logs. To check previous logs please navigate to the logs folder.");
    }

    AZ::Debug::Result LogTab::OnAssert(const AZ::Debug::TraceMessageParameters& parameters)
    {
        if (AssetProcessor::GetThreadLocalJobId())
        {
            return AZ::Debug::Result::Continue; // we are in a job thread
        }

        return AzToolsFramework::LogPanel::AZTracePrintFLogTab::OnAssert(parameters);
    }
    AZ::Debug::Result LogTab::OnException(const AZ::Debug::TraceMessageParameters& parameters)
    {
        if (AssetProcessor::GetThreadLocalJobId())
        {
            return AZ::Debug::Result::Continue; // we are in a job thread
        }

        return AzToolsFramework::LogPanel::AZTracePrintFLogTab::OnException(parameters);
    }

    AZ::Debug::Result LogTab::OnPrintf(const AZ::Debug::TraceMessageParameters& parameters)
    {
        if (AssetProcessor::GetThreadLocalJobId())
        {
            return AZ::Debug::Result::Continue; // we are in a job thread
        }

        return AzToolsFramework::LogPanel::AZTracePrintFLogTab::OnPrintf(parameters);
    }

    AZ::Debug::Result LogTab::OnError(const AZ::Debug::TraceMessageParameters& parameters)
    {
        if (AssetProcessor::GetThreadLocalJobId())
        {
            return AZ::Debug::Result::Continue; // we are in a job thread
        }

        return AzToolsFramework::LogPanel::AZTracePrintFLogTab::OnError(parameters);
    }

    AZ::Debug::Result LogTab::OnWarning(const AZ::Debug::TraceMessageParameters& parameters)
    {
        if (AssetProcessor::GetThreadLocalJobId())
        {
            return AZ::Debug::Result::Continue; // we are in a job thread
        }

        return AzToolsFramework::LogPanel::AZTracePrintFLogTab::OnWarning(parameters);
    }

}