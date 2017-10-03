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
#include <AzToolsFramework/UI/Logging/LogPanel_Panel.h>

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

#include <QMessageBox>
#include <QSettings>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

#include <string.h> // for base  strcpy
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <native/AssetManager/ToolsAssetSystemComponent.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
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
            if (AssetProcessor::GetThreadLocalJobId() != 0)
            {
                return; // we are in a job thread
            }

            AzFramework::LogComponent::OutputMessage(severity, window, message);
        }
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

    components.push_back(azrtti_typeid<AssetProcessor::ToolsAssetSystemComponent>());

    return components;
}

void AssetProcessorAZApplication::RegisterCoreComponents()
{
    AzToolsFramework::ToolsApplication::RegisterCoreComponents();

    RegisterComponentDescriptor(AssetProcessor::ToolsAssetSystemComponent::CreateDescriptor());
    RegisterComponentDescriptor(AzToolsFramework::Components::GenericComponentUnwrapper::CreateDescriptor());
}

void AssetProcessorAZApplication::ResolveModulePath(AZ::OSString& modulePath)
{
   AssetProcessor::AssetProcessorStatusEntry entry(AssetProcessor::AssetProcessorStatus::Initializing_Gems, 0, QString(modulePath.c_str()));
   Q_EMIT AssetProcessorStatus(entry);
   AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Loading (Gem) Module '%s'...\n", modulePath.c_str());
   AzFramework::Application::ResolveModulePath(modulePath);
}


ApplicationManager::ApplicationManager(int argc, char** argv, QObject* parent)
    : QObject(parent)
    , m_argc(argc)
    , m_argv(argv)
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
}

bool ApplicationManager::InitiatedShutdown() const
{
    return m_duringShutdown;
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

char** ApplicationManager::GetCommandArguments() const
{
    return m_argv;
}

int ApplicationManager::CommandLineArgumentsCount() const
{
    return m_argc;
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
    bool tryRestart = false;
    int count = 0;
    for (int idx = 0; idx < m_appDependencies.size(); ++idx)
    {
        ApplicationDependencyInfo* fileDependencyInfo = m_appDependencies[idx];
        QString fileName = fileDependencyInfo->FileName();
        QFile file(fileName);
        if (file.exists())
        {
            fileDependencyInfo->SetWasPresentEver();
            count++; // keeps track of how many files we have found

            QFileInfo fileInfo(fileName);
            QDateTime fileLastModifiedTime = fileInfo.lastModified();
            bool hasTimestampChanged = (fileDependencyInfo->Timestamp() != fileLastModifiedTime);
            if (!fileDependencyInfo->IsModified())
            {
                //if file is not reported as modified earlier
                //we check to see whether it is modified now
                if (hasTimestampChanged)
                {
                    fileDependencyInfo->SetTimestamp(fileLastModifiedTime);
                    fileDependencyInfo->SetIsModified(true);
                    fileDependencyInfo->SetStillUpdating(true);
                }
            }
            else
            {
                // if the file has been reported to have modified once
                //we check to see whether it is still changing
                if (hasTimestampChanged)
                {
                    fileDependencyInfo->SetTimestamp(fileLastModifiedTime);
                    fileDependencyInfo->SetStillUpdating(true);
                }
                else
                {
                    fileDependencyInfo->SetStillUpdating(false);
                    tryRestart = true;
                }
            }
        }
        else
        {
            // if one of the files is not present or is deleted we construct a null datetime for it and
            // continue checking

            if (!fileDependencyInfo->WasPresentEver())
            {
                // if it never existed in the first place, its okay to restart!
                // however, if it suddenly appears later or it disappears after it was there, it is relevant
                // and we need to wait for it to come back.
                ++count;
            }
            else
            {
                fileDependencyInfo->SetTimestamp(QDateTime());
                fileDependencyInfo->SetIsModified(true);
                fileDependencyInfo->SetStillUpdating(true);
            }
        }
    }

    if (tryRestart && count == m_appDependencies.size())
    {
        //we only check here if one file after getting modified is not getting changed any longer
        tryRestart = false;
        count = 0;
        for (int idx = 0; idx < m_appDependencies.size(); ++idx)
        {
            ApplicationDependencyInfo* fileDependencyInfo = m_appDependencies[idx];
            if (fileDependencyInfo->IsModified() && !fileDependencyInfo->StillUpdating())
            {
                //all those files which have modified and are not changing any longer
                count++;
            }
            else if (!fileDependencyInfo->IsModified())
            {
                //files that are not modified
                count++;
            }
        }

        if (count == m_appDependencies.size())
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "All dependencies accounted for.\n");

            //We will reach here only if all the modified files are not changing
            //we can now stop the timer and request exit
            Restart();
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

    QString builderDirPath = dir.filePath("Builders");
    QDir builderDir(builderDirPath);
    QStringList filter;
#if defined(AZ_PLATFORM_WINDOWS)
    filter.append("*.dll");
#elif defined(AZ_PLATFORM_LINUX)
    filter.append("*.so");
#elif defined(AZ_PLATFORM_APPLE)
    filter.append("*.dylib");
#endif
    QStringList fileList = builderDir.entryList(filter, QDir::Files);
    for (QString& file : fileList)
    {
        m_filesOfInterest.push_back(builderDir.filePath(file));
    }

    QDir assetRoot;
    AssetUtilities::ComputeAssetRoot(assetRoot);

    QString globalConfigPath = assetRoot.filePath("AssetProcessorPlatformConfig.ini");
    m_filesOfInterest.push_back(globalConfigPath);

    QString gameName = AssetUtilities::ComputeGameName();
    QString gamePlatformConfigPath = assetRoot.filePath(gameName + "/AssetProcessorGamePlatformConfig.ini");
    m_filesOfInterest.push_back(gamePlatformConfigPath);

    // if our Gems file changes, make sure we watch that, too.
    QString gemsConfigFile = assetRoot.filePath(gameName + "/gems.json");
    if (QFile::exists(gemsConfigFile))
    {
        m_filesOfInterest.push_back(gemsConfigFile);
    }

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


bool ApplicationManager::StartAZFramework()
{
    AzFramework::Application::Descriptor appDescriptor;
    AZ::ComponentApplication::StartupParameters params;
    QString dir;
    QString filename;
    QString binFolder;

    AssetUtilities::ComputeAppRootAndBinFolderFromApplication(dir, filename, binFolder);

    // The application will live in a bin folder one level up from the app root.
    char staticStorageForRootPath[AZ_MAX_PATH_LEN] = {0};
    azstrcpy(staticStorageForRootPath, AZ_MAX_PATH_LEN, dir.toUtf8().data());
    
    // Prevent script reflection warnings from bringing down the AssetProcessor
    appDescriptor.m_enableScriptReflection = false;

    params.m_appRootOverride = staticStorageForRootPath;
    m_frameworkApp.Start(appDescriptor, params);
    
    //Registering all the Components
    m_frameworkApp.RegisterComponentDescriptor(AzFramework::LogComponent::CreateDescriptor());
 
    AZ::SerializeContext* context;
    EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(context, "No serialize context");
    AzToolsFramework::LogPanel::BaseLogPanel::Reflect(context);
    
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

    QDir engineRoot;
    if (!AssetUtilities::ComputeEngineRoot(engineRoot))
    {
        AZ_Error(AssetProcessor::ConsoleChannel, false, "Cannot compute the root of the engine.  Is AssetProcessor being run from the appropriate folder?");
        return false;
    }
    engineRoot.cd(AssetUtilities::ComputeGameName());

    QString absoluteConfigFilePath = engineRoot.absoluteFilePath("Config/Editor.xml");

    return m_frameworkApp.ReflectModulesFromAppDescriptor(absoluteConfigFilePath.toUtf8().data());
}

void ApplicationManager::addRunningThread(AssetProcessor::ThreadWorker* thread)
{
    m_runningThreads.push_back(thread);
}


ApplicationManager::BeforeRunStatus ApplicationManager::BeforeRun()
{
    //Initialize and prepares Qt Directories
    if (!AssetUtilities::InitializeQtLibraries())
    {
        return ApplicationManager::BeforeRunStatus::Status_Failure;
    }

    CreateQtApplication();

    if (!StartAZFramework())
    {
        return ApplicationManager::BeforeRunStatus::Status_Failure;
    }

    if (!AssetUtilities::ComputeEngineRoot(m_systemRoot))
    {
        return ApplicationManager::BeforeRunStatus::Status_Failure;
    }

    AssetUtilities::UpdateBranchToken();

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
    qApp->setOrganizationName("Amazon");
    qApp->setOrganizationDomain("amazon.com");
    qApp->setApplicationName("Asset Processor");

    InstallTranslators();

    return true;
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

void ApplicationManager::UnRegisterComponentDescriptor(const AZ::ComponentDescriptor* descriptor)
{
    AZ_Assert(descriptor, "descriptor cannot be null");
    this->m_frameworkApp.UnregisterComponentDescriptor(descriptor);
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

bool ApplicationDependencyInfo::IsModified() const
{
    return m_isModified;
}

void ApplicationDependencyInfo::SetIsModified(bool hasModified)
{
    m_isModified = hasModified;
}

bool ApplicationDependencyInfo::StillUpdating() const
{
    return m_stillUpdating;
}

void ApplicationDependencyInfo::SetStillUpdating(bool stillUpdating)
{
    m_stillUpdating = stillUpdating;
}
QString ApplicationDependencyInfo::FileName() const
{
    return m_fileName;
}

void ApplicationDependencyInfo::SetFileName(QString fileName)
{
    m_fileName = fileName;
}

void ApplicationDependencyInfo::SetWasPresentEver()
{
    m_wasPresentEver = true;
}

bool ApplicationDependencyInfo::WasPresentEver() const
{
    return m_wasPresentEver;
}

#include <native/utilities/ApplicationManager.moc>

