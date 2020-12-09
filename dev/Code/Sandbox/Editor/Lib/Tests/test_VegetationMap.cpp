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

#include "StdAfx.h"
#include <AzTest/AzTest.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Lib/Tests/IEditorMock.h>
#include <Mocks/ISystemMock.h>
#include <Mocks/I3DEngineMock.h>
#include <Mocks/IConsoleMock.h>

#include <Cry3DEngine/Cry3DEngineBase.h>
#include <Cry3DEngine/cvars.h>
#include <CryCommon/StatObjBus.h>

#include "VegetationMap.h"
#include <VegetationObject.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <IMissingAssetResolver.h>

#ifdef LY_TERRAIN_EDITOR
#include <Terrain/Heightmap.h>

class CHeightmapMock
    : public IHeightmap
{
public:
    MOCK_METHOD6(UpdateEngineTerrain, void(int x1, int y1, int width, int height, bool bElevation, bool bInfoBits));
    MOCK_METHOD4(RecordAzUndoBatchTerrainModify, void(AZ::u32 x, AZ::u32 y, AZ::u32 width, AZ::u32 height));
    MOCK_CONST_METHOD0(GetOceanLevel, float());
    MOCK_METHOD0(GetUseTerrain, bool());
};
#endif //#ifdef LY_TERRAIN_EDITOR

namespace UnitTest
{
    struct ErrorReportMock
        : IErrorReport
    {

        MOCK_METHOD0(Clear, void());
        MOCK_METHOD0(Display, void());
        MOCK_METHOD1(GetError, CErrorRecord& (int));
        MOCK_CONST_METHOD0(GetErrorCount, int());
        MOCK_CONST_METHOD0(IsEmpty, bool());
        MOCK_CONST_METHOD0(IsImmediateMode, bool());
        MOCK_METHOD1(Report, void(SValidatorRecord&));
        MOCK_METHOD1(ReportError, void(CErrorRecord&));
        MOCK_METHOD1(SetCurrentFile, void(const QString&));
        MOCK_METHOD1(SetCurrentValidatorItem, void(CBaseLibraryItem*));
        MOCK_METHOD1(SetCurrentValidatorObject, void(CBaseObject*));
        MOCK_METHOD1(SetImmediateMode, void(bool));
        MOCK_METHOD1(SetShowErrors, void(bool));
    };

    struct MissingAssetResolverMock
        : IMissingAssetResolver
    {
        MOCK_METHOD2(AcceptRequest, void (uint32, const char*));
        MOCK_METHOD4(AddResolveRequest, uint32 (const char*, const TMissingAssetResolveCallback&, char, bool));
        MOCK_METHOD2(CancelRequest, void (const TMissingAssetResolveCallback&, uint32));
        MOCK_METHOD1(CancelRequest, void (uint32));
        MOCK_CONST_METHOD1(GetAssetSearcherById, IAssetSearcher* (int));
        MOCK_METHOD0(GetReport, CMissingAssetReport* ());
        MOCK_METHOD1(OnEditorNotifyEvent, void (EEditorNotifyEvent));
        MOCK_METHOD0(PumpEvents, void ());
        MOCK_METHOD0(Shutdown, void ());
        MOCK_METHOD0(StartResolver, void ());
        MOCK_METHOD0(StopResolver, void ());
    };

    class VegetationMapTestFixture
        : public UnitTest::AllocatorsTestFixture
        , private StatInstGroupEventBus::Handler
    {
    public:
        void SetUp() override
        {
            // capture prior state.
            m_savedState.m_Env = gEnv;

            UnitTest::AllocatorsTestFixture::SetUp();

            // LegacyAllocator is a lazily-created allocator, so it will always exist, but we still manually shut it down and
            // start it up again between tests so we can have consistent behavior.
            if (!AZ::AllocatorInstance<AZ::LegacyAllocator>::GetAllocator().IsReady())
            {
                AZ::AllocatorInstance<AZ::LegacyAllocator>::Create();
            }

            m_data = AZStd::make_unique<DataMembers>();

            // override state with our needed test mocks
            gEnv = &(m_data->m_mockGEnv);
            m_data->m_mockGEnv.pSystem = &(m_data->m_mockSystem);
            m_data->m_mockGEnv.pConsole = &(m_data->m_mockConsole);

            SetIEditor(&(m_data->m_mockEditor));
            ON_CALL(m_data->m_mockEditor, GetSystem()).WillByDefault(::testing::Return(&(m_data->m_mockSystem)));
            ON_CALL(m_data->m_mockEditor, GetRenderer()).WillByDefault(::testing::Return(nullptr));
            ON_CALL(m_data->m_mockEditor, Get3DEngine()).WillByDefault(::testing::Return(&(m_data->m_mock3DEngine)));
            ON_CALL(m_data->m_mockEditor, GetErrorReport()).WillByDefault(::testing::Return(&m_data->m_mockErrorReport));
            ON_CALL(m_data->m_mockEditor, GetMissingAssetResolver()).WillByDefault(::testing::Return(&(m_data->m_missingAssetResolver)));

            ON_CALL(m_data->m_mockSystem, GetI3DEngine()).WillByDefault(::testing::Return(&(m_data->m_mock3DEngine)));

            ON_CALL(m_data->m_mock3DEngine, GetWaterLevel()).WillByDefault(::testing::Return(0.0f));

#ifdef LY_TERRAIN_EDITOR
            ON_CALL(m_data->m_mockEditor, GetHeightmap()).WillByDefault(::testing::Return((CHeightmap*)&(m_data->m_mockHeightmap)));
            ON_CALL(m_data->m_mockHeightmap, GetOceanLevel()).WillByDefault(::testing::Return(0.0f));
#endif // #ifdef LY_TERRAIN_EDITOR

            m_localFileIO = aznew AZ::IO::LocalFileIO();

            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_localFileIO);

            StatInstGroupEventBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            StatInstGroupEventBus::Handler::BusDisconnect();

            AZ::IO::FileIOBase::SetInstance(nullptr);
            delete m_localFileIO;

            SetIEditor(nullptr);

            // Remove the other mocked systems
            m_data.reset();

            AZ::AllocatorInstance<AZ::LegacyAllocator>::Destroy();

            UnitTest::AllocatorsTestFixture::TearDown();

            // restore the global state that we modified during the test.
            gEnv = m_savedState.m_Env;
        }

    protected:
        StatInstGroupId GenerateStatInstGroupId() override
        {
            return m_groupId++;
        }

        void ReleaseStatInstGroupId(StatInstGroupId statInstGroupId) override {}
        void ReleaseStatInstGroupIdSet(const AZStd::unordered_set<StatInstGroupId>& statInstGroupIdSet) override {}
        void ReserveStatInstGroupIdRange(StatInstGroupId from, StatInstGroupId to) override {}


        struct DataMembers
        {
            SSystemGlobalEnvironment m_mockGEnv;
            ::testing::NiceMock<SystemMock> m_mockSystem;
            ::testing::NiceMock<CEditorMock> m_mockEditor;
            ::testing::NiceMock<I3DEngineMock> m_mock3DEngine;
            ::testing::NiceMock<ErrorReportMock> m_mockErrorReport;
            ::testing::NiceMock<MissingAssetResolverMock> m_missingAssetResolver;
#ifdef LY_TERRAIN_EDITOR
            ::testing::NiceMock<CHeightmapMock> m_mockHeightmap;
#endif // #ifdef LY_TERRAIN_EDITOR
            ::testing::NiceMock<ConsoleMock> m_mockConsole;
        };

        AZStd::unique_ptr<DataMembers> m_data;

        AZ::Entity* m_systemEntity = nullptr;
        AZ::IO::FileIOBase* m_localFileIO = nullptr;

        StatInstGroupId m_groupId{ 0 };

    private:
        struct SavedState
        {
            SSystemGlobalEnvironment* m_Env = nullptr;
            ISystem* m_system = nullptr;
        };

        SavedState m_savedState;
    };

    struct MockAssetSystem
        : AzFramework::AssetSystemRequestBus::Handler
    {
        MockAssetSystem()
        {
            BusConnect();
        }

        ~MockAssetSystem()
        {
            BusDisconnect();
        }

        AzFramework::AssetSystem::AssetStatus CompileAssetSync(const AZStd::string& assetPath) override
        {
            m_compileSync = assetPath;

            return AzFramework::AssetSystem::AssetStatus::AssetStatus_Queued;
        }

        MOCK_METHOD1(CompileAssetSync_FlushIO, AzFramework::AssetSystem::AssetStatus (const AZStd::string&));
        MOCK_METHOD1(CompileAssetSyncById, AzFramework::AssetSystem::AssetStatus (const AZ::Data::AssetId&));
        MOCK_METHOD1(CompileAssetSyncById_FlushIO, AzFramework::AssetSystem::AssetStatus (const AZ::Data::AssetId&));
        MOCK_METHOD4(ConfigureSocketConnection, bool (const AZStd::string&, const AZStd::string&, const AZStd::string&, const AZStd::string&));
        MOCK_METHOD1(Connect, bool (const char*));
        MOCK_METHOD2(ConnectWithTimeout, bool (const char*, AZStd::chrono::duration<float>));
        MOCK_METHOD0(Disconnect, bool ());
        MOCK_METHOD1(EscalateAssetBySearchTerm, bool (AZStd::string_view));
        MOCK_METHOD1(EscalateAssetByUuid, bool (const AZ::Uuid&));
        MOCK_METHOD0(GetAssetProcessorPingTimeMilliseconds, float ());
        MOCK_METHOD1(GetAssetStatus, AzFramework::AssetSystem::AssetStatus (const AZStd::string&));
        MOCK_METHOD1(GetAssetStatus_FlushIO, AzFramework::AssetSystem::AssetStatus (const AZStd::string&));
        MOCK_METHOD1(GetAssetStatusById, AzFramework::AssetSystem::AssetStatus (const AZ::Data::AssetId&));
        MOCK_METHOD1(GetAssetStatusById_FlushIO, AzFramework::AssetSystem::AssetStatus (const AZ::Data::AssetId&));
        MOCK_METHOD3(GetUnresolvedProductReferences, void (AZ::Data::AssetId, AZ::u32&, AZ::u32&));
        MOCK_METHOD0(SaveCatalog, bool ());
        MOCK_METHOD1(SetAssetProcessorIP, void (const AZStd::string&));
        MOCK_METHOD1(SetAssetProcessorPort, void (AZ::u16));
        MOCK_METHOD1(SetBranchToken, void (const AZStd::string&));
        MOCK_METHOD1(SetProjectName, void (const AZStd::string&));
        MOCK_METHOD0(ShowAssetProcessor, void ());
        MOCK_METHOD1(ShowInAssetProcessor, void (const AZStd::string&));

        AZStd::string m_compileSync;
    };

    TEST_F(VegetationMapTestFixture, LoadVegObject_AssetSyncRequested)
    {
        MockAssetSystem assetSystem;
        const int maxVegetationInstances = 1;

        CVegetationMap vegMap(maxVegetationInstances);
        const int worldSize = 1024;
        vegMap.Allocate(worldSize, false);

        CVegetationObject* testVegObject = vegMap.CreateObject();
        testVegObject->SetFileName("TestFile");
        testVegObject->LoadObject();

        ASSERT_STREQ(assetSystem.m_compileSync.c_str(), "TestFile");
    }

    TEST_F(VegetationMapTestFixture, InstantiateEmptySuccess)
    {
        CVegetationMap vegMap;
    }

    TEST_F(VegetationMapTestFixture, LY111661_CreateMaxInstancesThenCopyAreaWithoutCrashing)
    {
        const int maxVegetationInstances = 10000;

        // Create a default vegetation map set to an arbitrary world size.
        CVegetationMap vegMap(maxVegetationInstances);
        const int worldSize = 1024;
        vegMap.Allocate(worldSize, false);

        // Calculate the number of instances needed per row to space all our
        // veg instances evenly.  We add one to ensure that we overestimate
        // when going float->int instead of underestimate.  It's not *really*
        // important that we get the spacing 100% correct, we just want a roughly
        // even distribution.
        const int instancesPerRow = aznumeric_cast<int>(sqrt(maxVegetationInstances)) + 1;
        float worldSpacing = aznumeric_cast<float>(worldSize) / aznumeric_cast<float>(instancesPerRow);

        // Add the maximum number of instances possible to the vegetation map
        CVegetationObject *testVegObject = vegMap.CreateObject();
        for (int instance = 0; instance < maxVegetationInstances; instance++)
        {
            Vec3 worldPos(worldSpacing * (instance % instancesPerRow), worldSpacing * (instance / instancesPerRow), 0.0f);
            vegMap.PlaceObjectInstance(worldPos, testVegObject);
        }

        EXPECT_TRUE(vegMap.GetNumInstances() == maxVegetationInstances);

        // Copy a box from (50,50)-(150,150) to (250,250)-(350,350).  If the bug still exists,
        // this will crash from trying to create more than the max number of vegetation instances.
        vegMap.RepositionArea(AABB(Vec3(100.0f, 100.0f, 0.0f), 50.0f), Vec3(200.0f, 200.0f, 0), ImageRotationDegrees::Rotate0, true);
    }
} // namespace UnitTest
