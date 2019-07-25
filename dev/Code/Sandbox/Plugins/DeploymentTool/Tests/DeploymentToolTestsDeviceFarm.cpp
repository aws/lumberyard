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

#include "MockDeviceFarmClient.h"
#include "../DeviceFarmDriver.h"
#include <AzCore/Memory/SystemAllocator.h>

namespace DeployTool
{
    class DeploymentToolTestsDeviceFarm
        : public ::testing::Test
    {
    public:
        AZ_TEST_CLASS_ALLOCATOR(DeploymentToolTestsDeviceFarm);

        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

            m_deviceFarmClient.reset(new DeployTool::MockDeviceFarmClient());
            mDeviceFarmDriver.reset(new DeployTool::DeviceFarmDriver(m_deviceFarmClient));
        }

        void TearDown() override
        {
            mDeviceFarmDriver.reset();
            m_deviceFarmClient.reset();

            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }

        std::shared_ptr<DeployTool::MockDeviceFarmClient> m_deviceFarmClient;
        std::shared_ptr<DeployTool::DeviceFarmDriver> mDeviceFarmDriver;
    };

    TEST_F(DeploymentToolTestsDeviceFarm, DeploymentToolTestsDeviceFarm_TestProjects)
    {
        bool finished = false;
        bool success = false;

        // - Create a project
        finished = false;
        success = false;
        AZStd::string projectName = "TestProject01";
        DeployTool::DeviceFarmDriver::Project project("", "");

        mDeviceFarmDriver->CreateProject(
            projectName,
            [&](bool _success, const AZStd::string& msg, const DeployTool::DeviceFarmDriver::Project& _project)
        {
            success = _success;
            project = _project;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);
        ASSERT_TRUE(projectName == project.name);
        ASSERT_TRUE(!project.arn.empty());

        // - list projects and make sure the one we created is there
        finished = false;
        success = false;
        AZStd::vector<DeployTool::DeviceFarmDriver::Project> projects;

        mDeviceFarmDriver->ListProjects(
            [&](bool _success, const AZStd::string& _msg, AZStd::vector<DeployTool::DeviceFarmDriver::Project>& _projects)
        {
            success = _success;
            projects = _projects;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);
        bool foundProject = false;
        for (auto p : projects)
        {
            if (project.arn == p.arn)
            {
                foundProject = true;
            }
        }
        ASSERT_TRUE(foundProject);

        // - delete a project and expect success
        finished = false;
        success = false;

        mDeviceFarmDriver->DeleteProject(
            project.arn,
            [&](bool _success, const AZStd::string& msg)
        {
            success = _success;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);

        // - delete a project and expect failure
        finished = false;
        success = false;

        mDeviceFarmDriver->DeleteProject(
            project.arn,
            [&](bool _success, const AZStd::string& msg)
        {
            success = _success;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(!success);
    }

    TEST_F(DeploymentToolTestsDeviceFarm, DeploymentToolTestsDeviceFarm_TestDevicePools)
    {
        bool finished = false;
        bool success = false;

        // - Create a project
        finished = false;
        success = false;
        AZStd::string projectName = "TestProject01";
        DeployTool::DeviceFarmDriver::Project project("", "");

        mDeviceFarmDriver->CreateProject(
            projectName,
            [&](bool _success, const AZStd::string& msg, const DeployTool::DeviceFarmDriver::Project& _project)
        {
            success = _success;
            project = _project;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);
        ASSERT_TRUE(projectName == project.name);
        ASSERT_TRUE(!project.arn.empty());

        // - list devices
        finished = false;
        success = false;
        AZStd::vector<DeployTool::DeviceFarmDriver::Device> devices;

        mDeviceFarmDriver->ListDevices(
            project.arn,
            [&](bool _success, const AZStd::string& _msg, AZStd::vector<DeployTool::DeviceFarmDriver::Device>& _devices)
        {
            success = _success;
            devices = _devices;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);
        ASSERT_TRUE(devices.size() > 0);

        // - Create device pool
        finished = false;
        success = false;
        AZStd::string poolName = "TestPool01";
        AZStd::vector<AZStd::string> deviceArns;
        deviceArns.push_back(devices[0].name);
        DeployTool::DeviceFarmDriver::DevicePool devicePool("", "");

        mDeviceFarmDriver->CreateDevicePool(
            poolName,
            project.arn,
            "This is a test pool",
            deviceArns,
            [&](bool _success, const AZStd::string& msg, const DeployTool::DeviceFarmDriver::DevicePool& _devicePool)
        {
            success = _success;
            devicePool = _devicePool;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);
        ASSERT_TRUE(poolName == devicePool.name);
        ASSERT_TRUE(!devicePool.arn.empty());

        // - list device pools and make sure the one we created is there
        finished = false;
        success = false;
        AZStd::vector<DeployTool::DeviceFarmDriver::DevicePool> devicePools;

        mDeviceFarmDriver->ListDevicePools(
            project.arn,
            [&](bool _success, const AZStd::string& _msg, AZStd::vector<DeployTool::DeviceFarmDriver::DevicePool>& _devicePools)
        {
            success = _success;
            devicePools = _devicePools;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);
        bool foundDevicePool = false;
        for (auto d : devicePools)
        {
            if (devicePool.arn == d.arn)
            {
                foundDevicePool = true;
            }
        }
        ASSERT_TRUE(foundDevicePool);

        // - delete device pool and expect success
        finished = false;
        success = false;

        mDeviceFarmDriver->DeleteDevicePool(
            devicePool.arn,
            [&](bool _success, const AZStd::string& msg)
        {
            success = _success;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);

        // - delete device pool and expect false
        finished = false;
        success = false;

        mDeviceFarmDriver->DeleteDevicePool(
            devicePool.arn,
            [&](bool _success, const AZStd::string& msg)
        {
            success = _success;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(!success);
    }

    TEST_F(DeploymentToolTestsDeviceFarm, DeploymentToolTestsDeviceFarm_TestUploads)
    {
        bool finished = false;
        bool success = false;

        // - Create a project
        finished = false;
        success = false;
        AZStd::string projectName = "TestProject01";
        DeployTool::DeviceFarmDriver::Project project("", "");

        mDeviceFarmDriver->CreateProject(
            projectName,
            [&](bool _success, const AZStd::string& msg, const DeployTool::DeviceFarmDriver::Project& _project)
        {
            success = _success;
            project = _project;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);
        ASSERT_TRUE(projectName == project.name);
        ASSERT_TRUE(!project.arn.empty());

        // GetUpdate and expect it to fail
        finished = false;
        success = false;

        mDeviceFarmDriver->GetUpload(
            "bogus-arn",
            [&](bool _success, const AZStd::string& _msg, DeployTool::DeviceFarmDriver::Upload _upload)
        {
            success = _success;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(!success);

        // - Create an upload
        finished = false;
        success = false;
        DeployTool::DeviceFarmDriver::Upload upload;

        mDeviceFarmDriver->CreateUpload(
            "fake-path.apk",
            project.arn,
            [&](bool _success, const AZStd::string& msg, const DeployTool::DeviceFarmDriver::Upload& _upload)
        {
            success = _success;
            upload = _upload;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);
        ASSERT_TRUE(!upload.url.empty());
        ASSERT_TRUE(!upload.arn.empty());

        // GetUpdate and expect it to succeed
        finished = false;
        success = false;

        mDeviceFarmDriver->GetUpload(
            upload.arn,
            [&](bool _success, const AZStd::string& _msg, DeployTool::DeviceFarmDriver::Upload _upload)
        {
            success = _success;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);
    }

    TEST_F(DeploymentToolTestsDeviceFarm, DeploymentToolTestsDeviceFarm_TestRuns)
    {
        bool finished = false;
        bool success = false;

        // - Create a project
        finished = false;
        success = false;
        AZStd::string projectName = "TestProject01";
        DeployTool::DeviceFarmDriver::Project project("", "");

        mDeviceFarmDriver->CreateProject(
            projectName,
            [&](bool _success, const AZStd::string& msg, const DeployTool::DeviceFarmDriver::Project& _project)
        {
            success = _success;
            project = _project;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);
        ASSERT_TRUE(projectName == project.name);
        ASSERT_TRUE(!project.arn.empty());

        // - Create device pool
        finished = false;
        success = false;
        AZStd::string poolName = "TestPool01";
        AZStd::vector<AZStd::string> deviceArns;
        DeployTool::DeviceFarmDriver::DevicePool devicePool("", "");

        mDeviceFarmDriver->CreateDevicePool(
            poolName,
            project.arn,
            "This is a test pool",
            deviceArns,
            [&](bool _success, const AZStd::string& msg, const DeployTool::DeviceFarmDriver::DevicePool& _devicePool)
        {
            success = _success;
            devicePool = _devicePool;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);
        ASSERT_TRUE(poolName == devicePool.name);
        ASSERT_TRUE(!devicePool.arn.empty());

        // - Create an upload
        finished = false;
        success = false;
        DeployTool::DeviceFarmDriver::Upload upload;

        mDeviceFarmDriver->CreateUpload(
            "fake-path.apk",
            project.arn,
            [&](bool _success, const AZStd::string& msg, const DeployTool::DeviceFarmDriver::Upload& _upload)
        {
            success = _success;
            upload = _upload;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);
        ASSERT_TRUE(!upload.url.empty());
        ASSERT_TRUE(!upload.arn.empty());

        // - Schedule a run
        finished = false;
        success = false;
        AZStd::string runName = "TestRun01";
        DeployTool::DeviceFarmDriver::Run run("", "", "", 0, 0, "", "");

        mDeviceFarmDriver->ScheduleRun(
            runName,
            upload.arn,
            project.arn,
            devicePool.arn,
            5,
            [&](bool _success, const AZStd::string& msg, const DeployTool::DeviceFarmDriver::Run& _run)
        {
            success = _success;
            run = _run;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);
        ASSERT_TRUE(runName == run.name);
        ASSERT_TRUE(!run.arn.empty());

        // List the runs and make sure the one we added is there
        finished = false;
        success = false;
        AZStd::vector<DeployTool::DeviceFarmDriver::Run> runs;

        mDeviceFarmDriver->ListRuns(
            project.arn,
            [&](bool _success, const AZStd::string& _msg, AZStd::vector<DeployTool::DeviceFarmDriver::Run>& _runs)
        {
            success = _success;
            runs = _runs;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);
        bool foundRun = false;
        for (auto r : runs)
        {
            if (run.arn == r.arn)
            {
                foundRun = true;
            }
        }
        ASSERT_TRUE(foundRun);

        // - delete run and expect success
        finished = false;
        success = false;

        mDeviceFarmDriver->DeleteRun(
            run.arn,
            [&](bool _success, const AZStd::string& msg)
        {
            success = _success;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);

        // - delete run and expect failure
        finished = false;
        success = false;

        mDeviceFarmDriver->DeleteRun(
            run.arn,
            [&](bool _success, const AZStd::string& msg)
        {
            success = _success;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(!success);

        // - list runs and make sure the one we delete is gone
        finished = false;
        success = false;

        mDeviceFarmDriver->ListRuns(
            project.arn,
            [&](bool _success, const AZStd::string& _msg, AZStd::vector<DeployTool::DeviceFarmDriver::Run>& _runs)
        {
            success = _success;
            runs = _runs;
            finished = true;
        });
        while (!finished)
        {
            m_deviceFarmClient->Update();
        }
        ASSERT_TRUE(success);
        foundRun = false;
        for (auto r : runs)
        {
            if (run.arn == r.arn)
            {
                foundRun = true;
            }
        }
        ASSERT_TRUE(foundRun == false);
    }

    TEST_F(DeploymentToolTestsDeviceFarm, DeploymentToolTestsDeviceFarm_TestMisc)
    {
        // - test GetIdFromARN function
        AZStd::string projectId = mDeviceFarmDriver->GetIdFromARN("arn:aws:devicefarm:us-west-2:705582597265:project:2c21a412-bb7b-4657-a28c-d7d78b3888f7");
        ASSERT_TRUE(projectId == "2c21a412-bb7b-4657-a28c-d7d78b3888f7");
        AZStd::string runId = mDeviceFarmDriver->GetIdFromARN("arn:aws:devicefarm:us-west-2:705582597265:run:1c21a412-bb7b-4657-a28c-d7d78b3888f7");
        ASSERT_TRUE(runId == "1c21a412-bb7b-4657-a28c-d7d78b3888f7");

        // - test GetAwsConsoleJobUrl function
        AZStd::string url = mDeviceFarmDriver->GetAwsConsoleJobUrl(
            "arn:aws:devicefarm:us-west-2:705582597265:project:2c21a412-bb7b-4657-a28c-d7d78b3888f7",
            "arn:aws:devicefarm:us-west-2:705582597265:run:1c21a412-bb7b-4657-a28c-d7d78b3888f7");
        AZStd::string expectedUrl = "https://us-west-2.console.aws.amazon.com/devicefarm/home?region=us-west-2#/projects/2c21a412-bb7b-4657-a28c-d7d78b3888f7/runs/1c21a412-bb7b-4657-a28c-d7d78b3888f7";
        ASSERT_TRUE(url == expectedUrl);
    }

} // namespace DeployTool
#endif // AZ_TESTS_ENABLED