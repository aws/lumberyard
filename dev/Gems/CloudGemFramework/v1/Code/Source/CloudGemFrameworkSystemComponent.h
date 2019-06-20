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
#include <AzCore/Serialization/SerializeContext.h>

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
        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
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


    };
}
