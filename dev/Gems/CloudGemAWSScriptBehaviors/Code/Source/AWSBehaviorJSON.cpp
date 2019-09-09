
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

#include "AWSBehaviorJSON.h"

#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/stringbuffer.h>

namespace CloudGemAWSScriptBehaviors
{
    AWSBehaviorJSON::AWSBehaviorJSON()
    {

    }

    void AWSBehaviorJSON::ReflectSerialization(AZ::SerializeContext* serializeContext)
    {
        if (serializeContext)
        {
            serializeContext->Class<AWSBehaviorJSON>()
                ->Version(1);
        }
    }

    void AWSBehaviorJSON::ReflectBehaviors(AZ::BehaviorContext* behaviorContext)
    {
        behaviorContext->Class<AWSBehaviorJSON>("JSON")
            ->Method("ToString", &AWSBehaviorJSON::ToString, nullptr, "Convert the entire JSON object in to a readable string")
            ->Method("FromString", &AWSBehaviorJSON::FromString, nullptr, "Sets the entire JSON object from the provided string. Returns true if parse succeeded, otherwise false.")
            ->Method("EnterObject", &AWSBehaviorJSON::EnterObject, nullptr, "If the current value is an object, sets the current value to the value paired to the provided key string. Returns true if the key exists, otherwise false.")
            ->Method("ExitCurrentObject", &AWSBehaviorJSON::ExitCurrentObject, nullptr, "Sets the current value to whatever value contains the current value")
            ->Method("EnterArray", &AWSBehaviorJSON::EnterArray, nullptr, "If the current value is an array, sets the current value to the first value in the array and allows iteration of the array via NextArrayItem. Returns the size of the array.")
            ->Method("ExitArray", &AWSBehaviorJSON::ExitArray, nullptr, "Stops iteration of the current array and sets the current value to the array container.")
            ->Method("NextArrayItem", &AWSBehaviorJSON::NextArrayItem, nullptr, "Sets the current value to the next array value. If there are no more values, returns false")
            ->Method("IsObject", &AWSBehaviorJSON::IsObject, nullptr, "Returns true if the current value is an object, otherwise returns false.")
            ->Method("IsArray", &AWSBehaviorJSON::IsArray, nullptr, "Returns true if the current value is an array, otherwise returns false.")
            ->Method("IsDouble", &AWSBehaviorJSON::IsDouble, nullptr, "Returns true if the current value is a double floating point value, otherwise returns false.")
            ->Method("IsInteger", &AWSBehaviorJSON::IsInteger, nullptr, "Returns true if the current value is an integer, otherwise returns false.")
            ->Method("IsString", &AWSBehaviorJSON::IsString, nullptr, "Returns true if the current value is a string, otherwise returns false.")
            ->Method("IsBoolean", &AWSBehaviorJSON::IsBoolean, nullptr, "Returns true if the current value is a boolean, otherwise returns false.")
            ->Method("GetDouble", &AWSBehaviorJSON::GetDouble, nullptr, "Returns the double floating point value from the current value if it is a double, otherwise just returns 0")
            ->Method("GetInteger", &AWSBehaviorJSON::GetInteger, nullptr, "Returns the integer value from the current value if it is an integer, otherwise just returns 0")
            ->Method("GetString", &AWSBehaviorJSON::GetString, nullptr, "Returns the string value from the current value if it is a string, otherwise just returns an empty string")
            ->Method("GetBoolean", &AWSBehaviorJSON::GetBoolean, nullptr, "Returns the boolean value from the current value if it is a boolean, otherwise just returns an false")
            ->Method("LogToDebugger", &AWSBehaviorJSON::LogToDebugger, nullptr, "Prints a string representation of the current JSON object");
    }

    void AWSBehaviorJSON::ReflectEditParameters(AZ::EditContext* editContext)
    {
    }

    AZStd::string AWSBehaviorJSON::ToString()
    {
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        m_topLevelObject.Accept(writer);
        return buffer.GetString();
    }

    bool AWSBehaviorJSON::FromString(AZStd::string jsonStr)
    {
        m_currentValue = nullptr;
        m_topLevelObject.Parse<rapidjson::kParseStopWhenDoneFlag>(jsonStr.c_str());

        if (m_topLevelObject.HasParseError())
        {
            AZ_Printf("AWSBehaviorJSON", "Error parsing string: %d", m_topLevelObject.GetParseError());
            return false;
        }

        return true;
    }

    bool AWSBehaviorJSON::EnterObject(const AZStd::string& key)
    {
        if (m_currentValue)
        {
            if (m_currentValue->HasMember(key.c_str()))
            {
                m_prevValues.push(m_currentValue);
                m_currentValue = &m_currentValue->FindMember(key.c_str())->value;
                return true;
            }
        }
        else if (m_topLevelObject.HasMember(key.c_str()))
        {
            m_currentValue = &m_topLevelObject.FindMember(key.c_str())->value;
            return true;
        }

        AZ_Printf("AWSBehaviorJSON", "Unable to find object at key %s", key.c_str());
        return false;

    }

    void AWSBehaviorJSON::ExitCurrentObject()
    {
        if (!m_prevValues.empty())
        {
            m_currentValue = m_prevValues.top();
            m_prevValues.pop();
        }
        else
        {
            AZ_Printf("AWSBehaviorJSON", "Attempt to exit current object but already at top level");
        }
    }

    size_t AWSBehaviorJSON::EnterArray()
    {
        if (m_currentValue && m_currentValue->IsArray())
        {
            m_currentArray.clear();
            for (auto valueIter = m_currentValue->Begin(); valueIter != m_currentValue->End(); ++valueIter)
            {
                m_currentArray.push_back(valueIter);
            }
            m_currentArrayIndex = 0;
            if (m_currentArray.size() == 0)
            {
                return 0;
            }
            m_prevValues.push(m_currentValue);
            m_currentValue = m_currentArray[m_currentArrayIndex];
            return m_currentArray.size();
        }
        else
        {
            AZ_Printf("AWSBehaviorJSON", "Attempt to enter array but value type isn't array");
            return 0;
        }
    }
    
    void AWSBehaviorJSON::ExitArray()
    {
        m_currentArrayIndex = 0;
        m_currentValue = m_prevValues.top();
        m_prevValues.pop();
    }

    bool AWSBehaviorJSON::NextArrayItem()
    {
        if (m_currentArray.size() > 0)
        {
            ++m_currentArrayIndex;
            if (m_currentArrayIndex < m_currentArray.size())
            {
                m_currentValue = m_currentArray[m_currentArrayIndex];
                return true;
            }
            else
            {
                return false;
            }
        }
        return false;
    }

    bool AWSBehaviorJSON::IsObject()
    {
        if (m_currentValue)
        {
            return m_currentValue->IsObject();
        }
        return m_topLevelObject.IsObject();
    }

    bool AWSBehaviorJSON::IsArray()
    {
        return m_currentValue && m_currentValue->IsArray();
    }

    bool AWSBehaviorJSON::IsDouble()
    {
        return m_currentValue && m_currentValue->IsDouble();
    }

    bool AWSBehaviorJSON::IsInteger()
    {
        return m_currentValue && m_currentValue->IsInt();
    }

    bool AWSBehaviorJSON::IsString()
    {
        return m_currentValue && m_currentValue->IsString();
    }

    bool AWSBehaviorJSON::IsBoolean()
    {
        return m_currentValue && m_currentValue->IsBool();
    }

    double AWSBehaviorJSON::GetDouble()
    {
        if(m_currentValue && m_currentValue->IsDouble())
        { 
            return m_currentValue->GetDouble();
        }
        else
        {
            AZ_Printf("AWSBehaviorJSON", "Attempt to parse current value as double, but it's not a double.");
            return 0.0l;
        }
    }

    int AWSBehaviorJSON::GetInteger()
    {
        if (m_currentValue && m_currentValue->IsInt())
        {
            return m_currentValue->GetInt();
        }
        else
        {
            AZ_Printf("AWSBehaviorJSON", "Attempt to parse current value as integer, but it's not an integer.");
            return 0;
        }
    }
    
    AZStd::string AWSBehaviorJSON::GetString()
    {
        if (m_currentValue && m_currentValue->IsString())
        {
            return AZStd::string(m_currentValue->GetString());
        }
        else
        {
            AZ_Printf("AWSBehaviorJSON", "Attempt to parse current value as string, but it's not a string.");
            return AZStd::string();
        }
    }

    bool AWSBehaviorJSON::GetBoolean()
    {
        if (m_currentValue && m_currentValue->IsBool())
        {
            return m_currentValue->GetBool();
        }
        else
        {
            AZ_Printf("AWSBehaviorJSON", "Attempt to parse current value as boolean, but it's not a boolean.");
            return false;
        }
    }

    void AWSBehaviorJSON::LogToDebugger()
    {
        AZ_Printf("AWSBehaviorJSON", "JSON Value Dump:\n%s", ToString().c_str());
    }
}

