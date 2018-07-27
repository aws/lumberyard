
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

#include <CloudGemMetric/CloudGemMetricBus.h>
#include <CloudGemMetric/MetricManager.h>
#include <CloudCanvas/CloudCanvasIdentityBus.h>
#include <CrySystemBus.h>
#include "CloudGemMetric/MetricsEventParameter.h"

namespace CloudGemMetric
{
    class CloudGemMetricSystemComponent
        : public AZ::Component
        , public CloudGemFramework::CloudCanvasPlayerIdentityNotificationBus::Handler
        , public AZ::TickBus::Handler
        , protected CloudGemMetricRequestBus::Handler
        , protected CrySystemEventBus::Handler
    {
    public:
        CloudGemMetricSystemComponent();
        AZ_COMPONENT(CloudGemMetricSystemComponent, "{3DC9DFEE-2F15-4B8E-804B-97499182D939}");

        static void Reflect(AZ::ReflectContext* context);

        void OnIdentityReceived();        

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // CloudGemMetricRequestBus interface implementation

        // submit metrics, metrics will be buffered before sending in batch. return false if the metrics is filtered
        bool SubmitMetrics(const char* eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::vector<MetricsEventParameter>& metricsParameters);
        bool SubmitMetricsWithSourceOverride(const char* eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::string& metricSourceOverride, const AZStd::vector<MetricsEventParameter>& metricsParameters);

        // send metrics directly to server, return 0 if the metrics is filtered or player id is not available yet, return a positive id if the request is fired
        int SendMetrics(const char* eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::vector<MetricsEventParameter>& metricsParameters);
        int SendMetricsWithSourceOverride(const char* metricsName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::string& metricSourceOverride, const AZStd::vector<MetricsEventParameter>& metricsParameters);

        bool FilterAndSend(const AZStd::string& metricsName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::string& metricSourceOverride, const AZStd::vector<MetricsEventParameter>& metricsParameters) override;
        bool FilterAndSendCallbacks(const AZStd::string& metricsName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::string& metricSourceOverride, int requestId, OnSendMetricsSuccessCallback successCallback, OnSendMetricsFailureCallback failCallback, const AZStd::vector<MetricsEventParameter>& metricsParameters) override;

        // flush all metrics buffered in memory and on file
        void SendBufferedMetrics();
               
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CrySystemEventBus interface implementation
        virtual void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& params) override;
        ////////////////////////////////////////////////////////////////////////

        using Attributes = AZStd::vector<MetricsAttribute>;
        using Parameters = AZStd::vector<MetricsEventParameter>;

        struct AttributeSubmissionList
        {
            AZ_TYPE_INFO(AttributeSubmissionList, "{58217228-f288-11e7-8c3f-9a214cf093ae}")

            Attributes attributes;

            static void Reflect(AZ::ReflectContext* reflection)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

                if (serializeContext)
                {
                    serializeContext->Class<AttributeSubmissionList>()
                        ->Version(1);
                }

                AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
                if (behaviorContext)
                {
                    behaviorContext->Class<AttributeSubmissionList>("CloudGemMetric_AttributesSubmissionList")
                        ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                        ->Property("attributes", BehaviorValueProperty(&AttributeSubmissionList::attributes))
                        ;
                }
            }
        };

        struct EventParameterList
        {
            AZ_TYPE_INFO(EventParameterList, "{347FC436-CB4C-475F-8E53-59B9129C16FE}")

            Parameters parameters;

            static void Reflect(AZ::ReflectContext* reflection)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

                if (serializeContext)
                {
                    serializeContext->Class<EventParameterList>()
                        ->Version(1);
                }

                AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
                if (behaviorContext)
                {
                    behaviorContext->Class<EventParameterList>("CloudGemMetric_EventParameterList")
                        ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                        ->Property("parameters", BehaviorValueProperty(&EventParameterList::parameters))
                        ;
                }
            }
        };

        static void ReflectMetricsAttribute(AZ::ReflectContext* reflection);
        static void ReflectMetricsEventParameter(AZ::ReflectContext* reflection);
    private:
        MetricManager m_metricManager;
    };
}
