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
#ifndef ACTIONMANAGER_H
#define ACTIONMANAGER_H

#include <QObject>

#include <QPointer>
#include <QHash>
#include <QVector>
#include <QAction>
#include <QMenu>
#include <QToolBar>
#include <QIcon>
#include <QWidgetAction>

#include <QSet>

#include <functional>
#include <utility>

class QSignalMapper;
class QPixmap;
class QDockWidget;
class DynamicMenu;
class MainWindow;
class QtViewPaneManager;

class PatchedAction
    : public QAction
{
    // PatchedAction is a workaround for the fact that Qt doesn't honour Qt::WindowShortcut context for floating dock widgets.

    Q_OBJECT
public:
    explicit PatchedAction(const QString& name, QObject* parent = nullptr);
    bool event(QEvent*) override;
};


class WidgetAction
    : public QWidgetAction
{
    Q_OBJECT
public:
    explicit WidgetAction(int actionId, MainWindow* mainWindow, const QString& name, QObject* parent);
protected:
    QWidget* createWidget(QWidget* parent) override;
private:
    const int m_actionId;
    MainWindow* const m_mainWindow;
};

class ActionManager
    : public QObject
{
    Q_OBJECT

public:
    class ActionWrapper
    {
    public:
        ActionWrapper& SetText(const QString& text) { m_action->setText(text); return *this; }
        ActionWrapper& SetIcon(const QIcon& icon) { m_action->setIcon(icon); return *this; }
        ActionWrapper& SetIcon(const QPixmap& icon) { m_action->setIcon(QIcon(icon)); return *this; }
        ActionWrapper& SetShortcut(const QString& shortcut) { m_action->setShortcut(shortcut); return *this; }
        ActionWrapper& SetShortcut(const QKeySequence& shortcut) { m_action->setShortcut(shortcut); return *this; }
        ActionWrapper& SetToolTip(const QString& toolTip) { m_action->setToolTip(toolTip); return *this; }
        ActionWrapper& SetStatusTip(const QString& statusTip) { m_action->setStatusTip(statusTip); return *this; }

        ActionWrapper& SetCheckable(bool value) { m_action->setCheckable(value); return *this; }

        //ActionWrapper &SetMenu(QMenu *menu) { m_action->setMenu(menu); return *this; }
        //ActionWrapper &SetMenu(QMenu *menu) { m_action->setMenu(menu); return *this; }
        template <typename Func1, typename Func2>
        ActionWrapper& Connect(Func1 signal, Func2 slot)
        {
            QObject::connect(m_action, signal, [slot]()
                {
                    if (!GetIEditor()->IsInGameMode())
                    {
                        slot();
                    }
                });
            return *this;
        }

        template <typename Func1, typename Object, typename Func2>
        ActionWrapper& Connect(Func1 signal, Object* context, const Func2 slot, Qt::ConnectionType type = Qt::AutoConnection)
        {
            QObject::connect(m_action, signal, context, [context, slot]()
                {
                    if (!GetIEditor()->IsInGameMode())
                    {
                        (context->*slot)();
                    }
                }, type);

            return *this;
        }

        ActionWrapper& SetMenu(DynamicMenu* menu);
        ActionWrapper& SetMetricsIdentifier(const QString& metricsIdentifier);
        ActionWrapper& SetMetricsIdentifier(const QString& group, const QString& metricsIdentifier);
        ActionWrapper& SetApplyHoverEffect();

        operator QAction*() const {
            return m_action;
        }
        QAction* operator->() const { return m_action; }

        template<typename T>
        ActionWrapper& RegisterUpdateCallback(T* object, void (T::* method)(QAction*))
        {
            m_actionManager->RegisterUpdateCallback(m_action->data().toInt(), object, method);
            return *this;
        }

    private:
        friend ActionManager;
        friend DynamicMenu;
        ActionWrapper(QAction* action, ActionManager* am)
            : m_action(action)
            , m_actionManager(am) { Q_ASSERT(m_action); }

        QAction* m_action;
        ActionManager* m_actionManager;
    };

    class MenuWrapper
    {
    public:
        MenuWrapper() = default;
        MenuWrapper& SetTitle(const QString& text) { m_menu->setTitle(text); return *this; }
        MenuWrapper& SetIcon(const QIcon& icon) { m_menu->setIcon(icon); return *this; }

        QAction* AddAction(int id)
        {
            auto action = m_actionManager->GetAction(id);
            Q_ASSERT(action);
            m_menu->addAction(action);

            return action;
        }

        QAction* AddSeparator()
        {
            return m_menu->addSeparator();
        }

        MenuWrapper AddMenu(const QString& name)
        {
            auto menu = m_actionManager->AddMenu(name);
            m_menu->addMenu(menu);
            return menu;
        }

        bool isNull() const
        {
            return !m_menu;
        }

        operator QMenu*() const
        {
            return m_menu;
        }
        QMenu* operator->() const { return m_menu; }

        QMenu* Get() const
        {
            return m_menu;
        }

    private:
        friend ActionManager;
        MenuWrapper(QMenu* menu, ActionManager* am)
            : m_menu(menu)
            , m_actionManager(am) { Q_ASSERT(m_menu); }

        QPointer<QMenu> m_menu = nullptr;
        ActionManager* m_actionManager = nullptr;
    };

    class ToolBarWrapper
    {
    public:
        QAction* AddAction(int id)
        {
            auto action = m_actionManager->GetAction(id);
            Q_ASSERT(action);
            m_toolBar->addAction(action);

            return action;
        }

        QAction* AddSeparator()
        {
            QAction* action = m_toolBar->addSeparator();

            // For the Dnd to work:
            action->setData(ID_TOOLBAR_SEPARATOR);
            QWidget* w = m_toolBar->widgetForAction(action);
            w->addAction(action);

            return action;
        }

        void AddWidget(QWidget* widget)
        {
            m_toolBar->addWidget(widget);
        }

        operator QToolBar*() const {
            return m_toolBar;
        }
        QToolBar* operator->() const { return m_toolBar; }

    private:
        friend ActionManager;
        ToolBarWrapper(QToolBar* toolBar, ActionManager* am)
            : m_toolBar(toolBar)
            , m_actionManager(am) { Q_ASSERT(m_toolBar); }

        QToolBar* m_toolBar;
        ActionManager* m_actionManager;
    };

public:
    explicit ActionManager(MainWindow* parent, QtViewPaneManager* qtViewPaneManager);
    ~ActionManager();
    void AddMenu(QMenu* menu);
    MenuWrapper AddMenu(const QString& name);
    MenuWrapper CreateMenuPath(const QStringList& menuPath);
    bool ActionIsWidget(int actionId) const;

    void AddToolBar(QToolBar* toolBar);
    ToolBarWrapper AddToolBar(int id);

    void AddAction(QAction* action);
    void AddAction(int id, QAction* action);
    ActionWrapper AddAction(int id, const QString& name);
    bool HasAction(QAction*) const;
    bool HasAction(int id) const;

    QAction* GetAction(int id) const;
    QList<QAction*> GetActions() const;

    template<typename T>
    void RegisterUpdateCallback(int id, T* object, void (T::* method)(QAction*))
    {
        Q_ASSERT(m_actions.contains(id));
        auto f = std::bind(method, object, m_actions.value(id));
        m_updateCallbacks[id] = f;
    }

    template<typename T>
    void RegisterActionHandler(int id, T method)
    {
        m_actionHandlers[id] = method;
    }
    template<typename T>
    void RegisterActionHandler(int id, T* object, void (T::* method)())
    {
        m_actionHandlers[id] = std::bind(method, object);
    }
    template<typename T>
    void RegisterActionHandler(int id, T* object, void (T::* method)(UINT))
    {
        m_actionHandlers[id] = std::bind(method, object, id);
    }

    void SendMetricsEvent(int id);
    bool eventFilter(QObject* watched, QEvent* event);

Q_SIGNALS:
    void SendMetricsSignal(const char* viewPaneName, const char* openLocation);

public slots:
    void ActionTriggered(int id);

private slots:
    void UpdateMenu();
    void UpdateActions();

private:
    QMenu* GetMenu(const QString& name) const;
    QMenu* GetMenu(const QString& name, const QList<QAction*>& actions) const;
    QHash<int, QAction*> m_actions;
    QVector<QMenu*> m_menus;
    QVector<QToolBar*> m_toolBars;
    QSignalMapper* m_actionMapper;
    QHash<int, std::function<void()> > m_updateCallbacks;
    QHash<int, std::function<void()> > m_actionHandlers;
    MainWindow* const m_mainWindow;

    QtViewPaneManager* m_qtViewPaneManager;

    // for sending shortcut metrics events
    bool m_isShortcutEvent = false;

    // for sending toolbar metrics events
    QSet<int> m_editorToolbarIds;

    // for sending main menu metrics events
    QSet<int> m_registeredViewPaneIds;

    // Update the registered view pane Ids when the registered view pane list is modified
    void RebuildRegisteredViewPaneIds();
};

class DynamicMenu
    : public QObject
{
    Q_OBJECT

public:
    explicit DynamicMenu(QObject* parent = nullptr);
    void SetAction(QAction* action, ActionManager* am);
    void SetParentMenu(QMenu* menu, ActionManager* am);

protected:
    virtual void CreateMenu() = 0;
    virtual void OnMenuUpdate(int id, QAction* action) = 0;
    virtual void OnMenuChange(int id, QAction* action) = 0;

    void AddAction(int id, QAction* action);
    ActionManager::ActionWrapper AddAction(int id, const QString& name);
    void AddSeparator();

    void UpdateAllActions();
    ActionManager* m_actionManager;

private slots:
    void ShowMenu();
    void TriggerAction(int id);

private:
    QAction* m_action;
    QMenu* m_menu;
    QSignalMapper* m_actionMapper;
    QHash<int, QAction*> m_actions;
};

#endif // ACTIONMANAGER_H
