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

// These interferes with <aws/devicefarm/DeviceFarmClient.h> ...
#pragma push_macro("IN")
#pragma push_macro("GetJob")
#undef IN
#undef GetJob
#include <aws/devicefarm/DeviceFarmClient.h>
#pragma pop_macro("IN")
#pragma pop_macro("GetJob")

namespace DeployTool
{

//!  Device Farm Client Interface
/*!
*/
class IDeviceFarmClient
{
public:

    virtual void CreateDevicePoolAsync(
        const Aws::DeviceFarm::Model::CreateDevicePoolRequest& request,
        const Aws::DeviceFarm::CreateDevicePoolResponseReceivedHandler& handler,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) = 0;

    virtual void DeleteDevicePoolAsync(
        const Aws::DeviceFarm::Model::DeleteDevicePoolRequest& request,
        const Aws::DeviceFarm::DeleteDevicePoolResponseReceivedHandler& handler,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) = 0;

    virtual void CreateProjectAsync(
        const Aws::DeviceFarm::Model::CreateProjectRequest& request,
        const Aws::DeviceFarm::CreateProjectResponseReceivedHandler& handler,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) = 0;

    virtual void DeleteProjectAsync(
        const Aws::DeviceFarm::Model::DeleteProjectRequest& request,
        const Aws::DeviceFarm::DeleteProjectResponseReceivedHandler& handler,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) = 0;

    virtual void ListDevicesAsync(
        const Aws::DeviceFarm::Model::ListDevicesRequest& request,
        const Aws::DeviceFarm::ListDevicesResponseReceivedHandler& handler,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) = 0;

    virtual void ListDevicePoolsAsync(
        const Aws::DeviceFarm::Model::ListDevicePoolsRequest& request,
        const Aws::DeviceFarm::ListDevicePoolsResponseReceivedHandler& handler,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) = 0;

    virtual void ListProjectsAsync(
        const Aws::DeviceFarm::Model::ListProjectsRequest& request,
        const Aws::DeviceFarm::ListProjectsResponseReceivedHandler& handler,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) = 0;

    virtual void ListRunsAsync(
        const Aws::DeviceFarm::Model::ListRunsRequest& request,
        const Aws::DeviceFarm::ListRunsResponseReceivedHandler& handler,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) = 0;

    virtual void CreateUploadAsync(
        const Aws::DeviceFarm::Model::CreateUploadRequest& request,
        const Aws::DeviceFarm::CreateUploadResponseReceivedHandler& handler,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) = 0;

    virtual void GetUploadAsync(
        const Aws::DeviceFarm::Model::GetUploadRequest& request,
        const Aws::DeviceFarm::GetUploadResponseReceivedHandler& handler,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) = 0;

    virtual void ScheduleRunAsync(
        const Aws::DeviceFarm::Model::ScheduleRunRequest& request,
        const Aws::DeviceFarm::ScheduleRunResponseReceivedHandler& handler,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) = 0;

    virtual void DeleteRunAsync(
        const Aws::DeviceFarm::Model::DeleteRunRequest& request,
        const Aws::DeviceFarm::DeleteRunResponseReceivedHandler& handler,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context = nullptr) = 0;
};

} // namespace DeployTool
