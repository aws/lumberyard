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


#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/Debug/TraceContextMultiStackHandler.h>
#include <AzQtComponents/Components/StyledDetailsTableModel.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#include <QScopedPointer>
#include <QWidget>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>

class QCloseEvent;
class QLabel;
class QTimer;

namespace AzQtComponents
{
    class StyledBusyLabel;
    class StyledDetailsTableView;
}

namespace AzToolsFramework
{
    namespace Logging
    {
        class LogEntry;
    }
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            // The qt-generated ui code (from the .ui)
            namespace Ui
            {
                class ProcessingOverlayWidget;
            }

            class ProcessingHandler;
            class SCENE_UI_API ProcessingOverlayWidget 
                : public QWidget
                , public Debug::TraceMessageBus::Handler
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR(ProcessingOverlayWidget, SystemAllocator, 0);

                //! Layout configurations for the various stages the Scene Settings can be in.
                enum class Layout
                {
                    Loading,
                    Resetting,
                    Exporting
                };

                ProcessingOverlayWidget(UI::OverlayWidget* overlay, Layout layout, Uuid traceTag);
                ~ProcessingOverlayWidget() override;

                bool OnPrintf(const char* window, const char* message) override;
                bool OnError(const char* window, const char* message) override;
                bool OnWarning(const char* window, const char* message) override;
                bool OnAssert(const char* message) override;

                int PushToOverlay();

                void SetAndStartProcessingHandler(const AZStd::shared_ptr<ProcessingHandler>& handler);
                AZStd::shared_ptr<ProcessingHandler> GetProcessingHandler() const;

                bool GetAutoCloseOnSuccess() const;
                void SetAutoCloseOnSuccess(bool autoCloseOnSuccess);
                bool HasProcessingCompleted() const;

                void BlockClosing();
                void UnblockClosing();

            signals:
                void Closing();

            public slots:
                void AddLogEntry(const AzToolsFramework::Logging::LogEntry& entry);

                void OnLayerRemoved(int layerId);

                void OnSetStatusMessage(const AZStd::string& message);
                void OnProcessingComplete();

                void UpdateColumnSizes();

            private:
                void SetUIToCompleteState();

                bool CanClose() const;
                bool ShouldProcessMessage() const;
                void CopyTraceContext(AzQtComponents::StyledDetailsTableModel::TableEntry& entry) const;

                AzToolsFramework::Debug::TraceContextMultiStackHandler m_traceStackHandler;
                Uuid m_traceTag;
                QScopedPointer<Ui::ProcessingOverlayWidget> ui;
                AZStd::shared_ptr<ProcessingHandler> m_targetHandler;
                UI::OverlayWidget* m_overlay;
                AzQtComponents::StyledBusyLabel* m_busyLabel;
                AzQtComponents::StyledDetailsTableView* m_reportView;
                AzQtComponents::StyledDetailsTableModel* m_reportModel;
                QLabel* m_progressLabel;
                int m_layerId;
                QTimer* m_resizeTimer;
                
                bool m_isProcessingComplete;
                bool m_isClosingBlocked;
                bool m_autoCloseOnSuccess;
                bool m_encounteredIssues;
            };
        }
    }
}
