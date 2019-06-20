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

#include <AzCore/Memory/SystemAllocator.h>

#include <QApplication>
#include <QMainWindow>
#include <QSettings>

#ifdef Q_OS_WIN
#include <Windows.h>
#else
#include <iostream>
#endif

const QString g_ui_1_0_SettingKey = QStringLiteral("useUI_1_0");

static void LogToDebug(QtMsgType Type, const QMessageLogContext& Context, const QString& message)
{
#ifdef Q_OS_WIN
    OutputDebugStringW(L"Qt: ");
    OutputDebugStringW(reinterpret_cast<const wchar_t*>(message.utf16()));
    OutputDebugStringW(L"\n");
#else
    std::wcerr << L"Qt: " << message.toStdWString() << std::endl;
#endif
}

int main(int argv, char **argc)
{
    // Need to initialize AZ::SystemAllocator otherwise AzToolsFramework::Logging::LogLine can't be instantiated
    if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
    }

    QApplication::setOrganizationName("Amazon");
    QApplication::setOrganizationDomain("amazon.com");
    QApplication::setApplicationName("LumberyardWidgetGallery");

    AzQtComponents::PrepareQtPaths();

    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    qInstallMessageHandler(LogToDebug);

    QApplication app(argv, argc);
    AzQtComponents::StyleManager styleManager(&app);
    styleManager.Initialize(&app);

    QSettings settings;
    bool legacyUISetting = settings.value(g_ui_1_0_SettingKey, false).toBool();
    styleManager.SwitchUI(&app, legacyUISetting);

    auto wrapper = new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionNone);
    auto widget = new ComponentDemoWidget(legacyUISetting, wrapper);
    wrapper->setGuest(widget);
    widget->resize(550, 900);
    widget->show();

    wrapper->enableSaveRestoreGeometry("windowGeometry");
    wrapper->restoreGeometryFromSettings();

    QObject::connect(widget, &ComponentDemoWidget::styleChanged, &styleManager, [&styleManager, &app, &settings](bool checked) {
        styleManager.SwitchUI(&app, checked);

        settings.setValue(g_ui_1_0_SettingKey, checked);
        settings.sync();
    });

    QObject::connect(widget, &ComponentDemoWidget::refreshStyle, &styleManager, [&styleManager, &app]() {
        styleManager.Refresh(&app);
    });

    app.setQuitOnLastWindowClosed(true);

    app.exec();

    if (AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }
}
