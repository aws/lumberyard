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

#include <Tests/UI/UIFixture.h>
#include <Integration/System/SystemCommon.h>

#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/PluginManager.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzQtComponents/Utilities/QtPluginPaths.h>

#include <Editor/ReselectingTreeView.h>

#include <QtTest>
#include <QApplication>
#include <QWidget>
#include <QToolBar>

namespace EMotionFX
{
    void MakeQtApplicationBase::SetUp()
    {
        AzQtComponents::PrepareQtPaths();

        int argc = 0;
        m_uiApp = new QApplication(argc, nullptr);

        AzToolsFramework::EditorEvents::Bus::Broadcast(&AzToolsFramework::EditorEvents::NotifyRegisterViews);
    }

    MakeQtApplicationBase::~MakeQtApplicationBase()
    {
        delete m_uiApp;
    }

    void UIFixture::SetupQtAndFixtureBase()
    {
        UIFixtureBase::SetUp();
        MakeQtApplicationBase::SetUp();
        // Set ignore visibilty so that the visibility check can be ignored in plugins
        EMStudio::GetManager()->SetIgnoreVisibility(true);
    }
    void UIFixture::SetupPluginWindows()
    {
        // Plugins have to be created after both the QApplication object and
        // after the SystemComponent
        const uint32 numPlugins = EMStudio::GetPluginManager()->GetNumPlugins();
        for (uint32 i = 0; i < numPlugins; ++i)
        {
            EMStudio::EMStudioPlugin* plugin = EMStudio::GetPluginManager()->GetPlugin(i);
            EMStudio::GetPluginManager()->CreateWindowOfType(plugin->GetName());
        }
    }


    void UIFixture::SetUp()
    {
        SetupQtAndFixtureBase();
        SetupPluginWindows();
    }

    void UIFixture::TearDown()
    {
        CloseAllNotificationWindows();
        // Restore visibility
        EMStudio::GetManager()->SetIgnoreVisibility(false);

        UIFixtureBase::TearDown();
    }

    QWidget* UIFixture::FindTopLevelWidget(const QString& objectName)
    {
        //const QWidgetList topLevelWidgets = QApplication::topLevelWidgets(); // TODO: Check why QDialogs are no windows anymore and thus the topLevelWidgets() does not include them.
        const QWidgetList topLevelWidgets = QApplication::allWidgets();
        auto iterator = AZStd::find_if(topLevelWidgets.begin(), topLevelWidgets.end(),
            [=](const QWidget* widget)
            {
                return (widget->objectName() == objectName);
            });

        if (iterator != topLevelWidgets.end())
        {
            return *iterator;
        }

        return nullptr;
    }

    QWidget* UIFixture::GetWidgetFromToolbar(const QToolBar* toolbar, const QString &widgetText)
    {
        /*
        Searches a Toolbar for an action whose text exactly matches the widgetText parameter.
        Returns the widget by pointer if found, nullptr otherwise.
        */
        for (QAction* action : toolbar->actions())
        {
            if (action->text() == widgetText)
            {
                return toolbar->widgetForAction(action);
            }
        }
        return nullptr;
    }

    QWidget* UIFixture::GetWidgetFromToolbarWithObjectName(const QToolBar* toolbar, const QString &objectName)
    {
        for (QAction* action : toolbar->actions())
        {
            if (action->objectName() == objectName)
            {
                return toolbar->widgetForAction(action);
            }
        }
        return nullptr;
    }

    QWidget* UIFixture::GetWidgetWithNameFromNamedToolbar(const QWidget* widget, const QString &toolBarName, const QString &objectName)
    {
        auto toolBar = widget->findChild<QToolBar*>(toolBarName);
        if (!toolBar)
        {
            return nullptr;
        }

        return UIFixture::GetWidgetFromToolbarWithObjectName(toolBar, objectName);
    }

    QAction* UIFixture::GetNamedAction(const QWidget* widget, const QString& actionText)
    {
        const QList<QAction*> actions = widget->findChildren<QAction*>();

        for (QAction* action : actions)
        {
            if (action->text() == actionText)
            {
                return action;
            }
        }

        return nullptr;
    }

    bool UIFixture::GetActionFromContextMenu(QAction*& action, const QMenu* contextMenu, const QString& actionName)
    {
        const auto contextMenuActions = contextMenu->actions();
        auto contextAction = AZStd::find_if(contextMenuActions.begin(), contextMenuActions.end(), [actionName](const QAction* action) {
            return action->text() == actionName;
            });
        action = *contextAction;
        return contextAction != contextMenuActions.end();
    }

    void UIFixture::CloseAllPlugins()
    {
        const EMStudio::PluginManager::PluginVector plugins = EMStudio::GetPluginManager()->GetActivePlugins();
        for (EMStudio::EMStudioPlugin* plugin : plugins)
        {
            EMStudio::GetPluginManager()->RemoveActivePlugin(plugin);
        }
    }

    void UIFixture::CloseAllNotificationWindows()
    {
        while (EMStudio::GetManager()->GetNotificationWindowManager()->GetNumNotificationWindow() > 0)
        {
            EMStudio::NotificationWindow* window = EMStudio::GetManager()->GetNotificationWindowManager()->GetNotificationWindow(0);
            delete window;
        }
    }

    void UIFixture::BringUpContextMenu(const QTreeView* treeView, const QRect& rect)
    {
        QContextMenuEvent cme(QContextMenuEvent::Mouse, rect.center(), treeView->viewport()->mapTo(treeView->window(), rect.center()));
        QSpontaneKeyEvent::setSpontaneous(&cme);
        QApplication::instance()->notify(
            treeView->viewport(),
            &cme
        );
    }

    void UIFixture::SelectIndexes(const QModelIndexList& indexList, QTreeView* treeView, const int start, const int end)
    {
        QItemSelection selection;
        for (int i = start; i <= end; ++i)
        {
            const QModelIndex index = indexList[i];
            EXPECT_TRUE(index.isValid()) << "Unable to find a model index for the joint of the actor";

            selection.select(index, index);
        }
        treeView->selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        treeView->scrollTo(indexList[end]);
    }

    AzToolsFramework::PropertyRowWidget* UIFixture::GetNamedPropertyRowWidgetFromReflectedPropertyEditor(AzToolsFramework::ReflectedPropertyEditor* rpe, const QString& name)
    {
        // Search through the RPE's widgets to find the matching widget
        const AzToolsFramework::ReflectedPropertyEditor::WidgetList& widgets = rpe->GetWidgets();
        AzToolsFramework::PropertyRowWidget* finalRowWidget = nullptr;
        for (auto& widgetIter : widgets)
        {
            AzToolsFramework::InstanceDataNode* dataNode = widgetIter.first;
            AzToolsFramework::PropertyRowWidget* rowWidget = widgetIter.second;
            QString labelName = rowWidget->label();
            if (name == labelName)
            {
                finalRowWidget = rowWidget;
                break;
            }
        }
        return finalRowWidget;
    }



} // namespace EMotionFX
