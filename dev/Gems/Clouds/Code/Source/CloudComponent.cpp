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
#include "StdAfx.h"
#include <MathConversion.h>
#include "CloudComponentRenderNode.h"

namespace CloudsGem
{
    void CloudComponent::Reflect(AZ::ReflectContext* context)
    {
        CloudParticleData::Reflect(context);
        CloudComponentRenderNode::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CloudComponent, AZ::Component>()
                ->Version(1)
                ->Field("Cloud Render Node", &CloudComponent::m_renderNode);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<CloudComponent>()->RequestBus("CloudComponentBehaviorRequestBus");
            
            behaviorContext->EBus<CloudComponentBehaviorRequestBus>("CloudComponentBehaviorRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Environment")
                ->Event("SetAutoMove", &CloudComponentBehaviorRequestBus::Events::SetAutoMove)
                ->Event("GetAutoMove", &CloudComponentBehaviorRequestBus::Events::GetAutoMove)
                ->VirtualProperty("AutoMove", "GetAutoMove", "SetAutoMove")
                ->Event("SetVelocity", &CloudComponentBehaviorRequestBus::Events::SetVelocity)
                ->Event("GetVelocity", &CloudComponentBehaviorRequestBus::Events::GetVelocity)
                ->VirtualProperty("Velocity", "GetVelocity", "SetVelocity")
                ->Event("SetFadeDistance", &CloudComponentBehaviorRequestBus::Events::SetFadeDistance)
                ->Event("GetFadeDistance", &CloudComponentBehaviorRequestBus::Events::GetFadeDistance)
                ->VirtualProperty("FadeDistance", "GetFadeDistance", "SetFadeDistance")
                ->Event("SetVolumetricDensity", &CloudComponentBehaviorRequestBus::Events::SetVolumetricDensity)
                ->Event("GetVolumetricDensity", &CloudComponentBehaviorRequestBus::Events::GetVolumetricDensity)
                ->VirtualProperty("VolumetricDensity", "GetVolumetricDensity", "SetVolumetricDensity");
        }
    }

    void CloudComponent::Activate()
    {
        LmbrCentral::RenderNodeRequestBus::Handler::BusConnect(GetEntityId());
        m_renderNode.AttachToEntity(GetEntityId());
    }

    void CloudComponent::Deactivate()
    {
        m_renderNode.AttachToEntity(AZ::EntityId());
        LmbrCentral::RenderNodeRequestBus::Handler::BusDisconnect();
    }
}