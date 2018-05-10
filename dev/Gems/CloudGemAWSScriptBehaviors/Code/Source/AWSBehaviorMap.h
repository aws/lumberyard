
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

#include "AWSBehaviorBase.h"

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

namespace CloudGemAWSScriptBehaviors
{
    class StringMap : public AWSBehaviorBase
    {
    public:
        AWSBEHAVIOR_DEFINITION(StringMap, "{EBF5FF7D-644E-4744-9AA1-FFCD7D46740D}")
        void SetValue(const AZStd::string& key, const AZStd::string& value);

    private:
        AZStd::string GetValue(const AZStd::string& key);
        void RemoveKey(const AZStd::string& key);
        bool HasKey(const AZStd::string& key);
        int GetSize();
        void Clear();
        AZStd::string ToJSON();
        void FromJSON(const AZStd::string& jsonIn);
        void LogToDebugger();
        
        using StringMapType = AZStd::unordered_map<AZStd::string, AZStd::string>;
        StringMapType m_map;
    };
}