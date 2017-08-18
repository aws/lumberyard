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

#include "stdafx.h"
#include "ConfigFileContainer.h"
#include <regex>
#include <AzCore/JSON/document.h>

namespace
{
    // index of a key's value in smatch array
    // (first entry is entire match, subsequent entries for each sub-expression in regex)
    const int valueIndexInRegexMatch = 3;

    AZStd::string GetValueFromRegexSearch(const char* pattern, const char* content)
    {
        std::smatch matches;

        std::regex sysGameRegex(pattern);
        // Need this string to exist outside regex_seawrch otherwise it returns invalid data
        std::string contentToSearch(content);
        std::regex_search(contentToSearch, matches, sysGameRegex);

        if (matches.empty() || !matches.ready())
        {
            return "";
        }

        AZStd::string val = AZStd::string(matches[valueIndexInRegexMatch].str().c_str());

        // get rid of leading equals sign
        return val.substr(1);
    }
}

const char* ConfigFileContainer::s_commentPrefix = "(^\\s*--\\s*)";
const char* ConfigFileContainer::s_regexPrefix = "(^\\s*)";
const char* ConfigFileContainer::s_regexPostfix = "(\\s*)(=\\s*[\\w_.]*)\\b";

ConfigFileContainer::ConfigFileContainer(const AZStd::string& localFilePath)
{
    AZStd::string appRoot;
    EBUS_EVENT_RESULT(appRoot, AZ::ComponentApplicationBus, GetAppRoot);
    m_filePath = appRoot + localFilePath;
}

StringOutcome ConfigFileContainer::ReadContents()
{
    m_configValues.clear();
    return ReadFile(m_filePath.c_str(), m_fileContents);
}

StringOutcome ConfigFileContainer::WriteContents()
{
    return WriteFile(m_filePath.c_str(), m_fileContents);
}

AZStd::string ConfigFileContainer::GetString(const char* key) const
{
    // if key is not cached read it from m_fileContents
    AZStd::string keyString = AZStd::string(key);
    if (m_configValues.count(keyString) == 0)
    {
        m_configValues[keyString] = ReadValue(m_fileContents, keyString);
    }

    return m_configValues.at(keyString);
}

AZStd::string ConfigFileContainer::GetStringIncludingComments(const char* key) const
{
    // if key is not cached read it from m_fileContents
    AZStd::string keyString = AZStd::string(key);
    if (m_configValues.count(keyString) == 0)
    {
        m_configValues[keyString] = ReadValueIncludingComments(m_fileContents, keyString);
    }

    return m_configValues.at(keyString);
}

bool ConfigFileContainer::GetBool(const char* key) const
{
    return (GetString(key) == "1") ? true : false;
}

bool ConfigFileContainer::GetBoolIncludingComments(const char* key) const
{
    return (GetStringIncludingComments(key) == "1") ? true : false;
}

void ConfigFileContainer::SetString(const char* key, const AZStd::string& newValue)
{
    m_configValues[AZStd::string(key)] = newValue;
    if (HasKey(m_fileContents, key))
    {
        ReplaceValue(m_fileContents, key, newValue);
    }
    else
    {
        InsertKey(m_fileContents, key, newValue);
    }
}

void ConfigFileContainer::SetBool(const char* key, bool newValue)
{
    AZStd::string val = (newValue) ? "1" : "0";
    SetString(key, val);
}

// read contents of file into contents
StringOutcome ConfigFileContainer::ReadFile(const char* filePath, AZStd::string& contents)
{
    SystemFile settingsFile;
    LMBR_ENSURE(SystemFile::Exists(filePath), "%s file doesn't exist.", filePath);
    LMBR_ENSURE(settingsFile.Open(filePath, SystemFile::SF_OPEN_READ_ONLY), "Failed to open settings file %s.", filePath);
    contents = AZStd::string(settingsFile.Length(), '\0');
    settingsFile.Read(contents.size(), contents.data());
    settingsFile.Close();
    return AZ::Success();
}

StringOutcome ConfigFileContainer::WriteFile(const char* filePath, const AZStd::string& contents)
{
    // Delete and recreate file to prevent inconsistent state
    if (SystemFile::Exists(filePath))
    {
        LMBR_ENSURE(SystemFile::Delete(filePath), "Failed to delete existing settings file %s.", filePath);
    }
    SystemFile settingsFile;
    LMBR_ENSURE(settingsFile.Open(filePath, SystemFile::SF_OPEN_WRITE_ONLY | SystemFile::SF_OPEN_CREATE), "Failed to open settings file %s.", filePath);
    LMBR_ENSURE(settingsFile.Write(contents.c_str(), contents.size()) == contents.size(), "Failed to write to config file %s.", filePath);
    settingsFile.Close();
    return AZ::Success();
}

bool ConfigFileContainer::HasKey(const AZStd::string& fileContents, const AZStd::string& key)
{
    std::smatch matches;
    // Finds anything matching the pattern "[key] = [value]", with all possible whitespace variations accounted for.
    AZStd::string regexStr = s_regexPrefix + key + s_regexPostfix;
    std::regex sysGameRegex(regexStr.c_str());
    // need to store fileContentsStdStr in variable because matches[] contains pointers to it
    std::string fileContentsStdStr = fileContents.c_str();
    std::regex_search(fileContentsStdStr, matches, sysGameRegex);
    return !matches.empty();
}

// read value at key in .cfg type config file
AZStd::string ConfigFileContainer::ReadValue(const AZStd::string& fileContents, const AZStd::string& key)
{
    // Finds anything matching the pattern "[key] = [value]", with all possible whitespace variations accounted for.
    AZStd::string regexStr = AZStd::move(AZStd::string::format("%s%s%s", s_regexPrefix, key.c_str(), s_regexPostfix));
    return GetValueFromRegexSearch(regexStr.c_str(), fileContents.c_str());
}

AZStd::string ConfigFileContainer::ReadValueIncludingComments(const AZStd::string& fileContents, const AZStd::string& key)
{
    AZStd::string result = ReadValue(fileContents, key);
    if (!result.empty())
    {
        return result;
    }

    // Finds anything matching the pattern "-- [key] = [value]", with all possible whitespace variations accounted for.
    AZStd::string regexStr = AZStd::move(AZStd::string::format("%s%s%s", s_commentPrefix, key.c_str(), s_regexPostfix));
    return GetValueFromRegexSearch(regexStr.c_str(), fileContents.c_str());
}

void ConfigFileContainer::InsertKey(AZStd::string& fileContents, const AZStd::string& key, const AZStd::string& value)
{
    // insert key and value at the end of file
    fileContents = AZStd::string::format("%s\n%s=%s\n", fileContents.c_str(), key.c_str(), value.c_str());
}

void ConfigFileContainer::ReplaceValue(AZStd::string& fileContents, const AZStd::string& key, const AZStd::string& newValue)
{
    // Generate regex str and replacement str
    AZStd::string lhsStr = key;
    // Finds anything matching the pattern "[key] = [value]", with all possible whitespace variations accounted for.
    AZStd::string regexStr = s_regexPrefix + lhsStr + s_regexPostfix;
    AZStd::string replaceStr = "$1" + lhsStr + "=" + newValue;

    // Replace the current key with the new value
    std::regex sysGameRegex(regexStr.c_str());
    std::string fileContentsStdStr = fileContents.c_str();
    std::string result = std::regex_replace(fileContentsStdStr, sysGameRegex, replaceStr.c_str());
    fileContents = result.c_str();
}
