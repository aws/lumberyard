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
#include <Mocks/ISystemMock.h>
#include <Mocks/I3DEngineMock.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/IMemoryManagerMock.h>
#include "MergedMeshRenderNode.h"
#include "MergedMeshGeometry.h"


class MergedMeshOverrunDetectionTest
    : public UnitTest::AllocatorsTestFixture
{
public:
    void SetUp() override
    {
        // capture prior state.
        m_savedState.m_Env = gEnv;
        m_savedState.m_3DEngine = Cry3DEngineBase::m_p3DEngine;
        m_savedState.m_system = Cry3DEngineBase::m_pSystem;
        m_savedState.m_cvars = Cry3DEngineBase::m_pCVars;

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
        m_data->m_mockGEnv.p3DEngine = &(m_data->m_mock3DEngine);
        m_data->m_mockGEnv.pSystem = &(m_data->m_mockSystem);
        m_data->m_mockGEnv.pConsole = &(m_data->m_mockConsole);
        Cry3DEngineBase::m_pSystem = gEnv->pSystem;

        // NOTE:  We've mocked out I3DEngine, but we don't currently have a mock of C3DEngine.
        // For these unit tests, we need all code paths that we're testing to go through I3DEngine.
        // We set the engine's C3DEngine pointer to null to guarantee we aren't using it.
        Cry3DEngineBase::m_p3DEngine = nullptr;

        // Set GetISystem()->GetI3DEngine() to return our mocked I3DEngine.
        EXPECT_CALL(m_data->m_mockSystem, GetI3DEngine()).WillRepeatedly(Return(&(m_data->m_mock3DEngine)));

        // Create a default set of CVars for the MergedMeshManager to read from.
        // This needs to happen *after* we've set gEnv, since CVars() is dependent on gEnv.
        m_mockCVars = new CVars();
        Cry3DEngineBase::m_pCVars = m_mockCVars;
    }

    void TearDown() override
    {
        // Remove our mock cvars
        delete m_mockCVars;

        // Remove the other mocked systems
        m_data.reset();

        AZ::AllocatorInstance<AZ::LegacyAllocator>::Destroy();

        UnitTest::AllocatorsTestFixture::TearDown();

        // restore the global state that we modified during the test.
        gEnv = m_savedState.m_Env;
        Cry3DEngineBase::m_p3DEngine = m_savedState.m_3DEngine;
        Cry3DEngineBase::m_pSystem = m_savedState.m_system;
        Cry3DEngineBase::m_pCVars = m_savedState.m_cvars;
    }

protected:
    struct DataMembers
    {
        SSystemGlobalEnvironment m_mockGEnv;
        ::testing::NiceMock<I3DEngineMock> m_mock3DEngine;
        ::testing::NiceMock<SystemMock> m_mockSystem;
        ::testing::NiceMock<ConsoleMock> m_mockConsole;
        ::testing::NiceMock<MemoryManagerMock> m_mockMemoryManager;
    };

    AZStd::unique_ptr<DataMembers> m_data;

    CVars* m_mockCVars = nullptr;
    AZ::Entity* m_systemEntity = nullptr;
    AZ::ComponentApplication m_componentApp;

private:
    struct SavedState
    {
        SSystemGlobalEnvironment* m_Env = nullptr;
        C3DEngine* m_3DEngine = nullptr;
        ISystem* m_system = nullptr;
        CVars* m_cvars = nullptr;
    };

    SavedState m_savedState;
};


TEST_F(MergedMeshOverrunDetectionTest, MergedMeshRenderNode_TrivialInitialization)
{
    // Verify that we can trivially create and destroy a MergedMeshRenderNode.  This is a "trivial" test
    // because without the CMergedMeshesManager created and initialized, there's relatively little code
    // that executes.

    CMergedMeshRenderNode* mergedMeshNode = new CMergedMeshRenderNode();
    EXPECT_TRUE(mergedMeshNode != nullptr);

    delete mergedMeshNode;
}

TEST_F(MergedMeshOverrunDetectionTest, DISABLED_LY109183_MergedMeshShutdown_WithOverrunDetection_Crashes)
{
    // This test is disabled because not enough systems are mocked out to let it run successfully in isolation,
    // and there's no obvious way to enable / disable the overrun detection just for this test.
    // The intent is to provide regression testing for LY-109183 - the allocation and deallocation of a Merged Mesh group
    // inside of MergedMeshRenderNode when overrun detection is enabled used to crash, but should now succeed.  There needs
    // to be enough systems mocked out to let the MergedMeshRenderNode code create a pool allocator and attempt to allocate
    // and deallocate groups within it, using the same allocation / deallocation methods as the full engine.

    EXPECT_CALL(m_data->m_mockSystem, GetIMemoryManager()).WillRepeatedly(::testing::Return(&(m_data->m_mockMemoryManager)));

    CMergedMeshesManager mergedMeshManager;
    mergedMeshManager.Init();

    m_data->m_mockGEnv.SetDynamicMergedMeshGenerationEnabled(true);

    CMergedMeshRenderNode* mergedMeshNode = new CMergedMeshRenderNode();
    EXPECT_TRUE(mergedMeshNode != nullptr);

    SProcVegSample sample;
    mergedMeshNode->AddInstance(sample, true);

    delete mergedMeshNode;

    mergedMeshManager.Shutdown();
}
