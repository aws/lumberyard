
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

#pragma once

#include <AzCore/Component/Component.h>
#include "ImageLoaderBus.h"

namespace CloudGemDefectReporter
{
    class ImageLoaderSystemComponent :
        public AZ::Component,
        protected ImageLoaderRequestBus::Handler
    {
    public:
        AZ_COMPONENT(ImageLoaderSystemComponent, "{C3450E8E-9A59-4A49-858F-77DC2933B59C}");
        ImageLoaderSystemComponent();
        static void Reflect(AZ::ReflectContext* context);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        ////////////////////////////////////////////////////////////////////////
        // ImageLoaderRequests implementation
        ////////////////////////////////////////////////////////////////////////
        virtual bool CreateRenderTargetFromImageFile(const AZStd::string& renderTargetName, const AZStd::string& imagePath) override;
        virtual AZStd::string GetRenderTargetName() override;

        AZStd::string m_renderTargetName;
        int m_currentTextureID = -1;
    };
}