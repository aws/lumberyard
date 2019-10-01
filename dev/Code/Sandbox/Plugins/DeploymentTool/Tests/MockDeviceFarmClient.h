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

#pragma once
#include "DeploymentTool_precompiled.h"
#include <DeviceFarmClientInterface.h>

namespace DeployTool
{
    //!  Mock Device Farm Client
    /*!
    * A Mock Device Farm Client that implements enough functionality for some testing.
    * This can be dropped into the Device Farm Driver code as a Client to allow unit
    * tests to run.
    */
    class MockDeviceFarmClient : public IDeviceFarmClient
    {
        typedef std::function<void()> AsyncSimulatorCallback;

    public:
        MockDeviceFarmClient() {};
        virtual ~MockDeviceFarmClient() {};
       
        // Update function to drive simulation of async calls
        void Update();

        // Helper function to generate a new arn
        AZStd::string GenerateArn(const AZStd::string& arnType);

        // IDeviceFarmClient
        void CreateDevicePoolAsync(
            const Aws::DeviceFarm::Model::CreateDevicePoolRequest& request,
            const Aws::DeviceFarm::CreateDevicePoolResponseReceivedHandler& handler,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) override;

        void DeleteDevicePoolAsync(
            const Aws::DeviceFarm::Model::DeleteDevicePoolRequest& request,
            const Aws::DeviceFarm::DeleteDevicePoolResponseReceivedHandler& handler,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) override;

        void CreateProjectAsync(
            const Aws::DeviceFarm::Model::CreateProjectRequest& request,
            const Aws::DeviceFarm::CreateProjectResponseReceivedHandler& handler,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) override;

        void DeleteProjectAsync(
            const Aws::DeviceFarm::Model::DeleteProjectRequest& request,
            const Aws::DeviceFarm::DeleteProjectResponseReceivedHandler& handler,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) override;

        void ListDevicesAsync(
            const Aws::DeviceFarm::Model::ListDevicesRequest& request,
            const Aws::DeviceFarm::ListDevicesResponseReceivedHandler& handler,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) override;

        void ListDevicePoolsAsync(
            const Aws::DeviceFarm::Model::ListDevicePoolsRequest& request,
            const Aws::DeviceFarm::ListDevicePoolsResponseReceivedHandler& handler,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) override;

        void ListProjectsAsync(
            const Aws::DeviceFarm::Model::ListProjectsRequest& request,
            const Aws::DeviceFarm::ListProjectsResponseReceivedHandler& handler,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) override;

        void ListRunsAsync(
            const Aws::DeviceFarm::Model::ListRunsRequest& request,
            const Aws::DeviceFarm::ListRunsResponseReceivedHandler& handler,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) override;

        void CreateUploadAsync(
            const Aws::DeviceFarm::Model::CreateUploadRequest& request,
            const Aws::DeviceFarm::CreateUploadResponseReceivedHandler& handler,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) override;

        void GetUploadAsync(
            const Aws::DeviceFarm::Model::GetUploadRequest& request,
            const Aws::DeviceFarm::GetUploadResponseReceivedHandler& handler,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) override;

        void ScheduleRunAsync(
            const Aws::DeviceFarm::Model::ScheduleRunRequest& request,
            const Aws::DeviceFarm::ScheduleRunResponseReceivedHandler& handler,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) override;

        void DeleteRunAsync(
            const Aws::DeviceFarm::Model::DeleteRunRequest& request,
            const Aws::DeviceFarm::DeleteRunResponseReceivedHandler& handler,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) override;

    private:
        AZStd::vector<AsyncSimulatorCallback> m_asyncSimulatorCallbacks;
        int m_generateArnCounter = 0;

        Aws::Vector<Aws::DeviceFarm::Model::Project> m_projects;
        Aws::Vector<Aws::DeviceFarm::Model::DevicePool> m_devicePools;
        Aws::Vector<Aws::DeviceFarm::Model::Run> m_runs;
        Aws::Vector<Aws::DeviceFarm::Model::Upload> m_uploads;
    };

} // namespace DeployTool
