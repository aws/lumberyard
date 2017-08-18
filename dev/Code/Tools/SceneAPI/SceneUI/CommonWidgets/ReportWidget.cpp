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

#include <QLabel.h>
#include <time.h>
#include <CommonWidgets/ui_ReportWidget.h>
#include <SceneAPI/SceneUI/CommonWidgets/ReportCard.h>
#include <SceneAPI/SceneUI/CommonWidgets/ReportWidget.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            ReportWidget::ReportWidget(QWidget* parent)
                : QWidget(parent)
                , ui(new Ui::ReportWidget())
            {
                ui->setupUi(this);
                
                ui->m_taskDescription->hide();
            }

            ReportWidget::~ReportWidget()
            {
            }

            void ReportWidget::SetTaskDescription(const AZStd::string& taskDescription)
            {
                if (taskDescription.empty())
                {
                    ui->m_taskDescription->hide();
                }
                else
                {
                    ui->m_taskDescription->show();
                    ui->m_taskDescription->setText(taskDescription.c_str());
                }
            }

            void ReportWidget::AddReportCard(ReportCard* card)
            {
                int count = ui->m_cardsAreaLayout->count();
                if (count == 0)
                {
                    card->SetTopLineVisible(false);
                }

                ui->m_cardsAreaLayout->addWidget(card);
            }

            void ReportWidget::ReportToOverlay(OverlayWidget* overlay, Host host)
            {
                OverlayWidgetButtonList buttons;
                OverlayWidgetButton closeButton;
                closeButton.m_text = "Close";
                closeButton.m_triggersPop = true;
                closeButton.m_isCloseButton = true;
                buttons.push_back(&closeButton);

                AZ_Assert(host == Host::Center || host == Host::BreakoutWindow, "Invalid host type %i.", host);
                QWidget* center = host == Host::Center ? this : nullptr;
                QWidget* breakout = host == Host::BreakoutWindow ? this : nullptr;
                OverlayWidget::PushLayerToOverlay(overlay, center, breakout, "Report", buttons);
            }

            void ReportWidget::ReportAssert(const AZStd::string& title, const AZStd::string& message, OverlayWidget* overlay)
            {
                ReportCard* card = new ReportCard(ReportCard::Type::Assert);
                ReportSingleCard(card, title, message, overlay);
            }

            void ReportWidget::ReportError(const AZStd::string& title, const AZStd::string& message, OverlayWidget* overlay)
            {
                ReportCard* card = new ReportCard(ReportCard::Type::Error);
                ReportSingleCard(card, title, message, overlay);
            }

            void ReportWidget::ReportWarning(const AZStd::string& title, const AZStd::string& message, OverlayWidget* overlay)
            {
                ReportCard* card = new ReportCard(ReportCard::Type::Warning);
                ReportSingleCard(card, title, message, overlay);
            }

            void ReportWidget::ReportLog(const AZStd::string& title, const AZStd::string& message, OverlayWidget* overlay)
            {
                ReportCard* card = new ReportCard(ReportCard::Type::Log);
                ReportSingleCard(card, title, message, overlay);
            }

            void ReportWidget::ReportSingleCard(ReportCard* card, const AZStd::string& title, const AZStd::string& message, OverlayWidget* overlay)
            {
                card->SetMessage(message);

                time_t timeStamp;
                time(&timeStamp);
                card->SetTimeStamp(timeStamp);

                ReportWidget* report = new ReportWidget(nullptr);
                report->SetTaskDescription(title);
                report->AddReportCard(card);
                report->ReportToOverlay(overlay);
            }
        } // namespace UI
    } // namespace SceneAPI
} // namespace AZ

#include <CommonWidgets/ReportWidget.moc>
