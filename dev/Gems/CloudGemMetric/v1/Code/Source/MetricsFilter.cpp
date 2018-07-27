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

#include "CloudGemMetric/MetricsFilter.h"
#include "CloudGemMetric/MetricsAggregator.h"

namespace CloudGemMetric
{
    static const char* TYPE_ALL = "all";
    static const char* TYPE_ATTRIBUTE = "attribute";

    static bool FilterAll(const AZStd::unordered_set<AZStd::string>& attributesToFilter, const AZStd::string& eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, MetricsAggregator& outMetrics)
    {
        return true;
    }

    static bool NullFilter(const AZStd::unordered_set<AZStd::string>& attributesToFilter, const AZStd::string& eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, MetricsAggregator& outMetrics)
    {
        outMetrics.SetEventName(eventName);
        outMetrics.SetAttributes(metricsAttributes);

        return false;
    }

    static bool FilterAllIfHasAttributes(const AZStd::unordered_set<AZStd::string>& attributesToFilter, const AZStd::string& eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, MetricsAggregator& outMetrics)
    {
        AZStd::unordered_set<AZStd::string> attrSet;
        for (const auto& attr : metricsAttributes)
        {
            if (attributesToFilter.find(attr.GetName()) != attributesToFilter.end())
            {
                attrSet.insert(attr.GetName());
            }
        }

        if (attrSet.size() == attributesToFilter.size())
        {
            return true;
        }
        else
        {
            outMetrics.SetEventName(eventName);
            outMetrics.SetAttributes(metricsAttributes);
            return false;
        }
    }

    static bool FilterAttributes(const AZStd::unordered_set<AZStd::string>& attributesToFilter, const AZStd::string& eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, MetricsAggregator& outMetrics)
    {
        for (const auto& attr : metricsAttributes)
        {
            if (attributesToFilter.find(attr.GetName()) == attributesToFilter.end())
            {
                outMetrics.AddAttribute(attr);
            }
        }

        if (outMetrics.GetAttributes().size())
        {
            outMetrics.SetEventName(eventName);
            return false;
        }
        else
        {
            return true;
        }
    }

    MetricsFilter::MetricsFilter():
        m_filterFunc(NullFilter)
    {
    }

    MetricsFilter::~MetricsFilter()
    {
    }

    bool MetricsFilter::ShouldFilterMetrics(const AZStd::string& eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, MetricsAggregator& outMetrics) const
    {
        return m_filterFunc(m_attributes, eventName, metricsAttributes, outMetrics);
    }

    void MetricsFilter::Init()
    {
        if (m_type == TYPE_ALL)
        {
            if (m_attributes.size() == 0)
            {
                m_filterFunc = FilterAll;
            }
            else
            {
                m_filterFunc = FilterAllIfHasAttributes;
            }
        }
        else if (m_type == TYPE_ATTRIBUTE)
        {
            m_filterFunc = FilterAttributes;
        }
        else
        {
            m_filterFunc = NullFilter;
        }
    }

    void MetricsFilter::SerializeToJson(rapidjson::Document& doc, rapidjson::Value& filtersArrayVal) const
    {
        rapidjson::Value filterObjVal(rapidjson::kObjectType);

        filterObjVal.AddMember("event", rapidjson::StringRef(m_eventName.c_str()), doc.GetAllocator());

        // add attributes
        {
            rapidjson::Value attributesArrayVal(rapidjson::kArrayType);            
            for (auto it = m_attributes.begin(); it != m_attributes.end(); ++it)
            {
                attributesArrayVal.PushBack(rapidjson::StringRef(it->c_str()), doc.GetAllocator());
            }

            filterObjVal.AddMember("attributes", attributesArrayVal, doc.GetAllocator());
        }        

        filterObjVal.AddMember("type", rapidjson::StringRef(m_type.c_str()), doc.GetAllocator());

        filtersArrayVal.PushBack(filterObjVal, doc.GetAllocator());
    }

    bool MetricsFilter::ReadFromJson(const rapidjson::Value& filterObjVal)
    {
        if (!RAPIDJSON_IS_VALID_MEMBER(filterObjVal, "event", IsString) ||
            !RAPIDJSON_IS_VALID_MEMBER(filterObjVal, "type", IsString) ||
            !RAPIDJSON_IS_VALID_MEMBER(filterObjVal, "attributes", IsArray))
        {
            return false;
        }

        const rapidjson::Value& attributesArrayVal = filterObjVal["attributes"];

        for (int i = 0; i < attributesArrayVal.Size(); i++)
        {
            if (!attributesArrayVal[i].IsString())
            {
                return false;
            }            
        }

        m_eventName = filterObjVal["event"].GetString();
        m_type = filterObjVal["type"].GetString();

        m_attributes.clear();
        for (int i = 0; i < attributesArrayVal.Size(); i++)
        {
            m_attributes.insert(attributesArrayVal[i].GetString());
        }

        Init();

        return true;
    }

    void MetricsFilter::InitFromBackend(const CloudGemMetric::ServiceAPI::Filter& filter)
    {
        m_eventName = filter.event;
        m_type = filter.type;

        m_attributes.clear();
        for (const auto& attribute : filter.attributes)
        {
            m_attributes.insert(attribute);
        }

        Init();
    }
}