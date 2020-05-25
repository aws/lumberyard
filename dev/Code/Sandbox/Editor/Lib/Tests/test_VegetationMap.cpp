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

            ON_CALL(m_data->m_mockSystem, GetI3DEngine()).WillByDefault(::testing::Return(&(m_data->m_mock3DEngine)));

            ON_CALL(m_data->m_mock3DEngine, GetWaterLevel()).WillByDefault(::testing::Return(0.0f));
            ON_CALL(m_data->m_mock3DEngine, GetTerrainElevation(::testing::_, ::testing::_, ::testing::_)).WillByDefault(::testing::Return(0.0f));

#ifdef LY_TERRAIN_EDITOR
            ON_CALL(m_data->m_mockEditor, GetHeightmap()).WillByDefault(::testing::Return((CHeightmap*)&(m_data->m_mockHeightmap)));
            ON_CALL(m_data->m_mockHeightmap, GetOceanLevel()).WillByDefault(::testing::Return(0.0f));
#endif // #ifdef LY_TERRAIN_EDITOR

            StatInstGroupEventBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            StatInstGroupEventBus::Handler::BusDisconnect();

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
#ifdef LY_TERRAIN_EDITOR
            ::testing::NiceMock<CHeightmapMock> m_mockHeightmap;
#endif // #ifdef LY_TERRAIN_EDITOR
            ::testing::NiceMock<ConsoleMock> m_mockConsole;
        };

        AZStd::unique_ptr<DataMembers> m_data;

        AZ::Entity* m_systemEntity = nullptr;
        StatInstGroupId m_groupId{ 0 };

    private:
        struct SavedState
        {
            SSystemGlobalEnvironment* m_Env = nullptr;
            ISystem* m_system = nullptr;
        };

        SavedState m_savedState;
    };


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
