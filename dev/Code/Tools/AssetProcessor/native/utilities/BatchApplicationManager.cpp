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

#include <AzCore/std/containers/set.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/PlatformIncl.h>

#include <native/utilities/assetUtils.h>
#include <native/AssetManager/assetProcessorManager.h>
#include <native/utilities/PlatformConfiguration.h>
#include <native/FileWatcher/FileWatcherAPI.h>
#include <native/AssetManager/assetScanFolderInfo.h>
#include <native/resourcecompiler/rccontroller.h>
#include <native/AssetManager/assetScanner.h>
#include <native/utilities/ApplicationServer.h>
#include <native/connection/connectionManager.h>
#include <native/utilities/ByteArrayStream.h>
#include <native/AssetManager/AssetRequestHandler.h>
#include <native/FileProcessor/FileProcessor.h>
#include <native/utilities/CommunicatorTracePrinter.h>
#include <native/FileProcessor/FileProcessor.h>
#include <native/AssetDatabase/AssetDatabase.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/Logging/LogLine.h>
#include <AzToolsFramework/Application/Ticker.h>

#include <QTextStream>
#include <QCoreApplication>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzToolsFramework/Process/ProcessWatcher.h>
#include <AzToolsFramework/Process/internal/ProcessCommon_Win.h>
#include <AzCore/IO/Device.h>
#include <QElapsedTimer>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzCore/std/parallel/binary_semaphore.h>

#include <AzCore/std/sort.h>

#include <QStorageInfo>
#include <native/utilities/AssetServerHandler.h>

// in batch mode, we are going to show the log files of up to N failures.
// in order to not spam the logs, we limit this - its possible that something fundamental is broken and EVERY asset is failing
// and we don't want to thus write gigabytes of logs out.
const int s_MaximumFailuresToReport = 10;

//! Amount of time to wait between checking the status of the AssetBuilder process
static const int s_MaximumSleepTimeMS = 10;

//! CreateJobs will wait up to 2 minutes before timing out
//! This shouldn't need to be so high but very large slices can take a while to process currently
//! This should be reduced down to something more reasonable after slice jobs are sped up
static const int s_MaximumCreateJobsTimeSeconds = 60 * 2;

//! ProcessJobs will wait up to 1 hour before timing out
static const int s_MaximumProcessJobsTimeSeconds = 60 * 60;

//! Reserve extra disk space when doing disk space checks to leave a little room for logging, database operations, etc
static const qint64 s_ReservedDiskSpaceInBytes = 256 * 1024;

//! Maximum number of temp folders allowed
static const int s_MaximumTempFolders = 10000;

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

BatchApplicationManager::BatchApplicationManager(int* argc, char*** argv, QObject* parent)
    : ApplicationManager(argc, argv, parent)
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
    QObject::connect(this, &BatchApplicationManager::OnBuildersRegistered, m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::OnBuildersRegistered, Qt::QueuedConnection);

    QStringList args = QCoreApplication::arguments();
    for (QString arg : args)
    {
        if (arg.startsWith("--zeroAnalysisMode", Qt::CaseInsensitive))
        {
            m_assetProcessorManager->SetEnableModtimeSkippingFeature(true);
        }
    }
}

void BatchApplicationManager::Rescan()
{
    m_assetProcessorManager->SetEnableModtimeSkippingFeature(false);
    GetAssetScanner()->StartScan();
}

void BatchApplicationManager::InitAssetCatalog()
{
    using namespace AssetProcessor;
    ThreadController<AssetCatalog>* assetCatalogHelper = new ThreadController<AssetCatalog>();

    addRunningThread(assetCatalogHelper);
    m_assetCatalog = assetCatalogHelper->initialize([this, &assetCatalogHelper]()
            {
                AssetProcessor::AssetCatalog* catalog = new AssetCatalog(assetCatalogHelper, m_platformConfiguration);

                connect(m_assetProcessorManager, &AssetProcessorManager::AssetMessage, catalog, &AssetCatalog::OnAssetMessage);
                connect(m_assetProcessorManager, &AssetProcessorManager::SourceQueued, catalog, &AssetCatalog::OnSourceQueued);
                connect(m_assetProcessorManager, &AssetProcessorManager::SourceFinished, catalog, &AssetCatalog::OnSourceFinished);

                return catalog;
            });

    // schedule the asset catalog to build its registry in its own thread:
    QMetaObject::invokeMethod(m_assetCatalog, "BuildRegistry", Qt::QueuedConnection);
}

void BatchApplicationManager::InitRCController()
{
    m_rcController = new AssetProcessor::RCController(m_platformConfiguration->GetMinJobs(), m_platformConfiguration->GetMaxJobs());

    QObject::connect(m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetToProcess, m_rcController, &AssetProcessor::RCController::JobSubmitted);
    QObject::connect(m_rcController, &AssetProcessor::RCController::FileCompiled, m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetProcessed, Qt::UniqueConnection);
    QObject::connect(m_rcController, &AssetProcessor::RCController::FileFailed, m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetFailed);
    QObject::connect(m_rcController, &AssetProcessor::RCController::FileCancelled, m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetCancelled);
    QObject::connect(m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::EscalateJobs, m_rcController, &AssetProcessor::RCController::EscalateJobs);
    QObject::connect(m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::SourceDeleted, m_rcController, &AssetProcessor::RCController::RemoveJobsBySource);
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
    using namespace AssetProcessor;
    m_assetScanner = new AssetScanner(m_platformConfiguration);

    // asset processor manager
    QObject::connect(m_assetScanner, &AssetScanner::AssetScanningStatusChanged, m_assetProcessorManager, &AssetProcessorManager::OnAssetScannerStatusChange);
    QObject::connect(m_assetScanner, &AssetScanner::FilesFound,                 m_assetProcessorManager, &AssetProcessorManager::AssessFilesFromScanner);
    
    // file table
    QObject::connect(m_assetScanner, &AssetScanner::AssetScanningStatusChanged, m_fileProcessor.get(), &FileProcessor::OnAssetScannerStatusChange);
    QObject::connect(m_assetScanner, &AssetScanner::FilesFound,                 m_fileProcessor.get(), &FileProcessor::AssessFilesFromScanner);
    QObject::connect(m_assetScanner, &AssetScanner::FoldersFound,               m_fileProcessor.get(), &FileProcessor::AssessFoldersFromScanner);
    
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
    QString rootConfigFile = GetSystemRoot().absoluteFilePath("AssetProcessorPlatformConfig.ini");

    QDir assetRoot;
    AssetUtilities::ComputeAssetRoot(assetRoot);
    // the game project could be in a different location to the engine.
    QString gameConfigFile = assetRoot.absoluteFilePath(GetGameName() + "/AssetProcessorGamePlatformConfig.ini");

    return m_platformConfiguration->InitializeFromConfigFiles(rootConfigFile, gameConfigFile);
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

        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileAdded,
            m_fileProcessor.get(), &AssetProcessor::FileProcessor::AssessAddedFile);
        QObject::connect(newFolderWatch, &FolderWatchCallbackEx::fileRemoved,
            m_fileProcessor.get(), &AssetProcessor::FileProcessor::AssessDeletedFile);

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
    using namespace AzFramework::AssetSystem;
    using namespace AzToolsFramework::AssetSystem;

    AssetProcessor::PlatformConfiguration* platformConfiguration = GetPlatformConfiguration();
    m_connectionManager = new ConnectionManager(platformConfiguration);

    QObject* connectionAndChangeMessagesThreadContext = this;

    // AssetProcessor Manager related stuff
    auto forwardMessageFunction = [](QString platform, AzFramework::AssetSystem::AssetNotificationMessage message)
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

    result = QObject::connect(GetRCController(), &AssetProcessor::RCController::JobStarted, this,
            [](QString inputFile, QString platform)
            {
                QString msg = QCoreApplication::translate("Asset Processor", "Processing %1 (%2)...\n", "%1 is the name of the file, and %2 is the platform to process it for").arg(inputFile, platform);
                AZ_Printf(AssetProcessor::ConsoleChannel, "%s", msg.toUtf8().constData());
                AssetNotificationMessage message(inputFile.toUtf8().constData(), AssetNotificationMessage::JobStarted, AZ::Data::s_invalidAssetType);
                EBUS_EVENT(AssetProcessor::ConnectionBus, SendPerPlatform, 0, message, platform);
            }
            );
    AZ_Assert(result, "Failed to connect to RCController signal");

    result = QObject::connect(GetRCController(), &AssetProcessor::RCController::FileCompiled, this,
            [](AssetProcessor::JobEntry entry, AssetBuilderSDK::ProcessJobResponse /*response*/)
            {
                AssetNotificationMessage message(entry.m_pathRelativeToWatchFolder.toUtf8().constData(), AssetNotificationMessage::JobCompleted, AZ::Data::s_invalidAssetType);
                EBUS_EVENT(AssetProcessor::ConnectionBus, SendPerPlatform, 0, message, QString::fromUtf8(entry.m_platformInfo.m_identifier.c_str()));
            }
            );
    AZ_Assert(result, "Failed to connect to RCController signal");

    result = QObject::connect(GetRCController(), &AssetProcessor::RCController::FileFailed, this,
            [](AssetProcessor::JobEntry entry)
            {
                AssetNotificationMessage message(entry.m_pathRelativeToWatchFolder.toUtf8().constData(), AssetNotificationMessage::JobFailed, AZ::Data::s_invalidAssetType);
                EBUS_EVENT(AssetProcessor::ConnectionBus, SendPerPlatform, 0, message, QString::fromUtf8(entry.m_platformInfo.m_identifier.c_str()));
            }
            );
    AZ_Assert(result, "Failed to connect to RCController signal");

    result = QObject::connect(GetRCController(), &AssetProcessor::RCController::JobsInQueuePerPlatform, this,
            [](QString platform, int count)
            {
                AssetNotificationMessage message(QByteArray::number(count).constData(), AssetNotificationMessage::JobCount, AZ::Data::s_invalidAssetType);
                EBUS_EVENT(AssetProcessor::ConnectionBus, SendPerPlatform, 0, message, platform);
            }
            );
    AZ_Assert(result, "Failed to connect to RCController signal");

    m_connectionManager->RegisterService(RequestPing::MessageType(),
        AZStd::bind([](unsigned int connId, unsigned int /*type*/, unsigned int serial, QByteArray /*payload*/)
            {
                ResponsePing responsePing;
                EBUS_EVENT_ID(connId, AssetProcessor::ConnectionBus, SendResponse, serial, responsePing);
            }, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, AZStd::placeholders::_4)
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
                EBUS_EVENT_ID(connId, AssetProcessor::ConnectionBus, SendResponse, serial, responseAssetProcessorMessage);
            }
        };
    // connect the network messages to the Request handler:
    m_connectionManager->RegisterService(RequestAssetProcessorStatus::MessageType(), GetState);

    // ability to see if an asset platform is enabled or not
    using AzToolsFramework::AssetSystem::AssetProcessorPlatformStatusRequest;
    m_connectionManager->RegisterService(AssetProcessorPlatformStatusRequest::MessageType(),
        [](unsigned int connId, unsigned int, unsigned int serial, QByteArray payload, QString)
        {
            AssetProcessorPlatformStatusResponse responseMessage;

            AssetProcessorPlatformStatusRequest requestMessage;
            if (AssetProcessor::UnpackMessage(payload, requestMessage))
            {
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(responseMessage.m_isPlatformEnabled, 
                        &AzToolsFramework::AssetSystemRequestBus::Events::IsAssetPlatformEnabled, requestMessage.m_platform.c_str());
            }

            AssetProcessor::ConnectionBus::Event(connId, 
                &AssetProcessor::ConnectionBus::Events::SendResponse, serial, responseMessage);
        });


    // check the total number of assets remaining for a specified platform
    using AzToolsFramework::AssetSystem::AssetProcessorPendingPlatformAssetsRequest;
    m_connectionManager->RegisterService(AssetProcessorPendingPlatformAssetsRequest::MessageType(),
        [this](unsigned int connId, unsigned int, unsigned int serial, QByteArray payload, QString)
        {
            AssetProcessorPendingPlatformAssetsResponse responseMessage;

            AssetProcessorPendingPlatformAssetsRequest requestMessage;
            if (AssetProcessor::UnpackMessage(payload, requestMessage))
            {
                const char* platformIdentifier = requestMessage.m_platform.c_str();
                responseMessage.m_numberOfPendingJobs = 
                    GetRCController()->NumberOfPendingJobsPerPlatform(requestMessage.m_platform.c_str());
            }

            AssetProcessor::ConnectionBus::Event(connId, 
                &AssetProcessor::ConnectionBus::Events::SendResponse, serial, responseMessage);
        });
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
    using namespace AzFramework::AssetSystem;
    using namespace AzToolsFramework::AssetSystem;
    using namespace AzFramework::AssetSystem;
    using namespace AssetProcessor;

    m_assetRequestHandler = new AssetRequestHandler();

    m_assetRequestHandler->RegisterRequestHandler(AssetJobsInfoRequest::MessageType(), GetAssetProcessorManager());
    m_assetRequestHandler->RegisterRequestHandler(GetAbsoluteAssetDatabaseLocationRequest::MessageType(), GetAssetProcessorManager());
    m_assetRequestHandler->RegisterRequestHandler(AssetJobLogRequest::MessageType(), GetAssetProcessorManager());
    m_assetRequestHandler->RegisterRequestHandler(SaveAssetCatalogRequest::MessageType(), GetAssetCatalog());

    auto serviceRedirectHandler = [&](unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload, QString platform)
        {
            QMetaObject::invokeMethod(m_assetRequestHandler, "OnNewIncomingRequest", Qt::QueuedConnection, Q_ARG(unsigned int, connId), Q_ARG(unsigned int, serial), Q_ARG(QByteArray, payload), Q_ARG(QString, platform));
        };

    m_connectionManager->RegisterService(GetRelativeProductPathFromFullSourceOrProductPathRequest::MessageType(), serviceRedirectHandler);
    m_connectionManager->RegisterService(GetFullSourcePathFromRelativeProductPathRequest::MessageType(), serviceRedirectHandler);
    m_connectionManager->RegisterService(AssetJobsInfoRequest::MessageType(), serviceRedirectHandler);
    m_connectionManager->RegisterService(AssetJobLogRequest::MessageType(), serviceRedirectHandler);
    m_connectionManager->RegisterService(GetAbsoluteAssetDatabaseLocationRequest::MessageType(), serviceRedirectHandler);
    m_connectionManager->RegisterService(SourceAssetInfoRequest::MessageType(), serviceRedirectHandler);
    m_connectionManager->RegisterService(GetScanFoldersRequest::MessageType(), serviceRedirectHandler);
    m_connectionManager->RegisterService(GetAssetSafeFoldersRequest::MessageType(), serviceRedirectHandler);
    m_connectionManager->RegisterService(RegisterSourceAssetRequest::MessageType(), serviceRedirectHandler);
    m_connectionManager->RegisterService(UnregisterSourceAssetRequest::MessageType(), serviceRedirectHandler);
    m_connectionManager->RegisterService(SaveAssetCatalogRequest::MessageType(), serviceRedirectHandler);
    m_connectionManager->RegisterService(AssetInfoRequest::MessageType(), serviceRedirectHandler);
    // connect the "Does asset exist?" loop to each other:
    QObject::connect(m_assetRequestHandler, &AssetRequestHandler::RequestAssetExists, GetAssetProcessorManager(), &AssetProcessorManager::OnRequestAssetExists);
    QObject::connect(GetAssetProcessorManager(), &AssetProcessorManager::SendAssetExistsResponse, m_assetRequestHandler, &AssetRequestHandler::OnRequestAssetExistsResponse);

    // connect the network messages to the Request handler:
    m_connectionManager->RegisterService(RequestAssetStatus::MessageType(),
        AZStd::bind([&](unsigned int connId, unsigned int serial, QByteArray payload, QString platform)
            {
                QMetaObject::invokeMethod(m_assetRequestHandler, "OnNewIncomingRequest", Qt::QueuedConnection, Q_ARG(unsigned int, connId), Q_ARG(unsigned int, serial), Q_ARG(QByteArray, payload), Q_ARG(QString, platform));
            },
            AZStd::placeholders::_1, AZStd::placeholders::_3, AZStd::placeholders::_4, AZStd::placeholders::_5)
        );

    QObject::connect(GetAssetProcessorManager(), &AssetProcessorManager::FenceFileDetected, m_assetRequestHandler, &AssetRequestHandler::OnFenceFileDetected);
    // connect the Asset Request Handler to RC:
    QObject::connect(m_assetRequestHandler, &AssetRequestHandler::RequestCompileGroup, GetRCController(), &RCController::OnRequestCompileGroup);
    QObject::connect(GetRCController(), &RCController::CompileGroupCreated, m_assetRequestHandler, &AssetRequestHandler::OnCompileGroupCreated);
    QObject::connect(GetRCController(), &RCController::CompileGroupFinished, m_assetRequestHandler, &AssetRequestHandler::OnCompileGroupFinished);

    QObject::connect(GetAssetProcessorManager(), &AssetProcessor::AssetProcessorManager::NumRemainingJobsChanged, this, [this](int newNum)
        {
            if (!m_assetProcessorManagerIsReady)
            {
                if (m_remainingAPMJobs == newNum)
                {
                    return;
                }

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
    qRegisterMetaType<AzToolsFramework::AssetSystem::JobInfo>("AzToolsFramework::AssetSystem::JobInfo");

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

    qRegisterMetaType<AssetProcessor::AssetCatalogStatus>("AssetCatalogStatus");
    qRegisterMetaType<AssetProcessor::AssetCatalogStatus>("AssetProcessor::AssetCatalogStatus");

    qRegisterMetaType<QSet<QString> >("QSet<QString>");
    qRegisterMetaType<QSet<AssetProcessor::AssetFileInfo>>("QSet<AssetFileInfo>");

    AssetBuilderSDK::AssetBuilderBus::Handler::BusConnect();
    AssetProcessor::AssetBuilderRegistrationBus::Handler::BusConnect();
    AssetProcessor::AssetBuilderInfoBus::Handler::BusConnect();
    AZ::Debug::TraceMessageBus::Handler::BusConnect();
    AssetProcessor::DiskSpaceInfoBus::Handler::BusConnect();

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

    ShutdownBuilderManager();
    ShutDownFileProcessor();

    DestroyConnectionManager();
    DestroyAssetServerHandler();
    DestroyRCController();
    DestroyAssetScanner();
    DestroyFileMonitor();
    ShutDownAssetDatabase();
    DestroyPlatformConfiguration();
    DestroyApplicationServer();
}

bool BatchApplicationManager::Run()
{
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
        QuitRequested();
        startedSuccessfully = false;
    }

    AZ_Printf(AssetProcessor::ConsoleChannel, "Asset Processor Batch Processing Started.\n");
    AZ_Printf(AssetProcessor::ConsoleChannel, "-----------------------------------------\n");
    QElapsedTimer allAssetsProcessingTimer;
    allAssetsProcessingTimer.start();
    m_duringStartup = false;
    qApp->exec();

    AZ_Printf(AssetProcessor::ConsoleChannel, "-----------------------------------------\n");
    AZ_Printf(AssetProcessor::ConsoleChannel, "Asset Processor Batch Processing complete\n");
    AZ_Printf(AssetProcessor::ConsoleChannel, "Number of Assets Successfully Processed: %d.\n", ProcessedAssetCount());
    AZ_Printf(AssetProcessor::ConsoleChannel, "Number of Assets Failed to Process: %d.\n", FailedAssetsCount());
    AZ_Printf(AssetProcessor::ConsoleChannel, "Total Assets Processing Time: %fs\n", allAssetsProcessingTimer.elapsed() / 1000.0f);
    AZ_Printf(AssetProcessor::ConsoleChannel, "Asset Processor Batch Processing Completed.\n");

    RemoveOldTempFolders();
    Destroy();

    return (startedSuccessfully && FailedAssetsCount() == 0);
}

void BatchApplicationManager::CheckForIdle()
{
    if (InitiatedShutdown())
    {
        return;
    }

#if defined(BATCH_MODE)
    if (m_connectionsToRemoveOnShutdown.empty())
    {
        // we've already entered this state once.  Ignore repeats.  this can happen if another sender of events
        // rapidly flicks between idle/not idle and sends many "I'm done!" messages which are all queued up.
        return;
    }
#endif

    if ((m_rcController->IsIdle() && m_AssetProcessorManagerIdleState))
    {
#if defined(BATCH_MODE)
        // in batch mode, when we become idle, we save the registry and then we quit.
        AZ_Printf(AssetProcessor::ConsoleChannel, "No assets remain in the build queue.  Saving the catalog, and then shutting down.\n");
        // stop accepting any further idle messages, as we will shut down - don't want this function to repeat!
        for (const QMetaObject::Connection& connection : m_connectionsToRemoveOnShutdown)
        {
            QObject::disconnect(connection);
        }
        m_connectionsToRemoveOnShutdown.clear();

        // Checking the status of the asset catalog here using qt's signal slot mechanism
        // to ensure that we do not have any pending events in the event loop that can make the catalog dirty again
        QObject::connect(m_assetCatalog, &AssetProcessor::AssetCatalog::AsyncAssetCatalogStatusResponse, this, [&](AssetProcessor::AssetCatalogStatus status)
            {
                if (status == AssetProcessor::AssetCatalogStatus::RequiresSaving)
                {
                    AssetProcessor::AssetRegistryRequestBus::Broadcast(&AssetProcessor::AssetRegistryRequests::SaveRegistry);
                }

                QuitRequested();
            }, Qt::UniqueConnection);

        QMetaObject::invokeMethod(m_assetCatalog, "AsyncAssetCatalogStatusRequest", Qt::QueuedConnection);
#else
        // in GUI Mode, we save the registry when we become idle, but we stay running.
        AssetProcessor::AssetRegistryRequestBus::Broadcast(&AssetProcessor::AssetRegistryRequests::SaveRegistry);
#endif
    }
}

void BatchApplicationManager::InitBuilderManager()
{
    AZ_Assert(m_connectionManager != nullptr, "ConnectionManager must be started before the builder manager");
    m_builderManager = new AssetProcessor::BuilderManager(m_connectionManager);

    QObject::connect(m_connectionManager, &ConnectionManager::ConnectionDisconnected, this, [this](unsigned int connId)
        {
            m_builderManager->ConnectionLost(connId);
        });
}

void BatchApplicationManager::ShutdownBuilderManager()
{
    if (m_builderManager)
    {
        delete m_builderManager;
        m_builderManager = nullptr;
    }
}

bool BatchApplicationManager::InitAssetDatabase()
{
    AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler::BusConnect();

    // create or upgrade the asset database here, so that it is already good for the rest of the application and the rest
    // of the application does not have to worry about a failure to upgrade or create it.
    AssetProcessor::AssetDatabaseConnection database;
    if (!database.OpenDatabase())
    {
        return false;
    }

    database.CloseDatabase();

    return true;
}

void BatchApplicationManager::ShutDownAssetDatabase()
{
    AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler::BusDisconnect();
}

void BatchApplicationManager::InitFileProcessor() 
{
    AssetProcessor::ThreadController<AssetProcessor::FileProcessor>* fileProcessorHelper = new AssetProcessor::ThreadController<AssetProcessor::FileProcessor>();

    addRunningThread(fileProcessorHelper);
    m_fileProcessor.reset(fileProcessorHelper->initialize([this, &fileProcessorHelper]()
    {
        return new AssetProcessor::FileProcessor(m_platformConfiguration);
    }));
}

void BatchApplicationManager::ShutDownFileProcessor()
{
    m_fileProcessor.reset();
}

void BatchApplicationManager::InitAssetServerHandler()
{
    m_assetServerHandler = new AssetProcessor::AssetServerHandler();
    // This will cache whether AP is running in server mode or not.
    // It is also important to invoke it here because incase the asset server address is invalid, the error message should get captured in the AP log.
    AssetUtilities::InServerMode();
}

void BatchApplicationManager::DestroyAssetServerHandler()
{
    delete m_assetServerHandler;
    m_assetServerHandler = nullptr;
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

    QDir projectCache;
    if (!AssetUtilities::ComputeProjectCacheRoot(projectCache))
    {
        return false;
    }

    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "AssetProcessor will process assets from gameproject %s.\n", AssetUtilities::ComputeGameName().toUtf8().data());

    // Shutdown if the disk has less than 128MB of free space
    if (!CheckSufficientDiskSpace(projectCache.absolutePath(), 128 * 1024 * 1024, true))
    {
        return false;
    }

    bool appInited = InitApplicationServer();
    if (!appInited)
    {
        return false;
    }

    if (!InitAssetDatabase())
    {
        return false;
    }

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
    
    InitFileProcessor();

    InitAssetCatalog();
    InitFileMonitor();
    InitAssetScanner();
    InitAssetServerHandler();
    InitRCController();

    InitConnectionManager();
    InitAssetRequestHandler();

    InitBuilderManager();

    //We must register all objects that need to be notified if we are shutting down before we install the ctrlhandler

    // inserting in the front so that the application server is notified first
    // and we stop listening for new incoming connections during shutdown
    RegisterObjectForQuit(m_applicationServer, true);
    RegisterObjectForQuit(m_fileProcessor.get());
    RegisterObjectForQuit(m_connectionManager);
    RegisterObjectForQuit(m_assetProcessorManager);
    RegisterObjectForQuit(m_rcController);

    m_connectionsToRemoveOnShutdown << QObject::connect(
        m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetProcessorManagerIdleState, 
        this, [this](bool state)
        {
            if (state)
            {
                QMetaObject::invokeMethod(m_rcController, "SetDispatchPaused", Qt::QueuedConnection, Q_ARG(bool, false));
            }
        });

    m_connectionsToRemoveOnShutdown << QObject::connect(
        m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::AssetProcessorManagerIdleState,
        this, &BatchApplicationManager::OnAssetProcessorManagerIdleState);

    m_connectionsToRemoveOnShutdown << QObject::connect(
        m_rcController, &AssetProcessor::RCController::BecameIdle,
        this, [this]()
        {
            Q_EMIT CheckAssetProcessorManagerIdleState();
        });

    m_connectionsToRemoveOnShutdown << QObject::connect(
        this, &BatchApplicationManager::CheckAssetProcessorManagerIdleState, 
        m_assetProcessorManager, &AssetProcessor::AssetProcessorManager::CheckAssetProcessorIdleState);


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
            if (m_failedAssetsCount < s_MaximumFailuresToReport) // if we're in the situation where many assets are failing we need to stop spamming after a few
            {
                AssetJobLogRequest request;
                AssetJobLogResponse response;
                request.m_jobRunKey = entry.m_jobRunKey;
                QMetaObject::invokeMethod(GetAssetProcessorManager(), "ProcessGetAssetJobLogRequest", Qt::DirectConnection,  Q_ARG(const AssetJobLogRequest&, request), Q_ARG(AssetJobLogResponse &, response));
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
            else if (m_failedAssetsCount == s_MaximumFailuresToReport)
            {
                // notify the user that we're done here, and will not be notifying any more.
                AZ_Printf(AssetProcessor::ConsoleChannel, "%s\n", QCoreApplication::translate("Batch Mode", "Too Many Compile Errors - not printing out full logs for remaining errors").toUtf8().constData());
            }
        });

    m_connectionsToRemoveOnShutdown << QObject::connect(m_assetScanner, &AssetProcessor::AssetScanner::AssetScanningStatusChanged, this, 
        [this](AssetProcessor::AssetScanningStatus status)
        {
            if ((status == AssetProcessor::AssetScanningStatus::Completed) || (status == AssetProcessor::AssetScanningStatus::Stopped))
            {
                AZ_Printf(AssetProcessor::ConsoleChannel, QCoreApplication::translate("Batch Mode", "Analyzing scanned files for changes...\n").toUtf8().constData());
                CheckForIdle();
            }
        });

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
    m_connectionManager->LoadConnections();

    InitializeInternalBuilders();
    if (!InitializeExternalBuilders())
    {
        AZ_Error("AssetProcessor", false, "AssetProcessor is closing. Failed to initialize and load all the external builders. Please ensure that Builders_Temp directory is not read-only. Please see log for more information.\n");
        return false;
    }

    Q_EMIT OnBuildersRegistered();

    // 25 milliseconds is above the 'while loop' thing that QT does on windows (where small time ticks will spin loop instead of sleep)
    m_ticker = new AzToolsFramework::Ticker(nullptr, 25.0f);
    m_ticker->Start();
    connect(m_ticker, &AzToolsFramework::Ticker::Tick, this, []()
        {
            AZ::SystemTickBus::ExecuteQueuedEvents();
            AZ::SystemTickBus::Broadcast(&AZ::SystemTickEvents::OnSystemTick);
        });

    // now that everything is up and running, we start scanning and watching.  Before this, we don't want file events to start percolating through the 
    // asset system.

#if !defined(BATCH_MODE)
    // in batch mode, we don't watch for changes, since its just supposed to do with whatever files are present.
    m_fileWatcher.StartWatching();
#endif
    GetAssetScanner()->StartScan();

    return true;
}

void BatchApplicationManager::CreateQtApplication()
{
    m_qApp = new QCoreApplication(*m_frameworkApp.GetArgC(), *m_frameworkApp.GetArgV());
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


    // Get the list of external build modules (full paths)
    QStringList fileList;
    GetExternalBuilderFileList(fileList);

    for (const QString& filePath : fileList)
    {
        if (QLibrary::isLibrary(filePath))
        {
            AssetProcessor::ExternalModuleAssetBuilderInfo* externalAssetBuilderInfo = new AssetProcessor::ExternalModuleAssetBuilderInfo(filePath);
            if (!externalAssetBuilderInfo->Load())
            {
                AZ_Warning(AssetProcessor::DebugChannel, false, "AssetProcessor was not able to load the library: %s\n", filePath.toUtf8().data());
                delete externalAssetBuilderInfo;
                return false;
            }

            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Initializing and registering builder %s\n", externalAssetBuilderInfo->GetName().toUtf8().data());

            m_currentExternalAssetBuilder = externalAssetBuilderInfo;

            externalAssetBuilderInfo->Initialize();

            m_currentExternalAssetBuilder = nullptr;

            m_externalAssetBuilders.push_back(externalAssetBuilderInfo);
        }
    }

    // Also init external builders which may be inside of Gems
    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::CreateAndAddEntityFromComponentTags,
        AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }), "AssetBuilders Entity");

    return true;
}

bool BatchApplicationManager::WaitForBuilderExit(AzToolsFramework::ProcessWatcher* processWatcher, AssetBuilderSDK::JobCancelListener* jobCancelListener, AZ::u32 processTimeoutLimitInSeconds)
{
    AZ::u32 exitCode = 0;
    bool finishedOK = false;
    QElapsedTimer ticker;
    CommunicatorTracePrinter tracer(processWatcher->GetCommunicator(), "AssetBuilder");

    ticker.start();

    while (!finishedOK)
    {
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(s_MaximumSleepTimeMS));

        tracer.Pump();

        if (ticker.elapsed() > processTimeoutLimitInSeconds * 1000 || (jobCancelListener && jobCancelListener->IsCancelled()))
        {
            break;
        }

        if (!processWatcher->IsProcessRunning(&exitCode))
        {
            finishedOK = true; // we either cant wait for it, or it finished.
            break;
        }
    }

    tracer.Pump(); // empty whats left if possible.

    if (processWatcher->IsProcessRunning(&exitCode))
    {
        processWatcher->TerminateProcess(1);
    }

    if (exitCode != 0)
    {
        AZ_Error(AssetProcessor::ConsoleChannel, false, "AssetBuilder exited with error code %d", exitCode);
        return false;
    }
    else if (jobCancelListener && jobCancelListener->IsCancelled())
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "AssetBuilder was terminated. There was a request to cancel the job.\n");
        return false;
    }
    else if (!finishedOK)
    {
        AZ_Error(AssetProcessor::ConsoleChannel, false, "AssetBuilder failed to terminate within %d seconds", processTimeoutLimitInSeconds);
        return false;
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
    AZStd::string builderFilePath;
    if (m_currentExternalAssetBuilder)
    {
        m_currentExternalAssetBuilder->RegisterBuilderDesc(builderDesc.m_busId);
        builderFilePath = m_currentExternalAssetBuilder->GetModuleFullPath().toUtf8().data();
    }

    AssetBuilderSDK::AssetBuilderDesc modifiedBuilderDesc = builderDesc;
    if (builderDesc.IsExternalBuilder())
    {
        // We're going to override the createJob function so we can run it externally in AssetBuilder, rather than having it run inside the AP
        modifiedBuilderDesc.m_createJobFunction = [builderFilePath](const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
            {
                AssetProcessor::BuilderRef builderRef;
                AssetProcessor::BuilderManagerBus::BroadcastResult(builderRef, &AssetProcessor::BuilderManagerBusTraits::GetBuilder);

                if (builderRef)
                {
                    builderRef->RunJob<AssetBuilderSDK::CreateJobsNetRequest, AssetBuilderSDK::CreateJobsNetResponse>(request, response, s_MaximumCreateJobsTimeSeconds, "create", builderFilePath, nullptr);
                }
                else
                {
                    AZ_Error("AssetProcessor", false, "Failed to retrieve a valid builder to process job");
                }
            };

        // Also override the processJob function to run externally
        modifiedBuilderDesc.m_processJobFunction = [builderFilePath](const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
            {
                AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

                AssetProcessor::BuilderRef builderRef;
                AssetProcessor::BuilderManagerBus::BroadcastResult(builderRef, &AssetProcessor::BuilderManagerBusTraits::GetBuilder);

                if (builderRef)
                {
                    builderRef->RunJob<AssetBuilderSDK::ProcessJobNetRequest, AssetBuilderSDK::ProcessJobNetResponse>(request, response, s_MaximumProcessJobsTimeSeconds, "process", builderFilePath, &jobCancelListener, request.m_tempDirPath);
                }
                else
                {
                    AZ_Error("AssetProcessor", false, "Failed to retrieve a valid builder to process job");
                }
            };
    }

    if (m_builderDescMap.find(modifiedBuilderDesc.m_busId) != m_builderDescMap.end())
    {
        AZ_Warning(AssetProcessor::DebugChannel, false, "Uuid for %s builder is already registered.\n", modifiedBuilderDesc.m_name.c_str());
        return;
    }
    if (m_builderNameToId.find(modifiedBuilderDesc.m_name) != m_builderNameToId.end())
    {
        AZ_Warning(AssetProcessor::DebugChannel, false, "Duplicate builder detected.  A builder named '%s' is already registered.\n", modifiedBuilderDesc.m_name.c_str());
        return;
    }

    AZStd::sort(modifiedBuilderDesc.m_patterns.begin(), modifiedBuilderDesc.m_patterns.end(),
        [](const AssetBuilderSDK::AssetBuilderPattern& first, const AssetBuilderSDK::AssetBuilderPattern& second)
    {
        return first.ToString() < second.ToString();
    });

    m_builderDescMap[modifiedBuilderDesc.m_busId] = modifiedBuilderDesc;
    m_builderNameToId[modifiedBuilderDesc.m_name] = modifiedBuilderDesc.m_busId;

    for (const AssetBuilderSDK::AssetBuilderPattern& pattern : modifiedBuilderDesc.m_patterns)
    {
        AssetUtilities::BuilderFilePatternMatcher patternMatcher(pattern, modifiedBuilderDesc.m_busId);
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

void BatchApplicationManager::GetAllBuildersInfo(AssetProcessor::BuilderInfoList& builderInfoList)
{
    for (const auto &builderPair : m_builderDescMap)
    {
        builderInfoList.push_back(builderPair.second);
    }
}

bool BatchApplicationManager::OnError(const char* window, const char* message)
{
    // We don't need to print the message to stdout, the trace system will already do that

    return true;
}

bool BatchApplicationManager::CheckSufficientDiskSpace(const QString& savePath, qint64 requiredSpace, bool shutdownIfInsufficient)
{
    if (!QDir(savePath).exists())
    {
        QDir dir;
        dir.mkpath(savePath);
    }
    QStorageInfo storageInfo(savePath);
    qint64 bytesFree = storageInfo.bytesFree();

    AZ_Assert(bytesFree >= 0, "Unable to determine the amount of free space on drive containing path (%s).", savePath.toUtf8().constData());

    if (bytesFree < requiredSpace + s_ReservedDiskSpaceInBytes)
    {
        if (shutdownIfInsufficient)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "There is insufficient disk space to continue running.  AssetProcessor will now exit");
            QMetaObject::invokeMethod(this, "QuitRequested", Qt::QueuedConnection);
        }

        return false;
    }

    return true;
}

void BatchApplicationManager::RemoveOldTempFolders()
{
    QDir rootDir;
    if (!AssetUtilities::ComputeAssetRoot(rootDir))
    {
        return;
    }

    QString startFolder = rootDir.absolutePath();
    QDir root;
    if (!AssetUtilities::CreateTempRootFolder(startFolder, root))
    {
        return;
    }

    // We will remove old temp folders if either their modified time is older than the cutoff time or 
    // if the total number of temp folders have exceeded the maximum number of temp folders.   
    QFileInfoList entries = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time); // sorting by modification time
    int folderCount = 0;
    bool removeFolder = false;
    QDateTime cutoffTime = QDateTime::currentDateTime().addDays(-7);
    for (const QFileInfo& entry : entries)
    {
        if (!entry.fileName().startsWith("JobTemp-"))
        {
            continue;
        }

        // Since we are sorting the folders list from latest to oldest, we will either be in a state where we have to delete all the remaining folders or not
        // because either we have reached the folder limit or reached the cutoff date limit.
        removeFolder = removeFolder || (folderCount++ >= s_MaximumTempFolders) || 
            (entry.lastModified() < cutoffTime);
        
        if (removeFolder)
        {
            QDir dir(entry.absoluteFilePath());
            dir.removeRecursively();
        }
    }
}

void BatchApplicationManager::OnAssetProcessorManagerIdleState(bool isIdle)
{
    // these can come in during shutdown.
    if (InitiatedShutdown())
    {
        return;
    }

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
