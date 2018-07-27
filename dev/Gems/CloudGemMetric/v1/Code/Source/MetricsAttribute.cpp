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

#include "StdAfx.h"

#include "CloudGemMetric/MetricsAttribute.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/containers/vector.h>


namespace CloudGemMetric
{
    MetricsAttribute::MetricsAttribute(const AZStd::string& name, int intVal) :
        m_valType(VAL_TYPE::INT),
        m_name(name),
        m_intVal(intVal)
    {
    }

    MetricsAttribute::MetricsAttribute(const AZStd::string& name, double doubleVal) :
        m_valType(VAL_TYPE::DOUBLE),
        m_name(name),
        m_doubleVal(doubleVal)
    {
    }

    MetricsAttribute::MetricsAttribute(const AZStd::string& name, const AZStd::string& strVal) :
        m_valType(VAL_TYPE::STR),
        m_name(name),
        m_strVal(strVal)
    {
    }

    MetricsAttribute::MetricsAttribute()
    {
    }

    MetricsAttribute::~MetricsAttribute()
    {
    }

    void MetricsAttribute::SetVal(const char* val)
    {
        m_valType = VAL_TYPE::STR;
        m_strVal = val;
    }

    void MetricsAttribute::SetVal(int val)
    {
        m_valType = VAL_TYPE::INT;
        m_intVal = val;
    }

    void MetricsAttribute::SetVal(double val)
    {
        m_valType = VAL_TYPE::DOUBLE;
        m_doubleVal = val;
    }

    int MetricsAttribute::GetSizeInBytes() const
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
    }

    void MetricsAttribute::SerializeToJson(rapidjson::Document& doc, rapidjson::Value& metricsObjVal) const
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
    }

    bool MetricsAttribute::ReadFromJson(const rapidjson::Value& name, const rapidjson::Value& val)
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
    }
}