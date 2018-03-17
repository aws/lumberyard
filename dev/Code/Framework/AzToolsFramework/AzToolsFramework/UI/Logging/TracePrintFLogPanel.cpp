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
#include "TracePrintFLogPanel.h"

#include <QAction>
#include <QDateTime>
#include <QTimer>
#include <QTableView>
#include <AzCore/std/parallel/lock.h>

#include <UI/Logging/TracePrintFLogPanel.moc>

namespace AzToolsFramework
{
    namespace LogPanel
    {
        // some tweakables
        static int s_delayBetweenTraceprintfUpdates = 250; // milliseconds between pumping the traceprintf messages, lower will eat more performance but be more responsive

        TracePrintFLogPanel::TracePrintFLogPanel(QWidget* pParent /* = nullptr */)
            : BaseLogPanel(pParent)
        {
        }

        QWidget* TracePrintFLogPanel::CreateTab(const TabSettings& settings)
        {
            return aznew AZTracePrintFLogTab(this, settings);
        }

        AZTracePrintFLogTab::AZTracePrintFLogTab(QWidget* pParent, const TabSettings& in_settings)
            : BaseLogView(pParent)
            , m_settings(in_settings)
        {
            m_alreadyQueuedDrainMessage = false;
            ConnectModelToView(new RingBufferLogDataModel(m_ptrLogView));

            // add clear to context menu:
            QAction* actionClear = new QAction(tr("Clear"), this);
            connect(actionClear, SIGNAL(triggered()), this, SLOT(Clear()));
            addAction(actionClear);

            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }

        void AZTracePrintFLogTab::Clear()
        {
            RingBufferLogDataModel* pModel = qobject_cast<RingBufferLogDataModel*>(m_ptrLogView->model());
            if (pModel)
            {
                pModel->Clear();
            }
        }

        bool AZTracePrintFLogTab::OnAssert(const char* message)
        {
            if (m_settings.m_filterFlags & (0x01 << TabSettings::FILTER_ERROR))
            {
                LogTraceMessage(Logging::LogLine::TYPE_ERROR, "ASSERT", message);
            }

            return false;
        }
        bool AZTracePrintFLogTab::OnException(const char* message)
        {
            if (m_settings.m_filterFlags & (0x01 << TabSettings::FILTER_ERROR))
            {
                LogTraceMessage(Logging::LogLine::TYPE_ERROR, "EXCEPTION", message);
            }

            return false;
        }

        bool AZTracePrintFLogTab::OnError(const char* window, const char* message)
        {
            if (m_settings.m_filterFlags & (0x01 << TabSettings::FILTER_ERROR))
            {
                LogTraceMessage(Logging::LogLine::TYPE_ERROR, window, message);
            }
            return false;
        }

        bool AZTracePrintFLogTab::OnWarning(const char* window, const char* message)
        {
            if (m_settings.m_filterFlags & (0x01 << TabSettings::FILTER_WARNING))
            {
                LogTraceMessage(Logging::LogLine::TYPE_WARNING, window, message);
            }
            return false;
        }

        bool AZTracePrintFLogTab::OnPrintf(const char* window, const char* message)
        {
            if (
                (m_settings.m_filterFlags & (0x01 << TabSettings::FILTER_DEBUG)) &&
                (azstricmp(window, "debug") == 0)
                )
            {
                LogTraceMessage(Logging::LogLine::TYPE_DEBUG, window, message);
            }
            else if (m_settings.m_filterFlags & (0x01 << TabSettings::FILTER_NORMAL) &&
                (azstricmp(window, "debug") != 0))
            {
                LogTraceMessage(Logging::LogLine::TYPE_MESSAGE, window, message);
            }
            return false;
        }

        void AZTracePrintFLogTab::LogTraceMessage(Logging::LogLine::LogType type, const char* window, const char* message)
        {
            // note:  This is responding to a trace driller bus message
            // as such, the mutex is already locked but we could be called from any thread at all, so we buffer the lines.

            // is it for this window?
            // empty window discriminator string signifies that this OnLog receives ALL
            // hard coded "All" in the dialog to mean the same
            bool isMyWindowAll = (!m_settings.m_window.length()) || (azstricmp(m_settings.m_window.c_str(), "All") == 0);

            if (isMyWindowAll || ((window) && (azstricmp(window, m_settings.m_window.c_str()) == 0)))
            {
                if ((m_settings.m_textFilter.length() == 0) || (strstr(message, m_settings.m_textFilter.c_str())))
                {
                    {
                        AZStd::lock_guard<AZStd::mutex> lock(m_bufferedLinesMutex);
                        m_bufferedLines.push(Logging::LogLine(message, window ? window : "", type, QDateTime::currentMSecsSinceEpoch()));
                    }

                    // if we've already queued a timer tick, don't queue another one:
                    bool wasQueued = m_alreadyQueuedDrainMessage.exchange(true, AZStd::memory_order_acq_rel);
                    if (!wasQueued)
                    {
                        QTimer::singleShot(s_delayBetweenTraceprintfUpdates, this, &AZTracePrintFLogTab::DrainMessages);
                    }
                }
            }
        }

        AZTracePrintFLogTab::~AZTracePrintFLogTab()
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
            // qt autodeletes any qt  objects.
        }

        // we only tick when there's queue to drain.
        void AZTracePrintFLogTab::DrainMessages()
        {
            m_alreadyQueuedDrainMessage = false;

            bool wasAtMaxScroll = IsAtMaxScroll();
            RingBufferLogDataModel* pModel = ((RingBufferLogDataModel*)(m_ptrLogView->model()));
            Logging::LogLine currentLine;
            bool openedQuery = false;
            bool foundLine = false;
            do
            {
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_bufferedLinesMutex);
                    if (m_bufferedLines.empty())
                    {
                        foundLine = false;
                    }
                    else
                    {
                        foundLine = true;
                        currentLine = AZStd::move(m_bufferedLines.front());
                        m_bufferedLines.pop();
                    }
                }

                if (foundLine)
                {
                    if (!openedQuery)
                    {
                        openedQuery = true;
                    }

                    pModel->AppendLine(currentLine);
                }
            } while (foundLine); // keep doing this as long as there's line in the buffer.

            if (openedQuery)
            {
                pModel->CommitAdd();
                if (wasAtMaxScroll)
                {
                    m_ptrLogView->scrollToBottom();
                }
            }
        }
    } // namespace LogPanel
} // namespace AzToolsFramework

