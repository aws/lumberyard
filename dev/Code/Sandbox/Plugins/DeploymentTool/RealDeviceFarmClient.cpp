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

#include "RealDeviceFarmClient.h"

namespace DeployTool
{

RealDeviceFarmClient::RealDeviceFarmClient(const Aws::Auth::AWSCredentials& credentials, const Aws::Client::ClientConfiguration& clientConfiguration)
{
    mDeviceFarmClient.reset(new Aws::DeviceFarm::DeviceFarmClient(credentials, clientConfiguration));
}

RealDeviceFarmClient::~RealDeviceFarmClient()
{
}

void RealDeviceFarmClient::CreateDevicePoolAsync(
    const Aws::DeviceFarm::Model::CreateDevicePoolRequest& request,
    const Aws::DeviceFarm::CreateDevicePoolResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    mDeviceFarmClient->CreateDevicePoolAsync(request, handler, context);
}

void RealDeviceFarmClient::DeleteDevicePoolAsync(
    const Aws::DeviceFarm::Model::DeleteDevicePoolRequest& request,
    const Aws::DeviceFarm::DeleteDevicePoolResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    mDeviceFarmClient->DeleteDevicePoolAsync(request, handler, context);
}

void RealDeviceFarmClient::CreateProjectAsync(
    const Aws::DeviceFarm::Model::CreateProjectRequest& request,
    const Aws::DeviceFarm::CreateProjectResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    mDeviceFarmClient->CreateProjectAsync(request, handler, context);
}

void RealDeviceFarmClient::DeleteProjectAsync(
    const Aws::DeviceFarm::Model::DeleteProjectRequest& request,
    const Aws::DeviceFarm::DeleteProjectResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    mDeviceFarmClient->DeleteProjectAsync(request, handler, context);
}

void RealDeviceFarmClient::ListDevicesAsync(
    const Aws::DeviceFarm::Model::ListDevicesRequest& request,
    const Aws::DeviceFarm::ListDevicesResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    mDeviceFarmClient->ListDevicesAsync(request, handler, context);
}

void RealDeviceFarmClient::ListDevicePoolsAsync(
    const Aws::DeviceFarm::Model::ListDevicePoolsRequest& request,
    const Aws::DeviceFarm::ListDevicePoolsResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    mDeviceFarmClient->ListDevicePoolsAsync(request, handler, context);
}

void RealDeviceFarmClient::ListProjectsAsync(
    const Aws::DeviceFarm::Model::ListProjectsRequest& request,
    const Aws::DeviceFarm::ListProjectsResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    mDeviceFarmClient->ListProjectsAsync(request, handler, context);
}

void RealDeviceFarmClient::ListRunsAsync(
    const Aws::DeviceFarm::Model::ListRunsRequest& request,
    const Aws::DeviceFarm::ListRunsResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    mDeviceFarmClient->ListRunsAsync(request, handler, context);
}

void RealDeviceFarmClient::CreateUploadAsync(
    const Aws::DeviceFarm::Model::CreateUploadRequest& request,
    const Aws::DeviceFarm::CreateUploadResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    mDeviceFarmClient->CreateUploadAsync(request, handler, context);
}

void RealDeviceFarmClient::GetUploadAsync(
    const Aws::DeviceFarm::Model::GetUploadRequest& request,
    const Aws::DeviceFarm::GetUploadResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    mDeviceFarmClient->GetUploadAsync(request, handler, context);
}

void RealDeviceFarmClient::ScheduleRunAsync(
    const Aws::DeviceFarm::Model::ScheduleRunRequest& request,
    const Aws::DeviceFarm::ScheduleRunResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    mDeviceFarmClient->ScheduleRunAsync(request, handler, context);
}

void RealDeviceFarmClient::DeleteRunAsync(
    const Aws::DeviceFarm::Model::DeleteRunRequest& request,
    const Aws::DeviceFarm::DeleteRunResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    mDeviceFarmClient->DeleteRunAsync(request, handler, context);
}

} // namespace DeployTool
