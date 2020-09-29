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

#include <AzCore/Module/DynamicModuleHandle.h>
#include <SceneAPI/SceneData/SceneDataStandaloneAllocator.h>

class SceneUITestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    virtual ~SceneUITestEnvironment() {}

protected:
    void SetupEnvironment() override
    {
        AZ::Environment::Create(nullptr);
        AZ::SceneAPI::SceneDataStandaloneAllocator::Initialize(AZ::Environment::GetInstance());

        m_sceneCoreModule = AZ::DynamicModuleHandle::Create("SceneCore");
        AZ_Assert(m_sceneCoreModule, "SceneUI unit tests failed to create SceneCore module.");
        bool loaded = m_sceneCoreModule->Load(false);
        AZ_Assert(loaded, "SceneUI unit tests failed to load SceneCore module.");
        auto init = m_sceneCoreModule->GetFunction<AZ::InitializeDynamicModuleFunction>(AZ::InitializeDynamicModuleFunctionName);
        AZ_Assert(init, "SceneUI unit tests failed to find the initialization function the SceneCore module.");
        init(AZ::Environment::GetInstance());
    }

    void TeardownEnvironment() override
    {
        auto uninit = m_sceneCoreModule->GetFunction<AZ::UninitializeDynamicModuleFunction>(AZ::UninitializeDynamicModuleFunctionName);
        AZ_Assert(uninit, "SceneUI unit tests failed to find the uninitialization function the SceneCore module.");
        uninit();
        m_sceneCoreModule.reset();

        AZ::SceneAPI::SceneDataStandaloneAllocator::TearDown();
        AZ::Environment::Destroy();
    }

    AZStd::unique_ptr<AZ::DynamicModuleHandle> m_sceneCoreModule;
};

AZ_UNIT_TEST_HOOK(new SceneUITestEnvironment);

TEST(SceneUITest, SanityTest)
{
    ASSERT_EQ(1, 1);
}
