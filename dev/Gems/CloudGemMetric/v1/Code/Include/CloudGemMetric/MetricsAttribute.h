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
    class MetricsAttribute
    {                    
    public:
        AZ_TYPE_INFO(MetricsAttribute, "{f55007a4-f282-11e7-8c3f-9a214cf093ae}")

        MetricsAttribute(const AZStd::string& name, int intVal) :
            m_valType(VAL_TYPE::INT),
            m_name(name),
            m_intVal(intVal)
        {
        };
        MetricsAttribute(const AZStd::string& name, double doubleVal) :
            m_valType(VAL_TYPE::DOUBLE),
            m_name(name),
            m_doubleVal(doubleVal)
        {
        };
        MetricsAttribute(const AZStd::string& name, const AZStd::string& strVal) :
            m_valType(VAL_TYPE::STR),
            m_name(name),
            m_strVal(strVal)
        {
        };
        MetricsAttribute() {};
        ~MetricsAttribute() {};

        void SetName(const char* name) { m_name = name; };
        const AZStd::string GetName() const { return m_name; };

        void SetVal(const char* val)
        {
            m_valType = VAL_TYPE::STR;
            m_strVal = val;
        };

        void SetVal(int val)
        {
            m_valType = VAL_TYPE::INT;
            m_intVal = val;
        };

        void SetVal(double val)
        {
            m_valType = VAL_TYPE::DOUBLE;
            m_doubleVal = val;
        };

        int GetSizeInBytes() const
        {
            int nameSize = m_name.size() * sizeof(char);

            int valSize = 0;
            if (m_valType == VAL_TYPE::INT)
            {
                valSize = sizeof(int);
            }
            else if (m_valType == VAL_TYPE::DOUBLE)
            {
                valSize = sizeof(double);
            }
            else
            {
                valSize = m_strVal.size() * sizeof(char);
            }

            return nameSize + valSize;
        };

        void SerializeToJson(rapidjson::Document& doc, rapidjson::Value& metricsObjVal) const
        {
            if (m_valType == VAL_TYPE::INT)
            {
                metricsObjVal.AddMember(rapidjson::StringRef(m_name.c_str()), m_intVal, doc.GetAllocator());
            }
            else if (m_valType == VAL_TYPE::DOUBLE)
            {
                metricsObjVal.AddMember(rapidjson::StringRef(m_name.c_str()), m_doubleVal, doc.GetAllocator());
            }
            else
            {
                metricsObjVal.AddMember(rapidjson::StringRef(m_name.c_str()), rapidjson::StringRef(m_strVal.c_str()), doc.GetAllocator());
            }
        };

        bool ReadFromJson(const rapidjson::Value& name, const rapidjson::Value& val)
        {
            if (!val.IsInt() && !val.IsDouble() && !val.IsString())
            {
                return false;
            }

            if (val.IsInt())
            {
                m_valType = VAL_TYPE::INT;
                m_intVal = val.GetInt();
            }
            else if (val.IsDouble())
            {
                m_valType = VAL_TYPE::DOUBLE;
                m_doubleVal = val.GetDouble();
            }
            else
            {
                m_valType = VAL_TYPE::STR;
                m_strVal = val.GetString();
            }

            m_name = name.GetString();

            return true;
        };
    private:
        enum class VAL_TYPE
        {
            INT,
            DOUBLE,
            STR
        };

    private:
        VAL_TYPE m_valType;
        AZStd::string m_name;
        int m_intVal{ 0 };
        double m_doubleVal{ 0 };
        AZStd::string m_strVal;
    };        
} // namespace CloudGemMetric
