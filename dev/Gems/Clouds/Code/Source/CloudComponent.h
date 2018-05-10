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
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Rendering/RenderNodeBus.h>
#include "CloudComponentRenderNode.h"

namespace CloudsGem
{
    class CloudComponent
        : public AZ::Component
        , public LmbrCentral::RenderNodeRequestBus::Handler
    {
    public:
        friend class EditorCloudComponent;
        AZ_COMPONENT(CloudComponent, "{8102E7D9-B59A-4296-A205-92428AC43E63}");
        ~CloudComponent() override = default;

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // RenderNodeRequestBus::Handler
        IRenderNode* GetRenderNode() override { return &m_renderNode;  }
        float GetRenderNodeRequestBusOrder() const override { return 100.0f; }

    protected:

        // Services
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("SkyCloudService", 0x89fcf917));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("SkyCloudService", 0x89fcf917));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
        }

        // Reflection
        static void Reflect(AZ::ReflectContext* context);

        CloudComponentRenderNode m_renderNode;
    };
}