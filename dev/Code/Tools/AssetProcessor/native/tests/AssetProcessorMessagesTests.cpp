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

#include <AzTest/AzTest.h>
#include <utilities/BatchApplicationManager.h>
#include <utilities/ApplicationServer.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <connection/connectionManager.h>
#include <QCoreApplication>
#include <QTemporaryDir>

namespace AssetProcessorMessagesTests
{
    using namespace testing;
    using ::testing::NiceMock;

    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    static constexpr unsigned short AssetProcessorPort = static_cast<unsigned short>(888u);

    class AssetProcessorMessages;

    struct UnitTestBatchApplicationManager
        : BatchApplicationManager
    {
        UnitTestBatchApplicationManager(int* argc, char*** argv, QObject* parent)
            : BatchApplicationManager(argc, argv, parent)
        {

        }

        friend class AssetProcessorMessages;
    };

    class MockDatabaseLocationListener : public AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler
    {
    public:
        MOCK_METHOD1(GetAssetDatabaseLocation, bool(AZStd::string&));
    };

    struct MockAssetCatalog : AssetProcessor::AssetCatalog
    {
        MockAssetCatalog(QObject* parent, AssetProcessor::PlatformConfiguration* platformConfiguration)
            : AssetCatalog(parent, platformConfiguration)
        {
        }

        void RequestReady(AssetProcessor::NetworkRequestID requestId, AzFramework::AssetSystem::BaseAssetProcessorMessage* message, QString platform, bool fencingFailed) override
        {
            AssetCatalog::RequestReady(requestId, message, platform, fencingFailed);
            m_called = true;
        }

        bool m_called = false;
    };

    class AssetProcessorMessages
        : public ::UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        void SetUp() override
        {
            AssetUtilities::ResetGameName();

            m_temporarySourceDir = QDir(m_temporaryDir.path());
            m_databaseLocation = m_temporarySourceDir.absoluteFilePath("test_database.sqlite").toUtf8().constData();

            ON_CALL(m_databaseLocationListener, GetAssetDatabaseLocation(_))
                .WillByDefault(
                    DoAll( // set the 0th argument ref (string) to the database location and return true.
                        SetArgReferee<0>(m_databaseLocation.c_str()),
                        Return(true)));

            m_databaseLocationListener.BusConnect();

            m_dbConn.OpenDatabase();

            int argC = 0;
            m_batchApplicationManager = AZStd::make_unique<UnitTestBatchApplicationManager>(&argC, nullptr, nullptr);
            m_batchApplicationManager->BeforeRun();
            m_batchApplicationManager->m_platformConfiguration = new PlatformConfiguration();
            m_batchApplicationManager->InitAssetProcessorManager();

            m_assetCatalog = AZStd::make_unique<MockAssetCatalog>(nullptr, m_batchApplicationManager->m_platformConfiguration);

            m_batchApplicationManager->m_assetCatalog = m_assetCatalog.get();
            m_batchApplicationManager->InitRCController();
            m_batchApplicationManager->InitApplicationServer();
            m_batchApplicationManager->InitConnectionManager();
            m_batchApplicationManager->InitAssetRequestHandler();

            QObject::connect(m_batchApplicationManager->m_connectionManager, &ConnectionManager::ConnectionError, [](unsigned /*connId*/, QString error)
                {
                    AZ_Error("ConnectionManager", false, "%s", error.toUtf8().constData());
                });

            ASSERT_TRUE(m_batchApplicationManager->m_applicationServer->startListening(AssetProcessorPort));

            using namespace AzFramework;

            m_assetSystemComponent = AZStd::make_unique<AssetSystem::AssetSystemComponent>();

            m_assetSystemComponent->Init();
            m_assetSystemComponent->Activate();

            RunNetworkRequest([]()
                {
                    bool result = false;

                    AZStd::string appBranchToken;
                    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::CalculateBranchTokenForAppRoot, appBranchToken);

                    AssetSystemRequestBus::Broadcast(&AssetSystemRequestBus::Events::SetBranchToken, appBranchToken);
                    AssetSystemRequestBus::Broadcast(&AssetSystemRequestBus::Events::SetAssetProcessorIP, "127.0.0.1");
                    AssetSystemRequestBus::Broadcast(&AssetSystemRequestBus::Events::SetAssetProcessorPort, AssetProcessorPort);
                    AssetSystemRequestBus::BroadcastResult(result, &AssetSystemRequestBus::Events::Connect, "UNITTEST");

                    ASSERT_TRUE(result);
                });
        }

        void TearDown() override
        {
            QEventLoop eventLoop;

            QObject::connect(m_batchApplicationManager->m_connectionManager, &ConnectionManager::ReadyToQuit, &eventLoop, &QEventLoop::quit);

            m_batchApplicationManager->m_connectionManager->QuitRequested();

            eventLoop.exec();

            m_assetSystemComponent->Deactivate();
            m_batchApplicationManager->Destroy();
        }

        void RunNetworkRequest(AZStd::function<void()> func) const
        {
            AZStd::atomic_bool finished = false;

            auto thread = AZStd::thread([&finished, &func]()
                {
                    func();
                    finished = true;
                }
            );

            while (!finished)
            {
                QCoreApplication::processEvents();
            }

            thread.join();
        }
    protected:

        QTemporaryDir m_temporaryDir;
        AZStd::unique_ptr<UnitTestBatchApplicationManager> m_batchApplicationManager;
        AZStd::unique_ptr<AzFramework::AssetSystem::AssetSystemComponent> m_assetSystemComponent;
        NiceMock<MockDatabaseLocationListener> m_databaseLocationListener;
        AZStd::unique_ptr<MockAssetCatalog> m_assetCatalog = nullptr;
        QDir m_temporarySourceDir;
        AZStd::string m_databaseLocation;
        AssetDatabaseConnection m_dbConn;
    };

    TEST_F(AssetProcessorMessages, GetUnresolvedProductReferences_Succeeds)
    {
        using namespace AzToolsFramework::AssetDatabase;

        // Setup the database with all needed info
        ScanFolderDatabaseEntry scanfolder1("scanfolder1", "scanfolder1", "scanfolder1", "");
        ASSERT_TRUE(m_dbConn.SetScanFolder(scanfolder1));

        SourceDatabaseEntry source1(scanfolder1.m_scanFolderID, "source1.png", AZ::Uuid::CreateRandom(), "Fingerprint");
        SourceDatabaseEntry source2(scanfolder1.m_scanFolderID, "source2.png", AZ::Uuid::CreateRandom(), "Fingerprint");
        ASSERT_TRUE(m_dbConn.SetSource(source1));
        ASSERT_TRUE(m_dbConn.SetSource(source2));

        JobDatabaseEntry job1(source1.m_sourceID, "jobkey", 1234, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 1111);
        JobDatabaseEntry job2(source2.m_sourceID, "jobkey", 1234, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 2222);
        ASSERT_TRUE(m_dbConn.SetJob(job1));
        ASSERT_TRUE(m_dbConn.SetJob(job2));

        ProductDatabaseEntry product1(job1.m_jobID, 5, "source1.product", AZ::Data::AssetType::CreateRandom());
        ProductDatabaseEntry product2(job2.m_jobID, 15, "source2.product", AZ::Data::AssetType::CreateRandom());
        ASSERT_TRUE(m_dbConn.SetProduct(product1));
        ASSERT_TRUE(m_dbConn.SetProduct(product2));

        ProductDependencyDatabaseEntry dependency1(product1.m_productID, AZ::Uuid::CreateNull(), 0, {}, "pc", 0, "somefileA.txt", ProductDependencyDatabaseEntry::DependencyType::ProductDep_SourceFile);
        ProductDependencyDatabaseEntry dependency2(product1.m_productID, AZ::Uuid::CreateNull(), 0, {}, "pc", 0, "somefileB.txt", ProductDependencyDatabaseEntry::DependencyType::ProductDep_ProductFile);
        ProductDependencyDatabaseEntry dependency3(product1.m_productID, AZ::Uuid::CreateNull(), 0, {}, "pc", 0, "somefileC.txt");
        ProductDependencyDatabaseEntry dependency4(product1.m_productID, AZ::Uuid::CreateNull(), 0, {}, "pc", 0, ":somefileD.txt"); // Exclusion
        ProductDependencyDatabaseEntry dependency5(product1.m_productID, AZ::Uuid::CreateNull(), 0, {}, "pc", 0, "somefileE*.txt"); // Wildcard
        ASSERT_TRUE(m_dbConn.SetProductDependency(dependency1));
        ASSERT_TRUE(m_dbConn.SetProductDependency(dependency2));
        ASSERT_TRUE(m_dbConn.SetProductDependency(dependency3));
        ASSERT_TRUE(m_dbConn.SetProductDependency(dependency4));
        ASSERT_TRUE(m_dbConn.SetProductDependency(dependency5));

        // Setup the asset catalog
        AzFramework::AssetSystem::AssetNotificationMessage assetNotificationMessage("source1.product", AzFramework::AssetSystem::AssetNotificationMessage::NotificationType::AssetChanged, AZ::Data::AssetType::CreateRandom(), "pc");
        assetNotificationMessage.m_assetId = AZ::Data::AssetId(source1.m_sourceGuid, product1.m_subID);
        assetNotificationMessage.m_dependencies.push_back(AZ::Data::ProductDependency(AZ::Data::AssetId(source2.m_sourceGuid, product2.m_subID), {}));

        m_assetCatalog->OnAssetMessage(assetNotificationMessage);

        // Run the actual test
        RunNetworkRequest([&source1, &product1]()
            {
                using namespace AzFramework;

                AZ::u32 assetReferenceCount, pathReferenceCount;
                AZ::Data::AssetId assetId = AZ::Data::AssetId(source1.m_sourceGuid, product1.m_subID);
                AssetSystemRequestBus::Broadcast(&AssetSystemRequestBus::Events::GetUnresolvedProductReferences, assetId, assetReferenceCount, pathReferenceCount);

                ASSERT_EQ(assetReferenceCount, 1);
                ASSERT_EQ(pathReferenceCount, 3);
            });

        ASSERT_TRUE(m_assetCatalog->m_called);
    }
}
