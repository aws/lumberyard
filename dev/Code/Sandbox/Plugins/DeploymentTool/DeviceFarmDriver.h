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


namespace DeployTool
{
class IDeviceFarmClient;

//!  Device Farm Driver
/*!
   Provides access to the Aws Device Farm. Encapsulates the Device Farm Client and 
   helper code for using the Device Farm.
*/
class DeviceFarmDriver
{

public:

    // Base class for device farm objects uniquely identified by an Arn.
    struct ARNBase
    {
        ARNBase() {}
        ARNBase(const AZStd::string& _name, const AZStd::string& _arn) : name(_name), arn(_arn) {}
        AZStd::string name;
        AZStd::string arn;
    };

    // Device
    struct Device : public ARNBase
    {
        Device(
            const AZStd::string& name,
            const AZStd::string& arn,
            const AZStd::string& _platform,
            const AZStd::string& _os,
            const AZStd::string& _formFactor,
            int _resolutionWidth,
            int _resolutionHeight)
            : ARNBase(name, arn), platform(_platform), os(_os), formFactor(_formFactor), resolutionWidth(_resolutionWidth), resolutionHeight(_resolutionHeight) {}
        AZStd::string platform;
        AZStd::string os;
        AZStd::string formFactor;
        int resolutionWidth;
        int resolutionHeight;
    };

    // Device Pool
    struct DevicePool : public ARNBase
    {
        DevicePool(const AZStd::string& name, const AZStd::string& arn) : ARNBase(name, arn) {}
    };

    // Project
    struct Project : public ARNBase
    {
        Project(const AZStd::string& name, const AZStd::string& arn) : ARNBase(name, arn) {}
    };

    // Run
    struct Run : public ARNBase
    {
        Run(
            const AZStd::string& name,
            const AZStd::string& arn,
            const AZStd::string& _result,
            int _totalJobs,
            int _completedJobs,
            const AZStd::string& _devicePoolArn,
            const AZStd::string& _testType)
            : ARNBase(name, arn)
            , result(_result)
            , totalJobs(_totalJobs)
            , completedJobs(_completedJobs)
            , devicePoolArn(_devicePoolArn)
            , testType(_testType) {}

        AZStd::string result;
        int totalJobs;
        int completedJobs;
        AZStd::string devicePoolArn;
        AZStd::string testType;
    };

    // Upload
    struct Upload : public ARNBase
    {
        Upload() {}
        Upload(const AZStd::string& name, const AZStd::string& arn, const AZStd::string& _status, const AZStd::string& _url)
            : ARNBase(name, arn), status(_status), url(_url) {}
        AZStd::string status = "NOT_SET";
        AZStd::string url;
    };

    DeviceFarmDriver(std::shared_ptr<DeployTool::IDeviceFarmClient> deviceFarmClient);
    ~DeviceFarmDriver();

    // Create a Device Pool.
    typedef const std::function<void(bool success, const AZStd::string& msg, const DevicePool& devicePool)> CreateDevicePoolCallback;
    bool CreateDevicePool(const AZStd::string& devicePoolName, const AZStd::string& projectArn, const AZStd::string& poolDescription, const AZStd::vector<AZStd::string>& deviceArns, CreateDevicePoolCallback& callback);

    // Delete a Device Pool.
    typedef const std::function<void(bool success, const AZStd::string& msg)> DeleteDevicePoolCallback;
    bool DeleteDevicePool(const AZStd::string& devicePoolArn, DeleteDevicePoolCallback& callback);

    // Create a Project.
    typedef const std::function<void(bool success, const AZStd::string& msg, const Project& project)> CreateProjectCallback;
    bool CreateProject(const AZStd::string& projectName, CreateProjectCallback& callback);

    // Delete a Project.
    typedef const std::function<void(bool success, const AZStd::string& msg)> DeleteProjectCallback;
    bool DeleteProject(const AZStd::string& projectArn, DeleteProjectCallback& callback);

    // Get all of the available devices.
    typedef const std::function<void(bool success, const AZStd::string& msg, AZStd::vector<Device>& devices)> ListDevicesCallback;
    bool ListDevices(const AZStd::string& projectArn, ListDevicesCallback& callback);

    // Get all of the available device pools for a project.
    typedef const std::function<void(bool success, const AZStd::string& msg, AZStd::vector<DevicePool>& devicePools)> ListDevicePoolsCallback;
    bool ListDevicePools(const AZStd::string& projectArn, ListDevicePoolsCallback& callback);

    // Get all of the available projects.
    typedef const std::function<void(bool success, const AZStd::string& msg, AZStd::vector<Project>& projects)> ListProjectsCallback;
    bool ListProjects(ListProjectsCallback& callback);

    // Get all of the current runs for a project.
    typedef const std::function<void(bool success, const AZStd::string& msg, AZStd::vector<Run>& runs)> ListRunsCallback;
    bool ListRuns(const AZStd::string& projectArn, ListRunsCallback& callback);

    // Create an Upload for an app.
    typedef const std::function<void(bool success, const AZStd::string& msg, const Upload& upload)> CreateUploadCallback;
    bool CreateUpload(const AZStd::string& appPath, const AZStd::string& projectArn, CreateUploadCallback& callback);

    // Get an Upload for an app, useful to query status while uploading.
    typedef const std::function<void(bool success, const AZStd::string& msg, const Upload& upload)> GetUploadCallback;
    bool GetUpload(const AZStd::string& uploadArn, GetUploadCallback& callback);

    // Schedule a new run to start.
    typedef const std::function<void(bool success, const AZStd::string& msg, const Run& run)> ScheduleRunCallback;
    bool ScheduleRun(const AZStd::string& runName, const AZStd::string& uploadArn, const AZStd::string& projectArn, const AZStd::string& devicePoolArn, int jobTimeoutMinuites, ScheduleRunCallback& callback);

    // Delete an existing run.
    typedef const std::function<void(bool success, const AZStd::string& msg)> DeleteRunCallback;
    bool DeleteRun(const AZStd::string& runArn, DeleteRunCallback& callback);

    // Helper function to send an upload after it has been created.
    bool SendUpload(const Upload& upload);

    // Helper function to extract the id part of an ARN.
    static AZStd::string GetIdFromARN(const AZStd::string& arn);

    // Helper function to format a Aws console url for a given run.
    static AZStd::string GetAwsConsoleJobUrl(const AZStd::string& projectArn, const AZStd::string& runArn);

private:

    // The Aws Device Farm Client Interface
    std::shared_ptr<DeployTool::IDeviceFarmClient> m_deviceFarmClient;
};

} // namespace DeployTool
