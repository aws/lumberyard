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
#include "DeploymentTool_precompiled.h"

#ifdef AZ_TESTS_ENABLED

#include <AzTest/AzTest.h>

#include "../DeploymentConfig.h"
#include "../DeployWorkerBase.h"
#include <AzCore/Memory/SystemAllocator.h>

namespace DeployTool
{
    class DeploymentToolTestsDeployWorker
        : public ::testing::Test
    {
    public:
        AZ_TEST_CLASS_ALLOCATOR(DeploymentToolTestsDeployWorker);

        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
        }

        void TearDown() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }
    };

    TEST_F(DeploymentToolTestsDeployWorker, DeploymentToolTestsDeployWorker_WafSettingsTest)
    {
        // When running the test code, the current director is the output folder for the Editor Plugins, 
        // so walk back two folders to the dev root.
        const char* devRoot = "../../";

        // Make sure we can get at least get some values that the Deployment Tool will use
        // from the default settings config files.
        StringOutcome defaultFolderNameOutcomeAndroidARMv7 = DeployWorkerBase::GetPlatformSpecficDefaultAttributeValue("default_folder_name", PlatformOptions::Android_ARMv7, devRoot);

        ASSERT_TRUE(defaultFolderNameOutcomeAndroidARMv7.IsSuccess());

        StringOutcome defaultFolderNameOutcomeAndroidARMv8 = DeployWorkerBase::GetPlatformSpecficDefaultAttributeValue("default_folder_name", PlatformOptions::Android_ARMv8, devRoot);
        ASSERT_TRUE(defaultFolderNameOutcomeAndroidARMv8.IsSuccess());

        StringOutcome appOutputFolderExtNameOutcomeDebug = DeployWorkerBase::GetCommonBuildConfigurationsDefaultSettingsValue("debug", "default_output_ext", devRoot);
        ASSERT_TRUE(appOutputFolderExtNameOutcomeDebug.IsSuccess());

        StringOutcome appOutputFolderExtNameOutcomeProfile = DeployWorkerBase::GetCommonBuildConfigurationsDefaultSettingsValue("profile", "default_output_ext", devRoot);
        ASSERT_TRUE(appOutputFolderExtNameOutcomeProfile.IsSuccess());

        StringOutcome appOutputFolderExtNameOutcomePerformance = DeployWorkerBase::GetCommonBuildConfigurationsDefaultSettingsValue("performance", "default_output_ext", devRoot);
        ASSERT_TRUE(appOutputFolderExtNameOutcomePerformance.IsSuccess());

        StringOutcome appOutputFolderExtNameOutcomeRelease = DeployWorkerBase::GetCommonBuildConfigurationsDefaultSettingsValue("release", "default_output_ext", devRoot);
        ASSERT_TRUE(appOutputFolderExtNameOutcomeRelease.IsSuccess());

        // All of the build config output extensions should be different.
        ASSERT_TRUE(
            appOutputFolderExtNameOutcomeDebug.GetValue() != appOutputFolderExtNameOutcomeProfile.GetValue() &&
            appOutputFolderExtNameOutcomeDebug.GetValue() != appOutputFolderExtNameOutcomePerformance.GetValue() &&
            appOutputFolderExtNameOutcomeDebug.GetValue() != appOutputFolderExtNameOutcomeRelease.GetValue()
        );

        ASSERT_TRUE(
            appOutputFolderExtNameOutcomeProfile.GetValue() != appOutputFolderExtNameOutcomeDebug.GetValue() &&
            appOutputFolderExtNameOutcomeProfile.GetValue() != appOutputFolderExtNameOutcomePerformance.GetValue() &&
            appOutputFolderExtNameOutcomeProfile.GetValue() != appOutputFolderExtNameOutcomeRelease.GetValue()
        );

        ASSERT_TRUE(
            appOutputFolderExtNameOutcomePerformance.GetValue() != appOutputFolderExtNameOutcomeProfile.GetValue() &&
            appOutputFolderExtNameOutcomePerformance.GetValue() != appOutputFolderExtNameOutcomeDebug.GetValue() &&
            appOutputFolderExtNameOutcomePerformance.GetValue() != appOutputFolderExtNameOutcomeRelease.GetValue()
        );

        ASSERT_TRUE(
            appOutputFolderExtNameOutcomeRelease.GetValue() != appOutputFolderExtNameOutcomeProfile.GetValue() &&
            appOutputFolderExtNameOutcomeRelease.GetValue() != appOutputFolderExtNameOutcomePerformance.GetValue() &&
            appOutputFolderExtNameOutcomeRelease.GetValue() != appOutputFolderExtNameOutcomeDebug.GetValue()
        );

#if defined(AZ_PLATFORM_APPLE_OSX)
        // If this is a Mac, check the iOS settings that will be used by the Deployment Tool
        StringOutcome defaultFolderNameOutcomeiOS = DeployWorkerBase::GetPlatformSpecficDefaultAttributeValue("default_folder_name", PlatformOptions::iOS, devRoot);
        ASSERT_TRUE(defaultFolderNameOutcomeiOS.IsSuccess());

        const char* iosProjectSettingsGroup = "iOS Project Generator";
        const char* iosProjectFolderKey = "ios_project_folder";
        const char* iosProjectNameKey = "ios_project_name";

        StringOutcome folderNameOutcome = DeployWorkerBase::GetPlatformSpecficDefaultSettingsValue(iosProjectSettingsGroup, iosProjectFolderKey, PlatformOptions::iOS, devRoot);
        ASSERT_TRUE(!folderNameOutcome.IsSuccess());

        StringOutcome projectNameOutcome = DeployWorkerBase::GetPlatformSpecficDefaultSettingsValue(iosProjectSettingsGroup, iosProjectNameKey, PlatformOptions::iOS, devRoot);
        ASSERT_TRUE(!projectNameOutcome.IsSuccess());
#endif
    }

} // namespace DeployTool
#endif // AZ_TESTS_ENABLED