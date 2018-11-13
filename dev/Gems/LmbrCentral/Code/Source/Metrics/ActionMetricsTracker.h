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
#pragma once

#include <QMap>
#include <QObject>
#include <QPointer>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>

class QAction;
class QMenu;

namespace LyEditorMetrics
{
    
    class ActionMetricsTracker
        : public QObject
        , public AzToolsFramework::EditorEvents::Bus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ActionMetricsTracker, AZ::SystemAllocator, 0);
        explicit ActionMetricsTracker(QObject* parent = nullptr);
        ~ActionMetricsTracker();

        void RegisterAction(QAction* action, const QString& metricsText);
        void UnregisterAction(QAction* action);

        bool eventFilter(QObject* obj, QEvent* ev) final; // make it final so that the compiler knows it doesn't have to go through the vtable

        void NotifyQtApplicationAvailable(QApplication* application) override;

        void SendMetrics(const char* metricsText, AzToolsFramework::MetricsActionTriggerType triggerType);

    private:

        void ActionDestroyed(QObject* action);
        void ActionTriggered(QAction* action);

        QMap<QAction*, QString> m_actionToMetrics;
        AzToolsFramework::MetricsActionTriggerType m_activeTrigger = AzToolsFramework::MetricsActionTriggerType::Unknown;
        QMap<QAction*, QMetaObject::Connection> m_actionToTriggeredConnections; // we use a 
    };

} // namespace LyEditorMetrics
