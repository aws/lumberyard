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
#include "BatchApplicationManager.h"

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/PlatformIncl.h>

#include "native/utilities/assetUtils.h"
#include "native/AssetManager/assetProcessorManager.h"
#include "native/utilities/PlatformConfiguration.h"
#include "native/FileWatcher/FileWatcherAPI.h"
#include "native/AssetManager/assetScanFolderInfo.h"
#include "native/resourcecompiler/rccontroller.h"
#include "native/AssetManager/assetScanner.h"
#include "native/utilities/ApplicationServer.h"
#include "native/connection/connectionManager.h"
#include "native/utilities/ByteArrayStream.h"
#include "native/AssetManager/AssetRequestHandler.h"
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/Logging/LogLine.h>
#include <AzToolsFramework/Application/Ticker.h>

#include <QTextStream>
#include <QCoreApplication>

#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzCore/std/string/string.h>
#include <QMessageBox>
#include <AzCore/Asset/AssetManagerBus.h>

// in batch mode, we are going to show the log files of up to N failures. 
// in order to not spam the logs, we limit this - its possible that something fundamental is broken and EVERY asset is failing
// and we don't want to thus write gigabytes of logs out.
const int MAXIMUM_FAILURES_TO_REPORT = 10;

#if defined(AZ_PLATFORM_WINDOWS) && defined(BATCH_MODE)
namespace BatchApplicationManagerPrivate
{
    BatchApplicationManager* g_appManager = nullptr;
    BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
    {
        (void)dwCtrlType;
        AZ_Printf("AssetProcessor", "Asset Processor Batch Processing Interrupted. Quitting.\n");
        QMetaObject::invokeMethod(g_appManager, "QuitRequested", Qt::QueuedConnection);
        return TRUE;
    }
}
#endif  //#if defined(AZ_PLATFORM_WINDOWS) && defined(BATCH_MODE)

#ifdef UNIT_TEST
#include "native/connection/connectionManager.h"
#include "native/utilities/UnitTests.h"
#endif

BatchApplicationManager::BatchApplicationManager(int argc, char** argv, QObject* parent)
    : ApplicationManager(argc, argv, parent)
    , m_currentExternalAssetBuilder(nullptr)
{
    qRegisterMetaType<AZ::u32>("AZ::u32");
    qRegisterMetaType<AZ::Uuid>("AZ::Uuid");
}


BatchApplicationManager::~BatchApplicationManager()
{
    if (m_internalBuilder.get())
    {
        m_internalBuilder->UnInitialize();
    }

    for (AssetProcessor::ExternalModuleAssetBuilderInfo* externalAssetBuilderInfo : this->m_externalAssetBuilders)
    {
        externalAssetBuilderInfo->UnInitialize();
        delete externalAssetBuilderInfo;
    }

    Destroy();
}

AssetProcessor::RCController* BatchApplicationManager::GetRCController() const
{
    return m_rcController;
}

int BatchApplicationManager::ProcessedAssetCount() const
{
    return m_processedAssetCount;
}
int BatchApplicationManager::FailedAssetsCount() const
{
    return m_failedAssetsCount;
}

void BatchApplicationManager::ResetProcessedAssetCount()
{
    m_processedAssetCount = 0;
}

void BatchApplicationManager::ResetFailedAssetCount()
{
    m_failedAssetsCount = 0;
}


AssetProcessor::AssetScanner* BatchApplicationManager::GetAssetScanner() const
{
    return m_assetScanner;
}

AssetProcessor::AssetProcessorManager* BatchApplicationManager::GetAssetProcessorManager() const
{
    return m_assetProcessorManager;
}

AssetProcessor::PlatformConfiguration* BatchApplicationManager::GetPlatformConfiguration() const
{
    return m_platformConfiguration;
}

ConnectionManager* BatchApplicationManager::GetConnectionManager() const
{
    return m_connectionManager;
}

ApplicationServer* BatchApplicationManager::GetApplicationServer() const
{
    return m_applicationServer;
}

void BatchApplicationManager::InitAssetProcessorManager()
{
    AssetProcessor::ThreadController<AssetProcessor::AssetProcessorManager>* assetProcessorHelper = new AssetProcessor::ThreadController<AssetProcessor::AssetProcessorManager>();

    addRunningThread(assetProcessorHelper);
    m_assetProcessorManager = assetProcessorHelper->initialize([this, &assetProcessorHelper]()
            {
                return new AssetProcessor::AssetProcessorManager(m_platformConfiguration, assetProcessorHelper);
            });
}

void BatchApplicationManager::InitAssetCatalog()
{
    using namespace AssetProcessor;
    ThreadController<AssetCatalog>* assetCatalogHelper = new ThreadController<AssetCatalog>();

    addRunningThread(assetCatalogHelper);
    m_assetCatalog = assetCatalogHelper->initialize([this, &assetCatalogHelper]()
            {
                QStringList platforms;
                for (int idx = 0, end = m_platformConfiguration->PlatformCount(); idx < end; ++idx)
                {
                    platforms.push_back(m_platformConfiguration->PlatformAt(idx));
                }

                AssetProcessor::AssetCatalog* catalog = new AssetCatalog(assetCatalogHelper, platforms,
                        m_assetProcessorManager->GetDatabaseConnection());

                connect(m_assetProcessorManager, &AssetProcessorManager::AssetMessage, catalog, &AssetCatalog::OnAssetMessage);

                return catalog;
            });
}

void BatchApplicationManager::InitRCController()
{
    m_rcController = new AssetProcessor::RCController(m_platformConfiguration->GetMinJobs(), m_platformConfiguration->GetMaxJobs());

    QObject::connect(m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetToProcess, m_rcController, &AssetProcessor::RCController::JobSubmitted);
    QObject::connect(m_rcController, &AssetProcessor::RCController::FileCompiled, m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetProcessed, Qt::UniqueConnection);
    QObject::connect(m_rcController, &AssetProcessor::RCController::FileFailed, m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetFailed);
    QObject::connect(m_rcController, &AssetProcessor::RCController::FileCancelled, m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetCancelled);
    QObject::connect(m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::EscalateJobs, m_rcController, &AssetProcessor::RCController::EscalateJobs);
}

void BatchApplicationManager::DestroyRCController()
{
    if (m_rcController)
    {
        delete m_rcController;
        m_rcController = nullptr;
    }
}

void BatchApplicationManager::InitAssetScanner()
{
    m_assetScanner = new AssetProcessor::AssetScanner(m_platformConfiguration);
    QObject::connect(m_assetScanner, SIGNAL(AssetScanningStatusChanged(AssetProcessor::AssetScanningStatus)),
        m_assetProcessorManager, SLOT(OnAssetScannerStatusChange(AssetProcessor::AssetScanningStatus)));
    QObject::connect(m_assetScanner, SIGNAL(FileOfInterestFound(QString)),
        m_assetProcessorManager, SLOT(AssessModifiedFile(QString)));
}

void BatchApplicationManager::DestroyAssetScanner()
{
    if (m_assetScanner)
    {
        delete m_assetScanner;
        m_assetScanner = nullptr;
    }
}

bool BatchApplicationManager::InitPlatformConfiguration()
{
    m_platformConfiguration = new AssetProcessor::PlatformConfiguration();
    // load platform ini first
    QStringList configFiles = {
        GetSystemRoot().absoluteFilePath("AssetProcessorPlatformConfig.ini"),
    };

    // Read in Gems for the active game
    m_platformConfiguration->ReadGemsConfigFile(GetSystemRoot().absoluteFilePath(GetGameName() + "/gems.json"), configFiles);

    // Game config ini's loaded last, allowing it to override platform and gems config
    configFiles.push_back(GetSystemRoot().absoluteFilePath(GetGameName() + "/AssetProcessorGamePlatformConfig.ini"));

    // Read platforms first
    for (QString configFile : configFiles)
    {
        m_platformConfiguration->PopulateEnabledPlatforms(configFile);
    }
    

    // Then read recognizers (which depend on platforms)
    for (QString configFile : configFiles)
    {
        if (!m_platformConfiguration->ReadRecognizersFromConfigFile(configFile))
        {
            return false;
        }
    }

    // Then read metadata (which depends on scan folders)
    for (QString configFile : configFiles)
    {
        m_platformConfiguration->ReadMetaDataFromConfigFile(configFile);
    }

    return true;
}

void BatchApplicationManager::DestroyPlatformConfiguration()
{
    if (m_platformConfiguration)
    {
        delete m_platformConfiguration;
        m_platformConfiguration = nullptr;
    }
}

void BatchApplicationManager::InitFileMonitor()
{
    m_folderWatches.reserve(m_platformConfiguration->GetScanFolderCount());
    m_watchHandles.reserve(m_platformConfiguration->GetScanFolderCount());
    for (int folderIdx = 0; folderIdx < m_platformConfiguration->GetScanFolderCount(); ++folderIdx)
    {
        const AssetProcessor::ScanFolderInfo& info = m_platformConfiguration->GetScanFolderAt(folderIdx);

        FolderWatchCallbackEx* newFolderWatch = new FolderWatchCallbackEx(info.ScanPath(), "", info.RecurseSubFolders());
        // hook folder watcher to assess files on add/modify
        // relevant files will be sent to resource compiler
        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileAdded,
            m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssessAddedFile);
        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileModified,
            m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssessModifiedFile);
        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileRemoved,
            m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssessDeletedFile);

        m_folderWatches.push_back(AZStd::unique_ptr<FolderWatchCallbackEx>(newFolderWatch));
        m_watchHandles.push_back(m_fileWatcher.AddFolderWatch(newFolderWatch));
    }
    
    // also hookup monitoring for the cache (output directory)
    QDir cacheRoot;
    if (AssetUtilities::ComputeProjectCacheRoot(cacheRoot))
    {
        FolderWatchCallbackEx* newFolderWatch = new FolderWatchCallbackEx(cacheRoot.absolutePath(), "", true);

        // we only care about cache root deletions.
        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileRemoved,
            m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssessDeletedFile);

        m_folderWatches.push_back(AZStd::unique_ptr<FolderWatchCallbackEx>(newFolderWatch));
        m_watchHandles.push_back(m_fileWatcher.AddFolderWatch(newFolderWatch));
    }
    
    m_fileWatcher.StartWatching();
}

void BatchApplicationManager::DestroyFileMonitor()
{
    for (int watchHandle : m_watchHandles)
    {
        m_fileWatcher.RemoveFolderWatch(watchHandle);
    }
    m_folderWatches.resize(0);
}

bool BatchApplicationManager::InitApplicationServer()
{
    m_applicationServer = new ApplicationServer();
    return true;
}

void BatchApplicationManager::DestroyApplicationServer()
{
    if (m_applicationServer)
    {
        delete m_applicationServer;
        m_applicationServer = nullptr;
    }
}

void BatchApplicationManager::InitConnectionManager()
{
    using namespace std::placeholders;
    using namespace AzFramework::AssetSystem;
    using namespace AzToolsFramework::AssetSystem;

    m_connectionManager = new ConnectionManager(GetPlatformConfiguration(), AssetUtilities::ReadListeningPortFromBootstrap());

    QObject* connectionAndChangeMessagesThreadContext = this;

    // AssetProcessor Manager related stuff
    auto forwardMessageFunction = [this](QString platform, AzFramework::AssetSystem::AssetNotificationMessage message)
    {
        EBUS_EVENT(AssetProcessor::ConnectionBus, SendPerPlatform, 0, message, platform);
    };

    bool result = QObject::connect(GetAssetCatalog(), &AssetProcessor::AssetCatalog::SendAssetMessage, connectionAndChangeMessagesThreadContext, forwardMessageFunction, Qt::QueuedConnection);
    AZ_Assert(result, "Failed to connect to AssetCatalog signal");

    //Application manager related stuff

    // The AssetCatalog has to be rebuilt on connection, so we force the incoming connection messages to be serialized as they connect to the BatchApplicationManager
    result = QObject::connect(m_applicationServer, &ApplicationServer::newIncomingConnection, m_connectionManager, &ConnectionManager::NewConnection, Qt::QueuedConnection);

    AZ_Assert(result, "Failed to connect to ApplicationServer signal");

    //RcController related stuff
    result = QObject::connect(GetRCController(), &AssetProcessor::RCController::JobStatusChanged, GetAssetProcessorManager(), &AssetProcessor::AssetProcessorManager::OnJobStatusChanged);
    AZ_Assert(result, "Failed to connect to RCController signal");

#if !defined(FORCE_PROXY_MODE)
    result = QObject::connect(GetRCController(), &AssetProcessor::RCController::JobStarted,
        [this](QString inputFile, QString platform)
    {
        QString msg = QCoreApplication::translate("Asset Processor", "Processing %1 (%2)...\n", "%1 is the name of the file, and %2 is the platform to process it for").arg(inputFile, platform);
        AZ_Printf(AssetProcessor::ConsoleChannel, "%s", msg.toUtf8().constData());
        AssetNotificationMessage message(inputFile.toUtf8().constData(), AssetNotificationMessage::JobStarted, AZ::Data::s_invalidAssetType);
        EBUS_EVENT(AssetProcessor::ConnectionBus, SendPerPlatform, 0, message, platform);
    }
    );
    AZ_Assert(result, "Failed to connect to RCController signal");

    result = QObject::connect(GetRCController(), &AssetProcessor::RCController::FileCompiled,
        [this](AssetProcessor::JobEntry entry, AssetBuilderSDK::ProcessJobResponse /*response*/)
    {
        AssetNotificationMessage message(entry.m_relativePathToFile.toUtf8().constData(), AssetNotificationMessage::JobCompleted, AZ::Data::s_invalidAssetType);
        EBUS_EVENT(AssetProcessor::ConnectionBus, SendPerPlatform, 0, message, entry.m_platform);
    }
    );
    AZ_Assert(result, "Failed to connect to RCController signal");

    result = QObject::connect(GetRCController(), &AssetProcessor::RCController::FileFailed,
        [this](AssetProcessor::JobEntry entry)
    {
        AssetNotificationMessage message(entry.m_relativePathToFile.toUtf8().constData(), AssetNotificationMessage::JobFailed, AZ::Data::s_invalidAssetType);
        EBUS_EVENT(AssetProcessor::ConnectionBus, SendPerPlatform, 0, message, entry.m_platform);
    }
    );
    AZ_Assert(result, "Failed to connect to RCController signal");

    result = QObject::connect(GetRCController(), &AssetProcessor::RCController::JobsInQueuePerPlatform,
        [this](QString platform, int count)
    {
        AssetNotificationMessage message(QByteArray::number(count).constData(), AssetNotificationMessage::JobCount, AZ::Data::s_invalidAssetType);
        EBUS_EVENT(AssetProcessor::ConnectionBus, SendPerPlatform, 0, message, platform);
    }
    );
    AZ_Assert(result, "Failed to connect to RCController signal");
#endif //FORCE_PROXY_MODE

    m_connectionManager->RegisterService(RequestPing::MessageType(),
        std::bind([this](unsigned int connId, unsigned int /*type*/, unsigned int serial, QByteArray /*payload*/)
    {
        ResponsePing responsePing;
        EBUS_EVENT_ID(connId, AssetProcessor::ConnectionBus, Send, serial, responsePing);

    }, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)
    );

    //You can get Asset Processor Current State
    using AzFramework::AssetSystem::RequestAssetProcessorStatus;
    auto GetState = [this](unsigned int connId, unsigned int, unsigned int serial, QByteArray payload, QString)
    {
        RequestAssetProcessorStatus requestAssetProcessorMessage;

        if (AssetProcessor::UnpackMessage(payload, requestAssetProcessorMessage))
        {
            bool status = false;
            //check whether the scan is complete,the asset processor manager initial processing is complete and
            //the number of copy jobs are zero

            int numberOfPendingJobs = GetRCController()->NumberOfPendingCriticalJobsPerPlatform(requestAssetProcessorMessage.m_platform.c_str());
            status = (GetAssetScanner()->status() == AssetProcessor::AssetScanningStatus::Completed)
                && m_assetProcessorManagerIsReady
                && (!numberOfPendingJobs);

            ResponseAssetProcessorStatus responseAssetProcessorMessage;
            responseAssetProcessorMessage.m_isAssetProcessorReady = status;
            responseAssetProcessorMessage.m_numberOfPendingJobs = numberOfPendingJobs + m_remainingAPMJobs;
            if (responseAssetProcessorMessage.m_numberOfPendingJobs && m_highestConnId < connId)
            {
                // We will just emit this status message once per connId
                Q_EMIT ConnectionStatusMsg(QString(" Critical assets need to be processed for %1 platform. Editor/Game will launch once they are processed.").arg(requestAssetProcessorMessage.m_platform.c_str()));
                m_highestConnId = connId;
            }
            EBUS_EVENT_ID(connId, AssetProcessor::ConnectionBus, Send, serial, responseAssetProcessorMessage);
        }
    };
    // connect the network messages to the Request handler:
    m_connectionManager->RegisterService(RequestAssetProcessorStatus::MessageType(), GetState);
}

void BatchApplicationManager::DestroyConnectionManager()
{
    if (m_connectionManager)
    {
        delete m_connectionManager;
        m_connectionManager = nullptr;
    }
}

void BatchApplicationManager::InitAssetRequestHandler()
{
    using namespace std::placeholders;
    using namespace AzFramework::AssetSystem;
    using namespace AzToolsFramework::AssetSystem;
    using namespace AzFramework::AssetSystem;
    using namespace AssetProcessor;

    m_assetRequestHandler = new AssetRequestHandler();

    m_assetRequestHandler->RegisterRequestHandler(AssetJobsInfoRequest::MessageType(), GetAssetProcessorManager());
    m_assetRequestHandler->RegisterRequestHandler(AssetJobLogRequest::MessageType(), GetAssetProcessorManager());
    m_assetRequestHandler->RegisterRequestHandler(SourceFileInfoRequest::MessageType(), GetAssetProcessorManager());
    m_assetRequestHandler->RegisterRequestHandler(SaveAssetCatalogRequest::MessageType(), GetAssetCatalog());


    auto ProcessGetRelativeProductPathFromFullSourceOrProductPathRequest = [&](unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload, QString platform)
    {
        QMetaObject::invokeMethod(m_assetRequestHandler, "OnNewIncomingRequest", Qt::QueuedConnection, Q_ARG(unsigned int, connId), Q_ARG(unsigned int, serial), Q_ARG(QByteArray, payload), Q_ARG(QString, platform));
    };
    m_connectionManager->RegisterService(GetRelativeProductPathFromFullSourceOrProductPathRequest::MessageType(), std::bind(ProcessGetRelativeProductPathFromFullSourceOrProductPathRequest, _1, _2, _3, _4, _5));

    auto ProcessGetFullSourcePathFromRelativeProductPathRequest = [&](unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload, QString platform)
    {
        QMetaObject::invokeMethod(m_assetRequestHandler, "OnNewIncomingRequest", Qt::QueuedConnection, Q_ARG(unsigned int, connId), Q_ARG(unsigned int, serial), Q_ARG(QByteArray, payload), Q_ARG(QString, platform));
    };
    m_connectionManager->RegisterService(GetFullSourcePathFromRelativeProductPathRequest::MessageType(), std::bind(ProcessGetFullSourcePathFromRelativeProductPathRequest, _1, _2, _3, _4, _5));

    auto ProcessGetJobsInfoRequest = [&](unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload, QString platform)
    {
        QMetaObject::invokeMethod(m_assetRequestHandler, "OnNewIncomingRequest", Qt::QueuedConnection, Q_ARG(unsigned int, connId), Q_ARG(unsigned int, serial), Q_ARG(QByteArray, payload), Q_ARG(QString, platform));
    };
    m_connectionManager->RegisterService(AssetJobsInfoRequest::MessageType(), std::bind(ProcessGetJobsInfoRequest, _1, _2, _3, _4, _5));

    auto ProcessGetJobLogRequest = [&](unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload, QString platform)
    {
        QMetaObject::invokeMethod(m_assetRequestHandler, "OnNewIncomingRequest", Qt::QueuedConnection, Q_ARG(unsigned int, connId), Q_ARG(unsigned int, serial), Q_ARG(QByteArray, payload), Q_ARG(QString, platform));
    };
    m_connectionManager->RegisterService(AssetJobLogRequest::MessageType(), std::bind(ProcessGetJobLogRequest, _1, _2, _3, _4, _5));

    auto ProcessSourceFileInfoRequest = [&](unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload, QString platform)
    {
        QMetaObject::invokeMethod(m_assetRequestHandler, "OnNewIncomingRequest", Qt::QueuedConnection, Q_ARG(unsigned int, connId), Q_ARG(unsigned int, serial), Q_ARG(QByteArray, payload), Q_ARG(QString, platform));
    };
    m_connectionManager->RegisterService(SourceFileInfoRequest::MessageType(), std::bind(ProcessSourceFileInfoRequest, _1, _2, _3, _4, _5));

    auto SaveAssetCatalogRequest = [&](unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload, QString platform)
    {
        QMetaObject::invokeMethod(m_assetRequestHandler, "OnNewIncomingRequest", Qt::QueuedConnection, Q_ARG(unsigned int, connId), Q_ARG(unsigned int, serial), Q_ARG(QByteArray, payload), Q_ARG(QString, platform));
    };
    m_connectionManager->RegisterService(SaveAssetCatalogRequest::MessageType(), std::bind(SaveAssetCatalogRequest, _1, _2, _3, _4, _5));

    // connect the "Does asset exist?" loop to each other:
    QObject::connect(m_assetRequestHandler, &AssetRequestHandler::RequestAssetExists, GetAssetProcessorManager(), &AssetProcessorManager::OnRequestAssetExists);
    QObject::connect(GetAssetProcessorManager(), &AssetProcessorManager::SendAssetExistsResponse, m_assetRequestHandler, &AssetRequestHandler::OnRequestAssetExistsResponse);
    QObject::connect(GetAssetProcessorManager(), &AssetProcessorManager::AssetProcessorManagerIdleState, this,
        [this](bool state)
    {
        if (state)
        {
            // When we've completed work (i.e. no more pending jobs), update the on-disk asset catalog.
            AssetRegistryRequestBus::Broadcast(&AssetRegistryRequests::SaveRegistry);
        }
    });

    // connect the network messages to the Request handler:
    m_connectionManager->RegisterService(RequestAssetStatus::MessageType(),
        std::bind([&](unsigned int connId, unsigned int serial, QByteArray payload, QString platform)
    {
        QMetaObject::invokeMethod(m_assetRequestHandler, "OnNewIncomingRequest", Qt::QueuedConnection, Q_ARG(unsigned int, connId), Q_ARG(unsigned int, serial), Q_ARG(QByteArray, payload), Q_ARG(QString, platform));
    },
            std::placeholders::_1, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
    );

    QObject::connect(GetAssetProcessorManager(), &AssetProcessorManager::FenceFileDetected, m_assetRequestHandler, &AssetRequestHandler::OnFenceFileDetected);
#if !defined(FORCE_PROXY_MODE)
    // connect the Asset Request Handler to RC:
    QObject::connect(m_assetRequestHandler, &AssetRequestHandler::RequestCompileGroup, GetRCController(), &RCController::OnRequestCompileGroup);
    QObject::connect(GetRCController(), &RCController::CompileGroupCreated, m_assetRequestHandler, &AssetRequestHandler::OnCompileGroupCreated);
    QObject::connect(GetRCController(), &RCController::CompileGroupFinished, m_assetRequestHandler, &AssetRequestHandler::OnCompileGroupFinished);
#endif

    QObject::connect(GetAssetProcessorManager(), &AssetProcessor::AssetProcessorManager::NumRemainingJobsChanged, this, [this](int newNum)
    {
        if (!m_assetProcessorManagerIsReady)
        {
            m_remainingAPMJobs = newNum;

            if (!m_remainingAPMJobs)
            {
                m_assetProcessorManagerIsReady = true;
            }
        }

        AssetProcessor::AssetProcessorStatusEntry entry(AssetProcessor::AssetProcessorStatus::Analyzing_Jobs, newNum);
        Q_EMIT AssetProcessorStatusChanged(entry);
    });
}

ApplicationManager::BeforeRunStatus BatchApplicationManager::BeforeRun()
{
    ApplicationManager::BeforeRunStatus status = ApplicationManager::BeforeRun();

    if (status != ApplicationManager::BeforeRunStatus::Status_Success)
    {
        return status;
    }

    //Register all QMetatypes here
    qRegisterMetaType<AzFramework::AssetSystem::AssetStatus>("AzFramework::AssetSystem::AssetStatus");
    qRegisterMetaType<AzFramework::AssetSystem::AssetStatus>("AssetStatus");

    qRegisterMetaType<FileChangeInfo>("FileChangeInfo");

    qRegisterMetaType<AssetProcessor::AssetScanningStatus>("AssetScanningStatus");

    qRegisterMetaType<AssetProcessor::NetworkRequestID>("NetworkRequestID");

    qRegisterMetaType<AssetProcessor::JobEntry>("JobEntry");

    qRegisterMetaType<AssetBuilderSDK::ProcessJobResponse>("ProcessJobResponse");

    qRegisterMetaType<AzToolsFramework::AssetSystem::JobStatus>("AzToolsFramework::AssetSystem::JobStatus");
    qRegisterMetaType<AzToolsFramework::AssetSystem::JobStatus>("JobStatus");

    qRegisterMetaType<AssetProcessor::JobDetails>("JobDetails");
    qRegisterMetaType<AZ::Data::AssetId>("AZ::Data::AssetId");
    qRegisterMetaType<AZ::Data::AssetInfo>("AZ::Data::AssetInfo");

    qRegisterMetaType<AzToolsFramework::AssetSystem::AssetJobLogRequest>("AzToolsFramework::AssetSystem::AssetJobLogRequest");
    qRegisterMetaType<AzToolsFramework::AssetSystem::AssetJobLogRequest>("AssetJobLogRequest");

    qRegisterMetaType<AzToolsFramework::AssetSystem::AssetJobLogResponse>("AzToolsFramework::AssetSystem::AssetJobLogResponse");
    qRegisterMetaType<AzToolsFramework::AssetSystem::AssetJobLogResponse>("AssetJobLogResponse");

    qRegisterMetaType<AzFramework::AssetSystem::BaseAssetProcessorMessage*>("AzFramework::AssetSystem::BaseAssetProcessorMessage*");
    qRegisterMetaType<AzFramework::AssetSystem::BaseAssetProcessorMessage*>("BaseAssetProcessorMessage*");

    qRegisterMetaType<AssetProcessor::JobIdEscalationList>("AssetProcessor::JobIdEscalationList");
    qRegisterMetaType<AzFramework::AssetSystem::AssetNotificationMessage>("AzFramework::AssetSystem::AssetNotificationMessage");
    qRegisterMetaType<AzFramework::AssetSystem::AssetNotificationMessage>("AssetNotificationMessage");
    qRegisterMetaType<AZStd::string>("AZStd::string");

    AssetBuilderSDK::AssetBuilderBus::Handler::BusConnect();
    AssetProcessor::AssetBuilderRegistrationBus::Handler::BusConnect();
    AssetProcessor::AssetBuilderInfoBus::Handler::BusConnect();
    AZ::Debug::TraceMessageBus::Handler::BusConnect();

    return ApplicationManager::BeforeRunStatus::Status_Success;
}

void BatchApplicationManager::Destroy()
{
#if defined(AZ_PLATFORM_WINDOWS) && defined(BATCH_MODE)
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)BatchApplicationManagerPrivate::CtrlHandlerRoutine, FALSE);
    BatchApplicationManagerPrivate::g_appManager = nullptr;
#endif //#if defined(AZ_PLATFORM_WINDOWS) && defined(BATCH_MODE)
    
    delete m_ticker;
    m_ticker = nullptr;

    delete m_assetRequestHandler;
    m_assetRequestHandler = nullptr;

    DestroyConnectionManager();
    
#if !FORCE_PROXY_MODE
    DestroyRCController();
    DestroyAssetScanner();
    DestroyFileMonitor();
    ShutDownAssetDatabase();
#endif
    DestroyPlatformConfiguration();
    DestroyApplicationServer();
}

bool BatchApplicationManager::Run()
{
    QStringList args = QCoreApplication::arguments();
#ifdef UNIT_TEST
    for (QString arg : args)
    {
        bool runTests = false;

        if (arg.startsWith("/unittest", Qt::CaseInsensitive))
        {
            runTests = true;
        }

        // Use the AZ testing framework unit tests instead if enabled
#if !defined(AZ_TESTS_ENABLED)
        else if (arg.startsWith("--unittest", Qt::CaseInsensitive))
        {
            runTests = true;
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "the --unittest command line parameter has been deprecated.  please use /unittest[=TestName,TestName,TestName...] instead.");
        }
#endif
        if (runTests)
        {
            // Before running the unit tests we are disconnecting from the AssetBuilderInfo bus as we will be making a dummy handler in the unit test
            AssetProcessor::AssetBuilderInfoBus::Handler::BusDisconnect();
            return RunUnitTests();
        }
    }
#endif
    bool showErrorMessageOnRegistryProblem = false;
    RegistryCheckInstructions registryCheckInstructions = CheckForRegistryProblems(nullptr, showErrorMessageOnRegistryProblem);
    if (registryCheckInstructions != RegistryCheckInstructions::Continue)
    {
        return false;
    }

    if (!Activate())
    {
        return false;
    }

    bool startedSuccessfully = true;
    
    if (!PostActivate())
    {
        emit QuitRequested();
        startedSuccessfully = false;
    }

    AZ_Printf(AssetProcessor::ConsoleChannel, "Asset Processor Batch Processing Started.\n");
    m_duringStartup = false;
    qApp->exec();

    AZ_Printf(AssetProcessor::ConsoleChannel, "Number of Assets Successfully Processed: %d.\n", ProcessedAssetCount());
    AZ_Printf(AssetProcessor::ConsoleChannel, "Number of Assets Failed to Process: %d.\n", FailedAssetsCount());
    AZ_Printf(AssetProcessor::ConsoleChannel, "Asset Processor Batch Processing Completed.\n");

    Destroy();

    return (startedSuccessfully && FailedAssetsCount() == 0);
}

void BatchApplicationManager::CheckForIdle()
{
    if ((m_rcController->IsIdle() && m_AssetProcessorManagerIdleState))
    {
        QuitRequested();
    }
}

#ifdef UNIT_TEST
bool BatchApplicationManager::RunUnitTests()
{
    if (!ApplicationManager::Activate())
    {
        return false;
    }
    AssetProcessor::PlatformConfiguration config;
    ConnectionManager connectionManager(&config);
    RegisterObjectForQuit(&connectionManager);
    UnitTestWorker unitTestWorker;
    int testResult = 1;
    QObject::connect(&unitTestWorker, &UnitTestWorker::UnitTestCompleted, [&](int result)
        {
            testResult = result;
            QuitRequested();
        });

    m_duringStartup = false;
    unitTestWorker.Process();

    qApp->exec();

    if (!testResult)
    {
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "All Unit Tests passed.\n");
    }
    else
    {
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "WARNING: Unit Tests Failed.\n");
    }
    return !testResult ? true : false;
}
#endif

bool BatchApplicationManager::InitAssetDatabase()
{
    AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler::BusConnect();
    m_assetDatabaseConnection = new AssetProcessor::AssetDatabaseConnection();
    return m_assetDatabaseConnection->OpenDatabase();
}

void BatchApplicationManager::ShutDownAssetDatabase()
{
    AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler::BusDisconnect();
    delete m_assetDatabaseConnection;
    m_assetDatabaseConnection = nullptr;
}

AzToolsFramework::AssetDatabase::AssetDatabaseConnection* BatchApplicationManager::GetAssetDatabaseConnection() const
{
    return m_assetDatabaseConnection;
}

// IMPLEMENTATION OF -------------- AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Listener
bool BatchApplicationManager::GetAssetDatabaseLocation(AZStd::string& location)
{
    QDir cacheRoot;
    if (!AssetUtilities::ComputeProjectCacheRoot(cacheRoot))
    {
        location = "assetdb.sqlite";
    }

    location = cacheRoot.absoluteFilePath("assetdb.sqlite").toUtf8().data();
    return true;
}

// ------------------------------------------------------------

bool BatchApplicationManager::Activate()
{
#if defined(AZ_PLATFORM_WINDOWS) && defined(BATCH_MODE)
    BatchApplicationManagerPrivate::g_appManager = this;
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)BatchApplicationManagerPrivate::CtrlHandlerRoutine, TRUE);
#endif //defined(AZ_PLATFORM_WINDOWS) && defined(BATCH_MODE)

    bool appInited = InitApplicationServer();
    if (!appInited)
    {
        return false;
    }

    // since we're not doing unit tests, we can go ahead and let bus queries know where the real DB is.
#if !defined(FORCE_PROXY_MODE)
    if (!InitAssetDatabase())
    {
        return false;
    }
#endif

    if (!ApplicationManager::Activate())
    {
        return false;
    }

    if (!InitPlatformConfiguration())
    {
        AZ_Error("AssetProcessor", false, "Failed to Initialize from AssetProcessorPlatformConfig.ini - check the log files in the logs/ subfolder for more information.");
        return false;
    }

    m_isCurrentlyLoadingGems = true;
    if (!ActivateModules())
    {
        m_isCurrentlyLoadingGems = false;
        return false;
    }
    
    m_isCurrentlyLoadingGems = false;
    PopulateApplicationDependencies();

    InitAssetProcessorManager();
    AssetBuilderSDK::InitializeSerializationContext();
#if !FORCE_PROXY_MODE
    InitAssetCatalog();
    InitFileMonitor();
    InitAssetScanner();
    InitRCController();
#endif

    InitConnectionManager();
    InitAssetRequestHandler();

    //We must register all objects that need to be notified if we are shutting down before we install the ctrlhandler

    // inserting in the front so that the application server is notified first 
    // and we stop listening for new incoming connections during shutdown
    RegisterObjectForQuit(m_applicationServer, true);
    
    RegisterObjectForQuit(m_connectionManager);
    RegisterObjectForQuit(m_assetProcessorManager);
#if !defined(FORCE_PROXY_MODE)
    RegisterObjectForQuit(m_rcController);
#endif

    connect(m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetProcessorManagerIdleState, this,
        [this](bool state)
    {
        if (state)
        {
            QMetaObject::invokeMethod(m_rcController, "SetDispatchPaused", Qt::QueuedConnection, Q_ARG(bool, false));
        }
    });


#if defined(BATCH_MODE)
    QObject::connect(m_rcController, &AssetProcessor::RCController::FileCompiled,
        m_assetProcessorManager, [this](AssetProcessor::JobEntry /*entry*/, AssetBuilderSDK::ProcessJobResponse /*response*/)
        {
            m_processedAssetCount++;
        });

    QObject::connect(m_rcController, &AssetProcessor::RCController::FileFailed,
        m_assetProcessorManager, [this](AssetProcessor::JobEntry entry)
        {
            m_failedAssetsCount++;

            using AssetJobLogRequest = AzToolsFramework::AssetSystem::AssetJobLogRequest;
            using AssetJobLogResponse = AzToolsFramework::AssetSystem::AssetJobLogResponse;
            if (m_failedAssetsCount < MAXIMUM_FAILURES_TO_REPORT) // if we're in the situation where many assets are failing we need to stop spamming after a few
            {
                AssetJobLogRequest request;
                AssetJobLogResponse response;
                request.m_jobRunKey = entry.m_jobRunKey;
                QMetaObject::invokeMethod(GetAssetProcessorManager(), "ProcessGetAssetJobLogRequest", Qt::DirectConnection,  Q_ARG(const AssetJobLogRequest&, request), Q_ARG(AssetJobLogResponse&, response));
                if (response.m_isSuccess)
                {
                    // write the log to console!
                    AzToolsFramework::Logging::LogLine::ParseLog(response.m_jobLog.c_str(), response.m_jobLog.size(),  
                        [](AzToolsFramework::Logging::LogLine& target)
                    {
                        if ((target.GetLogType() == AzToolsFramework::Logging::LogLine::TYPE_WARNING) || (target.GetLogType() == AzToolsFramework::Logging::LogLine::TYPE_ERROR))
                        {
                            AZStd::string logString = target.ToString();
                            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "JOB LOG: %s", logString.c_str());
                        }
                    });
                }

            }
            else if (m_failedAssetsCount == MAXIMUM_FAILURES_TO_REPORT)
            {
                // notify the user that we're done here, and will not be notifying any more.
                AZ_Printf(AssetProcessor::ConsoleChannel, "%s\n", QCoreApplication::translate("Batch Mode", "Too Many Compile Errors - not printing out full logs for remaining errors").toUtf8().constData());
            }
            
        });

    AZ_Printf(AssetProcessor::ConsoleChannel, QCoreApplication::translate("Batch Mode", "Starting to scan for changes...\n").toUtf8().constData());

    connect(m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetProcessorManagerIdleState,
        this, &BatchApplicationManager::OnAssetProcessorManagerIdleState);
    connect(m_rcController, &AssetProcessor::RCController::BecameIdle,
        [this]()
    {
        Q_EMIT CheckAssetProcessorManagerIdleState();
    });

    connect(this, &BatchApplicationManager::CheckAssetProcessorManagerIdleState, m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::CheckAssetProcessorIdleState);

    QObject::connect(m_assetScanner, &AssetProcessor::AssetScanner::AssetScanningStatusChanged,
        [this](AssetProcessor::AssetScanningStatus status)
        {
            if ((status == AssetProcessor::AssetScanningStatus::Completed) || (status == AssetProcessor::AssetScanningStatus::Stopped))
            {
                AZ_Printf(AssetProcessor::ConsoleChannel, QCoreApplication::translate("Batch Mode", "Checking changes\n").toUtf8().constData());
                CheckForIdle();
            }
        });

    m_assetScanner->StartScan();


#endif // BATCH_MODE

    // only after everyones had a chance to init messages, we start listening.
    if (m_applicationServer)
    {
        if (!m_applicationServer->startListening())
        {
            return false;
        }
    }
    return true;
}

bool BatchApplicationManager::PostActivate()
{
#if !FORCE_PROXY_MODE
    InitializeInternalBuilders();
#endif
    if (!InitializeExternalBuilders())
    {
        AZ_Error("AssetProcessor", false, "AssetProcessor is closing. Failed to initialize and load all the external builders. Please ensure that Builders_Temp directory is not read-only. Please see log for more information.\n");
        return false;
    }

    // 25 milliseconds is above the 'while loop' thing that QT does on windows (where small time ticks will spin loop instead of sleep)
    m_ticker = new AzToolsFramework::Ticker(nullptr, 25.0f); 
    m_ticker->Start();
    connect(m_ticker, &AzToolsFramework::Ticker::Tick, this, []()
    {
        AZ::SystemTickBus::Broadcast(&AZ::SystemTickEvents::OnSystemTick);
    });

    return true;
}

void BatchApplicationManager::CreateQtApplication()
{
    m_qApp = new QCoreApplication(m_argc, m_argv);
}

bool BatchApplicationManager::InitializeInternalBuilders()
{
    m_internalBuilder = AZStd::make_shared<AssetProcessor::InternalRecognizerBasedBuilder>();
    return m_internalBuilder->Initialize(*this->m_platformConfiguration);
}

bool BatchApplicationManager::InitializeExternalBuilders()
{
    AssetProcessor::AssetProcessorStatusEntry entry(AssetProcessor::AssetProcessorStatus::Initializing_Builders);
    Q_EMIT AssetProcessorStatusChanged(entry);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QString dirPath;
    QString fileName;
    AssetUtilities::ComputeApplicationInformation(dirPath, fileName);
    QDir directory(dirPath);
    QString builderDirPath = directory.filePath("Builders");
    QDir builderDir(builderDirPath);
    QDir tempBuilderDir = builderDir;
    QStringList filter;
#if defined(AZ_PLATFORM_WINDOWS)
    filter.append("*.dll");
#elif defined(AZ_PLATFORM_LINUX)
    filter.append("*.so");
#elif defined(AZ_PLATFORM_APPLE)
    filter.append("*.dylib");
#endif
    QString exePath;
    QVector<AssetProcessor::ExternalModuleAssetBuilderInfo*>  externalBuilderList;

    QStringList fileList = tempBuilderDir.entryList(filter, QDir::Files);
    for (QString& file : fileList)
    {
        QString filePath = tempBuilderDir.filePath(file);
        bool    externalBuilderLoaded = false;
        if (QLibrary::isLibrary(filePath))
        {
            AssetProcessor::ExternalModuleAssetBuilderInfo* externalAssetBuilderInfo = new AssetProcessor::ExternalModuleAssetBuilderInfo(filePath);
            if (!externalAssetBuilderInfo->Load())
            {
                AZ_Warning(AssetProcessor::DebugChannel, false, "AssetProcessor was not able to load the library: %s\n", filePath.toUtf8().data());
                delete externalAssetBuilderInfo;
                return false;
            }

            AZ_TracePrintf(AssetProcessor::DebugChannel, "Initializing and registering builder %s\n", externalAssetBuilderInfo->GetName().toUtf8().data());

            this->m_currentExternalAssetBuilder = externalAssetBuilderInfo;

            externalAssetBuilderInfo->Initialize();

            this->m_currentExternalAssetBuilder = nullptr;

            this->m_externalAssetBuilders.push_back(externalAssetBuilderInfo);
        }
    }

    return true;
}

void BatchApplicationManager::RegisterBuilderInformation(const AssetBuilderSDK::AssetBuilderDesc& builderDesc)
{
    // Create Job Function validation
    AZ_Error(AssetProcessor::ConsoleChannel,
        builderDesc.m_createJobFunction,
        "Create Job Function (m_createJobFunction) for %s builder is empty.\n",
        builderDesc.m_name.c_str());

    // Process Job Function validation
    AZ_Error(AssetProcessor::ConsoleChannel,
        builderDesc.m_processJobFunction,
        "Process Job Function (m_processJobFunction) for %s builder is empty.\n",
        builderDesc.m_name.c_str());

    // Bus ID validation
    AZ_Error(AssetProcessor::ConsoleChannel,
        !builderDesc.m_busId.IsNull(),
        "Bus ID for %s builder is empty.\n",
        builderDesc.m_name.c_str());

    // This is an external builder registering, we will want to track its builder desc since it can register multiple ones
    if (m_currentExternalAssetBuilder)
    {
        m_currentExternalAssetBuilder->RegisterBuilderDesc(builderDesc.m_busId);
    }

    if (m_builderDescMap.find(builderDesc.m_busId) != m_builderDescMap.end())
    {
        AZ_Warning(AssetProcessor::DebugChannel, false, "Uuid for %s builder is already registered.\n", builderDesc.m_name.c_str());
        return;
    }
    if (m_builderNameToId.find(builderDesc.m_name) != m_builderNameToId.end())
    {
        AZ_Warning(AssetProcessor::DebugChannel, false, "Duplicate builder detected.  A builder named '%s' is already registered.\n", builderDesc.m_name.c_str());
        return;
    }

    m_builderDescMap[builderDesc.m_busId] = builderDesc;
    m_builderNameToId[builderDesc.m_name] = builderDesc.m_busId;

    for (const AssetBuilderSDK::AssetBuilderPattern& pattern : builderDesc.m_patterns)
    {
        AssetUtilities::BuilderFilePatternMatcher patternMatcher(pattern, builderDesc.m_busId);
        m_matcherBuilderPatterns.push_back(patternMatcher);
    }
}

void BatchApplicationManager::RegisterComponentDescriptor(AZ::ComponentDescriptor* descriptor)
{
    ApplicationManager::RegisterComponentDescriptor(descriptor);
    if (m_currentExternalAssetBuilder)
    {
        m_currentExternalAssetBuilder->RegisterComponentDesc(descriptor);
    }
    else
    {
        AZ_Warning(AssetProcessor::DebugChannel, false, "Component description can only be registered during component activation.\n");
    }
}

void BatchApplicationManager::BuilderLog(const AZ::Uuid& builderId, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    BuilderLogV(builderId, message, args);
    va_end(args);
}

void BatchApplicationManager::BuilderLogV(const AZ::Uuid& builderId, const char* message, va_list list)
{
    AZStd::string builderName;

    if (m_builderDescMap.find(builderId) != m_builderDescMap.end())
    {
        const AssetBuilderSDK::AssetBuilderDesc& builderDesc = m_builderDescMap[builderId];
        char messageBuffer[1024];
        azvsnprintf(messageBuffer, 1024, message, list);
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Builder name : %s Message : %s.\n", builderDesc.m_name.c_str(), messageBuffer);
    }
    else
    {
        // asset processor does not know about this builder id
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "AssetProcessor does not know about the builder id: %s. \n", builderId.ToString<AZStd::string>().c_str());
    }
}

void BatchApplicationManager::UnRegisterComponentDescriptor(const AZ::ComponentDescriptor* componentDescriptor)
{
    AZ_Assert(componentDescriptor, "componentDescriptor cannot be null");
    ApplicationManager::UnRegisterComponentDescriptor(componentDescriptor);
}

void BatchApplicationManager::UnRegisterBuilderDescriptor(const AZ::Uuid& builderId)
{
    if (m_builderDescMap.find(builderId) == m_builderDescMap.end())
    {
        AZ_Warning(AssetProcessor::DebugChannel, false, "Cannot unregister builder descriptor for Uuid %s, not currently registered.\n", builderId.ToString<AZStd::string>().c_str());
        return;
    }

    // Remove from the map
    AssetBuilderSDK::AssetBuilderDesc& descToUnregister = m_builderDescMap[builderId];
    AZStd::string descNameToUnregister = descToUnregister.m_name;
    descToUnregister.m_createJobFunction.clear();
    descToUnregister.m_processJobFunction.clear();
    m_builderDescMap.erase(builderId);
    m_builderNameToId.erase(descNameToUnregister);

    // Remove the matcher build pattern
    for (auto remover = this->m_matcherBuilderPatterns.begin();
         remover != this->m_matcherBuilderPatterns.end();
         remover++)
    {
        if (remover->GetBuilderDescID() == builderId)
        {
            auto deleteIter = remover;
            remover++;
            this->m_matcherBuilderPatterns.erase(deleteIter);
        }
    }
}

void BatchApplicationManager::GetMatchingBuildersInfo(const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList)
{
    AZStd::set<AZ::Uuid>  uniqueBuilderDescIDs;

    for (AssetUtilities::BuilderFilePatternMatcher& matcherPair : m_matcherBuilderPatterns)
    {
        if (uniqueBuilderDescIDs.find(matcherPair.GetBuilderDescID()) != uniqueBuilderDescIDs.end())
        {
            continue;
        }
        if (matcherPair.MatchesPath(assetPath))
        {
            const AssetBuilderSDK::AssetBuilderDesc& builderDesc = m_builderDescMap[matcherPair.GetBuilderDescID()];
            uniqueBuilderDescIDs.insert(matcherPair.GetBuilderDescID());
            builderInfoList.push_back(builderDesc);
        }
    }
}

bool BatchApplicationManager::OnError(const char* window, const char* message)
{
    // We don't need to print the message to stdout, the trace system will already do that

    return true;
}


void BatchApplicationManager::OnAssetProcessorManagerIdleState(bool isIdle)
{
    if (isIdle)
    {
        if (!m_AssetProcessorManagerIdleState)
        {
            // We want to again ask the APM for the idle state just incase it goes from idle to non idle in between
            Q_EMIT CheckAssetProcessorManagerIdleState();
        }
        else
        {
            CheckForIdle();
            return;
        }
    }
    m_AssetProcessorManagerIdleState = isIdle;
}

void BatchApplicationManager::OnActiveJobsCountChanged(unsigned int count)
{
    AssetProcessor::AssetProcessorStatusEntry entry(AssetProcessor::AssetProcessorStatus::Processing_Jobs, count);
    Q_EMIT AssetProcessorStatusChanged(entry);
}

#include <native/utilities/BatchApplicationManager.moc>
