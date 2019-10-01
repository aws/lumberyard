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

#include "DeviceFarmDriver.h"
#include "DeviceFarmClientInterface.h"

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

#include <aws/core/utils/Outcome.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <aws/core/http/HttpClientFactory.h> 
#include <aws/core/http/HttpClient.h>


namespace DeployTool
{

// There seems to be some sort of problem with AWS Device Farm code that is causing this warning to pop up, so just disable it.
// warning C4291: 'void *operator new(size_t,const AZ::Internal::AllocatorDummy *)': no matching operator delete found;
//   memory will not be freed if initialization throws an exception
#pragma warning (disable : 4291)

DeviceFarmDriver::DeviceFarmDriver(std::shared_ptr<DeployTool::IDeviceFarmClient> deviceFarmClient)
    : m_deviceFarmClient(deviceFarmClient)
{
}

DeviceFarmDriver::~DeviceFarmDriver()
{
}

bool DeviceFarmDriver::CreateDevicePool(const AZStd::string& devicePoolName, const AZStd::string& projectArn, const AZStd::string& poolDescription, const AZStd::vector<AZStd::string>& deviceArns, CreateDevicePoolCallback& callback)
{
    Aws::DeviceFarm::Model::CreateDevicePoolRequest createDevicePoolRequest;
    createDevicePoolRequest.SetName(devicePoolName.c_str());
    createDevicePoolRequest.SetDescription(poolDescription.c_str());
    createDevicePoolRequest.SetProjectArn(projectArn.c_str());

    // Add rules for selected devices
    Aws::DeviceFarm::Model::Rule arnRule;
    arnRule.SetAttribute(Aws::DeviceFarm::Model::DeviceAttribute::ARN);
#pragma push_macro("IN")
#undef IN
    arnRule.SetOperator(Aws::DeviceFarm::Model::RuleOperator::IN);
#pragma pop_macro("IN")

    // Create a comma separated list of ARN strings and set it as one rule.
    AZStd::string commaSeparatedList;
    AZStd::string separator;

    for (auto deviceArn : deviceArns)
    {
        commaSeparatedList = AZStd::string::format("%s%s\"%s\"", commaSeparatedList.c_str(), separator.c_str(), deviceArn.c_str());
        separator = ",";
    }

    arnRule.SetValue(AZStd::string::format("[%s]", commaSeparatedList.c_str()).c_str());
    createDevicePoolRequest.AddRules(arnRule);

    m_deviceFarmClient->CreateDevicePoolAsync(
        createDevicePoolRequest,
        [callback, devicePoolName](
            const Aws::DeviceFarm::DeviceFarmClient* createDevicePoolClient,
            const Aws::DeviceFarm::Model::CreateDevicePoolRequest& createDevicePoolRequest,
            const Aws::DeviceFarm::Model::CreateDevicePoolOutcome& createDevicePoolOutcome,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
    {
        const Aws::DeviceFarm::Model::DevicePool deviceFarmDevicePool = createDevicePoolOutcome.GetResult().GetDevicePool();
        DevicePool devicePool(
            devicePoolName,
            deviceFarmDevicePool.GetArn().c_str());
        callback(createDevicePoolOutcome.IsSuccess(), createDevicePoolOutcome.GetError().GetMessage().c_str(), devicePool);
    });

    return true;
}

bool DeviceFarmDriver::DeleteDevicePool(const AZStd::string& devicePoolArn, DeleteDevicePoolCallback& callback)
{
    Aws::DeviceFarm::Model::DeleteDevicePoolRequest deleteDevicePoolRequest;
    deleteDevicePoolRequest.SetArn(devicePoolArn.c_str());

    m_deviceFarmClient->DeleteDevicePoolAsync(
        deleteDevicePoolRequest,
        [callback](
            const Aws::DeviceFarm::DeviceFarmClient* deleteDevicePoolClient,
            const Aws::DeviceFarm::Model::DeleteDevicePoolRequest& deleteDevicePoolRequest,
            const Aws::DeviceFarm::Model::DeleteDevicePoolOutcome& deleteDevicePoolOutcome,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
    {
        callback(deleteDevicePoolOutcome.IsSuccess(), deleteDevicePoolOutcome.GetError().GetMessage().c_str());
    });

    return true;
}

bool DeviceFarmDriver::CreateProject(const AZStd::string& projectName, CreateProjectCallback& callback)
{
    Aws::DeviceFarm::Model::CreateProjectRequest createProjectRequest;
    createProjectRequest.SetName(projectName.c_str());

    m_deviceFarmClient->CreateProjectAsync(
        createProjectRequest,
        [callback, projectName](
            const Aws::DeviceFarm::DeviceFarmClient* createProjectClient,
            const Aws::DeviceFarm::Model::CreateProjectRequest& createProjectRequest,
            const Aws::DeviceFarm::Model::CreateProjectOutcome& createProjectOutcome,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
    {
        const Aws::DeviceFarm::Model::Project deviceFarmProject = createProjectOutcome.GetResult().GetProject();
        Project project(
            projectName,
            deviceFarmProject.GetArn().c_str());
        callback(createProjectOutcome.IsSuccess(), createProjectOutcome.GetError().GetMessage().c_str(), project);
    });

    return true;
}

bool DeviceFarmDriver::DeleteProject(const AZStd::string& projectArn, DeleteProjectCallback& callback)
{
    Aws::DeviceFarm::Model::DeleteProjectRequest deleteProjectRequest;
    deleteProjectRequest.SetArn(projectArn.c_str());

    m_deviceFarmClient->DeleteProjectAsync(
        deleteProjectRequest,
        [callback](
            const Aws::DeviceFarm::DeviceFarmClient* deleteProjectClient,
            const Aws::DeviceFarm::Model::DeleteProjectRequest& deleteProjectRequest,
            const Aws::DeviceFarm::Model::DeleteProjectOutcome& deleteProjectOutcome,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
    {
        callback(deleteProjectOutcome.IsSuccess(), deleteProjectOutcome.GetError().GetMessage().c_str());
    });

    return true;
}

bool DeviceFarmDriver::ListDevices(const AZStd::string& projectArn, ListDevicesCallback& callback)
{
    Aws::DeviceFarm::Model::ListDevicesRequest request;
    request.SetArn(projectArn.c_str());
    m_deviceFarmClient->ListDevicesAsync(
        request,
        [callback](const Aws::DeviceFarm::DeviceFarmClient* deviceFarmClient, const Aws::DeviceFarm::Model::ListDevicesRequest& listDevicesRequest, const Aws::DeviceFarm::Model::ListDevicesOutcome& listDevicesOutcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
    {
        AZStd::vector<Device> devices;
        devices.clear();
        devices.reserve(listDevicesOutcome.GetResult().GetDevices().size());
        for (auto device : listDevicesOutcome.GetResult().GetDevices())
        {            
            devices.push_back(
                Device(
                    device.GetName().c_str(),
                    device.GetArn().c_str(),
                    Aws::DeviceFarm::Model::DevicePlatformMapper::GetNameForDevicePlatform(device.GetPlatform()).c_str(),
                    device.GetOs().c_str(),
                    Aws::DeviceFarm::Model::DeviceFormFactorMapper::GetNameForDeviceFormFactor(device.GetFormFactor()).c_str(),
                    device.GetResolution().GetWidth(),
                    device.GetResolution().GetHeight()));
        }
        callback(listDevicesOutcome.IsSuccess(), listDevicesOutcome.GetError().GetMessage().c_str(), devices);
    });

    return true;
}

bool DeviceFarmDriver::ListDevicePools(const AZStd::string& projectArn, ListDevicePoolsCallback& callback)
{
    Aws::DeviceFarm::Model::ListDevicePoolsRequest request;
    request.SetArn(projectArn.c_str());
    m_deviceFarmClient->ListDevicePoolsAsync(
        request,
        [callback](const Aws::DeviceFarm::DeviceFarmClient* deviceFarmClient, const Aws::DeviceFarm::Model::ListDevicePoolsRequest& listDevicePoolsRequest, const Aws::DeviceFarm::Model::ListDevicePoolsOutcome& listDevicePoolsOutcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
    {
        AZStd::vector<DevicePool> devicePools;
        devicePools.clear();
        devicePools.reserve(listDevicePoolsOutcome.GetResult().GetDevicePools().size());
        for (auto devicePool : listDevicePoolsOutcome.GetResult().GetDevicePools())
        {
            devicePools.push_back(DevicePool(devicePool.GetName().c_str(), devicePool.GetArn().c_str()));
        }
        callback(listDevicePoolsOutcome.IsSuccess(), listDevicePoolsOutcome.GetError().GetMessage().c_str(), devicePools);
    });

    return true;
}

bool DeviceFarmDriver::ListProjects(ListProjectsCallback& callback)
{
    m_deviceFarmClient->ListProjectsAsync(
        Aws::DeviceFarm::Model::ListProjectsRequest(),
        [callback](const Aws::DeviceFarm::DeviceFarmClient* deviceFarmClient, const Aws::DeviceFarm::Model::ListProjectsRequest& listProjectsRequest, const Aws::DeviceFarm::Model::ListProjectsOutcome& listProjectsOutcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
        {
            AZStd::vector<Project> projects;
            projects.clear();
            projects.reserve(listProjectsOutcome.GetResult().GetProjects().size());
            for (auto project : listProjectsOutcome.GetResult().GetProjects())
            {
                projects.push_back(Project(project.GetName().c_str(), project.GetArn().c_str()));
            }
            callback(listProjectsOutcome.IsSuccess(), listProjectsOutcome.GetError().GetMessage().c_str(), projects);
        });

    return true;
}

bool DeviceFarmDriver::ListRuns(const AZStd::string& projectArn, ListRunsCallback& callback)
{
    Aws::DeviceFarm::Model::ListRunsRequest request;
    request.SetArn(projectArn.c_str());
    m_deviceFarmClient->ListRunsAsync(
        request,
        [callback](const Aws::DeviceFarm::DeviceFarmClient* deviceFarmClient, const Aws::DeviceFarm::Model::ListRunsRequest& listRunsRequest, const Aws::DeviceFarm::Model::ListRunsOutcome& listRunsOutcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
    {
        AZStd::vector<Run> runs;
        runs.clear();
        runs.reserve(listRunsOutcome.GetResult().GetRuns().size());
        for (auto run : listRunsOutcome.GetResult().GetRuns())
        {            
            AZStd::string result = Aws::DeviceFarm::Model::ExecutionResultMapper::GetNameForExecutionResult(run.GetResult()).c_str();
            AZStd::string testType = Aws::DeviceFarm::Model::TestTypeMapper::GetNameForTestType(run.GetType()).c_str();
            runs.push_back(
                Run(run.GetName().c_str(),
                    run.GetArn().c_str(),
                    result,
                    run.GetTotalJobs(),
                    run.GetCompletedJobs(),
                    run.GetDevicePoolArn().c_str(),
                    testType));
        }
        callback(listRunsOutcome.IsSuccess(), listRunsOutcome.GetError().GetMessage().c_str(), runs);
    });

    return true;
}

bool DeviceFarmDriver::CreateUpload(const AZStd::string& appPath, const AZStd::string& projectArn, CreateUploadCallback& callback)
{
    // Get just the filename part without the full path.
    AZStd::string fullFilename = appPath;
    AzFramework::StringFunc::Path::GetFullFileName(appPath.c_str(), fullFilename);

    AZStd::string extension = appPath;
    AzFramework::StringFunc::Path::GetExtension(appPath.c_str(), extension);

    Aws::DeviceFarm::Model::CreateUploadRequest createUploadRequest;
    createUploadRequest.SetType(AzFramework::StringFunc::Equal(extension.c_str(), ".apk", false) ? Aws::DeviceFarm::Model::UploadType::ANDROID_APP : Aws::DeviceFarm::Model::UploadType::IOS_APP);
    createUploadRequest.SetName(fullFilename.c_str());
    createUploadRequest.SetProjectArn(projectArn.c_str());
    createUploadRequest.SetContentType("application/octet-stream");

    m_deviceFarmClient->CreateUploadAsync(
        createUploadRequest,
        [callback, appPath](
            const Aws::DeviceFarm::DeviceFarmClient* createUploadClient,
            const Aws::DeviceFarm::Model::CreateUploadRequest& createUploadRequest,
            const Aws::DeviceFarm::Model::CreateUploadOutcome& createUploadOutcome,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
    {
        const Aws::DeviceFarm::Model::Upload deviceFarmUpload = createUploadOutcome.GetResult().GetUpload();
        Upload upload(
            appPath,
            deviceFarmUpload.GetArn().c_str(),
            Aws::DeviceFarm::Model::UploadStatusMapper::GetNameForUploadStatus(deviceFarmUpload.GetStatus()).c_str(),
            deviceFarmUpload.GetUrl().c_str());
        callback(createUploadOutcome.IsSuccess(), createUploadOutcome.GetError().GetMessage().c_str(), upload);
    });

    return true;
}

bool DeviceFarmDriver::GetUpload(const AZStd::string& uploadArn, GetUploadCallback& callback)
{
    Aws::DeviceFarm::Model::GetUploadRequest getUploadRequest;
    getUploadRequest.SetArn(uploadArn.c_str());

    m_deviceFarmClient->GetUploadAsync(
        getUploadRequest,
        [callback](
            const Aws::DeviceFarm::DeviceFarmClient* getUploadClient,
            const Aws::DeviceFarm::Model::GetUploadRequest& getUploadRequest,
            const Aws::DeviceFarm::Model::GetUploadOutcome& getUploadOutcome,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
    {
        const Aws::DeviceFarm::Model::Upload deviceFarmUpload = getUploadOutcome.GetResult().GetUpload();
        Upload upload(
            deviceFarmUpload.GetName().c_str(),
            deviceFarmUpload.GetArn().c_str(),
            Aws::DeviceFarm::Model::UploadStatusMapper::GetNameForUploadStatus(deviceFarmUpload.GetStatus()).c_str(),
            deviceFarmUpload.GetUrl().c_str());
        callback(getUploadOutcome.IsSuccess(), getUploadOutcome.GetError().GetMessage().c_str(), upload);
    });

    return true;
}

bool DeviceFarmDriver::ScheduleRun(const AZStd::string& runName, const AZStd::string& uploadArn, const AZStd::string& projectArn, const AZStd::string& devicePoolArn, int jobTimeoutMinuites, ScheduleRunCallback& callback)
{
    Aws::DeviceFarm::Model::ScheduleRunRequest scheduleRunRequest;
    scheduleRunRequest.SetProjectArn(projectArn.c_str());
    scheduleRunRequest.SetAppArn(uploadArn.c_str());
    scheduleRunRequest.SetName(runName.c_str());
    scheduleRunRequest.SetDevicePoolArn(devicePoolArn.c_str());

    // Fuzz is the only thing supported right now.
    Aws::DeviceFarm::Model::ScheduleRunTest scheduleRunTest;
    scheduleRunTest.SetType(Aws::DeviceFarm::Model::TestType::BUILTIN_FUZZ);
    scheduleRunTest.AddParameters("event_count", "600");
    scheduleRunTest.AddParameters("throttle", "1000");
    scheduleRunRequest.SetTest(scheduleRunTest);

    // Set the timeout limit
    Aws::DeviceFarm::Model::ExecutionConfiguration executionConfiguration;
    executionConfiguration.SetJobTimeoutMinutes(jobTimeoutMinuites);
    scheduleRunRequest.SetExecutionConfiguration(executionConfiguration);

    m_deviceFarmClient->ScheduleRunAsync(
        scheduleRunRequest,
        [callback](
            const Aws::DeviceFarm::DeviceFarmClient* scheduleRunRequestClient,
            const Aws::DeviceFarm::Model::ScheduleRunRequest& scheduleRunRequest,
            const Aws::DeviceFarm::Model::ScheduleRunOutcome& scheduleRunOutcome,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
    {
        const Aws::DeviceFarm::Model::Run deviceFarmRun = scheduleRunOutcome.GetResult().GetRun();
        AZStd::string result = Aws::DeviceFarm::Model::ExecutionResultMapper::GetNameForExecutionResult(deviceFarmRun.GetResult()).c_str();
        AZStd::string testType = Aws::DeviceFarm::Model::TestTypeMapper::GetNameForTestType(deviceFarmRun.GetType()).c_str();
        Run run(
            scheduleRunRequest.GetName().c_str(),
            deviceFarmRun.GetArn().c_str(),
            result,
            deviceFarmRun.GetTotalJobs(),
            deviceFarmRun.GetCompletedJobs(),
            deviceFarmRun.GetDevicePoolArn().c_str(),
            testType);
        callback(scheduleRunOutcome.IsSuccess(), scheduleRunOutcome.GetError().GetMessage().c_str(), run);
    });

    return true;
}

bool DeviceFarmDriver::DeleteRun(const AZStd::string& runArn, DeleteRunCallback& callback)
{
    Aws::DeviceFarm::Model::DeleteRunRequest deleteRunRequest;
    deleteRunRequest.SetArn(runArn.c_str());

    m_deviceFarmClient->DeleteRunAsync(
        deleteRunRequest,
        [callback](
            const Aws::DeviceFarm::DeviceFarmClient* deleteRunRequestClient,
            const Aws::DeviceFarm::Model::DeleteRunRequest& deleteRunRequest,
            const Aws::DeviceFarm::Model::DeleteRunOutcome& deleteRunOutcome,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
    {
        callback(deleteRunOutcome.IsSuccess(), deleteRunOutcome.GetError().GetMessage().c_str());
    });

    return true;
}

bool DeviceFarmDriver::SendUpload(const Upload& upload)
{
    // Try to read the file first.
    AZ::IO::FileIOStream fileStream(upload.name.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary);
    if (!fileStream.IsOpen())
    {
        return false;
    }

    AZ::u64 fileSize = fileStream.GetLength();
    AZStd::vector<char> buffer;
    buffer.resize(fileSize + 1);
    buffer[fileSize] = 0;
    auto bytesRead = fileStream.Read(fileSize, buffer.data());
    if (bytesRead != fileSize)
    {
        return false;
    }

    Aws::Client::ClientConfiguration clientConfiguration = Aws::Client::ClientConfiguration();

    // Device Farm only works in the lovely state of Oregon
    clientConfiguration.region = Aws::Region::US_WEST_2;

    // Set timeout for the file upload to something very long.
    clientConfiguration.requestTimeoutMs = 60 * 1000;

    std::shared_ptr<Aws::Http::HttpClient> httpClient = Aws::Http::CreateHttpClient(clientConfiguration);

    // The url comes encoded from the CreateUpload, but CreateHttpRequest will encode it, so decode it now to avoid a double encode.
    const Aws::String url = Aws::Utils::StringUtils::URLDecode(upload.url.c_str()).c_str();
    auto httpRequest(Aws::Http::CreateHttpRequest(url, Aws::Http::HttpMethod::HTTP_PUT, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod));

    const std::string contentLengthString = std::to_string(fileSize);
    httpRequest->SetContentLength(contentLengthString.c_str());
    auto body = std::make_shared<Aws::StringStream>();
    body->write(buffer.data(), fileSize);
    httpRequest->AddContentBody(body);

    httpRequest->SetContentType("application/octet-stream");

    const auto httpResponse(httpClient->MakeRequest(*httpRequest, nullptr, nullptr));

    return httpResponse->GetResponseCode() == Aws::Http::HttpResponseCode::OK;
}

AZStd::string DeviceFarmDriver::GetIdFromARN(const AZStd::string& arn)
{
    // example arn arn:aws:devicefarm:us-west-2:671993552890:project:3c36434c-6237-4c04-996e-9ab002724e8a
    // extract the last id portion of the an arn
    AZStd::string result = "";
    AZStd::vector<AZStd::string> tokens;
    const bool keepEmptyStrings = true;
    AzFramework::StringFunc::Tokenize(arn.c_str(), tokens, ':', keepEmptyStrings);
    if (tokens.size() >= 7)
    {
        result = tokens[6];
    }
    return result;
}

AZStd::string DeviceFarmDriver::GetAwsConsoleJobUrl(const AZStd::string& projectArn, const AZStd::string& runArn)
{
    const AZStd::string projectId = GetIdFromARN(projectArn);
    AZStd::string runId = GetIdFromARN(runArn);

    // The run Id needs to be further split down by a / to get the second part
    AZStd::vector<AZStd::string> tokens;
    AzFramework::StringFunc::Tokenize(runId.c_str(), tokens, '/');
    if (tokens.size() >= 2)
    {
        runId = tokens[1];
    }

    return AZStd::string::format("https://us-west-2.console.aws.amazon.com/devicefarm/home?region=us-west-2#/projects/%s/runs/%s",
        projectId.c_str(),
        runId.c_str());
}

#pragma warning (default : 4291)

} // namespace DeployTool
