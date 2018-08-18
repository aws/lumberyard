
#pragma once

#include "CloudGemMetric/MetricsAggregator.h"
#include "CloudGemMetric/MetricsAttribute.h"
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

/*
MetricsQueue is a collection of MetricsAggregator
*/
namespace CloudGemMetric
{   
    class MetricsConfigurations;

    class MetricsQueue
    {    
    public:
        MetricsQueue();
        ~MetricsQueue();

        void AddMetrics(const MetricsAggregator& metrics);
        void AppendAttributesToAllMetrics(const MetricsAttribute& attribute);
        void MoveMetrics(MetricsAggregator& metrics);
        void MoveMetrics(MetricsQueue& metricsQueue);        
        const AZStd::vector<MetricsAggregator>& GetMetrics() const { return m_metrics; };
        AZStd::vector<MetricsAggregator>& GetMetrics() { return m_metrics; };
        int GetNumMetrics() const { return m_metrics.size(); };
        int GetSizeInBytes() const { return m_sizeInBytes; };
        void FilterByPriority(int maxSizeInBytes, const MetricsConfigurations& metricsConfigurations);

        // use this version to avoid extra memory copy by passing in a string buffer
        void SerializeToJson(rapidjson::StringBuffer& outStrBuffer) const;
        AZStd::string SerializeToJson() const;

        // use this version to avoid extra memory copy by passing in a string buffer
        void SerializeToJsonForServiceAPIWithSizeLimit(AZStd::vector<AZStd::pair<AZStd::vector<MetricsEventParameter>, rapidjson::StringBuffer>>& outVector, int startIdx, int maxSizeInBytes, int& nextIdx, AZStd::vector<MetricsAttribute>* additionalAttributes = nullptr) const;
        AZStd::vector<AZStd::pair<AZStd::vector<MetricsEventParameter>, rapidjson::StringBuffer>> SerializeToJsonForServiceAPIWithSizeLimit(int startIdx, int maxSizeInBytes, int& nextIdx, AZStd::vector<MetricsAttribute>* additionalAttributes = nullptr) const;
        
        void SerializeToJsonForFileWithSizeLimit(rapidjson::StringBuffer& outStrBuffer, int startIdx, int maxSizeInBytes, int& nextIdx, AZStd::vector<MetricsAttribute>* additionalAttributes = nullptr) const;
        AZStd::string SerializeToJsonForFileWithSizeLimit(int startIdx, int maxSizeInBytes, int& nextIdx, AZStd::vector<MetricsAttribute>* additionalAttributes = nullptr) const;

        bool ReadFromJson(const char* filePath);        

    private:
        void CalcSizeInBytes();
    private:
        AZStd::vector<MetricsAggregator> m_metrics;
        int m_sizeInBytes;
    };    
} // namespace CloudGemMetric
