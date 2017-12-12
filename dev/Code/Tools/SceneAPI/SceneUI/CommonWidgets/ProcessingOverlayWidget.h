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
#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <SceneAPI/SceneUI/CommonWidgets/JobWatcher.h>
#include <SceneAPI/SceneUI/CommonWidgets/ReportCard.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#include <QSharedPointer>
#include <QScopedPointer>

class QCloseEvent;
class QLabel;

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            class ReportWidget;
            class OverlayWidget;
        }

        namespace SceneUI
        {
            // The qt-generated ui code (from the .ui)
            namespace Ui
            {
                class ProcessingOverlayWidget;
            }

            class ProcessingHandler;
            class SCENE_UI_API ProcessingOverlayWidget : public QWidget
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR(ProcessingOverlayWidget, SystemAllocator, 0)

                ProcessingOverlayWidget(UI::OverlayWidget* overlay, const AZStd::shared_ptr<ProcessingHandler>& handler);
                ~ProcessingOverlayWidget() override;

                int PushToOverlay();

                QSharedPointer<UI::ReportWidget> GetReportWidget() const;
                AZStd::shared_ptr<ProcessingHandler> GetProcessingHandler() const;

                bool GetAutoCloseOnSuccess() const;
                void SetAutoCloseOnSuccess(bool autoCloseOnSuccess);
                bool HasProcessingCompleted() const;

                void BlockClosing();
                void UnblockClosing();

            signals:
                void Closing();

            public slots:
                void AddInfo(const AZStd::string& infoMessage);
                void AddWarning(const AZStd::string& warningMessage);
                void AddError(const AZStd::string& errorMessage);
                void AddAssert(const AZStd::string& assertMessage);
                void AddLogEntry(const AzToolsFramework::Logging::LogEntry& entry);

                void OnLayerRemoved(int layerId);

                void OnUserMessageUpdated(const AZStd::string& userMessage);
                void OnSubtextUpdated(const AZStd::string& subtext);
                void OnProcessingComplete();

            private:
                void AddCard(const AZStd::string& message, UI::ReportCard::Type type);
                void SetUIToCompleteState();

                bool CanClose() const;

                QScopedPointer<Ui::ProcessingOverlayWidget> ui;
                QSharedPointer<UI::ReportWidget> m_reportWidget;
                AZStd::shared_ptr<ProcessingHandler> m_targetHandler;
                UI::OverlayWidget* m_overlay;
                QLabel* m_progressLabel;
                int m_layerId;
                
                bool m_isProcessingComplete;
                bool m_isClosingBlocked;
                bool m_autoCloseOnSuccess;
                bool m_encounteredAnyErrors;
            };
        }
    }
}