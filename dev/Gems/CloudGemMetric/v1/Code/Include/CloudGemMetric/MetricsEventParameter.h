/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/JSON/document.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/containers/vector.h>

/*
MetricsAttribute represents one attribute e.g. name: position_x, value: 10
Attribute value can be int, double or string
*/
namespace CloudGemMetric
{
    static const char* SENSITIVITY_TYPE = "sensitivity_type";
    static const char* COMPRESSION_MODE = "compression_mode";
    static const char* SENSITIVITY_TYPE_SENSITIVE = "Sensitive";
    static const char* SENSITIVITY_TYPE_INSENSITIVE = "Insensitive";
    static const char* COMPRESSION_MODE_COMPRESS = "Compress";
    static const char* COMPRESSION_MODE_NOCOMPRESS = "NoCompression";

    class MetricsEventParameter
    {
    public:
        AZ_TYPE_INFO(MetricsEventParameter, "{B8C0A8E0-51FF-4140-9C29-2E1B0B7B79AD}");

        MetricsEventParameter(const AZStd::string& name, int toggle) :
            m_name(name),
            m_value(toggle)
        {
        };
       
        MetricsEventParameter() {};
        ~MetricsEventParameter() {};

        void SetName(const char* name) { m_name = name; };
        AZStd::string GetName() const { return m_name; };
        AZStd::string GetSensitivityTypeName() const { return SENSITIVITY_TYPE; };        
        AZStd::string GetCompressionModeName() const { return COMPRESSION_MODE; };        
        
        void SetVal(int val)
        {           
            m_value = val;
        };                    

        int GetVal() const
        {
            return m_value;
        };
   
        size_t GetSizeInBytes() const
        {
            size_t nameSize = m_name.size() * sizeof(char);

            size_t valSize = sizeof(int);

            return nameSize + valSize;
        };

        void SerializeToJson(rapidjson::Document& doc, rapidjson::Value& metricsObjVal) const
        {           
            metricsObjVal.AddMember(rapidjson::StringRef(m_name.c_str()), m_value, doc.GetAllocator());
        };

        bool ReadFromJson(const rapidjson::Value& name, const rapidjson::Value& val)
        {
            if (!val.IsInt())
            {
                return false;
            }
             
            m_value = val.GetInt();
            m_name = name.GetString();

            return true;
        };
    private:        
        AZStd::string m_name;
        int m_value{ 0 };

    };
} // namespace CloudGemMetric
