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
#include "native/utilities/GUIApplicationManager.h"
#include "native/utilities/assetUtils.h"
#include "native/AssetManager/assetProcessorManager.h"
#include "native/connection/connectionManager.h"
#include "native/utilities/IniConfiguration.h"
#include "native/utilities/ApplicationServer.h"
#include "native/resourcecompiler/rccontroller.h"
#include "native/FileServer/fileServer.h"
#include "native/AssetManager/assetScanner.h"
#include "native/shadercompiler/shadercompilerManager.h"
#include "native/shadercompiler/shadercompilerModel.h"
#include "native/AssetManager/AssetRequestHandler.h"
#include "native/utilities/ByteArrayStream.h"
#include <functional>
#include <QProcess>
#include <QThread>
#include <QApplication>
#include <QStringList>
#include <QStyleFactory>


#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/StyleSheetCache.h>
#include <AzQtComponents/Components/TitleBarOverdrawHandler.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzCore/base.h>
#include <AzCore/Debug/Trace.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>

#if defined(EXTERNAL_CRASH_REPORTING)
#include <ToolsCrashHandler.h>
#endif

#if defined(AZ_PLATFORM_APPLE_OSX)
#include <AppKit/NSRunningApplication.h>
#include <CoreServices/CoreServices.h>

#include "MacDockIconHandler.h"
#endif
#include <AzCore/std/chrono/clocks.h>

using namespace AssetProcessor;

namespace
{
    static const int s_errorMessageBoxDelay = 5000;

    void RemoveTemporaries()
    {
        // get currently running app
        QString appDir;
        QString file;
        AssetUtilities::ComputeApplicationInformation(appDir, file);
        QDir appDirectory(appDir);
        QString applicationName = appDirectory.filePath(file);
        QFileInfo moduleFileInfo(file);
        QDir binaryDir = moduleFileInfo.absoluteDir();
        // strip extension
        QString applicationBase = file.left(file.indexOf(moduleFileInfo.suffix()) - 1);
        // add wildcard filter
        applicationBase.append("*_tmp");
        // set to qt
        binaryDir.setNameFilters(QStringList() << applicationBase);
        binaryDir.setFilter(QDir::Files);
        // iterate all matching
        foreach(QString tempFile, binaryDir.entryList())
        {
            binaryDir.remove(tempFile);
        }
    }
}


GUIApplicationManager::GUIApplicationManager(int* argc, char*** argv, QObject* parent)
    : BatchApplicationManager(argc, argv, parent)
{
#if defined(AZ_PLATFORM_APPLE_OSX)
    // Since AP is not a proper Mac application yet it will not receive keyboard focus
    // unless we tell the OS specifically to treat it as a foreground application
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);
#endif
}


GUIApplicationManager::~GUIApplicationManager()
{
    Destroy();
}



ApplicationManager::BeforeRunStatus GUIApplicationManager::BeforeRun()
{
    ApplicationManager::BeforeRunStatus status = BatchApplicationManager::BeforeRun();
    if (status != ApplicationManager::BeforeRunStatus::Status_Success)
    {
        return status;
    }

    // The build process may leave behind some temporaries, try to delete them
    RemoveTemporaries();

    QDir devRoot;
    AssetUtilities::ComputeAssetRoot(devRoot);
#if defined(EXTERNAL_CRASH_REPORTING)
    CrashHandler::ToolsCrashHandler::InitCrashHandler("AssetProcessor", devRoot.absolutePath().toStdString());
#endif
    AssetProcessor::MessageInfoBus::Handler::BusConnect();

    QString bootstrapPath = devRoot.filePath("bootstrap.cfg");
    m_fileWatcher.addPath(bootstrapPath);

    // we have to monitor both the cache folder and the database file and restart AP if either of them gets deleted
    // It is important to note that we are monitoring the parent folder and not the actual cache folder itself since
    // we want to handle the use case on Mac OS if the user moves the cache folder to the trash.
    m_fileWatcher.addPath(devRoot.absolutePath());

    QDir projectCacheRoot;
    AssetUtilities::ComputeProjectCacheRoot(projectCacheRoot);
    QString assetDbPath = projectCacheRoot.filePath("assetdb.sqlite");

    m_fileWatcher.addPath(assetDbPath);

    // if our Gems file changes, make sure we watch that, too.
    QString gameName = AssetUtilities::ComputeGameName();
    QString gemsConfigFile = devRoot.filePath(gameName + "/gems.json");

    m_fileWatcher.addPath(gemsConfigFile);

    QObject::connect(&m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &GUIApplicationManager::FileChanged);
    QObject::connect(&m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &GUIApplicationManager::DirectoryChanged);

    return ApplicationManager::BeforeRunStatus::Status_Success;
}

void GUIApplicationManager::Destroy()
{
    if (m_mainWindow)
    {
        delete m_mainWindow;
        m_mainWindow = nullptr;
    }

    AssetProcessor::MessageInfoBus::Handler::BusDisconnect();
    BatchApplicationManager::Destroy();

    DestroyIniConfiguration();
    DestroyFileServer();
    DestroyShaderCompilerManager();
    DestroyShaderCompilerModel();
}


bool GUIApplicationManager::Run()
{
    qRegisterMetaType<AZ::u32>("AZ::u32");
    qRegisterMetaType<AZ::Uuid>("AZ::Uuid");

    AzQtComponents::StyleManager* styleManager = new AzQtComponents::StyleManager(qApp);
    const bool useLegacyStyle = false;
    styleManager->Initialize(qApp, useLegacyStyle);

    QDir engineRoot;
    AssetUtilities::ComputeEngineRoot(engineRoot);
    AzQtComponents::StyleManager::addSearchPaths(
        QStringLiteral("style"),
        engineRoot.filePath(QStringLiteral("Code/Tools/AssetProcessor/native/ui/style")),
        QStringLiteral(":/AssetProcessor/style"));

    m_mainWindow = new MainWindow(this);
    auto wrapper = new AzQtComponents::WindowDecorationWrapper(
#if defined(AZ_PLATFORM_WINDOWS)
        // On windows we do want our custom title bar
        AzQtComponents::WindowDecorationWrapper::OptionAutoAttach
#else
        // On others platforms we don't want our custom title bar (ie, use native title bars).
        AzQtComponents::WindowDecorationWrapper::OptionDisabled
#endif
    );
    wrapper->setGuest(m_mainWindow);

    // Use this variant of the enableSaveRestoreGeometry because the global QCoreApplication::setOrganization and setApplicationName
    // are called in ApplicationManager::Activate, which happens much later on in this function.
    // ApplicationManager::Activate is shared between the Batch version of the AP and the GUI version,
    // and there are thing
    const bool restoreOnFirstShow = true;
    wrapper->enableSaveRestoreGeometry(GetOrganizationName(), GetApplicationName(), "MainWindow", restoreOnFirstShow);

    AzQtComponents::StyleManager::setStyleSheet(m_mainWindow, QStringLiteral("style:AssetProcessor.qss"));
    
    auto refreshStyleSheets = [styleManager]()
    {
        styleManager->Refresh(qApp);
    };

    // CheckForRegistryProblems can pop up a dialog, so we need to check after
    // we initialize the stylesheet
    bool showErrorMessageOnRegistryProblem = true;
    RegistryCheckInstructions registryCheckInstructions = CheckForRegistryProblems(m_mainWindow, showErrorMessageOnRegistryProblem);
    if (registryCheckInstructions != RegistryCheckInstructions::Continue)
    {
        if (registryCheckInstructions == RegistryCheckInstructions::Restart)
        {
            Restart();
        }

        return false;
    }

    bool startHidden = QApplication::arguments().contains("--start-hidden", Qt::CaseInsensitive);

    if (!startHidden)
    {
        wrapper->show();
    }
    else
    {
#ifdef AZ_PLATFORM_WINDOWS
        // Qt / Windows has issues if the main window isn't shown once
        // so we show it then hide it
        wrapper->show();

        // Have a delay on the hide, to make sure that the show is entirely processed
        // first
        QTimer::singleShot(0, wrapper, &QWidget::hide);
#endif
    }

#ifdef AZ_PLATFORM_APPLE_OSX
    connect(new MacDockIconHandler(this), &MacDockIconHandler::dockIconClicked, m_mainWindow, &MainWindow::ShowWindow);
#endif

    QAction* quitAction = new QAction(QObject::tr("Quit"), m_mainWindow);
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    m_mainWindow->addAction(quitAction);
    m_mainWindow->connect(quitAction, SIGNAL(triggered()), this, SLOT(QuitRequested()));

    QAction* refreshAction = new QAction(QObject::tr("Refresh Stylesheet"), m_mainWindow);
    refreshAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    m_mainWindow->addAction(refreshAction);
    m_mainWindow->connect(refreshAction, &QAction::triggered, this, refreshStyleSheets);

    QObject::connect(this, &GUIApplicationManager::ShowWindow, m_mainWindow, &MainWindow::ShowWindow);


    if (QSystemTrayIcon::isSystemTrayAvailable())
    {
        QAction* showAction = new QAction(QObject::tr("Show"), m_mainWindow);
        QObject::connect(showAction, &QAction::triggered, m_mainWindow, &MainWindow::ShowWindow);
        QAction* hideAction = new QAction(QObject::tr("Hide"), m_mainWindow);
        QObject::connect(hideAction, &QAction::triggered, wrapper, &AzQtComponents::WindowDecorationWrapper::hide);

        QMenu* trayIconMenu = new QMenu();
        trayIconMenu->addAction(showAction);
        trayIconMenu->addAction(hideAction);
        trayIconMenu->addSeparator();

#if defined(AZ_PLATFORM_APPLE)
        QAction* systemTrayQuitAction = new QAction(QObject::tr("Quit"), m_mainWindow);
        systemTrayQuitAction->setMenuRole(QAction::NoRole);
        m_mainWindow->connect(systemTrayQuitAction, SIGNAL(triggered()), this, SLOT(QuitRequested()));
        trayIconMenu->addAction(systemTrayQuitAction);
#else
        trayIconMenu->addAction(quitAction);
#endif

        m_trayIcon = new QSystemTrayIcon(m_mainWindow);
        m_trayIcon->setContextMenu(trayIconMenu);
        m_trayIcon->setToolTip(QObject::tr("Asset Processor"));
        m_trayIcon->setIcon(QIcon(":/AssetProcessor.png"));
        m_trayIcon->show();
        QObject::connect(m_trayIcon, &QSystemTrayIcon::activated, m_mainWindow, [&, wrapper](QSystemTrayIcon::ActivationReason reason)
            {
                if (reason == QSystemTrayIcon::DoubleClick)
                {
                    if (wrapper->isVisible())
                    {
                        wrapper->hide();
                    }
                    else
                    {
                        m_mainWindow->ShowWindow();
                    }
                }
            });

        QObject::connect(m_trayIcon, &QSystemTrayIcon::messageClicked, m_mainWindow, &MainWindow::ShowWindow);

        if (startHidden)
        {
            m_trayIcon->showMessage(
                QCoreApplication::translate("Tray Icon", "Lumberyard Asset Processor has started"),
                QCoreApplication::translate("Tray Icon", "The Lumberyard Asset Processor monitors raw project assets and converts those assets into runtime-ready data."),
                QSystemTrayIcon::Information, 3000);
        }
    }

    connect(this, &GUIApplicationManager::AssetProcessorStatusChanged, m_mainWindow, &MainWindow::OnAssetProcessorStatusChanged);

    if (!Activate())
    {
        return false;
    }

    m_mainWindow->Activate();

    connect(GetAssetScanner(), &AssetProcessor::AssetScanner::AssetScanningStatusChanged, this, [this](AssetScanningStatus status)
    {
        if (status == AssetScanningStatus::Started)
        {
            AssetProcessor::AssetProcessorStatusEntry entry(AssetProcessor::AssetProcessorStatus::Scanning_Started);
            m_mainWindow->OnAssetProcessorStatusChanged(entry);
        }
    });
    connect(GetRCController(), &AssetProcessor::RCController::ActiveJobsCountChanged, this, &GUIApplicationManager::OnActiveJobsCountChanged);
    connect(this, &GUIApplicationManager::ConnectionStatusMsg, this, &GUIApplicationManager::ShowTrayIconMessage);

    qApp->setQuitOnLastWindowClosed(false);

    QTimer::singleShot(0, this, [this]()
    {
        if (!PostActivate())
        {
            QuitRequested();
            m_startedSuccessfully = false;
        }
    });

    m_duringStartup = false;

    int resultCode =  qApp->exec(); // this blocks until the last window is closed.
    
    if(!InitiatedShutdown())
    {
        // if we are here it implies that AP did not stop the Qt event loop and is shutting down prematurely
        // we need to call QuitRequested and start the event loop once again so that AP shuts down correctly
        QuitRequested();
        resultCode = qApp->exec();
    }

    if (m_trayIcon)
    {
        m_trayIcon->hide();
        delete m_trayIcon;
    }

    m_mainWindow->SaveLogPanelState();

    // mainWindow indirectly uses some UserSettings so clean it up before we clean those up
    delete m_mainWindow;
    m_mainWindow = nullptr;

    AZ::SerializeContext* context;
    EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(context, "No serialize context");
    QDir projectCacheRoot;
    AssetUtilities::ComputeProjectCacheRoot(projectCacheRoot);
    m_localUserSettings.Save(projectCacheRoot.filePath("AssetProcessorUserSettings.xml").toUtf8().data(), context);
    m_localUserSettings.Deactivate();

    if (NeedRestart())
    {
        bool launched = Restart();
        if (!launched)
        {
            return false;
        }
    }

    Destroy();

    return !resultCode && m_startedSuccessfully;
}

void GUIApplicationManager::NegotiationFailed()
{
    QString message = QCoreApplication::translate("error", "An attempt to connect to the game or editor has failed. The game or editor appears to be running from a different folder. Please restart the asset processor from the correct branch.");
    QMetaObject::invokeMethod(this, "ShowMessageBox", Qt::QueuedConnection, Q_ARG(QString, QString("Negotiation Failed")), Q_ARG(QString, message), Q_ARG(bool, false));
}

void GUIApplicationManager::OnAssetFailed(const AZStd::string& sourceFileName)
{
    QString message = tr("Error : %1 failed to compile\nPlease check the Asset Processor for more information.").arg(QString::fromUtf8(sourceFileName.c_str()));
    QMetaObject::invokeMethod(this, "ShowTrayIconErrorMessage", Qt::QueuedConnection, Q_ARG(QString, message));
}

void GUIApplicationManager::ShowMessageBox(QString title,  QString msg, bool isCritical)
{
    if (!m_messageBoxIsVisible)
    {
        // Only show the message box if it is not visible
        m_messageBoxIsVisible = true;
        QMessageBox msgBox(m_mainWindow);
        msgBox.setWindowTitle(title);
        msgBox.setText(msg);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        if (isCritical)
        {
            msgBox.setIcon(QMessageBox::Critical);
        }
        msgBox.exec();
        m_messageBoxIsVisible = false;
    }
}

bool GUIApplicationManager::OnError(const char* window, const char* message)
{
    // if we're in a worker thread, errors must not pop up a dialog box
    if (AssetProcessor::GetThreadLocalJobId() != 0)
    {
        // just absorb the error, do not perform default op
        return true;
    }

    // If we're the main thread, then consider showing the message box directly.  
    // note that all other threads will PAUSE if they emit a message while the main thread is showing this box
    // due to the way the trace system EBUS is mutex-protected.
    Qt::ConnectionType connection = Qt::DirectConnection;

    if (QThread::currentThread() != qApp->thread())
    {
        connection = Qt::QueuedConnection;
    }

    if (m_isCurrentlyLoadingGems)
    {
        // if something goes wrong during gem initialization, this is a special case and we need to be extra helpful.
        QString friendlyErrorMessage = QString("An error occurred while loading gems.\n"
            "This can happen when new gems are added to a project, but those gems need to be built in order to function.\n"
            "This can also happen when switching to a different project, one which uses gems which are not yet built.\n"
            "To continue, please build the current project before attempting to run Asset Processor again.\n\n"
            "Full error text:\n"
            "%1").arg(message);
        QMetaObject::invokeMethod(this, "ShowMessageBox", connection, Q_ARG(QString, QString("Error")), Q_ARG(QString, friendlyErrorMessage), Q_ARG(bool, true));
    }
    else
    {
        QMetaObject::invokeMethod(this, "ShowMessageBox", connection, Q_ARG(QString, QString("Error")), Q_ARG(QString, QString(message)), Q_ARG(bool, true));
    }
    

    return true;
}

bool GUIApplicationManager::OnAssert(const char* message)
{
    if (!OnError(nullptr, message))
    {
        return false;
    }

    // Asserts should be severe enough for data corruption,
    // so the process should quit to avoid that happening for users.
    if (!AZ::Debug::Trace::IsDebuggerPresent())
    {
        QuitRequested();
        return true;
    }

    AZ::Debug::Trace::Break();
    return true;
}

bool GUIApplicationManager::Activate()
{
    AZ::SerializeContext* context;
    EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(context, "No serialize context");
    QDir projectCacheRoot;
    AssetUtilities::ComputeProjectCacheRoot(projectCacheRoot);
    m_localUserSettings.Load(projectCacheRoot.filePath("AssetProcessorUserSettings.xml").toUtf8().data(), context);
    m_localUserSettings.Activate(AZ::UserSettings::CT_LOCAL);
    
    InitIniConfiguration();
    InitFileServer();

    //activate the base stuff.
    if (!BatchApplicationManager::Activate())
    {
        return false;
    }
    
    InitShaderCompilerModel();
    InitShaderCompilerManager();

    return true;
}

bool GUIApplicationManager::PostActivate()
{
    if (!BatchApplicationManager::PostActivate())
    {
        return false;
    }
    return true;
}

void GUIApplicationManager::CreateQtApplication()
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Qt actually modifies the argc and argv, you must pass the real ones in as ref so it can.
    m_qApp = new QApplication(*m_frameworkApp.GetArgC(), *m_frameworkApp.GetArgV());
}

void GUIApplicationManager::DirectoryChanged(QString path)
{
    AZ_UNUSED(path);
    QDir devRoot = ApplicationManager::GetSystemRoot();
    QString cacheRoot = devRoot.filePath("Cache");
    if (!QDir(cacheRoot).exists())
    {
        //Cache directory is removed we need to restart
        QTimer::singleShot(200, this, [this]()
        {
            QMetaObject::invokeMethod(this, "Restart", Qt::QueuedConnection);
        });
    }
    else
    {
        QDir projectCacheRoot;
        AssetUtilities::ComputeProjectCacheRoot(projectCacheRoot);
        QString assetDbPath = projectCacheRoot.filePath("assetdb.sqlite");

        if (!QFile::exists(assetDbPath))
        {
            // even if cache directory exists but the the database file is missing we need to restart
            QTimer::singleShot(200, this, [this]()
            {
                QMetaObject::invokeMethod(this, "Restart", Qt::QueuedConnection);
            });
        }
    }
}

void GUIApplicationManager::FileChanged(QString path)
{
    QDir devRoot = ApplicationManager::GetSystemRoot();
    QString bootstrapPath = devRoot.filePath("bootstrap.cfg");
    QDir projectCacheRoot;
    AssetUtilities::ComputeProjectCacheRoot(projectCacheRoot);
    QString assetDbPath = projectCacheRoot.filePath("assetdb.sqlite");
    if (QString::compare(AssetUtilities::NormalizeFilePath(path), bootstrapPath, Qt::CaseInsensitive) == 0)
    {
        AssetUtilities::UpdateBranchToken();

        if (m_connectionManager)
        {
            m_connectionManager->UpdateWhiteListFromBootStrap();
        }

        // we only have to quit if the actual project name has changed, not if just the bootstrap has changed.
        QString previousGameName = AssetUtilities::ComputeGameName(); // get the cached version
        QString newGameName = AssetUtilities::ComputeGameName(QString(), true); // force=true!
        
        if (newGameName != previousGameName)
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Bootstrap.cfg Game Name changed from %s to %s.  Quitting\n", previousGameName.toUtf8().constData(), newGameName.toUtf8().constData());
            QMetaObject::invokeMethod(this, "QuitRequested", Qt::QueuedConnection);
        }
    }
    else if (QString::compare(AssetUtilities::NormalizeFilePath(path), assetDbPath, Qt::CaseInsensitive) == 0)
    {
        if (!QFile::exists(assetDbPath))
        {
            // if the database file is deleted we need to restart
            QTimer::singleShot(200, this, [this]()
            {
                QMetaObject::invokeMethod(this, "Restart", Qt::QueuedConnection);
            });
        }
    }
    else if (AssetUtilities::NormalizeFilePath(path).endsWith("gems.json", Qt::CaseInsensitive))
    {
        QList<AssetProcessor::GemInformation> oldGemsList = GetPlatformConfiguration()->GetGemsInformation();
        QList<AssetProcessor::GemInformation> newGemsList;
        GetPlatformConfiguration()->ReadGems(newGemsList);

        for (auto oldGemIter = oldGemsList.begin(); oldGemIter != oldGemsList.end();)
        {
            bool gemMatch = false;
            for (auto newGemIter = newGemsList.begin(); newGemIter != newGemsList.end();)
            {
                if (QString::compare(oldGemIter->m_identifier, newGemIter->m_identifier, Qt::CaseInsensitive) == 0)
                {
                    gemMatch = true;
                    newGemIter = newGemsList.erase(newGemIter);
                    break;
                }
                newGemIter++;
            }

            if (gemMatch)
            {
                oldGemIter = oldGemsList.erase(oldGemIter);
            }
            else
            {
                oldGemIter++;
            }
        }

        // oldGemslist should contain the list of gems that got removed and newGemsList should contain the list of gems that were added to the project
        // if the project requires to be built again then we will quit otherwise we can restart
        bool exitApp = false;
        for (const AssetProcessor::GemInformation& gemInfo : newGemsList)
        {
            if (!gemInfo.m_assetOnly)
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Gem %s was added to the project and require building. Quitting\n", gemInfo.m_displayName.toUtf8().data());
                exitApp = true;
            }
        }

        if (exitApp)
        {
            QuitRequested();
        }
        else
        {
            if (oldGemsList.size() || newGemsList.size())
            {
                if (oldGemsList.size())
                {
                    AZ_TracePrintf(AssetProcessor::DebugChannel, "Gem(s) were removed from the project. Restarting\n");
                }
                else if (newGemsList.size())
                {
                    AZ_TracePrintf(AssetProcessor::DebugChannel, "Assets only gem(s) were added to the project. Restarting\n");
                }
                QMetaObject::invokeMethod(this, "Restart", Qt::QueuedConnection);
            }
        }
    }
}

void GUIApplicationManager::InitConnectionManager()
{
    BatchApplicationManager::InitConnectionManager();

    using namespace std::placeholders;
    using namespace AzFramework::AssetSystem;
    using namespace AzToolsFramework::AssetSystem;

    //File Server related
    m_connectionManager->RegisterService(FileOpenRequest::MessageType(), std::bind(&FileServer::ProcessOpenRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(FileCloseRequest::MessageType(), std::bind(&FileServer::ProcessCloseRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(FileReadRequest::MessageType(), std::bind(&FileServer::ProcessReadRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(FileWriteRequest::MessageType(), std::bind(&FileServer::ProcessWriteRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(FileSeekRequest::MessageType(), std::bind(&FileServer::ProcessSeekRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(FileTellRequest::MessageType(), std::bind(&FileServer::ProcessTellRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(FileIsReadOnlyRequest::MessageType(), std::bind(&FileServer::ProcessIsReadOnlyRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(PathIsDirectoryRequest::MessageType(), std::bind(&FileServer::ProcessIsDirectoryRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(FileSizeRequest::MessageType(), std::bind(&FileServer::ProcessSizeRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(FileModTimeRequest::MessageType(), std::bind(&FileServer::ProcessModificationTimeRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(FileExistsRequest::MessageType(), std::bind(&FileServer::ProcessExistsRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(FileFlushRequest::MessageType(), std::bind(&FileServer::ProcessFlushRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(PathCreateRequest::MessageType(), std::bind(&FileServer::ProcessCreatePathRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(PathDestroyRequest::MessageType(), std::bind(&FileServer::ProcessDestroyPathRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(FileRemoveRequest::MessageType(), std::bind(&FileServer::ProcessRemoveRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(FileCopyRequest::MessageType(), std::bind(&FileServer::ProcessCopyRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(FileRenameRequest::MessageType(), std::bind(&FileServer::ProcessRenameRequest, m_fileServer, _1, _2, _3, _4));
    m_connectionManager->RegisterService(FindFilesRequest::MessageType(), std::bind(&FileServer::ProcessFindFileNamesRequest, m_fileServer, _1, _2, _3, _4));

    QObject::connect(m_connectionManager, SIGNAL(connectionAdded(uint, Connection*)), m_fileServer, SLOT(ConnectionAdded(unsigned int, Connection*)));
    QObject::connect(m_connectionManager, SIGNAL(ConnectionDisconnected(unsigned int)), m_fileServer, SLOT(ConnectionRemoved(unsigned int)));

    QObject::connect(m_fileServer, SIGNAL(AddBytesReceived(unsigned int, qint64, bool)), m_connectionManager, SLOT(AddBytesReceived(unsigned int, qint64, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddBytesSent(unsigned int, qint64, bool)), m_connectionManager, SLOT(AddBytesSent(unsigned int, qint64, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddBytesRead(unsigned int, qint64, bool)), m_connectionManager, SLOT(AddBytesRead(unsigned int, qint64, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddBytesWritten(unsigned int, qint64, bool)), m_connectionManager, SLOT(AddBytesWritten(unsigned int, qint64, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddOpenRequest(unsigned int, bool)), m_connectionManager, SLOT(AddOpenRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddCloseRequest(unsigned int, bool)), m_connectionManager, SLOT(AddCloseRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddOpened(unsigned int, bool)), m_connectionManager, SLOT(AddOpened(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddClosed(unsigned int, bool)), m_connectionManager, SLOT(AddClosed(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddReadRequest(unsigned int, bool)), m_connectionManager, SLOT(AddReadRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddWriteRequest(unsigned int, bool)), m_connectionManager, SLOT(AddWriteRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddTellRequest(unsigned int, bool)), m_connectionManager, SLOT(AddTellRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddSeekRequest(unsigned int, bool)), m_connectionManager, SLOT(AddSeekRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddIsReadOnlyRequest(unsigned int, bool)), m_connectionManager, SLOT(AddIsReadOnlyRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddIsDirectoryRequest(unsigned int, bool)), m_connectionManager, SLOT(AddIsDirectoryRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddSizeRequest(unsigned int, bool)), m_connectionManager, SLOT(AddSizeRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddModificationTimeRequest(unsigned int, bool)), m_connectionManager, SLOT(AddModificationTimeRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddExistsRequest(unsigned int, bool)), m_connectionManager, SLOT(AddExistsRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddFlushRequest(unsigned int, bool)), m_connectionManager, SLOT(AddFlushRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddCreatePathRequest(unsigned int, bool)), m_connectionManager, SLOT(AddCreatePathRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddDestroyPathRequest(unsigned int, bool)), m_connectionManager, SLOT(AddDestroyPathRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddRemoveRequest(unsigned int, bool)), m_connectionManager, SLOT(AddRemoveRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddCopyRequest(unsigned int, bool)), m_connectionManager, SLOT(AddCopyRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddRenameRequest(unsigned int, bool)), m_connectionManager, SLOT(AddRenameRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(AddFindFileNamesRequest(unsigned int, bool)), m_connectionManager, SLOT(AddFindFileNamesRequest(unsigned int, bool)));
    QObject::connect(m_fileServer, SIGNAL(UpdateConnectionMetrics()), m_connectionManager, SLOT(UpdateConnectionMetrics()));
    
    m_connectionManager->RegisterService(ShowAssetProcessorRequest::MessageType(),
        std::bind([this](unsigned int /*connId*/, unsigned int /*type*/, unsigned int /*serial*/, QByteArray /*payload*/)
    {
        Q_EMIT ShowWindow();
    }, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)
    );

    m_connectionManager->RegisterService(ShowAssetInAssetProcessorRequest::MessageType(),
        std::bind([this](unsigned int /*connId*/, unsigned int /*type*/, unsigned int /*serial*/, QByteArray payload)
    {
        ShowAssetInAssetProcessorRequest request;
        bool readFromStream = AZ::Utils::LoadObjectFromBufferInPlace(payload.data(), payload.size(), request);
        AZ_Assert(readFromStream, "GUIApplicationManager::ShowAssetInAssetProcessorRequest: Could not deserialize from stream");
        if (readFromStream)
        {
            m_mainWindow->HighlightAsset(request.m_assetPath.c_str());

            Q_EMIT ShowWindow();
        }
    }, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)
    );
}

void GUIApplicationManager::InitIniConfiguration()
{
    m_iniConfiguration = new IniConfiguration();
    m_iniConfiguration->readINIConfigFile();
    m_iniConfiguration->parseCommandLine();
}

void GUIApplicationManager::DestroyIniConfiguration()
{
    if (m_iniConfiguration)
    {
        delete m_iniConfiguration;
        m_iniConfiguration = nullptr;
    }
}

void GUIApplicationManager::InitFileServer()
{
    m_fileServer = new FileServer();
    m_fileServer->SetSystemRoot(GetSystemRoot());
}

void GUIApplicationManager::DestroyFileServer()
{
    if (m_fileServer)
    {
        delete m_fileServer;
        m_fileServer = nullptr;
    }
}

void GUIApplicationManager::InitShaderCompilerManager()
{
    m_shaderCompilerManager = new ShaderCompilerManager();
    
    //Shader compiler stuff
    m_connectionManager->RegisterService(AssetUtilities::ComputeCRC32Lowercase("ShaderCompilerProxyRequest"), std::bind(&ShaderCompilerManager::process, m_shaderCompilerManager, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    QObject::connect(m_shaderCompilerManager, SIGNAL(sendErrorMessageFromShaderJob(QString, QString, QString, QString)), m_shaderCompilerModel, SLOT(addShaderErrorInfoEntry(QString, QString, QString, QString)));

    
}

void GUIApplicationManager::DestroyShaderCompilerManager()
{
    if (m_shaderCompilerManager)
    {
        delete m_shaderCompilerManager;
        m_shaderCompilerManager = nullptr;
    }
}

void GUIApplicationManager::InitShaderCompilerModel()
{
    m_shaderCompilerModel = new ShaderCompilerModel();
}

void GUIApplicationManager::DestroyShaderCompilerModel()
{
    if (m_shaderCompilerModel)
    {
        delete m_shaderCompilerModel;
        m_shaderCompilerModel = nullptr;
    }
}

IniConfiguration* GUIApplicationManager::GetIniConfiguration() const
{
    return m_iniConfiguration;
}

FileServer* GUIApplicationManager::GetFileServer() const
{
    return m_fileServer;
}
ShaderCompilerManager* GUIApplicationManager::GetShaderCompilerManager() const
{
    return m_shaderCompilerManager;
}
ShaderCompilerModel* GUIApplicationManager::GetShaderCompilerModel() const
{
    return m_shaderCompilerModel;
}

void GUIApplicationManager::ShowTrayIconErrorMessage(QString msg)
{
    AZStd::chrono::system_clock::time_point currentTime = AZStd::chrono::system_clock::now();

    if (m_trayIcon && m_mainWindow)
    {
        if((currentTime - m_timeWhenLastWarningWasShown) >= AZStd::chrono::milliseconds(s_errorMessageBoxDelay))
        {
            m_timeWhenLastWarningWasShown = currentTime;
            m_trayIcon->showMessage(
                QCoreApplication::translate("Tray Icon", "Lumberyard Asset Processor"),
                QCoreApplication::translate("Tray Icon", msg.toUtf8().data()),
                QSystemTrayIcon::Critical, 3000);
        }
    }
}

void GUIApplicationManager::ShowTrayIconMessage(QString msg)
{
    if (m_trayIcon && m_mainWindow && !m_mainWindow->isVisible())
    {
        m_trayIcon->showMessage(
            QCoreApplication::translate("Tray Icon", "Lumberyard Asset Processor"),
            QCoreApplication::translate("Tray Icon", msg.toUtf8().data()),
            QSystemTrayIcon::Information, 3000);
    }
}

bool GUIApplicationManager::Restart()
{
    bool launched = QProcess::startDetached(QCoreApplication::applicationFilePath(), QCoreApplication::arguments());
    if (!launched)
    {
        QMessageBox::critical(nullptr,
            QCoreApplication::translate("application", "Unable to launch Asset Processor"),
            QCoreApplication::translate("application", "Unable to launch Asset Processor"));
    }

    return launched;
}

#include <native/utilities/GUIApplicationManager.moc>
