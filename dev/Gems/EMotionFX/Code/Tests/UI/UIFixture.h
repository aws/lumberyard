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

#include <Tests/SystemComponentFixture.h>

#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>

#include <QString>
#include <QToolBar>
#include <QTreeView>

QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QTreeView)
QT_FORWARD_DECLARE_CLASS(ReselectingTreeView)

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
}

namespace EMotionFX
{
    class MakeQtApplicationBase
    {
    public:
        MakeQtApplicationBase() = default;
        AZ_DEFAULT_COPY_MOVE(MakeQtApplicationBase);
        virtual ~MakeQtApplicationBase();

        void SetUp();

    protected:
        QApplication* m_uiApp = nullptr;
    };

    using UIFixtureBase = ComponentFixture<
        AZ::MemoryComponent,
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        AZ::UserSettingsComponent,
        AzToolsFramework::Components::PropertyManagerComponent,
        EMotionFX::Integration::SystemComponent
    >;

    // MakeQtApplicationBase is listed as the first base class, so that the
    // QApplication object is destroyed after the EMotionFX SystemComponent is
    // shut down
    class UIFixture
        : public MakeQtApplicationBase
        , public UIFixtureBase
    {
    public:
        void SetUp() override;
        void TearDown() override;
        static QWidget* FindTopLevelWidget(const QString& objectName);
        static QWidget* GetWidgetFromToolbar(const QToolBar* toolbar, const QString &widgetText);
        static QWidget* GetWidgetFromToolbarWithObjectName(const QToolBar* toolbar, const QString &objectName);
        static QWidget* GetWidgetWithNameFromNamedToolbar(const QWidget* widget, const QString &toolBarName, const QString &objectName);

        static QAction* GetNamedAction(const QWidget* widget, const QString& actionName);
        static bool GetActionFromContextMenu(QAction*& action, const QMenu* contextMenu, const QString& actionName);

        void CloseAllPlugins();
        void CloseAllNotificationWindows();
        void BringUpContextMenu(const QTreeView* treeview, const QRect& rect);
        void SelectIndexes(const QModelIndexList& indexList, QTreeView* treeView, const int start, const int end);
        AzToolsFramework::PropertyRowWidget* GetNamedPropertyRowWidgetFromReflectedPropertyEditor(AzToolsFramework::ReflectedPropertyEditor* rpe, const QString& name);
    protected:
        void SetupQtAndFixtureBase();
        void SetupPluginWindows();

        QApplication* m_uiApp = nullptr;
    };
} // end namespace EMotionFX
