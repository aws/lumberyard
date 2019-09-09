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


// Utility to build/deploy projects for ios devices
class DeployWorkerIos : public DeployWorkerBase
{
public:
    DeployWorkerIos();
    ~DeployWorkerIos();

    const char* GetSystemConfigFileName() const override;
    bool GetConnectedDevices(DeviceMap& connectedDevices) const override;


protected:
    AZStd::string GetWafBuildArgs() const override;
    AZStd::string GetWafDeployArgs() const override;

    StringOutcome Prepare() override;
    void StartDeploy() override;
    StringOutcome Launch() override;


private:
    AZ_DISABLE_COPY_MOVE(DeployWorkerIos);

    AZStd::string m_xcodeProject;
    QProcess* m_xcodebuildProcess;
};
