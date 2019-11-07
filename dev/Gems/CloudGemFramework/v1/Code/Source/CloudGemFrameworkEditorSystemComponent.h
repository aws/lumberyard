/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <AzCore/Component/Component.h>

#include <CloudCanvas/ICloudCanvasEditor.h>

namespace CloudGemFramework
{
    class CloudGemFrameworkEditorSystemComponent
        : public AZ::Component
        , protected CloudCanvas::CloudCanvasEditorRequestBus::Handler
        , public CrySystemEventBus::Handler
    {
    public:
        static const char* COMPONENT_DISPLAY_NAME;
        static const char* COMPONENT_DESCRIPTION;
        static const char* COMPONENT_CATEGORY;
        static const char* SERVICE_NAME;

        AZ_COMPONENT(CloudGemFrameworkEditorSystemComponent, "{1F0DD05C-2FBE-481B-B47B-35F74780B43A}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        //CloudCanvasCommonEditorRequestBus Handlers

        virtual CloudCanvas::AWSClientCredentials GetCredentials() override;
        virtual void SetCredentials(const CloudCanvas::AWSClientCredentials& credentials) override;

        virtual bool ApplyConfiguration() override;

        // CrySystemEventBus Handlers
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams&) override;
    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
    private:
        CloudCanvas::AWSClientCredentials m_editorCredentials;
    };
}
