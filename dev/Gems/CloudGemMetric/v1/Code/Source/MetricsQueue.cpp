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

#include "CloudGemMetric/MetricsQueue.h"
#include "CloudGemMetric/MetricsPriority.h"
#include "CloudGemMetric/MetricManager.h"
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzFramework/FileFunc/FileFunc.h>

namespace CloudGemMetric
{
    MetricsQueue::MetricsQueue():
        m_sizeInBytes(0)
    {
    }

    MetricsQueue::~MetricsQueue()
    {
    }

    void MetricsQueue::AddMetrics(const MetricsAggregator& metrics)
    {
        m_sizeInBytes += metrics.GetSizeInBytes();

        m_metrics.push_back(metrics);        
    }

    void MetricsQueue::MoveMetrics(MetricsAggregator& metrics)
    {
        m_sizeInBytes += metrics.GetSizeInBytes();

        m_metrics.emplace_back(AZStd::move(metrics));        
    }

    AZStd::string MetricsQueue::SerializeToJson() const
    {
        rapidjson::StringBuffer outStrBuffer;
        SerializeToJson(outStrBuffer);
        return outStrBuffer.GetString();
    }

    void MetricsQueue::SerializeToJson(rapidjson::StringBuffer& outStrBuffer) const
    {
        rapidjson::Document doc;

        doc.SetArray();

        for (const auto& metrics : m_metrics)
        {
            metrics.SerializeToJson(doc);
        }

        rapidjson::Writer<rapidjson::StringBuffer> writer(outStrBuffer);
        doc.Accept(writer);
    }

    bool MetricsQueue::ReadFromJson(const char* filePath)
    {        
        AZ::IO::FileIOBase* fileIO = MetricManager::GetFileIO();
        if (!fileIO)
        {
            return false;
        }

        auto result = AzFramework::FileFunc::ReadJsonFile(filePath, fileIO);
        if (!result.IsSuccess())
        {
            AZ_Error("CloudCanvas", false, "Failed to read metrics file");
            return false;
        }

        rapidjson::Document& doc = result.GetValue();
        if (!doc.IsArray())
        {
            AZ_Error("CloudCanvas", false, "Metrics file is invalid");
            return false;
        }       

        AZStd::vector<MetricsAggregator> metrics;
        metrics.resize(doc.Size());

        for (int i = 0; i < doc.Size(); i++)
        {
            if (!metrics[i].ReadFromJson(doc[i]))
            {
                AZ_Error("CloudCanvas", false, "Metrics file is invalid");
                return false;
            }
        }

        m_metrics = AZStd::move(metrics);

        CalcSizeInBytes();

        return true;
    }

    void MetricsQueue::CalcSizeInBytes()
    {
        m_sizeInBytes = 0;
        for (const auto& metrics : m_metrics)
        {
            m_sizeInBytes += metrics.GetSizeInBytes();
        }
    }

    void MetricsQueue::MoveMetrics(MetricsQueue& metricsQueue)
    {
        if (m_metrics.size() == 0)
        {
            m_metrics = AZStd::move(metricsQueue.m_metrics);
            m_sizeInBytes = metricsQueue.m_sizeInBytes;
        }        
        else
        {
            m_metrics.insert(m_metrics.end(), metricsQueue.m_metrics.begin(), metricsQueue.m_metrics.end());
            metricsQueue.m_metrics.clear();

            m_sizeInBytes += metricsQueue.m_sizeInBytes;
        }
        
        metricsQueue.m_sizeInBytes = 0;
    }

    void MetricsQueue::SerializeToJsonForServiceAPIWithSizeLimit(AZStd::vector<AZStd::pair<AZStd::vector<MetricsEventParameter>, rapidjson::StringBuffer>>& outVector, int startIdx, int maxSizeInBytes, int& nextIdx, AZStd::vector<MetricsAttribute>* additionalAttributes) const
    {
        //Serialize to JSON based on the service API request parameters.   Each event has a set of parameters that define whether that event should use secure data storage or compression during server side processing
        //Insensitive / Nocompression = 00
        auto v_00 = AZStd::vector<MetricsEventParameter>() = { MetricsEventParameter(SENSITIVITY_TYPE, 0), MetricsEventParameter(COMPRESSION_MODE, 0) };

        //Insensitive / Compression = 01
        auto v_01 = AZStd::vector<MetricsEventParameter>() = { MetricsEventParameter(SENSITIVITY_TYPE, 0), MetricsEventParameter(COMPRESSION_MODE, 1) };

        //Sensitive / NoCompression = 10
        auto v_10 = AZStd::vector<MetricsEventParameter>() = { MetricsEventParameter(SENSITIVITY_TYPE, 1), MetricsEventParameter(COMPRESSION_MODE, 0) };

        //Sensitive / NoCompression = 11
        auto v_11 = AZStd::vector<MetricsEventParameter>() = { MetricsEventParameter(SENSITIVITY_TYPE, 1), MetricsEventParameter(COMPRESSION_MODE, 1) };

        outVector.emplace_back(AZStd::pair<AZStd::vector<MetricsEventParameter>, rapidjson::StringBuffer>(v_00, {}));
        outVector.emplace_back(AZStd::pair<AZStd::vector<MetricsEventParameter>, rapidjson::StringBuffer>(v_01, {}));
        outVector.emplace_back(AZStd::pair<AZStd::vector<MetricsEventParameter>, rapidjson::StringBuffer>(v_10, {}));
        outVector.emplace_back(AZStd::pair<AZStd::vector<MetricsEventParameter>, rapidjson::StringBuffer>(v_11, {}));

        if (startIdx < 0 || startIdx >= m_metrics.size())
        {
            nextIdx = -1;
            return;
        }

        int curSize = 0;
        auto start = m_metrics.begin() + startIdx;
        auto end = start;        
        for (auto it = start; it != m_metrics.end(); ++it)
        {
            curSize += it->GetSizeInBytes();
            if (curSize <= maxSizeInBytes)
            {
                end = it + 1;
            }
            else
            {
                break;
            }
        }

        if (start == end)
        {
            nextIdx = -1;
            return;
        }

        nextIdx = end == m_metrics.end() ? -1 : end - m_metrics.begin();

        AZStd::vector<rapidjson::Document*> docs;
        //1 document for each type of payloads
        //Insensitive / Nocompression
        docs.emplace_back(new rapidjson::Document());
        //Insensitive / compression
        docs.emplace_back(new rapidjson::Document());
        //Sensitive / Nocompression
        docs.emplace_back(new rapidjson::Document());
        //Sensitive / compression
        docs.emplace_back(new rapidjson::Document());

        for (auto it = docs.begin(); it != docs.end(); it++) {
            (*it)->SetArray();
        }        
        
        for (auto it = start; it != end; ++it)
        {            
            auto params = it->GetEventParameters();
            int vector_idx = 0;
            for (auto paramIt = params.begin(); paramIt != params.end(); paramIt++) {
                auto type = paramIt->GetName();
                auto value = paramIt->GetVal();
                if (type == SENSITIVITY_TYPE)
                {
                    vector_idx |= value << 1;
                }
                else if (type == COMPRESSION_MODE)
                {
                    vector_idx |= value << 0;
                }
            }            
            auto doc = docs[vector_idx];
            it->SerializeToJson(*doc, additionalAttributes);            
        }

        int i = 0;
        for (auto it = docs.begin(); it != docs.end(); it++, i++)
        {            
            auto vout = &outVector[i];
            auto doc = *it;                                             
            if (!doc->Empty() && doc->IsArray())
            {           
                rapidjson::Document d;
                d.SetArray();
                for (rapidjson::Value::ConstValueIterator itr = doc->Begin(); itr != doc->End(); ++itr)
                {
                    if (itr->HasMember(JSON_ATTRIBUTE_EVENT_DEFINITION))
                    {                        
                        rapidjson::Value::ConstMemberIterator event_definition = itr->FindMember(JSON_ATTRIBUTE_EVENT_DEFINITION);
                        d.PushBack(rapidjson::Value(event_definition->value, d.GetAllocator()), d.GetAllocator());
                    }
                }
                rapidjson::Writer<rapidjson::StringBuffer> writer(vout->second);
                d.Accept(writer);
            }
            else
            {
                outVector.erase(outVector.begin() + i);
                i--;
            }
        }
    }

    AZStd::vector<AZStd::pair<AZStd::vector<MetricsEventParameter>, rapidjson::StringBuffer>> MetricsQueue::SerializeToJsonForServiceAPIWithSizeLimit(int startIdx, int maxSizeInBytes, int& nextIdx, AZStd::vector<MetricsAttribute>* additionalAttributes) const
    {        
        AZStd::vector<AZStd::pair<AZStd::vector<MetricsEventParameter>, rapidjson::StringBuffer>> vector;

        SerializeToJsonForServiceAPIWithSizeLimit(vector, startIdx, maxSizeInBytes, nextIdx, additionalAttributes);
        return vector;
    }

    void MetricsQueue::SerializeToJsonForFileWithSizeLimit(rapidjson::StringBuffer& outStrBuffer, int startIdx, int maxSizeInBytes, int& nextIdx, AZStd::vector<MetricsAttribute>* additionalAttributes) const
    {
        if (startIdx < 0 || startIdx >= m_metrics.size())
        {
            nextIdx = -1;
            return;
        }

        int curSize = 0;
        auto start = m_metrics.begin() + startIdx;
        auto end = start;
        for (auto it = m_metrics.begin() + startIdx; it != m_metrics.end(); ++it)
        {
            curSize += it->GetSizeInBytes();
            if (curSize <= maxSizeInBytes)
            {
                end = it + 1;
            }
            else
            {
                break;
            }
        }

        if (start == end)
        {
            nextIdx = -1;
            return;
        }

        nextIdx = end == m_metrics.end() ? -1 : end - m_metrics.begin();

        rapidjson::Document doc;

        doc.SetArray();
        for (auto it = start; it != end; ++it)
        {
            it->SerializeToJson(doc, additionalAttributes);
        }

        rapidjson::Writer<rapidjson::StringBuffer> writer(outStrBuffer);
        doc.Accept(writer);
    }

    AZStd::string MetricsQueue::SerializeToJsonForFileWithSizeLimit(int startIdx, int maxSizeInBytes, int& nextIdx, AZStd::vector<MetricsAttribute>* additionalAttributes) const
    {
        rapidjson::StringBuffer strbuf;
        SerializeToJsonForFileWithSizeLimit(strbuf, startIdx, maxSizeInBytes, nextIdx, additionalAttributes);
        return strbuf.GetString();
    }

    void MetricsQueue::FilterByPriority(int maxSizeInBytes, const MetricsConfigurations& metricsConfigurations)
    {
        if (m_sizeInBytes <= maxSizeInBytes)
        {
            return;
        }

        AZStd::vector<MetricsAggregator> filteredMetrics;
        metricsConfigurations.FilterByPriority(m_metrics, maxSizeInBytes, filteredMetrics);

        m_metrics = AZStd::move(filteredMetrics);
    }

    void MetricsQueue::AppendAttributesToAllMetrics(const MetricsAttribute& attribute)
    {
        for (auto &metrics : m_metrics)
        {
            metrics.AddAttribute(attribute);
        }
    }
}