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

#include "CloudGemFramework_precompiled.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/std/parallel/atomic.h>

#include <ISystem.h>
#include <CloudCanvas/ICloudCanvas.h>

#include "CloudGemFrameworkSystemComponent.h"
#include "CloudGemFramework/AwsApiJob.h"
#include "CloudGemFramework/HttpRequestJob.h"

#include <aws/core/auth/AWSCredentialsProvider.h>

#include <CloudCanvasCommon/CloudCanvasCommonBus.h>
#include <CloudCanvas/CloudCanvasMappingsBus.h>

#include <CloudGemFramework/Error.h>
#include <CloudGemFramework/HttpClientComponent.h>

namespace CloudGemFramework
{

    const char* CloudGemFrameworkSystemComponent::COMPONENT_DISPLAY_NAME = "CloudGemFramework";
    const char* CloudGemFrameworkSystemComponent::COMPONENT_DESCRIPTION = "Provides a framework for Gems that use AWS.";
    const char* CloudGemFrameworkSystemComponent::COMPONENT_CATEGORY = "CloudCanvas";
    const char* CloudGemFrameworkSystemComponent::SERVICE_NAME = "CloudGemFrameworkService";

    void CloudGemFrameworkSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        Error::Reflect(context);
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {

            serialize->Class<CloudGemFrameworkSystemComponent, AZ::Component>()
                ->Version(2, &CloudGemFrameworkSystemComponent::VersionConverter)
                ;

            AZ::EditContext* ec = serialize->GetEditContext();
            if (ec)
            {
                ec->Class<CloudGemFrameworkSystemComponent>(COMPONENT_DISPLAY_NAME, COMPONENT_DESCRIPTION)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, COMPONENT_CATEGORY)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC(COMPONENT_CATEGORY))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    //=========================================================================
    bool CloudGemFrameworkSystemComponent::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() == 1)
        {
            classElement.RemoveElementByName(AZ_CRC("ThreadCount"));
            classElement.RemoveElementByName(AZ_CRC("FirstThreadCPU"));
            classElement.RemoveElementByName(AZ_CRC("ThreadPriority"));
            classElement.RemoveElementByName(AZ_CRC("ThreadStackSize"));
        }

        return true;
    }

    void CloudGemFrameworkSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC(SERVICE_NAME));
    }

    void CloudGemFrameworkSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC(SERVICE_NAME));
    }

    void CloudGemFrameworkSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("CloudCanvasCommonService"));
        required.push_back(AZ_CRC("JobsService"));
    }

    void CloudGemFrameworkSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void CloudGemFrameworkSystemComponent::Init()
    {

    }

    void CloudGemFrameworkSystemComponent::Activate()
    {
        HttpRequestJob::StaticInit();
        CloudGemFrameworkRequestBus::Handler::BusConnect();
        CloudCanvasCommon::CloudCanvasCommonNotificationsBus::Handler::BusConnect();
    }

#ifdef _DEBUG

    AZStd::atomic_int g_jobCount{0};

    void CloudGemFrameworkSystemComponent::IncrementJobCount()
    {
        ++g_jobCount;
    }

    void CloudGemFrameworkSystemComponent::DecrementJobCount()
    {
        --g_jobCount;
    }

#endif

    void CloudGemFrameworkSystemComponent::Deactivate()
    {
        CloudCanvasCommon::CloudCanvasCommonNotificationsBus::Handler::BusDisconnect();
        CloudGemFrameworkRequestBus::Handler::BusDisconnect();

        EBUS_EVENT(InternalCloudGemFrameworkNotificationBus, OnCloudGemFrameworkDeactivated);

#ifdef _DEBUG
        if(g_jobCount > 0)
        {
            AZ_Error(COMPONENT_DISPLAY_NAME, g_jobCount == 0, "%i AwsApiJob objects were not deleted before CloudGemFrameworkSystemComponent was deactivated.", (int)g_jobCount);
        }
#endif
        HttpRequestJob::StaticShutdown();
    }

    AZStd::string CloudGemFrameworkSystemComponent::GetServiceUrl(const AZStd::string& serviceName) {
        AZStd::string configName = serviceName + ".ServiceApi";
        AZStd::string serviceUrl;
        EBUS_EVENT_RESULT(serviceUrl, CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, configName);
        AZ_Warning(COMPONENT_DISPLAY_NAME, !serviceUrl.empty(), "No mapping provided for the %s service.", serviceName.c_str());
        return serviceUrl;
    }

    CloudCanvas::RequestRootCAFileResult CloudGemFrameworkSystemComponent::GetRootCAFile(AZStd::string& filePath)
    {
        CloudCanvas::RequestRootCAFileResult requestResult;
        EBUS_EVENT_RESULT(requestResult, CloudCanvasCommon::CloudCanvasCommonRequestBus,RequestRootCAFile, filePath);
        return requestResult;
    }

    AZ::JobContext* CloudGemFrameworkSystemComponent::GetDefaultJobContext()
    {
        AZ::JobContext* jobContext{nullptr};
        EBUS_EVENT_RESULT(jobContext, CloudCanvasCommon::CloudCanvasCommonRequestBus, GetDefaultJobContext);
        return jobContext;
    }

    AwsApiJobConfig* CloudGemFrameworkSystemComponent::GetDefaultConfig()
    {
        return AwsApiJob::GetDefaultConfig();
    }

    void CloudGemFrameworkSystemComponent::RootCAFileSet(const AZStd::string& caPath)
    {
        AwsApiJobConfig* defaultConfig = GetDefaultConfig();
        if (defaultConfig)
        {
            defaultConfig->caFile = caPath.c_str();
        }
    }
}
