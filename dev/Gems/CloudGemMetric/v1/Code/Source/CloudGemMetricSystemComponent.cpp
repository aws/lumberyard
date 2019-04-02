/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "StdAfx.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include "CloudGemMetricSystemComponent.h"
#include "CloudGemMetric/DefaultAttributesGenerator.h"
#include "CloudGemMetric/MetricsAttribute.h"
#include "CloudGemMetric/MetricsEventParameter.h"


namespace CloudGemMetric
{
    CloudGemMetricSystemComponent::CloudGemMetricSystemComponent():
        m_metricManager(new DefaultAttributesGenerator())
    {

    }

    void CloudGemMetricSystemComponent::ReflectMetricsEventParameter(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<MetricsEventParameter>()
                ->Version(1);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->Class<MetricsEventParameter>("CloudGemMetric_MetricsEventParameter")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Method("SetName", &MetricsEventParameter::SetName)
                ->Method("SetVal", (void (MetricsEventParameter::*)(int)) &MetricsEventParameter::SetVal)
                ->Method("GetSensitivityTypeName", &MetricsEventParameter::GetSensitivityTypeName)
                ->Method("GetCompressionModeName", &MetricsEventParameter::GetCompressionModeName)
                ;
        }

        EventParameterList::Reflect(reflection);
    }

    void CloudGemMetricSystemComponent::ReflectMetricsAttribute(AZ::ReflectContext* reflection)
    {
        // Reflect MetricsAttribute
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<MetricsAttribute>()
                ->Version(1);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->Class<MetricsAttribute>("CloudGemMetric_MetricsAttribute")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Method("SetName", &MetricsAttribute::SetName)
                ->Method("SetStrValue", (void (MetricsAttribute::*)(const char*)) &MetricsAttribute::SetVal)
                ->Method("SetIntValue", (void (MetricsAttribute::*)(int)) &MetricsAttribute::SetVal)
                ->Method("SetDoubleValue", (void (MetricsAttribute::*)(double)) &MetricsAttribute::SetVal)
                ;
        }

        AttributeSubmissionList::Reflect(reflection);
    }


    void CloudGemMetricSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        ReflectMetricsAttribute(context);
        ReflectMetricsEventParameter(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudGemMetricSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudGemMetricSystemComponent>("CloudGemMetric", "Create events in your game, view daily dasbhoards in the Cloud Gem Portal and analyze the data in depth with Amazon QuickSight.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }            
        }    

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<CloudGemMetricRequestBus>("CloudGemMetricRequestBus")
                ->Event("SubmitMetrics", &CloudGemMetricRequestBus::Events::SubmitMetrics)
                ->Event("SendMetrics", &CloudGemMetricRequestBus::Events::SendMetrics)
                ->Event("FilterAndSend", &CloudGemMetricRequestBus::Events::FilterAndSend)
                ->Event("SubmitMetricsWithSourceOverride", &CloudGemMetricRequestBus::Events::SubmitMetricsWithSourceOverride)
                ->Event("SendMetricsWithSourceOverride", &CloudGemMetricRequestBus::Events::SendMetricsWithSourceOverride)
                ->Event("SendBufferedMetrics", &CloudGemMetricRequestBus::Events::SendBufferedMetrics)                
                ;
        }
    }

    void CloudGemMetricSystemComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        m_metricManager.OnTick(deltaTime, time);
    }    

    void CloudGemMetricSystemComponent::OnIdentityReceived()
    {
        m_metricManager.GetMetricsConfigsFromServerAsync();
        m_metricManager.RefreshPlayerId();
    }

    void CloudGemMetricSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemMetricService"));
    }

    void CloudGemMetricSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CloudGemMetricService"));
    }

    void CloudGemMetricSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void CloudGemMetricSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }


    void CloudGemMetricSystemComponent::Init()
    {
    }

    void CloudGemMetricSystemComponent::Activate()
    {       
        CloudGemMetricRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
        CloudGemFramework::CloudCanvasPlayerIdentityNotificationBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
    }

    void CloudGemMetricSystemComponent::Deactivate()
    {
        CrySystemEventBus::Handler::BusDisconnect();
        CloudGemFramework::CloudCanvasPlayerIdentityNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        CloudGemMetricRequestBus::Handler::BusDisconnect();
    }

    void CloudGemMetricSystemComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& params)
    {
        AZ_UNUSED(system);
        AZ_UNUSED(params);
        m_metricManager.Init();
    }

    void CloudGemMetricSystemComponent::OnCrySystemShutdown(ISystem& system)
    {
        AZ_UNUSED(system);
        m_metricManager.Shutdown();
    }

    bool CloudGemMetricSystemComponent::SubmitMetrics(const char* eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::vector<MetricsEventParameter>& metricsParameters)
    {
        return m_metricManager.SubmitMetrics(eventName, metricsAttributes, AZStd::string(), metricsParameters);
    }

    int CloudGemMetricSystemComponent::SendMetrics(const char* eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::vector<MetricsEventParameter>& metricsParameters)
    {
        return m_metricManager.SendMetrics(eventName, metricsAttributes,
            [](int requestId)
        {
            EBUS_EVENT(CloudGemMetricNotificationBus, OnSendMetricsSuccess, requestId);

            AZ_Printf("CloudCanvas", "Send metrics succeeded, requestId: %d", requestId);
        },
            [](int requestId)
        {
            EBUS_EVENT(CloudGemMetricNotificationBus, OnSendMetricsFailure, requestId);

            AZ_Printf("CloudCanvas", "Send metrics failed, requestId: %d", requestId);
        }, AZStd::string(), metricsParameters);
    }

    bool CloudGemMetricSystemComponent::FilterAndSend(const AZStd::string& metricsName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::string& metricSourceOverride, const AZStd::vector<MetricsEventParameter>& metricsParameters)
    {
        return FilterAndSendCallbacks(metricsName, metricsAttributes, metricSourceOverride, 0, {}, {}, metricsParameters);
    }

    bool CloudGemMetricSystemComponent::FilterAndSendCallbacks(const AZStd::string& metricsName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::string& metricSourceOverride, int requestId, OnSendMetricsSuccessCallback successCallback, OnSendMetricsFailureCallback failCallback, const AZStd::vector<MetricsEventParameter>& metricsParameters)
    {     
        return m_metricManager.FilterAndSendCallbacks(metricsName, metricsAttributes, metricSourceOverride, requestId, successCallback, failCallback, metricsParameters);
    }

    bool CloudGemMetricSystemComponent::SubmitMetricsWithSourceOverride(const char* eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::string& metricSourceOverride, const AZStd::vector<MetricsEventParameter>& metricsParameters)
    {
        return m_metricManager.SubmitMetrics(eventName, metricsAttributes, metricSourceOverride, metricsParameters);
    }

    int CloudGemMetricSystemComponent::SendMetricsWithSourceOverride(const char* eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::string& metricSourceOverride, const AZStd::vector<MetricsEventParameter>& metricsParameters)
    {
        return m_metricManager.SendMetrics(eventName, metricsAttributes,
            [](int requestId)
        {
            EBUS_EVENT(CloudGemMetricNotificationBus, OnSendMetricsSuccess, requestId);

            AZ_Printf("CloudCanvas", "Send metrics succeeded, requestId: %d", requestId);
        },
            [](int requestId)
        {
            EBUS_EVENT(CloudGemMetricNotificationBus, OnSendMetricsFailure, requestId);

            AZ_Printf("CloudCanvas", "Send metrics failed, requestId: %d", requestId);
        }, metricSourceOverride, metricsParameters);
    }

    void CloudGemMetricSystemComponent::SendBufferedMetrics()
    {
        m_metricManager.SendBufferedMetrics();
    }
   
}
