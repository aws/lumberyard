
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/JSON/document.h>


/*
MetricsFilterGroup is the collection of MetricsFilter
*/
namespace CloudGemMetric
{   
    class MetricsAggregator;
    class MetricsFilter;
    class MetricsAttribute;

    namespace ServiceAPI
    {
        struct Filter;
    }

    class MetricsFilterGroup
    {    
    public:
        MetricsFilterGroup();
        ~MetricsFilterGroup();

        void InitFromBackend(const AZStd::vector<CloudGemMetric::ServiceAPI::Filter>& filters);

        // returns true if this metrics should be filtered, false otherwise and outMetrics will be populated with correct attributes after filtering
        bool ShouldFilterMetrics(const AZStd::string& eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, MetricsAggregator& outMetrics) const;

        void SerializeToJson(rapidjson::Document& doc) const;

        bool ReadFromJson(const rapidjson::Document& doc);

    private:
        AZStd::unordered_map<AZStd::string, MetricsFilter> m_eventNameToFilterMap;
    };    
} // namespace CloudGemMetric
