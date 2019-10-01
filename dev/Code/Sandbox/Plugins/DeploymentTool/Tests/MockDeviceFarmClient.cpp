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

#include "MockDeviceFarmClient.h"

#include <aws/core/utils/Outcome.h>

#include <aws/devicefarm/model/CreateDevicePoolRequest.h>
#include <aws/devicefarm/model/CreateDevicePoolResult.h>
#include <aws/devicefarm/model/CreateProjectRequest.h>
#include <aws/devicefarm/model/CreateProjectResult.h>
#include <aws/devicefarm/model/CreateUploadRequest.h>
#include <aws/devicefarm/model/CreateUploadResult.h>
#include <aws/devicefarm/model/DeleteDevicePoolRequest.h>
#include <aws/devicefarm/model/DeleteDevicePoolResult.h>
#include <aws/devicefarm/model/DeleteProjectRequest.h>
#include <aws/devicefarm/model/DeleteProjectResult.h>
#include <aws/devicefarm/model/DeleteRunRequest.h>
#include <aws/devicefarm/model/DeleteRunResult.h>
#include <aws/devicefarm/model/GetUploadRequest.h>
#include <aws/devicefarm/model/GetUploadResult.h>
#include <aws/devicefarm/model/ListDevicePoolsRequest.h>
#include <aws/devicefarm/model/ListDevicePoolsResult.h>
#include <aws/devicefarm/model/ListDevicesRequest.h>
#include <aws/devicefarm/model/ListDevicesResult.h>
#include <aws/devicefarm/model/ListProjectsRequest.h>
#include <aws/devicefarm/model/ListProjectsResult.h>
#include <aws/devicefarm/model/ListRunsRequest.h>
#include <aws/devicefarm/model/ListRunsResult.h>
#include <aws/devicefarm/model/ScheduleRunRequest.h>
#include <aws/devicefarm/model/ScheduleRunResult.h>
#include <aws/devicefarm/model/ExecutionStatus.h>
#include <aws/devicefarm/model/UploadType.h>

namespace DeployTool
{

void MockDeviceFarmClient::Update()
{
    for (auto callback : m_asyncSimulatorCallbacks)
    {
        callback();
    }
    m_asyncSimulatorCallbacks.clear();
}

AZStd::string MockDeviceFarmClient::GenerateArn(const AZStd::string& arnType)
{
    return AZStd::string::format("arn:aws:devicefarm:us-west-2:705582597265:%s:00000000-0000-0000-0000-%012d", arnType.c_str(), m_generateArnCounter++);
}

void MockDeviceFarmClient::CreateDevicePoolAsync(
    const Aws::DeviceFarm::Model::CreateDevicePoolRequest& request,
    const Aws::DeviceFarm::CreateDevicePoolResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    // queue up a callback in the async simulator
    m_asyncSimulatorCallbacks.push_back([this, handler, request, context]
    {
        // Create the fake outcome
        Aws::DeviceFarm::Model::DevicePool devicePool;
        devicePool.SetName(request.GetName());
        devicePool.SetDescription(request.GetDescription());
        devicePool.SetArn(GenerateArn("devicepool").c_str());
        Aws::DeviceFarm::Model::CreateDevicePoolResult result;
        result.SetDevicePool(devicePool);
        Aws::DeviceFarm::Model::CreateDevicePoolOutcome outcome(result);
        m_devicePools.push_back(outcome.GetResult().GetDevicePool());
        handler(nullptr, request, outcome, context);
    });
}

void MockDeviceFarmClient::DeleteDevicePoolAsync(
    const Aws::DeviceFarm::Model::DeleteDevicePoolRequest& request,
    const Aws::DeviceFarm::DeleteDevicePoolResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    // queue up a callback in the async simulator
    m_asyncSimulatorCallbacks.push_back([this, handler, request, context]
    {
        bool deleted = false;
        for (auto i = m_devicePools.begin(); i != m_devicePools.end(); ++i)
        {
            if (i->GetArn() == request.GetArn())
            {
                m_devicePools.erase(i);
                deleted = true;
                break;
            }
        }
        if (deleted)
        {
            Aws::DeviceFarm::Model::DeleteDevicePoolResult result;
            Aws::DeviceFarm::Model::DeleteDevicePoolOutcome outcome(result);
            handler(nullptr, request, outcome, context);
        }
        else
        {
            Aws::DeviceFarm::Model::DeleteDevicePoolOutcome outcome(
                Aws::Client::AWSError<Aws::DeviceFarm::DeviceFarmErrors>(Aws::DeviceFarm::DeviceFarmErrors::NOT_FOUND,
                    "Missing element",
                    "arn not found",
                    false));
            handler(nullptr, request, outcome, context);
        }
    });
}

void MockDeviceFarmClient::CreateProjectAsync(
    const Aws::DeviceFarm::Model::CreateProjectRequest& request,
    const Aws::DeviceFarm::CreateProjectResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    // queue up a callback in the async simulator
    m_asyncSimulatorCallbacks.push_back([this, handler, request, context]
    {
        // Create the fake outcome
        Aws::DeviceFarm::Model::Project project;
        project.SetName(request.GetName());
        project.SetArn(GenerateArn("project").c_str());
        Aws::DeviceFarm::Model::CreateProjectResult result;
        result.SetProject(project);
        Aws::DeviceFarm::Model::CreateProjectOutcome outcome(result);
        m_projects.push_back(outcome.GetResult().GetProject());
        handler(nullptr, request, outcome, context);
    });
}

void MockDeviceFarmClient::DeleteProjectAsync(
    const Aws::DeviceFarm::Model::DeleteProjectRequest& request,
    const Aws::DeviceFarm::DeleteProjectResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    // queue up a callback in the async simulator
    m_asyncSimulatorCallbacks.push_back([this, handler, request, context]
    {
        bool deleted = false;
        for (auto i = m_projects.begin(); i != m_projects.end(); ++i)
        {
            if (i->GetArn() == request.GetArn())
            {
                m_projects.erase(i);
                deleted = true;
                break;
            }
        }
        if (deleted)
        {
            Aws::DeviceFarm::Model::DeleteProjectResult result;
            Aws::DeviceFarm::Model::DeleteProjectOutcome outcome(result);
            handler(nullptr, request, outcome, context);
        }
        else
        {
            Aws::DeviceFarm::Model::DeleteProjectOutcome outcome(
                Aws::Client::AWSError<Aws::DeviceFarm::DeviceFarmErrors>(Aws::DeviceFarm::DeviceFarmErrors::NOT_FOUND,
                    "Missing element",
                    "arn not found",
                    false));
            handler(nullptr, request, outcome, context);
        }
    });
}

void MockDeviceFarmClient::ListDevicesAsync(
    const Aws::DeviceFarm::Model::ListDevicesRequest& request,
    const Aws::DeviceFarm::ListDevicesResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    // queue up a callback in the async simulator
    m_asyncSimulatorCallbacks.push_back([this, handler, request, context]
    {
        // Create the fake outcome
        Aws::DeviceFarm::Model::ListDevicesResult result;
        Aws::DeviceFarm::Model::Device device;
        device.SetArn(GenerateArn("device").c_str());
        result.AddDevices(device);
        Aws::DeviceFarm::Model::ListDevicesOutcome outcome(result);
        handler(nullptr, request, outcome, context);
    });
}

void MockDeviceFarmClient::ListDevicePoolsAsync(
    const Aws::DeviceFarm::Model::ListDevicePoolsRequest& request,
    const Aws::DeviceFarm::ListDevicePoolsResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    // queue up a callback in the async simulator
    m_asyncSimulatorCallbacks.push_back([this, handler, request, context]
    {
        // Create the fake outcome
        Aws::DeviceFarm::Model::ListDevicePoolsResult result;
        result.SetDevicePools(m_devicePools);
        Aws::DeviceFarm::Model::ListDevicePoolsOutcome outcome(result);
        handler(nullptr, request, outcome, context);
    });
}

void MockDeviceFarmClient::ListProjectsAsync(
    const Aws::DeviceFarm::Model::ListProjectsRequest& request,
    const Aws::DeviceFarm::ListProjectsResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    // queue up a callback in the async simulator
    m_asyncSimulatorCallbacks.push_back([this, handler, request, context]
    {
        // Create the fake outcome
        Aws::DeviceFarm::Model::ListProjectsResult result;
        result.SetProjects(m_projects);
        Aws::DeviceFarm::Model::ListProjectsOutcome outcome(result);
        handler(nullptr, request, outcome, context);
    });
}

void MockDeviceFarmClient::ListRunsAsync(
    const Aws::DeviceFarm::Model::ListRunsRequest& request,
    const Aws::DeviceFarm::ListRunsResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    // queue up a callback in the async simulator
    m_asyncSimulatorCallbacks.push_back([this, handler, request, context]
    {    // Create the fake outcome
        Aws::DeviceFarm::Model::ListRunsResult result;
        result.SetRuns(m_runs);
        Aws::DeviceFarm::Model::ListRunsOutcome outcome(result);
        handler(nullptr, request, outcome, context);
    });
}

void MockDeviceFarmClient::CreateUploadAsync(
    const Aws::DeviceFarm::Model::CreateUploadRequest& request,
    const Aws::DeviceFarm::CreateUploadResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    // queue up a callback in the async simulator
    m_asyncSimulatorCallbacks.push_back([this, handler, request, context]
    {
        // Create the fake outcome
        Aws::DeviceFarm::Model::Upload upload;
        upload.SetUrl("https://testing-fake/url");
        upload.SetArn(GenerateArn("upload").c_str());
        Aws::DeviceFarm::Model::CreateUploadResult result;
        result.SetUpload(upload);
        Aws::DeviceFarm::Model::CreateUploadOutcome outcome(result);
        m_uploads.push_back(upload);
        handler(nullptr, request, outcome, context);
    });
}

void MockDeviceFarmClient::GetUploadAsync(
    const Aws::DeviceFarm::Model::GetUploadRequest& request,
    const Aws::DeviceFarm::GetUploadResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    // queue up a callback in the async simulator
    m_asyncSimulatorCallbacks.push_back([this, handler, request, context]
    {    
        // Create the fake outcome
        bool found = false;
        Aws::DeviceFarm::Model::GetUploadResult result;
        for (auto upload : m_uploads)
        {
            if (upload.GetArn() == request.GetArn())
            {
                result.SetUpload(upload);
                found = true;
                break;
            }
        }
        if (found)
        {
            Aws::DeviceFarm::Model::GetUploadOutcome outcome(result);
            handler(nullptr, request, outcome, context);
        }
        else
        {
            Aws::DeviceFarm::Model::GetUploadOutcome outcome(
                Aws::Client::AWSError<Aws::DeviceFarm::DeviceFarmErrors>(Aws::DeviceFarm::DeviceFarmErrors::NOT_FOUND,
                    "Missing element",
                    "arn not found",
                    false));
            handler(nullptr, request, outcome, context);
        }
    });
}

void MockDeviceFarmClient::ScheduleRunAsync(
    const Aws::DeviceFarm::Model::ScheduleRunRequest& request,
    const Aws::DeviceFarm::ScheduleRunResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    // Create the fake outcome
    Aws::DeviceFarm::Model::Run run;
    run.SetName(request.GetName());
    run.SetArn(GenerateArn("run").c_str());
    Aws::DeviceFarm::Model::ScheduleRunResult result;
    result.SetRun(run);
    Aws::DeviceFarm::Model::ScheduleRunOutcome outcome(result);

    // queue up a callback in the async simulator
    m_asyncSimulatorCallbacks.push_back([this, handler, request, outcome, context]
    {
        m_runs.push_back(outcome.GetResult().GetRun());
        handler(nullptr, request, outcome, context);
    });
}

void MockDeviceFarmClient::DeleteRunAsync(
    const Aws::DeviceFarm::Model::DeleteRunRequest& request,
    const Aws::DeviceFarm::DeleteRunResponseReceivedHandler& handler,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    // queue up a callback in the async simulator
    m_asyncSimulatorCallbacks.push_back([this, handler, request, context]
    {
        bool deleted = false;
        for (auto i = m_runs.begin(); i != m_runs.end(); ++i)
        {
            if (i->GetArn() == request.GetArn())
            {
                m_runs.erase(i);
                deleted = true;
                break;
            }
        }
        if (deleted)
        {
            Aws::DeviceFarm::Model::DeleteRunResult result;
            Aws::DeviceFarm::Model::DeleteRunOutcome outcome(result);
            handler(nullptr, request, outcome, context);
        }
        else
        {
            Aws::DeviceFarm::Model::DeleteRunOutcome outcome(
                Aws::Client::AWSError<Aws::DeviceFarm::DeviceFarmErrors>(Aws::DeviceFarm::DeviceFarmErrors::NOT_FOUND,
                "Missing element",
                "arn not found",
                false));
            handler(nullptr, request, outcome, context);
        }
    });
}

} // namespace DeployTool
