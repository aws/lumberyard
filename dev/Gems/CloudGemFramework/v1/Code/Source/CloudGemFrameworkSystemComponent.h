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

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobCancelGroup.h>

#include <CloudGemFramework/CloudGemFrameworkBus.h>
#include <CloudCanvasCommon/CloudCanvasCommonBus.h>

namespace CloudGemFramework
{
    class CloudGemFrameworkSystemComponent
        : public AZ::Component
        , protected CloudGemFrameworkRequestBus::Handler
        , public CloudCanvasCommon::CloudCanvasCommonNotificationsBus::Handler
    {
    public:

        static const char* COMPONENT_DISPLAY_NAME;
        static const char* COMPONENT_DESCRIPTION;
        static const char* COMPONENT_CATEGORY;
        static const char* SERVICE_NAME;

        AZ_COMPONENT(CloudGemFrameworkSystemComponent, "{3A468AF0-3D40-4E7C-95AF-E6F9FCF7F1EE}");

        CloudGemFrameworkSystemComponent() = default;
        
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // CloudGemFrameworkRequestBus interface implementation
        AZStd::string GetServiceUrl(const AZStd::string& serviceName) override;
        AZ::JobContext* GetDefaultJobContext() override;
        virtual CloudCanvas::RequestRootCAFileResult GetRootCAFile(AZStd::string& resultPath) override;
#ifdef _DEBUG
        virtual void IncrementJobCount() override;
        virtual void DecrementJobCount() override;
#endif
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        // CloudCanvasCommonBus Event Handlers
        virtual void RootCAFileSet(const AZStd::string& caPath) override;

        virtual AwsApiJobConfig* GetDefaultConfig() override;
    private:
#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
        // Workaround for VS2013 - Delete the copy constructor and make it private
        // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
        CloudGemFrameworkSystemComponent(const CloudGemFrameworkSystemComponent &) = delete;
#endif

        int m_threadCount{2};
        int m_firstThreadCPU{-1};
        int m_threadPriority{0};
        int m_threadStackSize{-1};

        // Order here is of importance. To be correct, JobContext needs to 
        // destruct before the JobManager and the JobCancelGroup.
        AZStd::unique_ptr<AZ::JobCancelGroup> m_jobCancelGroup{nullptr};
        AZStd::unique_ptr<AZ::JobManager> m_jobManager{nullptr};
        AZStd::unique_ptr<AZ::JobContext> m_jobContext{nullptr};

    };
}
