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

#include <AzCore/IO/SystemFile.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Component/ComponentApplicationBus.h>

using StringOutcome = AZ::Outcome<void, AZStd::string>;
using AZ::IO::SystemFile;

/**
* Shorthand for checking a condition, and failing if false.
* Works with any function that returns AZ::Outcome<..., AZStd::string>.
* Unlike assert, it is not removed in release builds.
* Ensure all strings are passed with c_str(), as they are passed to AZStd::string::format().
*/
#define LMBR_ENSURE(cond, ...) if (!(cond)) { return AZ::Failure(AZStd::string::format(__VA_ARGS__)); }

// general class to read/modify .cfg style config files
class ConfigFileContainer
{
public:
    ConfigFileContainer(const AZStd::string& localFilePath);
    ConfigFileContainer(const ConfigFileContainer& rhs) = delete;
    ConfigFileContainer& operator=(const ConfigFileContainer& rhs) = delete;
    ConfigFileContainer& operator=(ConfigFileContainer&& rhs) = delete;
    virtual ~ConfigFileContainer() = default;

    // read file into m_fileContents
    StringOutcome ReadContents();

    // write m_fileContents to original file location, overwriting original
    StringOutcome WriteContents();

    AZStd::string GetString(const char* key) const;
    bool GetBool(const char* key) const;

    AZStd::string GetStringIncludingComments(const char* key) const;
    bool GetBoolIncludingComments(const char* key) const;

    void SetString(const char* key, const AZStd::string& newValue);
    void SetBool(const char* key, bool newValue);

    // read file at filePath into contents
    static StringOutcome ReadFile(const char* filePath, AZStd::string& contents);

    // write contents to filePath, overwriting any existing file
    static StringOutcome WriteFile(const char* filePath, const AZStd::string& contents);

    // check if fileContents contains key
    static bool HasKey(const AZStd::string& fileContents, const AZStd::string& key);

    // get value at key if there is one
    static AZStd::string ReadValue(const AZStd::string& fileContents, const AZStd::string& key);
    static AZStd::string ReadValueIncludingComments(const AZStd::string& fileContents, const AZStd::string& key);

    // insert new key/value pair into fileContents
    static void InsertKey(AZStd::string& fileContents, const AZStd::string& key, const AZStd::string& value);

    // replace existing value at key in fileContents
    static void ReplaceValue(AZStd::string& fileContents, const AZStd::string& key, const AZStd::string& newValue);

protected:
    static const char* s_commentPrefix;
    static const char* s_regexPrefix;
    static const char* s_regexPostfix;

    AZStd::string m_filePath;
    AZStd::string m_fileContents;
    mutable AZStd::unordered_map <AZStd::string, AZStd::string> m_configValues;
};
