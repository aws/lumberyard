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

#include "ConfigFileContainer.h"

enum class PlatformOptions : unsigned char; 

// interface to read/modify bootstrap.cfg
class BootstrapConfigContainer
    : public ConfigFileContainer
{
public:
    BootstrapConfigContainer();
    ~BootstrapConfigContainer();

    StringOutcome ApplyConfiguration(const DeploymentConfig& deploymentConfig) override;

    AZStd::string GetHostAssetsType() const;
    AZStd::string GetAssetsTypeForPlatform(PlatformOptions platform) const;
    AZStd::string GetGameFolder() const;
    AZStd::string GetRemoteIP() const;


private:
    AZ_DISABLE_COPY_MOVE(BootstrapConfigContainer);
};

