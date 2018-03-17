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

#include "stdafx.h"
#include "ActionManager.h"
#include "ShortcutDispatcher.h"
#include "QtViewPaneManager.h"

#include <QApplication>
#include <QMainWindow>
#include <QEvent>
#include <QWidget>
#include <QAction>
#include <QShortcutEvent>
#include <QShortcut>
#include <QList>
#include <QDockWidget>
#include <QDebug>
#include <QMenuBar>
#include <QSet>
#include <QPointer>

#include <assert.h>

#include <LyMetricsProducer/LyMetricsAPI.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>

#include <AzQtComponents/Buses/ShortcutDispatch.h>

const char* FOCUSED_VIEW_PANE_EVENT_NAME = "FocusedViewPaneEvent";    //Sent when view panes are focused
const char* FOCUSED_VIEW_PANE_ATTRIBUTE_NAME = "FocusedViewPaneName"; //Name of the current focused view pane

// WAF sets _DEBUG when we're in debug mode; define SHOW_ACTION_INFO_IN_DEBUGGER so that when stepping through the debugger,
// we can see what the actions are and what their keyboard shortcuts are
#define SHOW_ACTION_INFO_IN_DEBUGGER _DEBUG

static QPointer<QWidget> s_lastFocus;

// Returns either a top-level or a dock widget (regardless of floating)
// This way when docking a main window Qt::WindowShortcut still works
QWidget* ShortcutDispatcher::FindParentScopeRoot(QWidget* w)
{
    // if the current scope root is a QDockWidget, we want to bubble out
    // so we move to the parent immediately
    if (qobject_cast<QDockWidget*>(w) != nullptr)
    {
        w = w->parentWidget();
    }

    QWidget* newScopeRoot = w;
    while (newScopeRoot && newScopeRoot->parent() && !qobject_cast<QDockWidget*>(newScopeRoot))
    {
        newScopeRoot = newScopeRoot->parentWidget();
    }

    // This method should always return a parent scope root; if it can't find one
    // it returns null
    if (newScopeRoot == w)
    {
        newScopeRoot = nullptr;

        if (w != nullptr)
        {
            // we couldn't find a valid parent; broadcast a message to see if something else wants to tell us about one
            AzQtComponents::ShortcutDispatchBus::EventResult(newScopeRoot, w, &AzQtComponents::ShortcutDispatchBus::Events::GetShortcutDispatchScopeRoot, w);
        }
    }

    return newScopeRoot;
}

// Returns true if a widget is ancestor of another widget
bool ShortcutDispatcher::IsAContainerForB(QWidget* a, QWidget* b)
{
    if (!a || !b)
    {
        return false;
    }

    while (b && (a != b))
    {
        b = b->parentWidget();
    }

    return (a == b);
}

// Returns the list of QActions which have this specific key shortcut
// Only QActions under scopeRoot are considered.
QList<QAction*> ShortcutDispatcher::FindCandidateActions(QWidget* scopeRoot, const QKeySequence& sequence, QSet<QWidget*>& previouslyVisited, bool checkVisibility)
{
    QList<QAction*> actions;
    if (!scopeRoot)
    {
        return actions;
    }

    if (previouslyVisited.contains(scopeRoot))
    {
        return actions;
    }
    previouslyVisited.insert(scopeRoot);

    if ((checkVisibility && !scopeRoot->isVisible()) || !scopeRoot->isEnabled())
    {
        return actions;
    }

#ifdef SHOW_ACTION_INFO_IN_DEBUGGER
    QString matchingAgainst = sequence.toString();
    (void) matchingAgainst; // avoid an unused variable warning; want this for debugging
#endif

    // Don't just call scopeRoot->actions()! It doesn't always return the proper list, especially with the dock widgets
    for (QAction* action : scopeRoot->findChildren<QAction*>(QString(), Qt::FindDirectChildrenOnly))   // Width first
    {
#ifdef SHOW_ACTION_INFO_IN_DEBUGGER
        QString actionName = action->text();
        (void)actionName; // avoid an unused variable warning; want this for debugging
        QString shortcut = action->shortcut().toString();
        (void)shortcut; // avoid an unused variable warning; want this for debugging
#endif

        if (action->shortcut() == sequence)
        {
            actions << action;
        }
    }

    // also have to check the actions on the object directly, without looking at children
    // specifically for the master MainWindow
    for (QAction* action : scopeRoot->actions())
    {
#ifdef SHOW_ACTION_INFO_IN_DEBUGGER
        QString actionName = action->text();
        (void)actionName; // avoid an unused variable warning; want this for debugging
        QString shortcut = action->shortcut().toString();
        (void)shortcut; // avoid an unused variable warning; want this for debugging
#endif

        if (action->shortcut() == sequence)
        {
            actions << action;
        }
    }

    // Menubars have child widgets that have actions
    // But menu bar child widgets (menu items) are only visible when they've been clicked on
    // so we don't want to test visibility for child widgets of menubars
    if (qobject_cast<QMenuBar*>(scopeRoot))
    {
        checkVisibility = false;
    }

    // check the dock's central widget and the main window's
    // In some cases, they aren't in the scopeRoot's children, despite having the scopeRoot as their parent
    QDockWidget* dockWidget = qobject_cast<QDockWidget*>(scopeRoot);
    if (dockWidget)
    {
        actions << FindCandidateActions(dockWidget->widget(), sequence, previouslyVisited, checkVisibility);
    }

    QMainWindow* mainWindow = qobject_cast<QMainWindow*>(scopeRoot);
    if (mainWindow)
    {
        actions << FindCandidateActions(mainWindow->centralWidget(), sequence, previouslyVisited, checkVisibility);
    }

    for (QWidget* child : scopeRoot->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
    {
        bool isDockWidget = (qobject_cast<QDockWidget*>(child) != nullptr);
        if (isDockWidget && !actions.isEmpty())
        {
            // If we already found a candidate, don't go into dock widgets, they have lower priority
            // Since they are not focused.
            continue;
        }

        actions << FindCandidateActions(child, sequence, previouslyVisited, checkVisibility);
    }

    return actions;
}

bool ShortcutDispatcher::FindCandidateActionAndFire(QWidget* focusWidget, QShortcutEvent* shortcutEvent, QList<QAction*>& candidates, QSet<QWidget*>& previouslyVisited)
{
    candidates = FindCandidateActions(focusWidget, shortcutEvent->key(), previouslyVisited);
    QSet<QAction*> candidateSet = QSet<QAction*>::fromList(candidates);
    QAction* chosenAction = nullptr;
    int numCandidates = candidateSet.size();
    if (numCandidates == 1)
    {
        chosenAction = candidates.first();
    }
    else if (numCandidates > 1)
    {
        // If there are multiple candidates, choose the one that is parented to the ActionManager
        // since there are cases where panes with their own menu shortcuts that are docked
        // in the main window can be found in the same parent scope
        for (QAction* action : candidateSet)
        {
            if (qobject_cast<ActionManager*>(action->parent()))
            {
                chosenAction = action;
                break;
            }
        }
    }
    if (chosenAction)
    {
        if (chosenAction->isEnabled())
        {
            // Navigation triggered - shortcut in general
            AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::Shortcut);

            // has to be send, not post, or the dispatcher will get the event again and won't know that it was the one that queued it
            bool isAmbiguous = false;
            QShortcutEvent newEvent(shortcutEvent->key(), isAmbiguous);
            QApplication::sendEvent(chosenAction, &newEvent);
        }
        shortcutEvent->accept();
        return true;
    }

    return false;
}

ShortcutDispatcher::ShortcutDispatcher(QObject* parent)
    : QObject(parent)
    , m_currentlyHandlingShortcut(false)
{
    qApp->installEventFilter(this);
}

ShortcutDispatcher::~ShortcutDispatcher()
{
}

bool ShortcutDispatcher::eventFilter(QObject* obj, QEvent* ev)
{
    switch (ev->type())
    {
    case QEvent::Shortcut:
        return shortcutFilter(obj, static_cast<QShortcutEvent*>(ev));
        break;

    case QEvent::MouseButtonPress:
        if (!s_lastFocus || !IsAContainerForB(qobject_cast<QWidget*>(obj), s_lastFocus))
        {
            setNewFocus(obj);
        }
        break;

    case QEvent::FocusIn:
        setNewFocus(obj);
        break;

        // we don't really care about focus out, because something should always have the focus
        // but I'm leaving this here, so that it's clear that this is intentional
        //case QEvent::FocusOut:
        //    break;
    }

    return false;
}

QWidget* ShortcutDispatcher::focusWidget()
{
    QWidget* focusWidget = s_lastFocus; // check the widget we tracked last
    if (!focusWidget)
    {
        // we don't have anything, so fall back to using the focus object
        focusWidget = qobject_cast<QWidget*>(qApp->focusObject()); // QApplication::focusWidget() doesn't always work
    }

    return focusWidget;
}

bool ShortcutDispatcher::shortcutFilter(QObject* obj, QShortcutEvent* shortcutEvent)
{
    if (m_currentlyHandlingShortcut)
    {
        return false;
    }

    AutoBool recursiveCheck(&m_currentlyHandlingShortcut);

    QWidget* currentFocusWidget = focusWidget(); // check the widget we tracked last
    if (!currentFocusWidget)
    {
        qWarning() << Q_FUNC_INFO << "No focus widget"; // Defensive. Doesn't happen.
        return false;
    }

    // Shortcut is ambiguous, lets resolve ambiguity and give preference to QActions in the most inner scope

    // Try below the focusWidget first:
    QSet<QWidget*> previouslyVisited;
    QList<QAction*> candidates;
    if (FindCandidateActionAndFire(currentFocusWidget, shortcutEvent, candidates, previouslyVisited))    {
        return true;
    }

    // Now incrementally try bigger scopes. This handles complex cases several levels docking nesting

    QWidget* correctedTopLevel = nullptr;
    QWidget* p = currentFocusWidget;
    while (correctedTopLevel = FindParentScopeRoot(p))
    {
        if (FindCandidateActionAndFire(correctedTopLevel, shortcutEvent, candidates, previouslyVisited))
        {
            return true;
        }

        p = correctedTopLevel;
    }


    // Nothing else to do... shortcut is really ambiguous, something for the developer to fix.
    // Here's some debug info :

    qWarning() << Q_FUNC_INFO << "Ambiguous shortcut" << shortcutEvent->key() << "; focusWidget="
               << qApp->focusWidget() << "Candidate =: " << candidates << "; obj = " << obj
               << "Focused top-level=" << currentFocusWidget;

    for (auto ambiguousAction : candidates)
    {
        qWarning() << ambiguousAction << ambiguousAction->parentWidget() << ambiguousAction->associatedWidgets() << ambiguousAction->shortcut();
    }

    return false;
}

void ShortcutDispatcher::setNewFocus(QObject* obj)
{
    // Unless every widget has strong focus, mouse clicks don't change the current focus widget
    // which is a little unintuitive, compared to how we expect focus to work, right now.
    // So instead of putting strong focus on everything, we detect focus change and mouse clicks

    QWidget* widget = qobject_cast<QWidget*>(obj);

    // we only watch widgets
    if (widget == nullptr)
    {
        return;
    }

#ifdef AZ_PLATFORM_APPLE
    if (!widget->isActiveWindow())
    {
        widget->activateWindow();
        widget->setFocus();
    }
#endif

    // track it for later
    s_lastFocus = widget;

    sendFocusMetricsData(obj);
}

void ShortcutDispatcher::sendFocusMetricsData(QObject* obj)
{
    // if we can't find the parent widget in the following search, then we assume it's a main window
    QString parentWidgetName = "MainWindow";

    while (obj != nullptr)
    {
        if (QtViewPaneManager::instance()->GetView(obj->objectName()) == obj)
        {
            parentWidgetName = obj->objectName().toUtf8();
            break;
        }
        obj = obj->parent();
    }

    if (!m_previousWidgetName.isEmpty() && parentWidgetName.compare(m_previousWidgetName) != 0)
    {
        SubmitMetricsEvent(parentWidgetName.toUtf8());
    }

    m_previousWidgetName = parentWidgetName;
}

void ShortcutDispatcher::SubmitMetricsEvent(const char* attributeName)
{
    //Send metrics event for the current focused view pane
    auto eventId = LyMetrics_CreateEvent(FOCUSED_VIEW_PANE_EVENT_NAME);

    // Add attribute to show what pane is focused
    LyMetrics_AddAttribute(eventId, FOCUSED_VIEW_PANE_ATTRIBUTE_NAME, attributeName);
    LyMetrics_SubmitEvent(eventId);
}


#include <ShortcutDispatcher.moc>
