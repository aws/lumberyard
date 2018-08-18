
#pragma once

#include "CloudGemMetric/MetricsEventParameter.h"
#include "CloudGemMetric/MetricsAttribute.h"
#include <AzCore/JSON/document.h>

/*
MetricsAggregator is used to represent one event which contains multiple metrics attributes

e.g. a gun shot event can contain player position, bullet direction etc
*/
namespace CloudGemMetric
{   
    static const char* JSON_ATTRIBUTE_PARAMETERS = "parameters";
    static const char* JSON_ATTRIBUTE_EVENT_DEFINITION = "event_definition";

    class MetricsAggregator
    {    
    public:
        MetricsAggregator(const AZStd::string& name, const AZStd::vector<MetricsAttribute>& metricsAttributes);
        MetricsAggregator();
        ~MetricsAggregator();

        const AZStd::string& GetEventName() const { return m_name; };
        void SetEventName(const char* name) { m_name = name; };
        void SetEventName(const AZStd::string& name) { m_name = name; };

        const AZStd::vector<MetricsAttribute>& GetAttributes() const { return m_attributes; };        
        void SetAttributes(const AZStd::vector<MetricsAttribute>& attributes) { m_attributes = attributes; };
        void SetEventParameters(const AZStd::vector<MetricsEventParameter>& parameters) { m_parameters = parameters; };        
        void AddAttribute(const MetricsAttribute& attribute) {             
            m_attributes.push_back(attribute); 
        };
        const AZStd::vector<MetricsEventParameter>& GetEventParameters() const { return m_parameters; };

        void SetPlayerIdIfNotExist(const AZStd::string& playerId);
        bool HasPlayerId() const { return m_playerId.empty() ? false : true; };

        int GetSizeInBytes() const;

        rapidjson::Value SerializeEventToJson(rapidjson::Document& doc, AZStd::vector<MetricsAttribute>* additionalAttributes) const;
        void SerializeToJson(rapidjson::Document& doc, AZStd::vector<MetricsAttribute>* additionalAttributes = nullptr) const;
        AZStd::string SerializeToJson(AZStd::vector<MetricsAttribute>* additionalAttributes = nullptr) const;
        bool ReadFromJson(rapidjson::Value& metricsObjVal);

    private:
        AZStd::string m_name;
        AZStd::string m_playerId;
        AZStd::vector<MetricsAttribute> m_attributes;
        AZStd::vector<MetricsEventParameter> m_parameters;
    };    
} // namespace CloudGemMetric
