
#pragma once

#include <AzCore/EBus/EBus.h>
#include "CloudGemMetric/MetricsAttribute.h"
#include "CloudGemMetric/MetricsEventParameter.h"

namespace CloudGemMetric
{   
    using OnSendMetricsSuccessCallback = AZStd::function<void(int)>;
    using OnSendMetricsFailureCallback = AZStd::function<void(int)>;

    class CloudGemMetricRequests
        : public AZ::EBusTraits
    {    
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // submit metrics, metrics will be buffered before sending in batch. return false if the metrics is filtered
        virtual bool SubmitMetrics(const char* eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::vector<MetricsEventParameter>& metricsParameters) = 0;
        virtual bool SubmitMetricsWithSourceOverride(const char* eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::string& metricSourceOverride, const AZStd::vector<MetricsEventParameter>& metricsParameters) = 0;

        // send metrics directly to server, return 0 if the metrics is filtered or player id is not available yet, return a positive id if the request is fired
        virtual int SendMetrics(const char* metricsName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::vector<MetricsEventParameter>& metricsParameters) = 0;
        virtual int SendMetricsWithSourceOverride(const char* metricsName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::string& metricSourceOverride, const AZStd::vector<MetricsEventParameter>& metricsParameters) = 0;

        virtual bool FilterAndSend(const AZStd::string& metricsName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::string& metricSourceOverride, const AZStd::vector<MetricsEventParameter>& metricsParameters) { return false; }
        virtual bool FilterAndSendCallbacks(const AZStd::string& metricsName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::string& metricSourceOverride, int requestId, OnSendMetricsSuccessCallback successCallback, OnSendMetricsFailureCallback failCallback, const AZStd::vector<MetricsEventParameter>& metricsParameters) { return false; }

        // flush all metrics buffered in memory and on file
        virtual void SendBufferedMetrics() = 0;

    };

    using CloudGemMetricRequestBus = AZ::EBus<CloudGemMetricRequests>;


    class CloudGemMetricNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual void OnSendMetricsSuccess(int requestId) {}

        virtual void OnSendMetricsFailure(int requestId) {}
    };

    using CloudGemMetricNotificationBus = AZ::EBus<CloudGemMetricNotifications>;
} // namespace CloudGemMetric
