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

#include "Rain_precompiled.h"
#include "RainComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <MathConversion.h>

#include "IGameFramework.h"
#include "IViewSystem.h"

namespace Rain
{
    void RainComponent::Reflect(AZ::ReflectContext* context)
    {
        RainOptions::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RainComponent, AZ::Component>()
                ->Version(1)
                ->Field("Enabled", &RainComponent::m_enabled)
                ->Field("Rain Options", &RainComponent::m_rainOptions)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<RainComponentRequestBus>("RainComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)

                ->Event("Enable", &RainComponentRequestBus::Events::Enable)

                ->Event("Disable", &RainComponentRequestBus::Events::Disable)

                ->Event("Toggle", &RainComponentRequestBus::Events::Toggle)

                ->Event("IsEnabled", &RainComponentRequestBus::Events::IsEnabled)

                ->Event("SetUseVisAreas", &RainComponentRequestBus::Events::SetUseVisAreas)
                ->Event("GetUseVisAreas", &RainComponentRequestBus::Events::GetUseVisAreas)
                ->VirtualProperty("UseVisAreas", "GetUseVisAreas", "SetUseVisAreas")

                ->Event("SetDisableOcclusion", &RainComponentRequestBus::Events::SetDisableOcclusion)
                ->Event("GetDisableOcclusion", &RainComponentRequestBus::Events::GetDisableOcclusion)
                ->VirtualProperty("DisableOcclusion", "GetDisableOcclusion", "SetDisableOcclusion")

                ->Event("SetRadius", &RainComponentRequestBus::Events::SetRadius)
                ->Event("GetRadius", &RainComponentRequestBus::Events::GetRadius)
                ->VirtualProperty("Radius", "GetRadius", "SetRadius")

                ->Event("SetAmount", &RainComponentRequestBus::Events::SetAmount)
                ->Event("GetAmount", &RainComponentRequestBus::Events::GetAmount)
                ->VirtualProperty("Amount", "GetAmount", "SetAmount")

                ->Event("SetDiffuseDarkening", &RainComponentRequestBus::Events::SetDiffuseDarkening)
                ->Event("GetDiffuseDarkening", &RainComponentRequestBus::Events::GetDiffuseDarkening)
                ->VirtualProperty("DiffuseDarkening", "GetDiffuseDarkening", "SetDiffuseDarkening")

                ->Event("SetRainDropsAmount", &RainComponentRequestBus::Events::SetRainDropsAmount)
                ->Event("GetRainDropsAmount", &RainComponentRequestBus::Events::GetRainDropsAmount)
                ->VirtualProperty("RainDropsAmount", "GetRainDropsAmount", "SetRainDropsAmount")

                ->Event("SetRainDropsSpeed", &RainComponentRequestBus::Events::SetRainDropsSpeed)
                ->Event("GetRainDropsSpeed", &RainComponentRequestBus::Events::GetRainDropsSpeed)
                ->VirtualProperty("RainDropsSpeed", "GetRainDropsSpeed", "SetRainDropsSpeed")

                ->Event("SetRainDropsLighting", &RainComponentRequestBus::Events::SetRainDropsLighting)
                ->Event("GetRainDropsLighting", &RainComponentRequestBus::Events::GetRainDropsLighting)
                ->VirtualProperty("RainDropsLighting", "GetRainDropsLighting", "SetRainDropsLighting")

                ->Event("SetPuddlesAmount", &RainComponentRequestBus::Events::SetPuddlesAmount)
                ->Event("GetPuddlesAmount", &RainComponentRequestBus::Events::GetPuddlesAmount)
                ->VirtualProperty("PuddlesAmount", "GetPuddlesAmount", "SetPuddlesAmount")

                ->Event("SetPuddlesMaskAmount", &RainComponentRequestBus::Events::SetPuddlesMaskAmount)
                ->Event("GetPuddlesMaskAmount", &RainComponentRequestBus::Events::GetPuddlesMaskAmount)
                ->VirtualProperty("PuddlesMaskAmount", "GetPuddlesMaskAmount", "SetPuddlesMaskAmount")

                ->Event("SetPuddlesRippleAmount", &RainComponentRequestBus::Events::SetPuddlesRippleAmount)
                ->Event("GetPuddlesRippleAmount", &RainComponentRequestBus::Events::GetPuddlesRippleAmount)
                ->VirtualProperty("PuddlesRippleAmount", "GetPuddlesRippleAmount", "SetPuddlesRippleAmount")

                ->Event("SetSplashesAmount", &RainComponentRequestBus::Events::SetSplashesAmount)
                ->Event("GetSplashesAmount", &RainComponentRequestBus::Events::GetSplashesAmount)
                ->VirtualProperty("SplashesAmount", "GetSplashesAmount", "SetSplashesAmount")

                ->Event("SetRainOptions", &RainComponentRequestBus::Events::SetRainOptions)
                ->Event("GetRainOptions", &RainComponentRequestBus::Events::GetRainOptions)
                ->VirtualProperty("RainOptions", "GetRainOptions", "SetRainOptions")

                ->Event("UpdateRain", &RainComponentRequestBus::Events::UpdateRain)

                ;

            behaviorContext->Class<RainComponent>()->RequestBus("RainComponentRequestBus");
        }
    }

    void RainOptions::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RainOptions>()
                ->Version(2, &VersionConverter)
                ->Field("UseVisAreas", &RainOptions::m_useVisAreas)
                ->Field("Disable Occlusion", &RainOptions::m_disableOcclusion)
                ->Field("Radius", &RainOptions::m_radius)
                ->Field("Amount", &RainOptions::m_amount)
                ->Field("DiffuseDarkening", &RainOptions::m_diffuseDarkening)
                ->Field("RainDropsAmount", &RainOptions::m_rainDropsAmount)
                ->Field("RainDropsSpeed", &RainOptions::m_rainDropsSpeed)
                ->Field("RainDropsLighting", &RainOptions::m_rainDropsLighting)
                ->Field("PuddlesAmount", &RainOptions::m_puddlesAmount)
                ->Field("PuddlesMaskAmount", &RainOptions::m_puddlesMaskAmount)
                ->Field("PuddlesRippleAmount", &RainOptions::m_puddlesRippleAmount)
                ->Field("SplashesAmount", &RainOptions::m_splashesAmount)
                ;
        }
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RainOptions>()
                ->Property("UseVisAreas", BehaviorValueProperty(&RainOptions::m_useVisAreas))
                ->Property("DisableOcclusion", BehaviorValueProperty(&RainOptions::m_disableOcclusion))
                ->Property("Radius", BehaviorValueProperty(&RainOptions::m_radius))
                ->Property("Amount", BehaviorValueProperty(&RainOptions::m_amount))
                ->Property("DiffuseDarkening", BehaviorValueProperty(&RainOptions::m_diffuseDarkening))
                ->Property("RainDropsAmount", BehaviorValueProperty(&RainOptions::m_rainDropsAmount))
                ->Property("RainDropsSpeed", BehaviorValueProperty(&RainOptions::m_rainDropsSpeed))
                ->Property("RainDropsLighting", BehaviorValueProperty(&RainOptions::m_rainDropsLighting))
                ->Property("PuddlesAmount", BehaviorValueProperty(&RainOptions::m_puddlesAmount))
                ->Property("PuddlesMaskAmount", BehaviorValueProperty(&RainOptions::m_puddlesMaskAmount))
                ->Property("PuddlesRippleAmount", BehaviorValueProperty(&RainOptions::m_puddlesRippleAmount))
                ->Property("SplashesAmount", BehaviorValueProperty(&RainOptions::m_splashesAmount))
                ;
        }
    }

    bool RainOptions::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        // conversion from version 1 to version 2:
        // - Need to rename IgnoreVisAreas to UseVisAreas
        // UseVisAreas is the Inverse of IgnoreVisAreas
        if (classElement.GetVersion() <= 1)
        {
            int ignoreVisAreasIndex = classElement.FindElement(AZ_CRC("IgnoreVisAreas", 0x3fbb253e));

            if (ignoreVisAreasIndex < 0)
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode& useVisAreasNode = classElement.GetSubElement(ignoreVisAreasIndex);
            useVisAreasNode.SetName("UseVisAreas");

            bool ignoreVisAreas = true;
            if (!useVisAreasNode.GetData<bool>(ignoreVisAreas))
            {
                return false;
            }

            if (!useVisAreasNode.SetData<bool>(context, !ignoreVisAreas))
            {
                return false;
            }
        }

        return true;
    }

    void UpdateRainSettings(const AZ::Transform& worldTransform, RainOptions options)
    {
        const AZ::Vector3 position = worldTransform.GetTranslation();

        const AZ::Quaternion azRotation = AZ::Quaternion::CreateFromTransform(worldTransform);
        Quat rotation = AZQuaternionToLYQuaternion(azRotation);

        //Remove Z axis rotation as it, according to a comment in Rain.cpp,
        //affects occlusion quality negatively. 
        Ang3 eulRot(rotation);
        eulRot.z = 0;
        rotation.SetRotationXYZ(eulRot);
        rotation.NormalizeSafe();

        SRainParams params;

        params.vWorldPos = AZVec3ToLYVec3(position);
        params.qRainRotation = rotation;
        params.fCurrentAmount = options.m_amount;

        params.bIgnoreVisareas = !options.m_useVisAreas;
        params.bDisableOcclusion = options.m_disableOcclusion;
        params.fRadius = options.m_radius;
        params.fAmount = options.m_amount;
        params.fDiffuseDarkening = options.m_diffuseDarkening;
        params.fPuddlesAmount = options.m_puddlesAmount;
        params.fPuddlesMaskAmount = options.m_puddlesMaskAmount;
        params.fPuddlesRippleAmount = options.m_puddlesRippleAmount;
        params.fRainDropsAmount = options.m_rainDropsAmount;
        params.fRainDropsSpeed = options.m_rainDropsSpeed;
        params.fRainDropsLighting = options.m_rainDropsLighting;
        params.fSplashesAmount = options.m_splashesAmount;

        gEnv->p3DEngine->SetRainParams(params);
    }

    void TurnOffRain()
    {
        SRainParams params;
        params.fAmount = 0.0f;
        params.fRadius = 0.0f;
        gEnv->p3DEngine->SetRainParams(params);
    }

    RainComponent::~RainComponent()
    {
        //When the rain component is destroyed, try to free any textures that were loaded.
        //Don't do this in deactivation because we don't want to have to re-load textures if the component
        //is re-activated. Also we don't want to free textures that other rain components may want to use.
        for (size_t i = 0; i < m_Textures.size(); ++i)
        {
            ITexture* tex = m_Textures[i];
            if (tex)
            {
                tex->Release();
            }
        }
    }

    void RainComponent::Activate()
    {
#ifndef NULL_RENDERER
        PrecacheTextures();
#endif
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        Rain::RainComponentRequestBus::Handler::BusConnect(GetEntityId());
    
        //Get transform at start
        AZ::TransformBus::EventResult(m_currentWorldTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        UpdateRain();
    }

    void RainComponent::Deactivate()
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        Rain::RainComponentRequestBus::Handler::BusDisconnect(GetEntityId());

        //Tell the 3D engine that there is no more rain
        m_enabled = false;
        UpdateRain();
    }
    
    void RainComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentWorldTransform = world;
        UpdateRain();
    }

    void RainComponent::Enable()
    {
        m_enabled = true;
        UpdateRain();
    }

    void RainComponent::Disable()
    {
        m_enabled = false;
        UpdateRain();
    }

    void RainComponent::Toggle()
    {
        m_enabled = !m_enabled;
        UpdateRain();
    }

    void RainComponent::SetUseVisAreas(bool useVisAreas)
    {
        m_rainOptions.m_useVisAreas = useVisAreas;
        UpdateRain();
    }

    void RainComponent::SetDisableOcclusion(bool disableOcclusion)
    {
        m_rainOptions.m_disableOcclusion = disableOcclusion;
        UpdateRain();
    }

    void RainComponent::SetRadius(float radius)
    {
        m_rainOptions.m_radius = radius;
        UpdateRain();
    }

    void RainComponent::SetAmount(float amount)
    {
        m_rainOptions.m_amount = amount;
        UpdateRain();
    }

    void RainComponent::SetDiffuseDarkening(float diffuseDarkening)
    {
        m_rainOptions.m_diffuseDarkening = diffuseDarkening;
        UpdateRain();
    }

    void RainComponent::SetRainDropsAmount(float rainDropsAmount)
    {
        m_rainOptions.m_rainDropsAmount = rainDropsAmount;
        UpdateRain();
    }

    void RainComponent::SetRainDropsSpeed(float rainDropsSpeed)
    {
        m_rainOptions.m_rainDropsSpeed = rainDropsSpeed;
        UpdateRain();
    }

    void RainComponent::SetRainDropsLighting(float rainDropsLighting)
    {
        m_rainOptions.m_rainDropsLighting = rainDropsLighting;
        UpdateRain();
    }

    void RainComponent::SetPuddlesAmount(float puddlesAmount)
    {
        m_rainOptions.m_puddlesAmount = puddlesAmount;
        UpdateRain();
    }

    void RainComponent::SetPuddlesMaskAmount(float puddlesMaskAmount)
    {
        m_rainOptions.m_puddlesMaskAmount = puddlesMaskAmount;
        UpdateRain();
    }

    void RainComponent::SetPuddlesRippleAmount(float puddlesRippleAmount)
    {
        m_rainOptions.m_puddlesRippleAmount = puddlesRippleAmount;
        UpdateRain();
    }

    void RainComponent::SetSplashesAmount(float splashesAmount)
    {
        m_rainOptions.m_splashesAmount = splashesAmount;
        UpdateRain();
    }

    void RainComponent::SetRainOptions(RainOptions rainOptions)
    {
        m_rainOptions = rainOptions;
        UpdateRain();
    }

    void RainComponent::UpdateRain()
    {
        //If the rain component is disabled, set the rain amount to 0
        if (!m_enabled)
        {
            TurnOffRain();
            return;
        }

        UpdateRainSettings(m_currentWorldTransform, m_rainOptions);
    }

#ifndef NULL_RENDERER
    void RainComponent::PrecacheTextures()
    {
        uint32 nDefaultFlags = FT_DONT_STREAM;

        XmlNodeRef root = GetISystem()->LoadXmlFromFile("@assets@/raintextures.xml");
        if (root)
        {
            for (int i = 0; i < root->getChildCount(); i++)
            {
                XmlNodeRef entry = root->getChild(i);
                if (!entry->isTag("entry"))
                {
                    continue;
                }

                uint32 nFlags = nDefaultFlags;

                // check attributes to modify the loading flags
                int nNoMips = 0;
                if (entry->getAttr("nomips", nNoMips) && nNoMips)
                {
                    nFlags |= FT_NOMIPS;
                }

                ITexture* texture = gEnv->pRenderer->EF_LoadTexture(entry->getContent(), nFlags);
                if (texture)
                {
                    m_Textures.push_back(texture);
                }
            }
        }
    }
#endif
}