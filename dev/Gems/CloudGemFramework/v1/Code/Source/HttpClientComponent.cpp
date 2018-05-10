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

#include "CloudGemFramework_precompiled.h"

#include <CloudGemFramework/HttpClientComponent.h>
#include <CloudGemFramework/HttpRequestJob.h>
#include <CloudGemFramework/ServiceJobUtil.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace CloudGemFramework
{
    class BehaviorHttpClientComponentNotificationBusHandler : public HttpClientComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorHttpClientComponentNotificationBusHandler, "{6DE753FD-6BE9-4C1C-B566-8F382666E7CE}", AZ::SystemAllocator,
            OnHttpRequestSuccess, OnHttpRequestFailure);

        void OnHttpRequestSuccess(int responseCode, AZStd::string responseBody) override
        {
            Call(FN_OnHttpRequestSuccess, responseCode, responseBody);
        }

        void OnHttpRequestFailure(int responseCode) override
        {
            Call(FN_OnHttpRequestFailure, responseCode);
        }
    };

    void HttpClientComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<HttpClientComponent, AZ::Component>()
                ->Version(1);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<HttpClientComponent>("HttpClientComponent", "CloudCanvas Http Client Component")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Cloud Gem Framework")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/HttpClientComponent.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/HttpClientComponent.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"));
            }
        }
        
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->EBus<HttpClientComponentRequestBus>("HttpClientComponentRequestBus")
                ->Event("MakeHttpRequest", &HttpClientComponentRequestBus::Events::MakeHttpRequest)
                ;

            behaviorContext->EBus<HttpClientComponentNotificationBus>("HttpClientComponentNotificationBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Handler<BehaviorHttpClientComponentNotificationBusHandler>()
                ;

        }
    }

    void HttpClientComponent::Init()
    {
    }

    void HttpClientComponent::Activate()
    {
        HttpClientComponentRequestBus::Handler::BusConnect(m_entity->GetId());
    }

    void HttpClientComponent::Deactivate()
    {
        HttpClientComponentRequestBus::Handler::BusDisconnect();
    }

    /*
     * HttpClientComponentRequestBus::Handler
     */
    void HttpClientComponent::MakeHttpRequest(AZStd::string url, AZStd::string method, AZStd::string jsonBody)
    {
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        auto job = aznew HttpRequestJob(true, HttpRequestJob::GetDefaultConfig());
        AZ::EntityId entityId = m_entity->GetId();
        
        job->SetCallbacks(
            [entityId](const AZStd::shared_ptr<HttpRequestJob::Response>& response)
            {
                EBUS_EVENT_ID(entityId, HttpClientComponentNotificationBus, OnHttpRequestSuccess, response->GetResponseCode(), response->GetResponseBody());
            },
            [entityId](const AZStd::shared_ptr<HttpRequestJob::Response>& response)
            {
                EBUS_EVENT_ID(entityId, HttpClientComponentNotificationBus, OnHttpRequestFailure, response->GetResponseCode());
            }
        );
        job->SetUrl(url);
        job->SetMethod(method);
        ConfigureJsonServiceRequest(*job, jsonBody);

        job->Start();
#endif // #if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
    }

} // namespace CloudGemFramework
