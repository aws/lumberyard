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

#include "DeployWorkerBase.h"
#include "DeviceFarmDriver.h"

namespace DeployTool
{

// 
//!  Deploy worker for Device Farm
/*!
* Utility to build/deploy projects to the Aws Device Farm
*/
class DeployWorkerDeviceFarm : public DeployWorkerBase
{
    // States for "Deploying" an app to the Aws Device Farm
    enum class DeviceFarmDeployState
    {
        Idle,
        UploadCreate,
        UploadCreating,
        UploadCreated,
        UploadSending,
        UploadCheckingStatus,
        UploadCheckedStatus,
        SchedulingRun
    };

public:
    DeployWorkerDeviceFarm(
        std::shared_ptr<DeployTool::DeviceFarmDriver> deviceFarmDriver,
        int timeoutInMinutes,
        const AZStd::string& projectArn,
        const AZStd::string& devicePoolArn);
    ~DeployWorkerDeviceFarm();

    const char* GetSystemConfigFileName() const override;
    bool GetConnectedDevices(DeviceMap& connectedDevices) const override;


protected:
    AZStd::string GetWafBuildArgs() const override;
    AZStd::string GetWafDeployArgs() const override;

    StringOutcome Prepare() override;
    void StartDeploy() override;
    StringOutcome Launch() override;

    // Deploy state update functions
    void DeployStateUpdateUploadCreate();
    void DeployStateUpdateUploadCreated();
    void DeployStateUpdateUploadCheckedStatus();

private:
    AZ_DISABLE_COPY_MOVE(DeployWorkerDeviceFarm);

    // Main update loop while Launching (deploying to device farm).
    void Update();

    AZStd::string m_appPath;
    DeviceFarmDeployState m_deviceFarmDeployState = DeviceFarmDeployState::Idle;
    std::shared_ptr<DeployTool::DeviceFarmDriver> m_deviceFarmDriver;
    DeployTool::DeviceFarmDriver::Upload m_deviceFarmUpload;
    int m_timeoutInMinutes;
    AZStd::string m_projectArn;
    AZStd::string m_devicePoolArn;

    // Run the deploy worker in a thread so it doesn't freeze up the UI
    AZStd::thread m_deployWorkerThread;
};

} // namespace DeployTool