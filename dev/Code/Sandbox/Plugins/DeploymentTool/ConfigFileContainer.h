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

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>


struct DeploymentConfig;

using StringOutcome = AZ::Outcome<AZStd::string, AZStd::string>;


StringOutcome ReadFile(const AZStd::string& file, AZStd::string& fileContents);


// general class to read/modify .cfg style config files
class ConfigFileContainer
{
public:
    ConfigFileContainer(const AZStd::string& filePath);
    virtual ~ConfigFileContainer() = default;

    virtual StringOutcome ApplyConfiguration(const DeploymentConfig& deploymentConfig);

    StringOutcome Load();
    StringOutcome Reload();
    StringOutcome Write() const;

    const AZStd::string& GetString(const AZStd::string& key, bool includeComments = false) const;
    bool GetBool(const AZStd::string& key, bool includeComments = false) const;

    void SetString(const AZStd::string& key, const AZStd::string& newValue);
    void SetBool(const AZStd::string& key, bool newValue);


private:
    AZ_DISABLE_COPY_MOVE(ConfigFileContainer);

    bool IsKeyInFile(const AZStd::string& key) const;


    AZStd::string m_filePath;
    AZStd::string m_fileContents;
    mutable AZStd::unordered_map<AZStd::string, AZStd::string> m_configValues;
};
