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
#include "stdafx.h"
#include <AzTest/AzTest.h>

// memory management
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/OSAllocator.h>
#include <LmbrAWS/Memory/LmbrAWSMemoryManager.h>
#include <aws/core/utils/memory/AWSMemory.h>

// mocks
#include <Mocks/ICryPakMock.h>
#include <Mocks/ISystemMock.h>
#include <Mocks/IThreadTaskMock.h>

namespace
{
    using ::testing::NiceMock;

    class LmbrAWSTestEnvironment final
        : public AZ::Test::ITestEnvironment
    {
    public:
        AZ_TEST_CLASS_ALLOCATOR(LmbrAWSTestEnvironment);

    protected:
        void SetupEnvironment() override
        {
            // required memory management
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
            AZ::AllocatorInstance<AZ::OSAllocator>::Create();
            Aws::Utils::Memory::InitializeAWSMemorySystem(m_awsMemoryManager);

            // Constructor of CryAwsExecutor accesses gEnv->pSystem->GetIThreadTaskManager, so
            // even though it may not USE the thread manager in tests, it still needs a valid
            // instance or else it will assert.
            ON_CALL(m_stubSystem, GetIThreadTaskManager())
                .WillByDefault(::testing::Return(&m_stubThreadTaskManager));
            ::testing::Mock::AllowLeak(&m_stubSystem);

            // setup global environment
            m_stubEnv.pCryPak = &m_stubPak;
            m_stubEnv.pSystem = &m_stubSystem;
            gEnv = &m_stubEnv;
        }

        void TeardownEnvironment() override
        {
            Aws::Utils::Memory::ShutdownAWSMemorySystem();
            AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }

    private:
        LmbrAWS::LmbrAWSMemoryManager m_awsMemoryManager;

        SSystemGlobalEnvironment m_stubEnv;

        NiceMock<CryPakMock> m_stubPak;
        NiceMock<SystemMock> m_stubSystem;
        NiceMock<ThreadTaskManagerMock> m_stubThreadTaskManager;
    };
}


AZ_UNIT_TEST_HOOK(new LmbrAWSTestEnvironment);


TEST(LmbrAWSSanityTest, Sanity)
{
    EXPECT_EQ(1, 1);
}
