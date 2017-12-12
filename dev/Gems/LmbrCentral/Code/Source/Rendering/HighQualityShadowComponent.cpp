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
#include "HighQualityShadowComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <LmbrCentral/Rendering/RenderNodeBus.h>
#include <I3DEngine.h>


namespace LmbrCentral
{
    //////////////////////////////////////////////////////////////////////////

    HighQualityShadowConfig::HighQualityShadowConfig()
        : m_enabled(true)
        , m_constBias(0.001f)
        , m_slopeBias(0.01f)
        , m_jitter(0.01f)
        , m_bboxScale(1,1,1)
        , m_shadowMapSize(1024)
    {

    }

    void HighQualityShadowConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<HighQualityShadowConfig>()
                ->Version(1)
                ->Field("Enabled", &HighQualityShadowConfig::m_enabled)
                ->Field("ConstBias", &HighQualityShadowConfig::m_constBias)
                ->Field("SlopeBias", &HighQualityShadowConfig::m_slopeBias)
                ->Field("Jitter", &HighQualityShadowConfig::m_jitter)
                ->Field("BBoxScale", &HighQualityShadowConfig::m_bboxScale)
                ->Field("ShadowMapSize", &HighQualityShadowConfig::m_shadowMapSize)
                ;
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void HighQualityShadowComponent::Reflect(AZ::ReflectContext* context)
    {
        HighQualityShadowConfig::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<HighQualityShadowComponent, AZ::Component>()
                ->Version(1)
                ->Field("HighQualityShadowConfig", &HighQualityShadowComponent::m_config);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<HighQualityShadowComponentRequestBus>("HighQualityShadowComponentRequestBus")
                ->Event("SetEnabled", &HighQualityShadowComponentRequestBus::Events::SetEnabled, { { {"Enabled"} } })
                ->Event("GetEnabled", &HighQualityShadowComponentRequestBus::Events::GetEnabled)
                ->VirtualProperty("Enabled", "GetEnabled", "SetEnabled");
        }
    }



    void HighQualityShadowComponent::Activate()
    {
        HighQualityShadowComponentUtils::ApplyShadowSettings(GetEntityId(), m_config);
        HighQualityShadowComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void HighQualityShadowComponent::Deactivate()
    {
        HighQualityShadowComponentUtils::RemoveShadow(GetEntityId());
        HighQualityShadowComponentRequestBus::Handler::BusDisconnect();
    }

    bool HighQualityShadowComponent::GetEnabled()
    {
        return m_config.m_enabled;
    }

    void HighQualityShadowComponent::SetEnabled(bool enabled)
    {
        m_config.m_enabled = enabled;
        HighQualityShadowComponentUtils::ApplyShadowSettings(GetEntityId(), m_config);
    }

    void HighQualityShadowComponentUtils::ApplyShadowSettings(AZ::EntityId entityId, const HighQualityShadowConfig& config)
    {
        IRenderNode* renderNode = nullptr;
        LmbrCentral::RenderNodeRequestBus::EventResult(renderNode, entityId, &LmbrCentral::RenderNodeRequests::GetRenderNode);
        if (renderNode)
        {
            if (config.m_enabled)
            {
                const Vec3 bboxScaleVec3(config.m_bboxScale.GetX(), config.m_bboxScale.GetY(), config.m_bboxScale.GetZ());
                gEnv->p3DEngine->AddPerObjectShadow(renderNode, config.m_constBias, config.m_slopeBias, config.m_jitter, bboxScaleVec3, config.m_shadowMapSize);
            }
            else
            {
                gEnv->p3DEngine->RemovePerObjectShadow(renderNode);
            }
        }
    }

    void HighQualityShadowComponentUtils::RemoveShadow(AZ::EntityId entityId)
    {
        IRenderNode* renderNode = nullptr;
        LmbrCentral::RenderNodeRequestBus::EventResult(renderNode, entityId, &LmbrCentral::RenderNodeRequests::GetRenderNode);
        if (renderNode)
        {
            gEnv->p3DEngine->RemovePerObjectShadow(renderNode);
        }
    }

} // namespace LmbrCentral

