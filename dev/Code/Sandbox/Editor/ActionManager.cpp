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
#include "ActionManager.h"
#include "ShortcutDispatcher.h"
#include "MainWindow.h"

#include "ToolbarManager.h"
#include "QtViewPaneManager.h"
#include <QAction>
#include <QKeySequence>
#include <QMenu>
#include <QSignalMapper>
#include <QApplication>
#include <QAbstractEventDispatcher>
#include <QDockWidget>
#include <QDebug>
#include <QWidgetAction>
#include <QShortcutEvent>
#include <QSettings>
#include <QMenuBar>
#include <QScopedValueRollback>

#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>

PatchedAction::PatchedAction(const QString& name, QObject* parent)
    : QAction(name, parent)
{
}

bool PatchedAction::event(QEvent* ev)
{
    // *Really* honour Qt::WindowShortcut. Floating dock widgets are a separate window (Qt::Window flag is set) even though they have a parent.

    if (ev->type() == QEvent::Shortcut && shortcutContext() == Qt::WindowShortcut)
    {
        // This prevents shortcuts from firing while we're in a long running operation
        // started by a shortcut
        static bool reentranceLock = false;
        if (reentranceLock)
        {
            return true;
        }

        QScopedValueRollback<bool> reset(reentranceLock, true);

        QWidget* focusWidget = ShortcutDispatcher::focusWidget();
        if (!focusWidget)
        {
            return QAction::event(ev);
        }

        for (QWidget* associatedWidget : associatedWidgets())
        {
            QWidget* associatedWindow = associatedWidget->window();
            QWidget* focusWindow = focusWidget->window();
            if (associatedWindow == focusWindow)
            {
                // Fair enough, we accept it.
                return QAction::event(ev);
            }
            else if (associatedWindow && focusWindow)
            {
                QString focusWindowName = focusWindow->objectName();
                if (focusWindowName.isEmpty())
                {
                    continue;
                }

                QWidget* child = associatedWindow->findChild<QWidget*>(focusWindowName);
                if (child)
                {
                    // Also accept if the focus window is a child of the associated window
                    return QAction::event(ev);
                }
            }
        }

        // Bug detected. Consume the event instead of processing it.
        qDebug() << "Discarding buggy shortcut";
        return true;
    }

    return QAction::event(ev);
}

/////////////////////////////////////////////////////////////////////////////
// ActionWrapper
/////////////////////////////////////////////////////////////////////////////
ActionManager::ActionWrapper& ActionManager::ActionWrapper::SetMenu(DynamicMenu* menu)
{
    menu->SetAction(m_action, m_actionManager);
    return *this;
}

ActionManager::ActionWrapper& ActionManager::ActionWrapper::SetMetricsIdentifier(const QString& metricsIdentifier)
{
    // for the time being, allow these metrics to be quickly and easily removed
    QSettings settings;
    if (settings.value("DisableMenuMetrics").toBool())
    {
        return *this;
    }

    using namespace AzToolsFramework;
    EditorMetricsEventsBus::Broadcast(&EditorMetricsEventsBus::Events::RegisterAction, m_action, metricsIdentifier);

    return *this;
}

ActionManager::ActionWrapper& ActionManager::ActionWrapper::SetMetricsIdentifier(const QString& group, const QString& metricsIdentifier)
{
    return SetMetricsIdentifier(QString("%1%2").arg(group).arg(metricsIdentifier));
}

ActionManager::ActionWrapper& ActionManager::ActionWrapper::SetApplyHoverEffect()
{
    // Our standard toolbar icons, when hovered on, get a white color effect.
    // But for this to work we need .pngs that look good with this effect, so this only works with the standard toolbars
    // and looks very ugly for other toolbars, including toolbars loaded from XML (which just show a white rectangle)
    m_action->setProperty("IconHasHoverEffect", true);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////
// DynamicMenu
/////////////////////////////////////////////////////////////////////////////
DynamicMenu::DynamicMenu(QObject* parent)
    : QObject(parent)
    , m_action(nullptr)
    , m_menu(nullptr)
{
    m_actionMapper = new QSignalMapper(this);
    connect(m_actionMapper, SIGNAL(mapped(int)), this, SLOT(TriggerAction(int)));
}

void DynamicMenu::SetAction(QAction* action, ActionManager* am)
{
    Q_ASSERT(action);
    Q_ASSERT(am);
    m_action = action;
    m_menu = new QMenu();
    m_action->setMenu(m_menu);
    connect(m_menu, &QMenu::aboutToShow, this, &DynamicMenu::ShowMenu);
    m_actionManager = am;
    setParent(m_action);
}

void DynamicMenu::SetParentMenu(QMenu* menu, ActionManager* am)
{
    Q_ASSERT(menu && !m_menu);
    Q_ASSERT(!m_action);
    m_menu = menu;
    connect(m_menu, &QMenu::aboutToShow, this, &DynamicMenu::ShowMenu);
    m_actionManager = am;
}

void DynamicMenu::AddAction(int id, QAction* action)
{
    Q_ASSERT(!m_actions.contains(id));
    action->setData(id);
    m_actions[id] = action;
    m_menu->addAction(action);

    connect(action, SIGNAL(triggered()), m_actionMapper, SLOT(map()));
    m_actionMapper->setMapping(action, id);
}

void DynamicMenu::AddSeparator()
{
    Q_ASSERT(m_menu);
    m_menu->addSeparator();
}

ActionManager::ActionWrapper DynamicMenu::AddAction(int id, const QString& name)
{
    QAction* action = new PatchedAction(name, this);
    AddAction(id, action);
    return ActionManager::ActionWrapper(action, m_actionManager);
}

void DynamicMenu::UpdateAllActions()
{
    foreach(auto action, m_menu->actions())
    {
        int id = action->data().toInt();
        OnMenuUpdate(id, action);
    }
}

void DynamicMenu::ShowMenu()
{
    if (m_actions.isEmpty())
    {
        CreateMenu();
    }
    UpdateAllActions();
}

void DynamicMenu::TriggerAction(int id)
{
    OnMenuChange(id, m_actions.value(id));
    UpdateAllActions();
}


#include <QTimer>

/////////////////////////////////////////////////////////////////////////////
// ActionManager
/////////////////////////////////////////////////////////////////////////////
ActionManager::ActionManager(MainWindow* parent, QtViewPaneManager* const qtViewPaneManager)
    : QObject(parent)
    , m_mainWindow(parent)
    , m_qtViewPaneManager(qtViewPaneManager)
{
    m_actionMapper = new QSignalMapper(this);
    connect(m_actionMapper, SIGNAL(mapped(int)), this, SLOT(ActionTriggered(int)));

    connect(m_qtViewPaneManager, &QtViewPaneManager::registeredPanesChanged, this, &ActionManager::RebuildRegisteredViewPaneIds);

    // KDAB_TODO: This will be used later, particularly for the toolbars
    //connect(QCoreApplication::eventDispatcher(), SIGNAL(aboutToBlock()),
    //    this, SLOT(UpdateActions()));
    // so long use a simple timer to make it work
    QTimer* timer = new QTimer(this);
    timer->setInterval(250);
    connect(timer, &QTimer::timeout, this, &ActionManager::UpdateActions);
    timer->start();
}

ActionManager::~ActionManager()
{
}

void ActionManager::AddMenu(QMenu* menu)
{
    m_menus.push_back(menu);
    connect(menu, &QMenu::aboutToShow, this, &ActionManager::UpdateMenu);
}

ActionManager::MenuWrapper ActionManager::AddMenu(const QString& title)
{
    auto menu = new QMenu(title);
    AddMenu(menu);
    return MenuWrapper(menu, this);
}

void ActionManager::AddToolBar(QToolBar* toolBar)
{
    m_toolBars.push_back(toolBar);
}

ActionManager::MenuWrapper ActionManager::CreateMenuPath(const QStringList& menuPath)
{
    MenuWrapper parentMenu(GetMenu(menuPath.value(0)), this);
    if (parentMenu.isNull())
    {
        // This doesn't happen, the root menu must always exist, we don't support creating arbitrary top-level menus
        Q_ASSERT(false);
        return {};
    }

    const int numMenus = menuPath.size();
    for (int i = 1; i < numMenus; ++i)
    {
        QList<QAction*> actions = parentMenu->actions();
        QMenu* subMenu = GetMenu(menuPath[i], actions);
        if (subMenu)
        {
            parentMenu = MenuWrapper(subMenu, this);
        }
        else
        {
            parentMenu = parentMenu.AddMenu(menuPath.at(i));
        }
    }

    return parentMenu;
}

QMenu* ActionManager::GetMenu(const QString& name) const
{
    auto it = std::find_if(m_menus.cbegin(), m_menus.cend(), [&name](QMenu* m) { return m->title() == name; });
    return it == m_menus.cend() ? nullptr : *it;
}

QMenu* ActionManager::GetMenu(const QString& name, const QList<QAction*>& actions) const
{
    auto it = std::find_if(actions.cbegin(), actions.cend(), [&name](QAction* a) { return a->menu() && a->menu()->title() == name; });
    return it == actions.cend() ? nullptr : (*it)->menu();
}

ActionManager::ToolBarWrapper ActionManager::AddToolBar(int id)
{
    AmazonToolbar t = m_mainWindow->GetToolbarManager()->GetToolbar(id);
    Q_ASSERT(t.IsInstantiated());

    AddToolBar(t.Toolbar());
    return ToolBarWrapper(t.Toolbar(), this);
}

bool ActionManager::eventFilter(QObject* watched, QEvent* event)
{
    // if events are shortcut events, we don't want to filter out
    if (event->type() == QEvent::Shortcut)
    {
        m_isShortcutEvent = true;
    }

    return false;
}


void ActionManager::AddAction(int id, QAction* action)
{
    action->setData(id);
    AddAction(action);
}

void ActionManager::AddAction(QAction* action)
{
    const int id = action->data().toInt();

    if (m_actions.contains(id))
    {
        qWarning() << "ActionManager already contains action with id=" << id;
        Q_ASSERT(false);
    }

    m_actions[id] = action;
    connect(action, SIGNAL(triggered()), m_actionMapper, SLOT(map()));
    m_actionMapper->setMapping(action, id);

    action->installEventFilter(this);

    // Add the action if the parent is a widget
    auto widget = qobject_cast<QWidget*>(parent());
    if (widget)
    {
        widget->addAction(action);
    }

    // This is to prevent icons being shown in the main menu.
    // Currently, the goal is to show icons in the toolbar but not in the main menu,
    // and the code that handles showing icon in the LevelEditorMenuHandler.cpp
    // has been removed. This fix is a short term solution as in the future
    // we need to add custom different icons on the menus.
    action->setIconVisibleInMenu(false);
}

ActionManager::ActionWrapper ActionManager::AddAction(int id, const QString& name)
{
    QAction* action = ActionIsWidget(id) ? new WidgetAction(id, m_mainWindow, name, this)
        : static_cast<QAction*>(new PatchedAction(name, this)); // static cast to base so ternary compiles
    AddAction(id, action);
    return ActionWrapper(action, this);
}

bool ActionManager::HasAction(QAction* action) const
{
    return action && HasAction(action->data().toInt());
}

bool ActionManager::HasAction(int id) const
{
    return m_actions.contains(id);
}

QAction* ActionManager::GetAction(int id) const
{
    auto it = m_actions.find(id);

    if (it == m_actions.cend())
    {
        qWarning() << Q_FUNC_INFO << "Couldn't get action " << id;
        Q_ASSERT(false);
        return nullptr;
    }
    else
    {
        return *it;
    }
}

void ActionManager::ActionTriggered(int id)
{
    if (m_mainWindow->menuBar()->isEnabled())
    {
        if (m_actionHandlers.contains(id))
        {
            SendMetricsEvent(id);
            m_actionHandlers[id]();
        }
    }
}

void ActionManager::RebuildRegisteredViewPaneIds()
{
    QtViewPanes views = QtViewPaneManager::instance()->GetRegisteredPanes();

    for (auto& view : views)
    {
        m_registeredViewPaneIds.insert(view.m_id);
    }
}

void ActionManager::SendMetricsEvent(int id)
{
    QByteArray viewPaneName = this->HasAction(id) ? this->GetAction(id)->text().toUtf8() : QString("Missing text (id: %0)").arg(id).toUtf8();
    
    // only do this once for performance issue
    if (m_editorToolbarIds.empty())
    {
        AmazonToolbar standardToolbarDefault = m_mainWindow->GetToolbarManager()->GetEditorsToolbar();

        for (int currentId : standardToolbarDefault.ActionIds())
        {
            m_editorToolbarIds.insert(currentId);
        }
    }
    
    // Send metrics signal
    if (m_editorToolbarIds.contains(id))
    {
        emit SendMetricsSignal(viewPaneName, "Toolbar");
    }

    else
    {
        if (m_registeredViewPaneIds.contains(id))
        {
            if (m_isShortcutEvent)
            {
                emit SendMetricsSignal(viewPaneName, "Shortcut");
            }

            else
            {
                emit SendMetricsSignal(viewPaneName, "MainMenu");
            }
        }
    }

    m_isShortcutEvent = false;
}

void ActionManager::UpdateMenu()
{
    auto menu = qobject_cast<QMenu*>(sender());

    // Call all update callbacks for the given menu
    foreach (auto action, menu->actions())
    {
        int id = action->data().toInt();
        if (m_updateCallbacks.contains(id))
        {
            m_updateCallbacks.value(id)();
        }
    }
}

void ActionManager::UpdateActions()
{
    for (auto it = m_updateCallbacks.constBegin(); it != m_updateCallbacks.constEnd(); ++it)
    {
        it.value()();
    }
}

QList<QAction*> ActionManager::GetActions() const
{
    return m_actions.values();
}

bool ActionManager::ActionIsWidget(int id) const
{
    return id >= ID_TOOLBAR_WIDGET_FIRST && id <= ID_TOOLBAR_WIDGET_LAST;
}

WidgetAction::WidgetAction(int actionId, MainWindow* mainWindow, const QString& name, QObject* parent)
    : QWidgetAction(parent)
    , m_actionId(actionId)
    , m_mainWindow(mainWindow)
{
    setText(name);
}

QWidget* WidgetAction::createWidget(QWidget* parent)
{
    QWidget* w = m_mainWindow->CreateToolbarWidget(m_actionId);
    if (w)
    {
        w->setParent(parent);
    }

    return w;
}

#include <ActionManager.moc>
