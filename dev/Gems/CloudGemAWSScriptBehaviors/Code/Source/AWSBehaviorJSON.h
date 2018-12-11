
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

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>
#include <AzCore/std/containers/vector.h>


namespace CloudGemAWSScriptBehaviors
{
    class AWSBehaviorJSON : public AWSBehaviorBase
    {
    public:
        AWSBEHAVIOR_DEFINITION(AWSBehaviorJSON, "{06050250-21BF-4E6F-9AA6-D9F356914384}")
    
    private:
        rapidjson::Document m_topLevelObject;
        rapidjson::Value* m_currentValue{ nullptr };
        AZStd::stack<rapidjson::Value*> m_prevValues;
        AZStd::vector<rapidjson::Value*> m_currentArray;
        size_t m_currentArrayIndex{ 0 };
        
        ///////////////////////////////////////////////////////////////////////
        // Full object operations
        ///////////////////////////////////////////////////////////////////////

        // Convert entire JSON object to string
        AZStd::string ToString();
        // Load entire JSON object from string
        bool FromString(AZStd::string jsonStr);
    
        ///////////////////////////////////////////////////////////////////////
        // JSON object traversal operations
        ///////////////////////////////////////////////////////////////////////

        // Object traversal
        bool EnterObject(const AZStd::string& key);
        void ExitCurrentObject();

        // Array traversal
        size_t EnterArray();    // returns array size
        void ExitArray();
        bool NextArrayItem();   // returns false if no more items

        ///////////////////////////////////////////////////////////////////////
        // Current value get operations
        ///////////////////////////////////////////////////////////////////////
        bool IsObject();
        bool IsArray();
        bool IsDouble();
        bool IsInteger();
        bool IsString();
        bool IsBoolean();

        // Value operations
        double GetDouble();
        int GetInteger();
        AZStd::string GetString();
        bool GetBoolean();

        ///////////////////////////////////////////////////////////////////////
        // Debug operations
        ///////////////////////////////////////////////////////////////////////
        void LogToDebugger();
    };
}

