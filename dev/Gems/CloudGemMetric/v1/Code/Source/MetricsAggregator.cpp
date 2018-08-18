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

#include "CloudGemMetric/MetricsAggregator.h"
#include "CloudGemMetric/MetricsAttribute.h"
#include <AzCore/JSON/error/en.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/prettywriter.h>

namespace CloudGemMetric
{
    MetricsAggregator::MetricsAggregator(const AZStd::string& name, const AZStd::vector<MetricsAttribute>& metricsAttributes):
        m_name(name),
        m_attributes(metricsAttributes)
    {
    }

    MetricsAggregator::MetricsAggregator()
    {
    }

    MetricsAggregator::~MetricsAggregator()
    {
    }

    int MetricsAggregator::GetSizeInBytes() const
    {
        int nameSize = m_name.size() * sizeof(char);

        int attributeSize = 0;
        for (const auto& attr : m_attributes)
        {
            attributeSize += attr.GetSizeInBytes();
        }

        return nameSize + attributeSize;
    }

    rapidjson::Value MetricsAggregator::SerializeEventToJson(rapidjson::Document& doc, AZStd::vector<MetricsAttribute>* additionalAttributes) const
    {

        rapidjson::Value rootObjVal(rapidjson::kObjectType);

        rapidjson::Value parametersObjVal(rapidjson::kObjectType);
        rapidjson::Value metricsObjVal(rapidjson::kObjectType);

        metricsObjVal.AddMember("event", rapidjson::StringRef(m_name.c_str()), doc.GetAllocator());
        metricsObjVal.AddMember("uid", rapidjson::StringRef(m_playerId.c_str()), doc.GetAllocator());

        for (const auto& attr : m_attributes)
        {
            attr.SerializeToJson(doc, metricsObjVal);
        }      

        if (additionalAttributes)
        {
            for (const auto& attr : *additionalAttributes)
            {
                attr.SerializeToJson(doc, metricsObjVal);
            }
        }
        
        return metricsObjVal;
    }

    void MetricsAggregator::SerializeToJson(rapidjson::Document& doc, AZStd::vector<MetricsAttribute>* additionalAttributes) const
    {
        rapidjson::Value rootObjVal(rapidjson::kObjectType);
        rapidjson::Value parametersObjVal(rapidjson::kObjectType);        

        rapidjson::Value metricsObjVal = SerializeEventToJson(doc, additionalAttributes);

        for (const auto& param : m_parameters)
        {
            param.SerializeToJson(doc, parametersObjVal);
        }          

        rootObjVal.AddMember(rapidjson::StringRef(JSON_ATTRIBUTE_PARAMETERS), parametersObjVal, doc.GetAllocator());
        rootObjVal.AddMember(rapidjson::StringRef(JSON_ATTRIBUTE_EVENT_DEFINITION), metricsObjVal, doc.GetAllocator());
        doc.PushBack(rootObjVal, doc.GetAllocator());
    }

    AZStd::string MetricsAggregator::SerializeToJson(AZStd::vector<MetricsAttribute>* additionalAttributes) const
    {
        rapidjson::Document doc;       

        doc.SetArray();

        rapidjson::Value metricsObjVal = SerializeEventToJson(doc, additionalAttributes);
        doc.PushBack(metricsObjVal, doc.GetAllocator());

        rapidjson::StringBuffer outStrBuffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(outStrBuffer);        
        doc.Accept(writer);

        return outStrBuffer.GetString();
    }

    bool MetricsAggregator::ReadFromJson(rapidjson::Value& metricsObjVal)
    {
        if (!RAPIDJSON_IS_VALID_MEMBER(metricsObjVal, JSON_ATTRIBUTE_EVENT_DEFINITION, IsObject) || !RAPIDJSON_IS_VALID_MEMBER(metricsObjVal, JSON_ATTRIBUTE_PARAMETERS, IsObject))
        {
            return false;
        }
                
        AZStd::string name = metricsObjVal[JSON_ATTRIBUTE_EVENT_DEFINITION]["event"].GetString();
        metricsObjVal[JSON_ATTRIBUTE_EVENT_DEFINITION].RemoveMember("event");

        AZStd::string playerId = metricsObjVal[JSON_ATTRIBUTE_EVENT_DEFINITION]["uid"].GetString();
        metricsObjVal[JSON_ATTRIBUTE_EVENT_DEFINITION].RemoveMember("uid");

        AZStd::vector<MetricsAttribute> attributes;        
        attributes.resize(metricsObjVal[JSON_ATTRIBUTE_EVENT_DEFINITION].MemberCount());

        int i = 0;
        for (auto it = metricsObjVal[JSON_ATTRIBUTE_EVENT_DEFINITION].MemberBegin(); it != metricsObjVal[JSON_ATTRIBUTE_EVENT_DEFINITION].MemberEnd(); ++it, ++i)
        {
            attributes[i].ReadFromJson(it->name, it->value);
        }

        i = 0;
        AZStd::vector<MetricsEventParameter> parameters;
        parameters.resize(metricsObjVal[JSON_ATTRIBUTE_PARAMETERS].MemberCount());
        for (auto it = metricsObjVal[JSON_ATTRIBUTE_PARAMETERS].MemberBegin(); it != metricsObjVal[JSON_ATTRIBUTE_PARAMETERS].MemberEnd(); ++it, ++i)
        {
            parameters[i].ReadFromJson(it->name, it->value);
        }

        m_parameters = AZStd::move(parameters);
        m_attributes = AZStd::move(attributes);
        m_name = AZStd::move(name);
        m_playerId = AZStd::move(playerId);

        return true;
    }

    void MetricsAggregator::SetPlayerIdIfNotExist(const AZStd::string& playerId)
    {
        if (!m_playerId.empty())
        {
            return;
        }

        m_playerId = playerId;
    }
}