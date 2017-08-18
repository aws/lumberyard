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

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Outcome/Outcome.h>

using StringOutcome = AZ::Outcome<void, AZStd::string>;

/**
* Shorthand for checking a condition, and failing if false.
* Works with any function that returns AZ::Outcome<..., AZStd::string>.
* Unlike assert, it is not removed in release builds.
* Ensure all strings are passed with c_str(), as they are passed to AZStd::string::format().
*/
#define LMBR_ENSURE(cond, ...) if (!(cond)) { return AZ::Failure(AZStd::string::format(__VA_ARGS__)); }

class ConfigController
{
public:
    ConfigController();
    ~ConfigController();

    StringOutcome ReadFromCfgFile(const char* fileName, const char* key, AZStd::string& result);
    StringOutcome ReplaceInCfgFile(const char* fileName, const char* key, const AZStd::string& newValue);

private:
    string m_appRoot;

    const char* regexPrefix = "(^|\\n\\s*)";
    const char* regexPostfix = "(\\s*)=(\\s*)([\\w_.]*)\\b";

    AZStd::string GeneratePath(const char* fileName);
};

