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

#ifndef SHORTCUT_DISPATCHER_H
#define SHORTCUT_DISPATCHER_H

#include <QObject>

template<typename T> class QSet;
template<typename T> class QList;
template<typename T> class QPointer;

class QWidget;
class QShortcutEvent;

// This class provides a workaround against Qt's buggy implementation of shortcut contexts when using dock widgets.

class ShortcutDispatcher
    : public QObject
{
    Q_OBJECT

    struct AutoBool
    {
        AutoBool(bool* value)
            : m_value(value)
        {
            * m_value = true;
        }

        ~AutoBool()
        {
            * m_value = false;
        }

        bool* m_value;
    };

public:
    explicit ShortcutDispatcher(QObject* parent = nullptr);
    ~ShortcutDispatcher();
    bool eventFilter(QObject* obj, QEvent* ev) override;

    static QWidget* focusWidget();

    static void SubmitMetricsEvent(const char* attributeName);

private:
    bool shortcutFilter(QObject* obj, QShortcutEvent* ev);
    void setNewFocus(QObject* obj);

    QWidget* FindParentScopeRoot(QWidget* w);
    bool IsAContainerForB(QWidget* a, QWidget* b);
    QList<QAction*> FindCandidateActions(QWidget* scopeRoot, const QKeySequence& sequence, QSet<QWidget*>& previouslyVisited, bool checkVisibility = true);
    bool FindCandidateActionAndFire(QWidget* focusWidget, QShortcutEvent* shortcutEvent, QList<QAction*>& candidates, QSet<QWidget*>& previouslyVisited);

    void sendFocusMetricsData(QObject* obj);

    bool m_currentlyHandlingShortcut;

    QString m_previousWidgetName;
};

#endif
