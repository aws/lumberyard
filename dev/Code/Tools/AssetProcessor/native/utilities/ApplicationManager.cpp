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

#include "native/utilities/ApplicationManager.h"

#include <AzCore/base.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Module/ModuleManagerBus.h>

#include <AzFramework/Logging/LoggingComponent.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>

#if !defined(BATCH_MODE)
#include <AzToolsFramework/UI/Logging/LogPanel_Panel.h>
#endif

#include "native/utilities/assetUtils.h"
#include "native/utilities/ApplicationManagerAPI.h"
#include "native/assetprocessor.h"
#include "native/resourcecompiler/RCBuilder.h"
#include "native/utilities/AssetBuilderInfo.h"
#include "native/utilities/assetUtils.h"

#include <QCoreApplication>
#include <QLocale>
#include <QFileInfo>
#include <QTranslator>
#include <QByteArray>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include <QSettings>

#if !defined(BATCH_MODE)
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#endif

#include <string.h> // for base  strcpy
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzToolsFramework/Entity/EditorEntityFixupComponent.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <LyShine/UiAssetTypes.h>

namespace AssetProcessor
{
    void MessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
    {
        switch (type)
        {
        case QtDebugMsg:
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Qt-Debug: %s (%s:%u, %s)\n", msg.toUtf8().constData(), context.file, context.line, context.function);
            break;
        case QtWarningMsg:
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Qt-Warning: %s (%s:%u, %s)\n", msg.toUtf8().constData(), context.file, context.line, context.function);
            break;
        case QtCriticalMsg:
            AZ_Warning(AssetProcessor::ConsoleChannel, false, "Qt-Critical: %s (%s:%u, %s)\n", msg.toUtf8().constData(), context.file, context.line, context.function);
            break;
        case QtFatalMsg:
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Qt-Fatal: %s (%s:%u, %s)\n", msg.toUtf8().constData(), context.file, context.line, context.function);
            abort();
        }
    }

    //! we filter the main app logs to only include non-job-thread messages:
    class FilteredLogComponent
        : public AzFramework::LogComponent
    {
    public:
        void OutputMessage(AzFramework::LogFile::SeverityLevel severity, const char* window, const char* message) override
        {
            // if we receive an exception it means we are likely to crash.  in that case, even if it occurred in a job thread
            // it occurred in THIS PROCESS, which will now die.  So we log these even in the case of them happening in a job thread.
            if ((m_inException)||(severity == AzFramework::LogFile::SEV_EXCEPTION))
            {
                if (!m_inException)
                {
                    m_inException = true; // from this point on, consume all messages regardless of what severity they are.
                    AZ::Debug::Trace::HandleExceptions(false);
                }
                AzFramework::LogComponent::OutputMessage(AzFramework::LogFile::SEV_EXCEPTION, ConsoleChannel, message);
                // note that OutputMessage will only output to the log, we want this kind of info to make its way into the
                // regular stderr too
                fprintf(stderr, "Exception log: %s - %s", window, message);
                fflush(stderr);
                return;
            }

            if (AssetProcessor::GetThreadLocalJobId() != 0)
            {
                // we are in a job thread - return early to make it so that the global log file does not get this message
                // there will also be a log listener in the actual job log thread which will get the message too, and that one
                // will write it to the individual log.
                return; 
            }

            AzFramework::LogComponent::OutputMessage(severity, window, message);
        }

    protected:
        bool m_inException = false;
    };
}

uint qHash(const AZ::Uuid& key, uint seed)
{
    (void) seed;
    return azlossy_caster(AZStd::hash<AZ::Uuid>()(key));
}


AZ::ComponentTypeList AssetProcessorAZApplication::GetRequiredSystemComponents() const
{
    AZ::ComponentTypeList components = AzFramework::Application::GetRequiredSystemComponents();

    for (auto iter = components.begin(); iter != components.end();)
    {
        if (*iter == azrtti_typeid<AzFramework::AssetSystem::AssetSystemComponent>() // AP does not need asset system component to handle AssetRequestBus calls
            || *iter == azrtti_typeid<AzFramework::AssetCatalogComponent>() // AP will use its own AssetCatalogComponent
            || *iter == AZ::Uuid("{624a7be2-3c7e-4119-aee2-1db2bdb6cc89}") // ScriptDebugAgent
            || *iter == AZ::Uuid("{CAF3A025-FAC9-4537-B99E-0A800A9326DF}") // InputSystemComponent
            || *iter == azrtti_typeid<AssetProcessor::ToolsAssetCatalogComponent>()
           ) 
        {
            // AP does not require the above components to be active
            iter = components.erase(iter);
        }
        else
        {
            ++iter;
        }
    }


    return components;
}

void AssetProcessorAZApplication::RegisterCoreComponents()
{
    AzToolsFramework::ToolsApplication::RegisterCoreComponents();

    RegisterComponentDescriptor(AzToolsFramework::EditorEntityFixupComponent::CreateDescriptor());
    RegisterComponentDescriptor(AzToolsFramework::AssetBrowser::AssetBrowserComponent::CreateDescriptor());
}

void AssetProcessorAZApplication::ResolveModulePath(AZ::OSString& modulePath)
{
   AssetProcessor::AssetProcessorStatusEntry entry(AssetProcessor::AssetProcessorStatus::Initializing_Gems, 0, QString(modulePath.c_str()));
   Q_EMIT AssetProcessorStatus(entry);
   AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Loading (Gem) Module '%s'...\n", modulePath.c_str());

   AzFramework::Application::ResolveModulePath(modulePath);
}


ApplicationManager::ApplicationManager(int* argc, char*** argv, QObject* parent)
    : QObject(parent)
    , m_frameworkApp(argc, argv)
{
    qInstallMessageHandler(&AssetProcessor::MessageHandler);
}

ApplicationManager::~ApplicationManager()
{
    // if any of the threads are running, destroy them
    // Any QObjects that have these ThreadWorker as parent will also be deleted
    if (m_runningThreads.size())
    {
        for (int idx = 0; idx < m_runningThreads.size(); idx++)
        {
            m_runningThreads.at(idx)->Destroy();
        }
    }
    for (int idx = 0; idx < m_appDependencies.size(); idx++)
    {
        delete m_appDependencies[idx];
    }

    qInstallMessageHandler(nullptr);

    //deleting QCoreApplication/QApplication
    delete m_qApp;

    if (m_entity)
    {
        //Deactivate all the components
        m_entity->Deactivate();
        delete m_entity;
        m_entity = nullptr;
    }

    //Unregistering and deleting all the components
    EBUS_EVENT_ID(azrtti_typeid<AzFramework::LogComponent>(), AZ::ComponentDescriptorBus, ReleaseDescriptor);

    //Stop AZFramework
    m_frameworkApp.Stop();
    AZ::Debug::Trace::HandleExceptions(false);
}

bool ApplicationManager::InitiatedShutdown() const
{
    return m_duringShutdown;
}

void ApplicationManager::GetExternalBuilderFileList(QStringList& externalBuilderModules)
{
    externalBuilderModules.clear();

    static const char* builder_folder_name = "Builders";

    QString builderFolderName(builder_folder_name);

    QStringList builderPaths;

    // First priority: locate the Builders based on the current asset processor exe folder
    QString builderPath1 = QDir::toNativeSeparators(QString(this->m_frameworkApp.GetExecutableFolder()) + builderFolderName);
    builderPaths.append(builderPath1);

    // Second priority, locate the Builds based on the engine root path + bin folder
    AZStd::string_view binFolderName;
    AZ::ComponentApplicationBus::BroadcastResult(binFolderName, &AZ::ComponentApplicationRequests::GetBinFolder);

    QString builderPath2 = QDir::toNativeSeparators(QString(this->m_frameworkApp.GetEngineRoot()) + QString("%1" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING).arg(binFolderName.data()) + builderFolderName);
#if defined (AZ_PLATFORM_WINDOWS)
    bool isDuplicate = (builderPath1.compare(builderPath2, Qt::CaseInsensitive)==0);
#else
    bool isDuplicate = (builderPath1.compare(builderPath2, Qt::CaseSensitive)==0);
#endif
    if (!isDuplicate)
    {
        // Only add the second priorty if its different
        builderPaths.append(builderPath2);
    }

    // Keep track of the builder names so we dont load duplicates (the first instance of a builder takes priority)
    QSet<QString> detectedBuilderNames;

    QStringList filter;
    filter.append("*" AZ_DYNAMIC_LIBRARY_EXTENSION);

    QDir builderDir;
    bool builderDirFound = false;
    for (auto builderPath : builderPaths)
    {
        builderDir.setPath(builderPath);
        if (builderDir.exists())
        {
            QStringList fileList = builderDir.entryList(filter, QDir::Files);
            for (const QString& file : fileList)
            {
                if (!detectedBuilderNames.contains(file))
                {
                    detectedBuilderNames.insert(file);
                    QString fileFullPath = builderDir.filePath(file);
                    externalBuilderModules.append(fileFullPath);
                }
                else
                {
                    AZ_Warning(AssetProcessor::ConsoleChannel, false, "Skipping duplicate builder '%s'\n", file.toUtf8().data());
                }
            }
        }
    }
    if (externalBuilderModules.size() == 0)
    {
        AZ_Warning(AssetProcessor::ConsoleChannel, false, "AssetProcessor was unable to locate any builders\n");
    }
}



QDir ApplicationManager::GetSystemRoot() const
{
    return m_systemRoot;
}
QString ApplicationManager::GetGameName() const
{
    return m_gameName;
}

QCoreApplication* ApplicationManager::GetQtApplication()
{
    return m_qApp;
}

void ApplicationManager::RegisterObjectForQuit(QObject* source, bool insertInFront)
{
    Q_ASSERT(!m_duringShutdown);

    if (m_duringShutdown)
    {
        AZ_Warning(AssetProcessor::DebugChannel, false, "You may not register objects for quit during shutdown.\n");
        return;
    }

    QuitPair quitPair(source, false);
    if (!m_objectsToNotify.contains(quitPair))
    {
        if (insertInFront)
        {
            m_objectsToNotify.push_front(quitPair);
        }
        else
        {
            m_objectsToNotify.push_back(quitPair);
        }
        if (!connect(source, SIGNAL(ReadyToQuit(QObject*)), this, SLOT(ReadyToQuit(QObject*))))
        {
            AZ_Warning(AssetProcessor::DebugChannel, false, "ApplicationManager::RegisterObjectForQuit was passed an object of type %s which has no ReadyToQuit(QObject*) signal.\n", source->metaObject()->className());
        }
        connect(source, SIGNAL(destroyed(QObject*)), this, SLOT(ObjectDestroyed(QObject*)));
    }
}

void ApplicationManager::ObjectDestroyed(QObject* source)
{
    for (int notifyIdx = 0; notifyIdx < m_objectsToNotify.size(); ++notifyIdx)
    {
        if (m_objectsToNotify[notifyIdx].first == source)
        {
            m_objectsToNotify.erase(m_objectsToNotify.begin() + notifyIdx);
            if (m_duringShutdown)
            {
                if (!m_queuedCheckQuit)
                {
                    QTimer::singleShot(0, this, SLOT(CheckQuit()));
                    m_queuedCheckQuit = true;
                }
            }
            return;
        }
    }
}

void ApplicationManager::QuitRequested()
{
    if (m_duringShutdown)
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "QuitRequested() - already during shutdown\n");
        return;
    }

    if (m_duringStartup)
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "QuitRequested() - during startup - waiting\n");
        // if we're still starting up, spin until we're ready to shut down.
        QMetaObject::invokeMethod(this, "QuitRequested", Qt::QueuedConnection);
        return;
    }
    AZ_TracePrintf(AssetProcessor::DebugChannel, "QuitRequested() - ready!\n");
    m_duringShutdown = true;

    //Inform all the builders to shutdown
    EBUS_EVENT(AssetBuilderSDK::AssetBuilderCommandBus, ShutDown);

    // the following call will invoke on the main thread of the application, since its a direct bus call.
    EBUS_EVENT(AssetProcessor::ApplicationManagerNotifications::Bus, ApplicationShutdownRequested);

    // while it may be tempting to just collapse all of this to a bus call,  Qt Objects have the advantage of being
    // able to automatically queue calls onto their own thread, and a lot of these involved objects are in fact
    // on their own threads.  So even if we used a bus call we'd ultimately still have to invoke a queued
    // call there anyway.

    for (const QuitPair& quitter : m_objectsToNotify)
    {
        if (!quitter.second)
        {
            QMetaObject::invokeMethod(quitter.first, "QuitRequested", Qt::QueuedConnection);
        }
    }
#if !defined(BATCH_MODE)
    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "App quit requested %d listeners notified.\n", m_objectsToNotify.size());
#endif
    if (!m_queuedCheckQuit)
    {
        QTimer::singleShot(0, this, SLOT(CheckQuit()));
        m_queuedCheckQuit = true;
    }
}

void ApplicationManager::CheckQuit()
{
    m_queuedCheckQuit = false;
    for (const QuitPair& quitter : m_objectsToNotify)
    {
        if (!quitter.second)
        {
#if !defined(BATCH_MODE)
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "App Quit: Object of type %s is not yet ready to quit.\n", quitter.first->metaObject()->className());
#endif
            return;
        }
    }
#if !defined(BATCH_MODE)
    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "App quit requested, and all objects are ready.  Quitting app.\n");
#endif

    // We will now loop over all the running threads and destroy them
    // Any QObjects that have these ThreadWorker as parent will also be deleted
    for (int idx = 0; idx < m_runningThreads.size(); idx++)
    {
        m_runningThreads.at(idx)->Destroy();
    }

    m_runningThreads.clear();
    // all good.
    qApp->quit();
}

void ApplicationManager::CheckForUpdate()
{
    for (int idx = 0; idx < m_appDependencies.size(); ++idx)
    {
        ApplicationDependencyInfo* fileDependencyInfo = m_appDependencies[idx];
        QString fileName = fileDependencyInfo->FileName();
        QFileInfo fileInfo(fileName);
        if (fileInfo.exists())
        {
            QDateTime fileLastModifiedTime = fileInfo.lastModified();
            bool hasTimestampChanged = (fileDependencyInfo->Timestamp() != fileLastModifiedTime);
            if (hasTimestampChanged)
            {
                QuitRequested();
            }
        }
        else
        {
            // if one of the files is not present we construct a null datetime for it and
            // continue checking
            fileDependencyInfo->SetTimestamp(QDateTime());
        }
    }
}

void ApplicationManager::PopulateApplicationDependencies()
{
    connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(CheckForUpdate()));
    m_updateTimer.start(5000);

    QString currentDir(QCoreApplication::applicationDirPath());
    QDir dir(currentDir);
    QString applicationPath = QCoreApplication::applicationFilePath();

    m_filesOfInterest.push_back(applicationPath);

    // add some known-dependent files (this can be removed when they are no longer a dependency)
    // Note that its not necessary for any of these files to actually exist.  It is considered a "change" if they 
    // change their file modtime, or if they go from existing to not existing, or if they go from not existing, to existing.
    // any of those should cause AP to drop.
    for (const QString& pathName : { "CrySystem", "CryAction", "SceneCore", 
                                     "SceneData", "FbxSceneBuilder", "AzQtComponents", 
                                     "LyIdentity_shared", "LyMetricsProducer_shared", "LyMetricsShared_shared" 
                                     })
    {
        QString pathWithPlatformExtension = pathName + QString(AZ_DYNAMIC_LIBRARY_EXTENSION);
        m_filesOfInterest.push_back(dir.absoluteFilePath(pathWithPlatformExtension));
    }

    // Get the external builder modules to add to the files of interest
    QStringList builderModuleFileList;
    GetExternalBuilderFileList(builderModuleFileList);
    for (const QString& builderModuleFile : builderModuleFileList)
    {
        m_filesOfInterest.push_back(builderModuleFile);
    }


    QDir assetRoot;
    AssetUtilities::ComputeAssetRoot(assetRoot);

    QString globalConfigPath = assetRoot.filePath("AssetProcessorPlatformConfig.ini");
    m_filesOfInterest.push_back(globalConfigPath);

    QString gameName = AssetUtilities::ComputeGameName();
    QString gamePlatformConfigPath = assetRoot.filePath(gameName + "/AssetProcessorGamePlatformConfig.ini");
    m_filesOfInterest.push_back(gamePlatformConfigPath);

    // add app modules
    AZ::ModuleManagerRequestBus::Broadcast(&AZ::ModuleManagerRequestBus::Events::EnumerateModules,
        [this](const AZ::ModuleData& moduleData)
        {
            AZ::DynamicModuleHandle* handle = moduleData.GetDynamicModuleHandle();
            if (handle)
            {
                QFileInfo fi(handle->GetFilename().c_str());
                if (fi.exists())
                {
                    m_filesOfInterest.push_back(fi.absoluteFilePath());
                }
            }
            return true; // keep iterating.
        });

    //find timestamps of all the files
    for (int idx = 0; idx < m_filesOfInterest.size(); idx++)
    {
        QString fileName = m_filesOfInterest.at(idx);
        QFileInfo fileInfo(fileName);
        QDateTime fileLastModifiedTime = fileInfo.lastModified();
        ApplicationDependencyInfo* applicationDependencyInfo = new ApplicationDependencyInfo(fileName, fileLastModifiedTime);
        // if some file does not exist than null datetime will be stored
        m_appDependencies.push_back(applicationDependencyInfo);
    }
}


bool ApplicationManager::StartAZFramework(QString appRootOverride)
{
    AzFramework::Application::Descriptor appDescriptor;
    AZ::ComponentApplication::StartupParameters params;
    QString dir;
    QString filename;
    QString binFolder;

    AssetUtilities::ComputeAppRootAndBinFolderFromApplication(dir, filename, binFolder);

    // The application will live in a bin folder one level up from the app root.
    char staticStorageForRootPath[AZ_MAX_PATH_LEN] = { 0 };
    if (appRootOverride.isEmpty())
    {
        azstrcpy(staticStorageForRootPath, AZ_MAX_PATH_LEN, dir.toUtf8().data());
    }
    else
    {
        azstrcpy(staticStorageForRootPath, AZ_MAX_PATH_LEN, appRootOverride.toUtf8().data());
    }

    // Prevent script reflection warnings from bringing down the AssetProcessor
    appDescriptor.m_enableScriptReflection = false;

    // start listening for exceptions occuring so if something goes wrong we have at least SOME output...
    AZ::Debug::Trace::HandleExceptions(true);

    params.m_appRootOverride = staticStorageForRootPath;
    m_frameworkApp.Start(appDescriptor, params);
    
    //Registering all the Components
    m_frameworkApp.RegisterComponentDescriptor(AzFramework::LogComponent::CreateDescriptor());
 
#if !defined(BATCH_MODE)
    AZ::SerializeContext* context;
    EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(context, "No serialize context");
    AzToolsFramework::LogPanel::BaseLogPanel::Reflect(context);
#endif
    
    // the log folder currently goes in the bin folder:
    AZStd::string fullBinFolder;
    AzFramework::StringFunc::Path::Join(dir.toUtf8().constData(), binFolder.toUtf8().constData(), fullBinFolder);
    AZ::IO::FileIOBase::GetInstance()->SetAlias("@log@", fullBinFolder.c_str());
    m_entity = aznew AZ::Entity("Application Entity");
    if (m_entity)
    {
        AssetProcessor::FilteredLogComponent* logger = aznew AssetProcessor::FilteredLogComponent();
        m_entity->AddComponent(logger);
        if (logger)
        {
#if BATCH_MODE
            // Prevent files overwriting each other if you run batch at same time as GUI (unit tests, for example)
            logger->SetLogFileBaseName("AP_Batch");
#else
            logger->SetLogFileBaseName("AP_GUI");
#endif
        }

        //Activate all the components
        m_entity->Init();
        m_entity->Activate();

        return true;
    }
    else
    {
        //aznew failed
        return false;
    }
}

bool ApplicationManager::ActivateModules()
{
    // we load the editor xml for our modules since it contains the list of gems we need for tools to function (not just runtime)
    connect(&m_frameworkApp, &AssetProcessorAZApplication::AssetProcessorStatus, this,
        [this](AssetProcessor::AssetProcessorStatusEntry entry)
    {
        Q_EMIT AssetProcessorStatusChanged(entry);
        QCoreApplication::processEvents(QEventLoop::AllEvents);
    });

    QDir assetRoot;
    if (!AssetUtilities::ComputeAssetRoot(assetRoot))
    {
        AZ_Error(AssetProcessor::ConsoleChannel, false, "Cannot compute the asset root folder.  Is AssetProcessor being run from the appropriate folder?");
        return false;
    }
    assetRoot.cd(AssetUtilities::ComputeGameName());

    QString absoluteConfigFilePath = assetRoot.absoluteFilePath("Config/Editor.xml");

    return m_frameworkApp.ReflectModulesFromAppDescriptor(absoluteConfigFilePath.toUtf8().data());
}

void ApplicationManager::addRunningThread(AssetProcessor::ThreadWorker* thread)
{
    m_runningThreads.push_back(thread);
}

QString ApplicationManager::ParseOptionAppRootArgument()
{
    AZ_Assert(m_qApp!=nullptr,"m_qApp not initialized.  QT application must be created before this call.")
    // Parse any parameters.
    static const char* app_root_parameter = "app-root";
    static const char* app_root_parameter_desc = "Optional external path outside of the current engine to set as the application root.";
    QCommandLineOption  appRootPathOption(QString(app_root_parameter), tr(app_root_parameter_desc), QString("path"));
    QCommandLineParser  parser;
    parser.setApplicationDescription("Asset Processor");
    parser.addOption(appRootPathOption);
    parser.parse(m_qApp->arguments());

    QString appRootArgValue = parser.value(appRootPathOption);
    appRootArgValue.remove(QChar('\"'));
    return appRootArgValue.trimmed();
}

bool ApplicationManager::ValidateExternalAppRoot(QString appRootPath) const 
{
    static const char* bootstrap_cfg_name = "bootstrap.cfg";

    QDir testAppRootPath(appRootPath);

    // Make sure the path exists
    if (!testAppRootPath.exists())
    {
        AZ_Warning(AssetProcessor::ConsoleChannel, testAppRootPath.exists(), "Invalid Application Root path override (--app-root): %s.  Directory does not exist.\n", appRootPath.toUtf8().data());
        return false;
    }

    // Make sure the path contains bootstrap.cfg
    if (!testAppRootPath.exists(bootstrap_cfg_name))
    {
        AZ_Warning(AssetProcessor::ConsoleChannel, testAppRootPath.exists(), "Invalid Application Root path override (--app-root): %s.  Directory does not contain %s.\n", appRootPath.toUtf8().data(), bootstrap_cfg_name);
        return false;
    }

    // Make sure we can read the 'sys_game_folder' settings from bootstrap.cfg
    QSettings settings(testAppRootPath.absoluteFilePath(bootstrap_cfg_name), QSettings::Format::IniFormat);
    static const char* sysGameFolderKeyName = "sys_game_folder";
    auto sysGameFolderSettings = settings.value(sysGameFolderKeyName);
    if (!sysGameFolderSettings.isValid() || sysGameFolderSettings.isNull())
    {
        AZ_Warning(AssetProcessor::ConsoleChannel, testAppRootPath.exists(), "Invalid Application Root path override (--app-root): %s.  %s in the path is not valid.\n", appRootPath.toUtf8().data(), bootstrap_cfg_name);
        return false;
    }

    // Make sure the 'sys_game_folder' value in the external bootstrap.cfg points to a valid subfolder in that path
    QString sysGameFolder = sysGameFolderSettings.toString();
    QDir    gameFolderPath(appRootPath);
    if (!gameFolderPath.cd(sysGameFolder))
    {
        AZ_Warning(AssetProcessor::ConsoleChannel, testAppRootPath.exists(), "Invalid Application Root path override (--app-root): %s.  Configured Game folder %s in the path is not valid.\n", appRootPath.toUtf8().data(), sysGameFolder.toUtf8().data());
        return false;
    }

    return true;
}

ApplicationManager::BeforeRunStatus ApplicationManager::BeforeRun()
{
    // Initialize and prepares Qt Directories
    if (!AssetUtilities::InitializeQtLibraries())
    {
        return ApplicationManager::BeforeRunStatus::Status_Failure;
    }

    // Create the Qt Application
    CreateQtApplication();

    // Calculate the override app root path if provided and validate it before passing it along
    QString overrideAppRootPath = ParseOptionAppRootArgument();
    bool invalidOverrideAppRoot = false;
    if (!overrideAppRootPath.isEmpty())
    {
        if (ValidateExternalAppRoot(overrideAppRootPath))
        {
            QDir overrideAppRoot(overrideAppRootPath);
            QDir resultAppRoot;
            AssetUtilities::ComputeAssetRoot(resultAppRoot, &overrideAppRoot);
        }
        else
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Invalid override app root folder '%s'.", overrideAppRootPath.toUtf8().data());
            return ApplicationManager::BeforeRunStatus::Status_Failure;
        }
    }

    if (!StartAZFramework(overrideAppRootPath))
    {
        return ApplicationManager::BeforeRunStatus::Status_Failure;
    }

    if (!AssetUtilities::ComputeEngineRoot(m_systemRoot))
    {
        return ApplicationManager::BeforeRunStatus::Status_Failure;
    }

    if (!AssetUtilities::UpdateBranchToken())
    {
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Asset Processor was unable to open  the bootstrap file and verify/update the branch token. \
            Please ensure that the bootstrap.cfg file is present and not locked by any other program.\n");
        return ApplicationManager::BeforeRunStatus::Status_Failure;
    }

    return ApplicationManager::BeforeRunStatus::Status_Success;
}

bool ApplicationManager::Activate()
{
    if (!AssetUtilities::ComputeAssetRoot(m_systemRoot))
    {
        return false;
    }

    QString appDir;
    QString appFileName;
    QString binFolderName;
    AssetUtilities::ComputeAppRootAndBinFolderFromApplication(appDir, appFileName, binFolderName);

    m_gameName = AssetUtilities::ComputeGameName(appDir);

    if (m_gameName.isEmpty())
    {
        AZ_Error(AssetProcessor::ConsoleChannel, false, "Unable to detect name of current game project.  Is bootstrap.cfg appropriately configured?");
        return false;
    }
    
    // the following controls what registry keys (or on mac or linux what entries in home folder) are used
    // so they should not be translated!
    qApp->setOrganizationName(GetOrganizationName());
    qApp->setOrganizationDomain("amazon.com");
    qApp->setApplicationName(GetApplicationName());

    InstallTranslators();

    return true;
}

QString ApplicationManager::GetOrganizationName() const
{
    return "Amazon";
}

QString ApplicationManager::GetApplicationName() const
{
    return "Asset Processor";
}

bool ApplicationManager::PostActivate()
{
    return true;
}

void ApplicationManager::InstallTranslators()
{
    QTranslator guiTranslator; // for autogenerated qt files.
    QStringList args = QCoreApplication::arguments();

    QString languageName = QString(":/i18n/qml_") + QLocale::system().name().toLower() + QString(".qm");
    guiTranslator.load(languageName);
    qApp->installTranslator(&guiTranslator);

    if ((args.contains("--wubbleyous", Qt::CaseInsensitive)) ||
        (args.contains("/w", Qt::CaseInsensitive)) ||
        (args.contains("-w", Qt::CaseInsensitive)))
    {
        guiTranslator.load(":/i18n/qml_ww_ww.qm");
    }
}

bool ApplicationManager::NeedRestart() const
{
    return m_needRestart;
}

void ApplicationManager::Restart()
{
    if (m_needRestart)
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "Restart() - already restarting\n");
        return;
    }
    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "AssetProcessor is restarting.\n");
    m_needRestart = true;
    m_updateTimer.stop();
    QuitRequested();
}

void ApplicationManager::ReadyToQuit(QObject* source)
{
    if (!source)
    {
        return;
    }
#if !defined(BATCH_MODE)
    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "App Quit Object of type %s indicates it is ready.\n", source->metaObject()->className());
#endif

    for (int notifyIdx = 0; notifyIdx < m_objectsToNotify.size(); ++notifyIdx)
    {
        if (m_objectsToNotify[notifyIdx].first == source)
        {
            // replace it.
            m_objectsToNotify[notifyIdx] = QuitPair(m_objectsToNotify[notifyIdx].first, true);
        }
    }
    if (!m_queuedCheckQuit)
    {
        QTimer::singleShot(0, this, SLOT(CheckQuit()));
        m_queuedCheckQuit = true;
    }
}

void ApplicationManager::RegisterComponentDescriptor(const AZ::ComponentDescriptor* descriptor)
{
    AZ_Assert(descriptor, "descriptor cannot be null");
    this->m_frameworkApp.RegisterComponentDescriptor(descriptor);
}

ApplicationManager::RegistryCheckInstructions ApplicationManager::CheckForRegistryProblems(QWidget* parentWidget, bool showPopupMessage)
{
#if defined(AZ_PLATFORM_WINDOWS)
    // There's a bug that prevents rc.exe from closing properly, making it appear
    // that jobs never complete. The issue is that Windows sometimes decides to put
    // an exe into a special compatibility mode and tells FreeLibrary calls to stop
    // doing anything. Once the registry entry for this is written, it never gets
    // removed unless the user goes and does it manually in RegEdit.
    // To prevent this from being a problem, we check for that registry key
    // and tell the user to remove it.
    // Here's a link with the same problem reported: https://social.msdn.microsoft.com/Forums/vstudio/en-US/3abe477b-ba6f-49d2-894f-efd42165e620/why-windows-generates-an-ignorefreelibrary-entry-in-appcompatflagslayers-registry-?forum=windowscompatibility
    // Here's a link to someone else with the same problem mentioning the problem registry key: https://software.intel.com/en-us/forums/intel-visual-fortran-compiler-for-windows/topic/606006
    //
    // Also note that this is also solved by telling the PCA (Program Compatibility Assistant) not to
    // set the registry entry in the first place. That's done by adding something extra to the manifest
    // in the WAF build system. That fix only works to stop the registry problem in the first place.
    // It won't stop people already in that situation, which is why this code is here now.

    QString compatibilityRegistryGroupName = "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers";
    QSettings settings(compatibilityRegistryGroupName, QSettings::NativeFormat);

    auto keys = settings.childKeys();
    for (auto key : keys)
    {
        if (key.contains("rc.exe", Qt::CaseInsensitive))
        {
            // Windows will allow us to see that there is an entry, but won't allow us to
            // read the entry or to modify it, so we'll have to warn the user instead

            // Qt displays the key with the slashes flipped; flip them back because we're on windows
            QString windowsFriendlyRegPath = key.replace('/', '\\');

            QString warningText = QObject::tr(
                "The AssetProcessor will not function correctly with certain registry settings. To correct the problem, please:\n"
                "1) Open RegEdit\n"
                "2) When Windows asks if you'd like to allow the app to make changes to your device, click \"Yes\"\n"
                "3) Open the registry group for the path %0\n"
                "4) Delete the key for %1\n"
                "5) %2"
            ).arg(compatibilityRegistryGroupName, windowsFriendlyRegPath);
#if !defined(BATCH_MODE)
            if (showPopupMessage)
            {
                warningText = warningText.arg(tr("Click the Restart button"));

                // Doing all of this as a custom dialog because QMessageBox
                // has a fixed width, which doesn't display the extremely large
                // block of warning text well.
                QDialog dialog(nullptr, Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
                dialog.setWindowTitle("Asset Processor Error");

                QVBoxLayout* layout = new QVBoxLayout(&dialog);
                layout->addSpacing(16);

                QHBoxLayout* messageLayout = new QHBoxLayout(&dialog);
                QLabel* icon = new QLabel("", &dialog);
                QPixmap errorIcon(":/stylesheet/img/lineedit-invalid.png");
                errorIcon = errorIcon.scaled(errorIcon.size() * 4);
                icon->setPixmap(errorIcon);
                icon->setMaximumSize(errorIcon.size());
                QLabel* label = new QLabel(warningText, &dialog);
                messageLayout->addWidget(icon);
                messageLayout->addSpacing(16);
                messageLayout->addWidget(label);
                layout->addLayout(messageLayout);

                layout->addSpacing(16);

                QDialogButtonBox* buttons = new QDialogButtonBox(&dialog);
                QPushButton* exitButton = buttons->addButton(tr("Exit"), QDialogButtonBox::RejectRole);
                connect(exitButton, &QPushButton::pressed, &dialog, &QDialog::reject);
                QPushButton* restartButton = buttons->addButton(tr("Restart"), QDialogButtonBox::AcceptRole);
                connect(restartButton, &QPushButton::pressed, &dialog, &QDialog::accept);
                layout->addWidget(buttons);

                if (dialog.exec() == QDialog::Accepted)
                {
                    return RegistryCheckInstructions::Restart;
                }
            }
            else
#endif // BATCH MODE
            {
                warningText = warningText.arg(tr("Restart the Asset Processor"));
                QByteArray warningUtf8 = warningText.toUtf8();
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, warningUtf8.data());
            }

            return RegistryCheckInstructions::Exit;
        }
    }
#endif

    return RegistryCheckInstructions::Continue;
}


QDateTime ApplicationDependencyInfo::Timestamp() const
{
    return m_timestamp;
}

void ApplicationDependencyInfo::SetTimestamp(const QDateTime& timestamp)
{
    m_timestamp = timestamp;
}

QString ApplicationDependencyInfo::FileName() const
{
    return m_fileName;
}

void ApplicationDependencyInfo::SetFileName(QString fileName)
{
    m_fileName = fileName;
}


#include <native/utilities/ApplicationManager.moc>

