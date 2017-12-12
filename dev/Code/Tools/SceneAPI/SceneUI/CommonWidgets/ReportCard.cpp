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

#include <ctime>
#include <QDateTime.h>
#include <QTableWidget.h>
#include <CommonWidgets/ui_ReportCard.h>
#include <AzToolsFramework/Debug/TraceContextStackInterface.h>
#include <AzToolsFramework/Debug/TraceContextLogFormatter.h>
#include <AzToolsFramework/UI/Logging/LogEntry.h>
#include <SceneAPI/SceneUI/CommonWidgets/ReportCard.h>
#include <SceneAPI/SceneUI/CommonWidgets/ExpandCollapseToggler.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            ReportCard::ReportCard(Type type)
                : QWidget(nullptr)
                , ui(new Ui::ReportCard())
            {
                QSignalBlocker blocker(this);
                ui->setupUi(this);

                // Setup the expanded state tracking for the context display
                connect(ui->m_contextExpander, &SceneUI::ExpandCollapseToggler::ExpandedChanged, this, &ReportCard::OnExpandOrCollapseContext);
                ui->m_contextExpander->SetExpanded(false);
                OnExpandOrCollapseContext(false);

                ui->m_messageLabel->hide();
                ui->m_timeStamp->hide();
                ui->m_contextArea->hide();

                SetType(type);
            }

            ReportCard::ReportCard(const AzToolsFramework::Logging::LogEntry& entry)
                : QWidget(nullptr)
                , ui(new Ui::ReportCard())
            {
                QSignalBlocker blocker(this);
                ui->setupUi(this);

                switch (entry.GetSeverity())
                {
                case AzToolsFramework::Logging::LogEntry::Severity::Error:
                    SetType(Type::Error);
                    break;
                case AzToolsFramework::Logging::LogEntry::Severity::Warning:
                    SetType(Type::Warning);
                    break;
                case AzToolsFramework::Logging::LogEntry::Severity::Message:
                    // fall through
                default:
                    SetType(Type::Log);
                    break;
                }

                const AzToolsFramework::Logging::LogEntry::FieldStorage& fields = entry.GetFields();
                if (fields.find(AzToolsFramework::Logging::LogEntry::s_messageField) != fields.end())
                {
                    const AZStd::string& value = fields.at(AzToolsFramework::Logging::LogEntry::s_messageField).m_value;
                    ui->m_messageLabel->setText(value.c_str());
                }
                else
                {
                    ui->m_messageLabel->hide();
                }

                SetTimeStamp(entry.GetRecordedTime());

                ui->m_contextArea->hide(); 
                for (auto& field : fields)
                {
                    if (AzToolsFramework::Logging::LogEntry::IsCommonField(field.first))
                    {
                        continue;
                    }
                    AddContextLine(field.second.m_name, field.second.m_value);
                }
                connect(ui->m_contextExpander, &SceneUI::ExpandCollapseToggler::ExpandedChanged, this, &ReportCard::OnExpandOrCollapseContext);
                ui->m_contextExpander->SetExpanded(false);
                OnExpandOrCollapseContext(false);
            }

            void ReportCard::SetType(Type type)
            {
                const char* imageName = nullptr;
                switch (type)
                {
                case Type::Log:
                    ui->m_severity->hide();
                    break;
                case Type::Warning:
                    ui->m_severityLabel->setText("Warning");
                    ui->m_severityLabel->setStyleSheet("color : orange;");
                    imageName = ":/SceneUI/Common/WarningIcon.png";
                    break;
                case Type::Error:
                    ui->m_severityLabel->setText("Error");
                    ui->m_severityLabel->setStyleSheet("color : red;");
                    imageName = ":/SceneUI/Common/ErrorIcon.png";
                    break;
                case Type::Assert:
                    ui->m_severityLabel->setText("Assert");
                    ui->m_severityLabel->setStyleSheet("color : red;");
                    imageName = ":/SceneUI/Common/AssertIcon.png";
                    break;
                default:
                    ui->m_severityLabel->setText("<Unsupported type>");
                    ui->m_severityLabel->setStyleSheet("color : red;");
                    imageName = ":/SceneUI/Common/ErrorIcon.png";
                    break;
                }

                if (imageName)
                {
                    int width = ui->m_icon->width();
                    int height = ui->m_icon->height();
                    ui->m_icon->setPixmap(QPixmap(imageName).scaled(width, height, Qt::KeepAspectRatio));
                }
            }

            void ReportCard::SetMessage(const AZStd::string& message)
            {
                ui->m_messageLabel->setText(message.c_str());
                ui->m_messageLabel->show();
            }

            void ReportCard::SetTimeStamp(const AZStd::string& timeStamp)
            {
                ui->m_timeStamp->setText(timeStamp.c_str());
                ui->m_timeStamp->show();
            }

            void ReportCard::SetTimeStamp(const time_t& timeStamp)
            {
                struct tm timeInfo;
#if defined(AZ_PLATFORM_WINDOWS)
                localtime_s(&timeInfo, &timeStamp);
#else
                localtime_r(&timeStamp, &timeInfo);
#endif

                char buffer[128];
                std::strftime(buffer, sizeof(buffer), "%A, %B %d, %Y %H:%M:%S", &timeInfo);

                ui->m_timeStamp->setText(buffer);
                ui->m_timeStamp->show();
            }

            void ReportCard::SetTimeStamp(u64 timeStamp)
            {
                time_t time = QDateTime::fromMSecsSinceEpoch(timeStamp).toTime_t();
                SetTimeStamp(time);
            }

            void ReportCard::SetContext(const AzToolsFramework::Debug::TraceContextStackInterface& stack, bool includeUuids)
            {
                size_t count = stack.GetStackCount();
                for (int i = 0; i < count; ++i)
                {
                    if (!includeUuids && stack.GetType(i) == AzToolsFramework::Debug::TraceContextStackInterface::ContentType::UuidType)
                    {
                        continue;
                    }

                    AZStd::string valueText;
                    AzToolsFramework::Debug::TraceContextLogFormatter::PrintValue(valueText, stack, i);
                    AddContextLine(stack.GetKey(i), valueText);
                }
            }

            void ReportCard::AddContextLine(const AZStd::string& key, const AZStd::string& value)
            {
                int index = ui->m_contextLayout->rowCount();
                if (index == 1)
                {
                    ui->m_contextLayout->setColumnStretch(0, 1);
                    ui->m_contextLayout->setColumnStretch(1, 4);

                    ui->m_contextArea->show();
                }

                QLabel* keyLabel = new QLabel(key.c_str());
                keyLabel->setWordWrap(true);
                ui->m_contextLayout->addWidget(keyLabel, index, 0);

                QLabel* valueLabel = new QLabel(value.c_str());
                valueLabel->setWordWrap(true);
                ui->m_contextLayout->addWidget(valueLabel, index, 1);
            }

            void ReportCard::SetTopLineVisible(bool visible)
            {
                if (visible)
                {
                    ui->m_topLine->show();
                }
                else
                {
                    ui->m_topLine->hide();
                }
            }

            void ReportCard::OnExpandOrCollapseContext(bool isExpanded)
            {
                if (isExpanded)
                {
                    ui->m_contextWidget->show();
                }
                else
                {
                    ui->m_contextWidget->hide();
                }
            }
        } // UI
    } // SceneAPI
} // AZ

#include <CommonWidgets/ReportCard.moc>
