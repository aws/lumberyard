
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/JSON/document.h>
#include "AWS/ServiceAPI/CloudGemMetricClientComponent.h"

/*
MetricsFilter defines rule to filter out metrics
*/
namespace CloudGemMetric
{   
    class MetricsAttribute;
    class MetricsAggregator;

    class MetricsFilter
    {    
    public:
        MetricsFilter();
        ~MetricsFilter();

        void InitFromBackend(const CloudGemMetric::ServiceAPI::Filter& filter);

        void SetEventName(const char* eventName) { m_eventName = eventName; };
        const AZStd::string& GetEventName() const { return m_eventName; };

        void AddAttribute(const char* attr) { m_attributes.insert(attr); };
        const AZStd::unordered_set<AZStd::string>& GetAttributes() const { return m_attributes; };

        void SetType(const char* type) { m_type = type; };
        const AZStd::string& GetType() const { return m_type; };

        // after event name, attributes, type are set, call this to initialize the filter
        void Init();

        // returns true if this metrics should be filtered, false otherwise and outMetrics will be populated with correct attributes after filtering
        bool ShouldFilterMetrics(const AZStd::string& eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, MetricsAggregator& outMetrics) const;

        void SerializeToJson(rapidjson::Document& doc, rapidjson::Value& filtersArrayVal) const;
        bool ReadFromJson(const rapidjson::Value& filterObjVal);
    private:
        bool(*m_filterFunc)(const AZStd::unordered_set<AZStd::string>& attributesToFilter, const AZStd::string& eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, MetricsAggregator& outMetrics);
        AZStd::string m_eventName;
        AZStd::unordered_set<AZStd::string> m_attributes;
        AZStd::string m_type;
    };    
} // namespace CloudGemMetric
