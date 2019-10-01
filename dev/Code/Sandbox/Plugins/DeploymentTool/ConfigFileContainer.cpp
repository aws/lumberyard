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

#include <QDir>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/regex.h>

#include "ConfigFileContainer.h"


namespace
{
    const char* s_commentPrefix = "^\\s*--\\s*";
    const char* s_regexPrefix = "^\\s*";
    const char* s_regexPostfix = "\\s*=\\s*(\\S+)\\b";


    AZStd::string BuildStandardRegexString(const AZStd::string& key)
    {
        return AZStd::string::format("%s%s%s", s_regexPrefix, key.c_str(), s_regexPostfix);
    }

    AZStd::string BuildCommentRegexString(const AZStd::string& key)
    {
        return AZStd::string::format("%s%s%s", s_commentPrefix, key.c_str(), s_regexPostfix);
    }

    AZStd::string GetValueFromRegexSearch(const AZStd::string& pattern, const AZStd::string& content)
    {
        AZStd::string value;

        AZStd::regex configRegex(pattern, AZStd::regex::ECMAScript | AZStd::regex::icase);
        AZStd::smatch match;
        if (AZStd::regex_search(content, match, configRegex))
        {
            value = match[1].str();
        }

        return value;
    }
}

StringOutcome ReadFile(const AZStd::string& file, AZStd::string& fileContents)
{
    using AZ::IO::SystemFile;
    const char* filePath = file.c_str();

    if (!SystemFile::Exists(filePath))
    {
        return AZ::Failure(AZStd::string::format("%s file doesn't exist.", filePath));
    }

    SystemFile settingsFile;
    if (!settingsFile.Open(filePath, SystemFile::SF_OPEN_READ_ONLY))
    {
        return AZ::Failure(AZStd::string::format("Failed to open settings file %s.", filePath));
    }

    fileContents = AZStd::string(settingsFile.Length(), '\0');
    settingsFile.Read(fileContents.size(), fileContents.data());

    settingsFile.Close();
    return AZ::Success(AZStd::string());
}

ConfigFileContainer::ConfigFileContainer(const AZStd::string& filePath)
    : m_filePath(filePath)
    , m_fileContents()
    , m_configValues()
{
    if (!QDir::isAbsolutePath(filePath.c_str()))
    {
        AZStd::string appRoot;
        EBUS_EVENT_RESULT(appRoot, AZ::ComponentApplicationBus, GetAppRoot);
        m_filePath = appRoot + filePath;
    }
}

StringOutcome ConfigFileContainer::ApplyConfiguration(const DeploymentConfig&)
{
    return AZ::Success(AZStd::string());
}

StringOutcome ConfigFileContainer::Load()
{
    return ReadFile(m_filePath, m_fileContents);
}

StringOutcome ConfigFileContainer::Reload()
{
    m_configValues.clear();
    return ReadFile(m_filePath, m_fileContents);
}

StringOutcome ConfigFileContainer::Write() const
{
    using AZ::IO::SystemFile;
    const char* filePath = m_filePath.c_str();

    // Delete and recreate file to prevent inconsistent state
    if (SystemFile::Exists(filePath))
    {
        if (!SystemFile::Delete(filePath))
        {
            return AZ::Failure(AZStd::string::format("Failed to delete existing settings file %s.", filePath));
        }
    }

    SystemFile settingsFile;
    if (!settingsFile.Open(filePath, SystemFile::SF_OPEN_WRITE_ONLY | SystemFile::SF_OPEN_CREATE))
    {
        return AZ::Failure(AZStd::string::format("Failed to open settings file %s for write.", filePath));
    }

    if (settingsFile.Write(m_fileContents.c_str(), m_fileContents.size()) != m_fileContents.size())
    {
        return AZ::Failure(AZStd::string::format("Failed to write to config file %s.", filePath));
    }

    settingsFile.Close();
    return AZ::Success(AZStd::string());
}

const AZStd::string& ConfigFileContainer::GetString(const AZStd::string& key, bool includeComments) const
{
    if (m_configValues.count(key) == 0)
    {
        AZStd::string regexStr = AZStd::move(BuildStandardRegexString(key));
        AZStd::string value = AZStd::move(GetValueFromRegexSearch(regexStr, m_fileContents));

        if (includeComments && value.empty())
        {
            regexStr = AZStd::move(BuildCommentRegexString(key));
            value = AZStd::move(GetValueFromRegexSearch(regexStr, m_fileContents));
        }

        m_configValues[key] = value;
    }

    return m_configValues.at(key);
}

bool ConfigFileContainer::GetBool(const AZStd::string& key, bool includeComments) const
{
    const AZStd::string& value = GetString(key, includeComments);
    return (value == "1");
}

void ConfigFileContainer::SetString(const AZStd::string& key, const AZStd::string& newValue)
{
    m_configValues[key] = newValue;

    if (IsKeyInFile(key))
    {
        AZStd::string regexStr = AZStd::move(BuildStandardRegexString(key));
        AZStd::string replaceStr = AZStd::move(AZStd::string::format("%s=%s", key.c_str(), newValue.c_str()));

        m_fileContents = AZStd::regex_replace(m_fileContents, AZStd::regex(regexStr), replaceStr);
    }
    else
    {
        m_fileContents = AZStd::string::format("%s\n%s=%s", m_fileContents.c_str(), key.c_str(), newValue.c_str());
    }
}

void ConfigFileContainer::SetBool(const AZStd::string& key, bool newValue)
{
    SetString(key, (newValue ? "1" : "0"));
}

bool ConfigFileContainer::IsKeyInFile(const AZStd::string& key) const
{
    AZStd::string regexStr = AZStd::move(BuildStandardRegexString(key));
    AZStd::string value = AZStd::move(GetValueFromRegexSearch(regexStr, m_fileContents));
    return !value.empty();
}
