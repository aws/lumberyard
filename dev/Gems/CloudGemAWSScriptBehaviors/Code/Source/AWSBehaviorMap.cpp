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

#include <CloudGemAWSScriptBehaviors_precompiled.h>

#include "AWSBehaviorMap.h"
#include <AzCore/JSON/document.h>

namespace CloudGemAWSScriptBehaviors
{
    StringMap::StringMap() :
        m_map()
    {
    }

    void StringMap::ReflectSerialization(AZ::SerializeContext* serializeContext)
    {
        if (serializeContext)
        {
            serializeContext->Class<StringMap>()
                ->Field("map", &StringMap::m_map)
                ->Version(2);
        }
    }

    void StringMap::ReflectBehaviors(AZ::BehaviorContext* behaviorContext)
    {
        behaviorContext->Class<StringMap>("StringMap")
            ->Property("map", nullptr, BehaviorValueSetter(&StringMap::m_map))
            ->Method("SetValue", &StringMap::SetValue, nullptr, "Sets a value for a key in the map")
            ->Method("GetValue", &StringMap::GetValue, nullptr, "Gets a value for a key in the map, returns empty string if key not found")
            ->Method("RemoveKey", &StringMap::RemoveKey, nullptr, "Removes a key and its value from the map")
            ->Method("HasKey", &StringMap::HasKey, nullptr, "Returns true if the map contains the specified key, otherwise false")
            ->Method("GetSize", &StringMap::GetSize, nullptr, "Returns the number of key-value pairs in the map")
            ->Method("Clear", &StringMap::Clear, nullptr, "Removes all key-value pairs from the map")
            ->Method("LogToDebugger", &StringMap::LogToDebugger, nullptr, "Prints all the key value pairs in the map")
            ->Method("ToJSON", &StringMap::ToJSON, nullptr, "Returns the string map as a JSON string")
            ->Method("FromJSON", &StringMap::FromJSON, nullptr, "Parses a JSON string of key:string pairs (error if not correct format)")
            ;

    }

    void StringMap::ReflectEditParameters(AZ::EditContext* editContext)
    {
    }

    void StringMap::SetValue(const AZStd::string& key, const AZStd::string& value)
    {
        m_map[key] = value;
    }

    AZStd::string StringMap::GetValue(const AZStd::string& key)
    {
        AZStd::string retVal;
        if (HasKey(key))
        {
            retVal = m_map[key];
        }
        return retVal;
    }

    void StringMap::RemoveKey(const AZStd::string& key)
    {
        m_map.erase(key);
    }

    bool StringMap::HasKey(const AZStd::string& key)
    {
        return m_map.find(key) != m_map.end();
    }

    int StringMap::GetSize()
    {
        return m_map.size();
    }

    void StringMap::Clear()
    {
        m_map.clear();
    }

    AZStd::string StringMap::ToJSON()
    {
        rapidjson::Document jsonDoc;
        jsonDoc.SetObject();

        if (m_map.size() == 0)
        {
            return{};
        }

        auto& allocator = jsonDoc.GetAllocator();

        for (auto& pair : m_map)
        {
            jsonDoc.AddMember(
                rapidjson::StringRef(pair.first.c_str(), pair.first.size()),
                rapidjson::StringRef(pair.second.c_str(), pair.second.size()),
                allocator);
        }

        rapidjson::StringBuffer jsonStringBuffer;
        rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(jsonStringBuffer);
        jsonDoc.Accept(jsonWriter);

        AZStd::string returnString{ jsonStringBuffer.GetString() };
        return returnString;
    }

    void StringMap::FromJSON(const AZStd::string& jsonIn)
    {
        rapidjson::Document jsonDoc;
        jsonDoc.Parse(jsonIn.c_str());
        m_map.clear();
        for (rapidjson::Value::ConstMemberIterator iter = jsonDoc.MemberBegin(); iter != jsonDoc.MemberEnd(); ++iter)
        {
            if (iter->value.IsString())
            {
                SetValue(iter->name.GetString(), iter->value.GetString());
            }
            else
            {
                AZ_Warning("AWSBehavior", false, "StringMap::FromJSON JSON Element %s contains non-string value of type %d", iter->name.GetString(), iter->value.GetType());
            }
        }
    }

    void StringMap::LogToDebugger()
    {
        AZ_Printf("AWSBehavior", "AWSBehaviorMap Debug Dump")
        for (auto it : m_map)
        {
            AZ_Printf("AWSBehavior", "\tKey: %s\tValue: %s", it.first.c_str(), it.second.c_str());
        }
    }

}