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

#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/LumberyardStylesheet.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include "ComponentDemoWidget.h"

#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QAction>
#include <QSettings>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

const QString g_ui_1_0_SettingKey = QStringLiteral("useUI_1_0");

static void LogToDebug(QtMsgType Type, const QMessageLogContext& Context, const QString& message)
{
#ifdef Q_OS_WIN
    OutputDebugStringW(L"Qt: ");
    OutputDebugStringW(reinterpret_cast<const wchar_t*>(message.utf16()));
    OutputDebugStringW(L"\n");
#endif
}

int main(int argv, char **argc)
{
    QApplication::setOrganizationName("Amazon");
    QApplication::setOrganizationDomain("amazon.com");
    QApplication::setApplicationName("LumberyardWidgetGallery");

    AzQtComponents::PrepareQtPaths();

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    qInstallMessageHandler(LogToDebug);

    QApplication app(argv, argc);
    AzQtComponents::StyleManager styleManager(&app);
    styleManager.Initialize(&app);

    QSettings settings;
    bool legacyUISetting = settings.value(g_ui_1_0_SettingKey, false).toBool();
    styleManager.SwitchUI(&app, legacyUISetting);

    auto wrapper = new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionAutoAttach);
    auto widget = new ComponentDemoWidget(wrapper);
    widget->resize(550, 900);
    widget->show();

    wrapper->enableSaveRestoreGeometry("windowGeometry");
    wrapper->restoreGeometryFromSettings();
    //wrapper->show();

    auto fileMenu = widget->menuBar()->addMenu("&File");

    auto styleToggle = fileMenu->addAction("Enable UI 1.0");
    styleToggle->setShortcut(QKeySequence("Ctrl+T"));
    styleToggle->setCheckable(true);
    styleToggle->setChecked(legacyUISetting);
    QObject::connect(styleToggle, &QAction::toggled, &styleManager, [&styleManager, &app, &settings](bool checked) {
        styleManager.SwitchUI(&app, checked);

        settings.setValue(g_ui_1_0_SettingKey, checked);
        settings.sync();
    });

    QAction* refreshAction = fileMenu->addAction("Refresh Stylesheet");
    QObject::connect(refreshAction, &QAction::triggered, &styleManager, [&styleManager, &app]() {
        styleManager.Refresh(&app);
    });
    fileMenu->addSeparator();

#if defined(Q_OS_MACOS)
    QAction* quitAction = fileMenu->addAction("&Quit");
#else
    QAction* quitAction = fileMenu->addAction("E&xit");
#endif
    quitAction->setShortcut(QKeySequence::Quit);
    QObject::connect(quitAction, &QAction::triggered, widget, &QMainWindow::close);

    app.setQuitOnLastWindowClosed(true);

    app.exec();
}
