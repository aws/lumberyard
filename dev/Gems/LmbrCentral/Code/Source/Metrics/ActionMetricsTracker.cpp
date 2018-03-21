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

#include "LmbrCentral_precompiled.h"

#ifdef METRICS_SYSTEM_COMPONENT_ENABLED

#include "ActionMetricsTracker.h"
#include <assert.h>
#include <QMenu>
#include <QApplication>
#include <QString>
#include <QAction>
#include <QToolButton>
#include "LyScopedMetricsEvent.h"

namespace LyEditorMetrics
{

namespace EventNames
{
    // Event Names
    static const char* MenuTriggered = "MenuTriggered";
}

namespace AttributeNames
{
    // Attribute Names
    static const char* Identifier = "Identifier";
    static const char* ActionTriggerType = "ActionTriggerType";
}

ActionMetricsTracker::ActionMetricsTracker(QObject* parent)
    : QObject(parent)
{
    if (QCoreApplication::instance() != nullptr)
    {
        QCoreApplication::instance()->installEventFilter(this);
    }
    else
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }
}

ActionMetricsTracker::~ActionMetricsTracker()
{
}

void ActionMetricsTracker::RegisterAction(QAction* action, const QString& metricsText)
{
    if (!m_actionToMetrics.contains(action))
    {
        connect(action, &QObject::destroyed, this, &ActionMetricsTracker::ActionDestroyed);
        m_actionToTriggeredConnections[action] = connect(action, &QAction::triggered, this, [this, action](bool checked){
            Q_UNUSED(checked);

            ActionTriggered(action);
        });
    }

    m_actionToMetrics[action] = metricsText;
}

void ActionMetricsTracker::UnregisterAction(QAction* action)
{
    if (m_actionToMetrics.contains(action))
    {
        disconnect(action, &QObject::destroyed, this, &ActionMetricsTracker::ActionDestroyed);
        disconnect(m_actionToTriggeredConnections[action]);

        ActionDestroyed(action);
    }
}

bool ActionMetricsTracker::eventFilter(QObject* obj, QEvent* ev)
{
    using namespace AzToolsFramework;

    switch (ev->type())
    {
        case QEvent::Shortcut:
        {
            QAction* action = qobject_cast<QAction*>(obj);
            if ((action != nullptr) && (m_actionToMetrics.contains(action)))
            {
                m_activeTrigger = MetricsActionTriggerType::Shortcut;
            }
        }
        break;

        case QEvent::MouseButtonRelease:
        {
            QToolButton* toolButton = qobject_cast<QToolButton*>(obj);
            if (toolButton != nullptr)
            {
                QAction* action = toolButton->defaultAction();
                if (m_actionToMetrics.contains(action))
                {
                    m_activeTrigger = MetricsActionTriggerType::ToolButton;
                }
            }
            else
            {
                QMenu* menu = qobject_cast<QMenu*>(obj);
                if (menu != nullptr)
                {
                    m_activeTrigger = MetricsActionTriggerType::MenuClick;
                }
            }
        }
        break;

        case QEvent::KeyPress:
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(ev);
            switch (keyEvent->key())
            {
                case Qt::Key_Space:
                case Qt::Key_Select:
                case Qt::Key_Return:
                case Qt::Key_Enter:
                {
                    QMenu* menu = qobject_cast<QMenu*>(obj);
                    if (menu != nullptr)
                    {
                        // followed according to QMenu::keyPressEvent()
                        if ((keyEvent->key() == Qt::Key_Space) && !menu->style()->styleHint(QStyle::SH_Menu_SpaceActivatesItem, 0, menu))
                            break;

                        m_activeTrigger = MetricsActionTriggerType::MenuAltKey;
                    }
                }
                break;
            }
        }
        break;
    }

    const bool filterEventOut = false;
    return filterEventOut;
}

void ActionMetricsTracker::NotifyQtApplicationAvailable(QApplication* application)
{
    if (application != nullptr)
    {
        application->installEventFilter(this);
    }

    AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
}

void ActionMetricsTracker::SendMetrics(const char* metricsText, AzToolsFramework::MetricsActionTriggerType triggerType)
{
    using namespace AzToolsFramework;

    static const char* triggerTypeNames[] = {
        "Unknown",      // ActionTriggerType::Unknown,
        "MenuClick",    // ActionTriggerType::MenuClick,
        "MenuAltKey",   // ActionTriggerType::MenuAltKey,
        "ToolButton",   // ActionTriggerType::ToolButton,
        "Shortcut",      // ActionTriggerType::Shortcut,
        "DragAndDrop"   // ActionTriggerType::DragAndDrop
    };

    // make sure the array has the right number of items
    static_assert(static_cast<int>(MetricsActionTriggerType::Count) == (sizeof(triggerTypeNames) / sizeof(triggerTypeNames[0])), "A string needs to be defined for each MetricsActionTriggerType!");

    if (static_cast<int>(triggerType) >= static_cast<int>(MetricsActionTriggerType::Count))
    {
        assert(false && "There's a random, unknown trigger type!");
        triggerType = MetricsActionTriggerType::Unknown;
    }

    const char* triggerTypeName = triggerTypeNames[static_cast<int>(triggerType)];
    if (triggerTypeName == nullptr)
    {
        assert(false && "There's a random, unknown trigger type, without a string to send to metrics!");
        triggerTypeName = "Error";
    }

    LyMetrics::LyScopedMetricsEvent menuTriggeredMetric(EventNames::MenuTriggered,
    {
        {
            AttributeNames::Identifier, metricsText
        },
        {
            AttributeNames::ActionTriggerType, triggerTypeName
        }
    });
}

void ActionMetricsTracker::ActionDestroyed(QObject* obj)
{
    QAction* action = qobject_cast<QAction*>(obj);
    if ((action != nullptr) && m_actionToMetrics.contains(action))
    {
        m_actionToTriggeredConnections.remove(action);

        m_actionToMetrics.remove(action);
    }
}

void ActionMetricsTracker::ActionTriggered(QAction* action)
{
    using namespace AzToolsFramework;

    if (!m_actionToMetrics.contains(action))
    {
        return;
    }

    QString actionIdentifier = m_actionToMetrics[action];
    QByteArray actionIdentifierUtf8 = actionIdentifier.toUtf8();
    SendMetrics(actionIdentifierUtf8.data(), m_activeTrigger);

    m_activeTrigger = MetricsActionTriggerType::Unknown;
}

} // namespace LyEditorMetrics

#endif // #ifdef METRICS_SYSTEM_COMPONENT_ENABLED
