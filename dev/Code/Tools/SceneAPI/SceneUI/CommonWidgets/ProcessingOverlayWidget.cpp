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

#include <AzToolsFramework/UI/Logging/LogEntry.h>
#include <CommonWidgets/ui_ProcessingOverlayWidget.h>
#include <SceneAPI/SceneUI/CommonWidgets/ProcessingOverlayWidget.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/ProcessingHandler.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>
#include <SceneAPI/SceneUI/CommonWidgets/ReportWidget.h>
#include <time.h> // For card timestamp
#include <QCloseEvent>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            ProcessingOverlayWidget::ProcessingOverlayWidget(UI::OverlayWidget* overlay, const AZStd::shared_ptr<ProcessingHandler>& handler)
                : QWidget()
                , ui(new Ui::ProcessingOverlayWidget())
                , m_reportWidget(aznew UI::ReportWidget(nullptr))
                , m_targetHandler(handler)
                , m_overlay(overlay)
                , m_progressLabel(nullptr)
                , m_layerId(UI::OverlayWidget::s_invalidOverlayIndex)
                , m_isProcessingComplete(false)
                , m_isClosingBlocked(false)
                , m_autoCloseOnSuccess(false)
                , m_encounteredAnyErrors(false)
            {
                ui->setupUi(this);
                AZ_Assert(handler, "Processing handler was null");

                connect(m_targetHandler.get(), &ProcessingHandler::UserMessageUpdated, this, &ProcessingOverlayWidget::OnUserMessageUpdated);
                connect(m_targetHandler.get(), &ProcessingHandler::SubtextUpdated, this, &ProcessingOverlayWidget::OnSubtextUpdated);
                connect(m_targetHandler.get(), &ProcessingHandler::AddInfo, this, &ProcessingOverlayWidget::AddInfo);
                connect(m_targetHandler.get(), &ProcessingHandler::AddWarning, this, &ProcessingOverlayWidget::AddWarning);
                connect(m_targetHandler.get(), &ProcessingHandler::AddError, this, &ProcessingOverlayWidget::AddError);
                connect(m_targetHandler.get(), &ProcessingHandler::AddAssert, this, &ProcessingOverlayWidget::AddAssert);
                connect(m_targetHandler.get(), &ProcessingHandler::AddLogEntry, this, &ProcessingOverlayWidget::AddLogEntry);
                connect(m_targetHandler.get(), &ProcessingHandler::ProcessingComplete, this, &ProcessingOverlayWidget::OnProcessingComplete);
                connect(m_overlay, &UI::OverlayWidget::LayerRemoved, this, &ProcessingOverlayWidget::OnLayerRemoved);

                m_reportWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
                ui->m_reportArea->addWidget(m_reportWidget.data());

                m_targetHandler->BeginProcessing();
            }

            ProcessingOverlayWidget::~ProcessingOverlayWidget()
            {
            }

            void ProcessingOverlayWidget::OnLayerRemoved(int layerId)
            {
                if (layerId == m_layerId)
                {
                    delete m_progressLabel;
                    m_progressLabel = nullptr;

                    layerId = UI::OverlayWidget::s_invalidOverlayIndex;
                    emit Closing();
                }
            }

            int ProcessingOverlayWidget::PushToOverlay()
            {
                AZ_Assert(m_layerId == UI::OverlayWidget::s_invalidOverlayIndex, "Processing overlay widget already pushed.");
                if (m_layerId != UI::OverlayWidget::s_invalidOverlayIndex)
                {
                    return m_layerId;
                }

                UI::OverlayWidgetButtonList buttons;

                UI::OverlayWidgetButton button;
                button.m_text = "Ok";
                button.m_triggersPop = true;
                button.m_isCloseButton = true;
                button.m_enabledCheck = [this]() -> bool
                {
                    return CanClose();
                };
                buttons.push_back(&button);

                m_progressLabel = new QLabel("Processing...");
                m_progressLabel->setAlignment(Qt::AlignCenter);
                m_layerId = m_overlay->PushLayer(m_progressLabel, this, "File progress", buttons);
                return m_layerId;
            }

            bool ProcessingOverlayWidget::GetAutoCloseOnSuccess() const
            {
                return m_autoCloseOnSuccess;
            }

            void ProcessingOverlayWidget::SetAutoCloseOnSuccess(bool closeOnComplete)
            {
                m_autoCloseOnSuccess = closeOnComplete;
            }

            bool ProcessingOverlayWidget::HasProcessingCompleted() const
            {
                return m_isProcessingComplete;
            }
            
            QSharedPointer<UI::ReportWidget> ProcessingOverlayWidget::GetReportWidget() const
            {
                return m_reportWidget;
            }
            
            AZStd::shared_ptr<ProcessingHandler> ProcessingOverlayWidget::GetProcessingHandler() const
            {
                return m_targetHandler;
            }

            void ProcessingOverlayWidget::BlockClosing()
            {
                m_isClosingBlocked = true;
            }

            void ProcessingOverlayWidget::UnblockClosing()
            {
                m_isClosingBlocked = false;
                SetUIToCompleteState();
            }

            void ProcessingOverlayWidget::AddInfo(const AZStd::string& infoMessage)
            {
                AddCard(infoMessage, UI::ReportCard::Type::Log);
            }

            void ProcessingOverlayWidget::AddWarning(const AZStd::string& warningMessage)
            {
                AddCard(warningMessage, UI::ReportCard::Type::Warning);
            }

            void ProcessingOverlayWidget::AddError(const AZStd::string& errorMessage)
            {
                AddCard(errorMessage, UI::ReportCard::Type::Error);
                m_encounteredAnyErrors = true;
            }

            void ProcessingOverlayWidget::AddAssert(const AZStd::string& assertMessage)
            {
                AddCard(assertMessage, UI::ReportCard::Type::Assert);
                m_encounteredAnyErrors = true;
            }

            void ProcessingOverlayWidget::AddLogEntry(const AzToolsFramework::Logging::LogEntry& entry)
            {
                if (entry.GetSeverity() == AzToolsFramework::Logging::LogEntry::Severity::Error)
                {
                    m_encounteredAnyErrors = true;
                }
                m_reportWidget->AddReportCard(aznew UI::ReportCard(entry));
            }

            void ProcessingOverlayWidget::OnProcessingComplete()
            {
                m_isProcessingComplete = true;
                SetUIToCompleteState();

                if (!m_encounteredAnyErrors && m_autoCloseOnSuccess)
                {
                    close();
                }
                else if (m_progressLabel != nullptr)
                {
                    m_progressLabel->setText("Close the processing report to continue editing settings.");
                }
            }

            void ProcessingOverlayWidget::OnUserMessageUpdated(const AZStd::string& userMessage)
            {
                ui->m_headerLabel->setText(userMessage.c_str());
            }
            
            void ProcessingOverlayWidget::OnSubtextUpdated(const AZStd::string& userMessage)
            {
                ui->m_subtext->setText(userMessage.c_str());
            }

            void ProcessingOverlayWidget::AddCard(const AZStd::string& message, UI::ReportCard::Type type)
            {
                UI::ReportCard* card = aznew UI::ReportCard(type);
                card->SetMessage(message);

                time_t timeStamp;
                time(&timeStamp);
                card->SetTimeStamp(timeStamp);
                m_reportWidget->AddReportCard(card);
            }

            void ProcessingOverlayWidget::SetUIToCompleteState()
            {
                if (CanClose())
                {
                    if (m_overlay && m_layerId != UI::OverlayWidget::s_invalidOverlayIndex)
                    {
                        m_overlay->RefreshLayer(m_layerId);
                    }

                    ui->m_progressIndicator->setVisible(false);
                }
            }

            bool ProcessingOverlayWidget::CanClose() const
            {
                return !m_isClosingBlocked && m_isProcessingComplete;
            }
        } // SceneUI
    } // SceneAPI
} // AZ

#include <CommonWidgets/ProcessingOverlayWidget.moc>