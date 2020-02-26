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

#include <AzCore/Module/Environment.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <SceneAPI/SceneCore/SceneCore.h>

class ResourceCompilerSceneTestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    virtual ~ResourceCompilerSceneTestEnvironment()
    {}

protected:
    void SetupEnvironment() override
    {
        if (!AZ::AllocatorInstance<AZ::SystemAllocator>().IsReady())
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>().Create();
            m_hasLocalMemoryAllocator = true;
        }

#if defined(AZ_PLATFORM_LINUX)
        AZ::SceneCore::SceneCore::Initialize();
#else
        sceneCoreModule = AZ::DynamicModuleHandle::Create("SceneCore");
        AZ_Assert(sceneCoreModule, "ResourceCompilerScene unit tests failed to create SceneCore module.");
        bool loaded = sceneCoreModule->Load(false);
        AZ_Assert(loaded, "ResourceCompilerScene unit tests failed to load SceneCore module.");
        auto init = sceneCoreModule->GetFunction<AZ::InitializeDynamicModuleFunction>(AZ::InitializeDynamicModuleFunctionName);
        AZ_Assert(init, "ResourceCompilerScene unit tests failed to find the initialization function the SceneCore module.");
        (*init)(AZ::Environment::GetInstance());
#endif // defined(AZ_PLATFORM_LINUX)
        sceneDataModule = AZ::DynamicModuleHandle::Create("SceneData");
        AZ_Assert(sceneDataModule, "ResourceCompilerScene unit tests failed to create SceneData module.");
        loaded = sceneDataModule->Load(false);
        AZ_Assert(loaded, "ResourceCompilerScene unit tests failed to load SceneData module.");
        init = sceneDataModule->GetFunction<AZ::InitializeDynamicModuleFunction>(AZ::InitializeDynamicModuleFunctionName);
        AZ_Assert(init, "ResourceCompilerScene unit tests failed to find the initialization function the SceneData module.");
        (*init)(AZ::Environment::GetInstance());

        fbxSceneBuilderModule = AZ::DynamicModuleHandle::Create("FbxSceneBuilder");
        AZ_Assert(fbxSceneBuilderModule, "ResourceCompilerScene unit tests failed to create FbxSceneBuilder module.");
        loaded = fbxSceneBuilderModule->Load(false);
        AZ_Assert(loaded, "ResourceCompilerScene unit tests failed to load FbxSceneBuilder module.");
    }

    void TeardownEnvironment() override
    {
        fbxSceneBuilderModule.reset();

        auto uninit = sceneDataModule->GetFunction<AZ::UninitializeDynamicModuleFunction>(AZ::UninitializeDynamicModuleFunctionName);
        AZ_Assert(uninit, "FbxSceneBuilder unit tests failed to find the uninitialization function the SceneData module.");
        (*uninit)();
        sceneDataModule.reset();

#if defined(AZ_PLATFORM_LINUX)
        AZ::SceneCore::SceneCore::Uninitialize();
#else
        uninit = sceneCoreModule->GetFunction<AZ::UninitializeDynamicModuleFunction>(AZ::UninitializeDynamicModuleFunctionName);
        AZ_Assert(uninit, "FbxSceneBuilder unit tests failed to find the uninitialization function the SceneCore module.");
        (*uninit)();
        sceneCoreModule.reset();
#endif // defined(AZ_PLATFORM_LINUX)
        if (m_hasLocalMemoryAllocator)
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>().Destroy();
        }
    }

private:
    bool m_hasLocalMemoryAllocator = false;
#if !defined(AZ_PLATFORM_LINUX)
    AZStd::unique_ptr<AZ::DynamicModuleHandle> sceneCoreModule;
#endif // defined(AZ_PLATFORM_LINUX)
    AZStd::unique_ptr<AZ::DynamicModuleHandle> sceneDataModule;
    AZStd::unique_ptr<AZ::DynamicModuleHandle> fbxSceneBuilderModule;
};

AZ_UNIT_TEST_HOOK(new ResourceCompilerSceneTestEnvironment);

TEST(ResourceCompilerSceneTests, Sanity)
{
    EXPECT_EQ(1, 1);
}