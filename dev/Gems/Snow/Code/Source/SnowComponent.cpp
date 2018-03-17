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

#include "Snow_precompiled.h"
#include "SnowComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <MathConversion.h>

namespace Snow
{
    void SnowComponent::Reflect(AZ::ReflectContext* context)
    {
        SnowOptions::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SnowComponent, AZ::Component>()
                ->Version(1)
                ->Field("Enabled", &SnowComponent::m_enabled)
                ->Field("Snow Options", &SnowComponent::m_snowOptions)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<SnowComponentRequestBus>("SnowComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)

                ->Event("Enable", &SnowComponentRequestBus::Events::Enable)

                ->Event("Disable", &SnowComponentRequestBus::Events::Disable)

                ->Event("Toggle", &SnowComponentRequestBus::Events::Toggle)

                ->Event("IsEnabled", &SnowComponentRequestBus::Events::IsEnabled)

                ->Event("SetRadius", &SnowComponentRequestBus::Events::SetRadius)
                ->Event("GetRadius", &SnowComponentRequestBus::Events::GetRadius)
                ->VirtualProperty("Radius", "GetRadius", "SetRadius")

                ->Event("SetSnowAmount", &SnowComponentRequestBus::Events::SetSnowAmount)
                ->Event("GetSnowAmount", &SnowComponentRequestBus::Events::GetSnowAmount)
                ->VirtualProperty("SnowAmount", "GetSnowAmount", "SetSnowAmount")

                ->Event("SetFrostAmount", &SnowComponentRequestBus::Events::SetFrostAmount)
                ->Event("GetFrostAmount", &SnowComponentRequestBus::Events::GetFrostAmount)
                ->VirtualProperty("FrostAmount", "GetFrostAmount", "SetFrostAmount")

                ->Event("SetSurfaceFreezing", &SnowComponentRequestBus::Events::SetSurfaceFreezing)
                ->Event("GetSurfaceFreezing", &SnowComponentRequestBus::Events::GetSurfaceFreezing)
                ->VirtualProperty("SurfaceFreezing", "GetSurfaceFreezing", "SetSurfaceFreezing")

                ->Event("SetSnowFlakeCount", &SnowComponentRequestBus::Events::SetSnowFlakeCount)
                ->Event("GetSnowFlakeCount", &SnowComponentRequestBus::Events::GetSnowFlakeCount)
                ->VirtualProperty("SnowFlakeCount", "GetSnowFlakeCount", "SetSnowFlakeCount")

                ->Event("SetSnowFlakeSize", &SnowComponentRequestBus::Events::SetSnowFlakeSize)
                ->Event("GetSnowFlakeSize", &SnowComponentRequestBus::Events::GetSnowFlakeSize)
                ->VirtualProperty("SnowFlakeSize", "GetSnowFlakeSize", "SetSnowFlakeSize")

                ->Event("SetSnowFallBrightness", &SnowComponentRequestBus::Events::SetSnowFallBrightness)
                ->Event("GetSnowFallBrightness", &SnowComponentRequestBus::Events::GetSnowFallBrightness)
                ->VirtualProperty("SnowFallBrightness", "GetSnowFallBrightness", "SetSnowFallBrightness")

                ->Event("SetSnowFallGravityScale", &SnowComponentRequestBus::Events::SetSnowFallGravityScale)
                ->Event("GetSnowFallGravityScale", &SnowComponentRequestBus::Events::GetSnowFallGravityScale)
                ->VirtualProperty("SnowFallGravityScale", "GetSnowFallGravityScale", "SetSnowFallGravityScale")

                ->Event("SetSnowFallWindScale", &SnowComponentRequestBus::Events::SetSnowFallWindScale)
                ->Event("GetSnowFallWindScale", &SnowComponentRequestBus::Events::GetSnowFallWindScale)
                ->VirtualProperty("SnowFallWindScale", "GetSnowFallWindScale", "SetSnowFallWindScale")

                ->Event("SetSnowFallTurbulence", &SnowComponentRequestBus::Events::SetSnowFallTurbulence)
                ->Event("GetSnowFallTurbulence", &SnowComponentRequestBus::Events::GetSnowFallTurbulence)
                ->VirtualProperty("SnowFallTurbulence", "GetSnowFallTurbulence", "SetSnowFallTurbulence")

                ->Event("SetSnowFallTurbulenceFreq", &SnowComponentRequestBus::Events::SetSnowFallTurbulenceFreq)
                ->Event("GetSnowFallTurbulenceFreq", &SnowComponentRequestBus::Events::GetSnowFallTurbulenceFreq)
                ->VirtualProperty("SnowFallTurbulenceFreq", "GetSnowFallTurbulenceFreq", "SetSnowFallTurbulenceFreq")

                ->Event("SetSnowOptions", &SnowComponentRequestBus::Events::SetSnowOptions)
                ->Event("GetSnowOptions", &SnowComponentRequestBus::Events::GetSnowOptions)
                ->VirtualProperty("SnowOptions", "GetSnowOptions", "SetSnowOptions")

                ->Event("UpdateSnow", &SnowComponentRequestBus::Events::UpdateSnow)
                ;

            behaviorContext->Class<SnowComponent>()->RequestBus("SnowComponentRequestBus");
        }
    }

    void SnowOptions::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SnowOptions>()
                ->Version(1, &VersionConverter)
                ->Field("Radius", &SnowOptions::m_radius)
                ->Field("SnowAmount", &SnowOptions::m_snowAmount)
                ->Field("FrostAmount", &SnowOptions::m_frostAmount)
                ->Field("SurfaceFreezing", &SnowOptions::m_surfaceFreezing)
                ->Field("SnowFlakeCount", &SnowOptions::m_snowFlakeCount)
                ->Field("SnowFlakeSize", &SnowOptions::m_snowFlakeSize)
                ->Field("SnowFallBrightness", &SnowOptions::m_snowFallBrightness)
                ->Field("SnowFallGravityScale", &SnowOptions::m_snowFallGravityScale)
                ->Field("SnowFallWindScale", &SnowOptions::m_snowFallWindScale)
                ->Field("SnowFallTurbulence", &SnowOptions::m_snowFallTurbulence)
                ->Field("SnowFallTurbulenceFreq", &SnowOptions::m_snowFallTurbulenceFreq)
                ;
        }
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SnowOptions>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Property("Radius", BehaviorValueProperty(&SnowOptions::m_radius))
                ->Property("SnowAmount", BehaviorValueProperty(&SnowOptions::m_snowAmount))
                ->Property("FrostAmount", BehaviorValueProperty(&SnowOptions::m_frostAmount))
                ->Property("SurfaceFreezing", BehaviorValueProperty(&SnowOptions::m_surfaceFreezing))
                ->Property("SnowFlakeCount", BehaviorValueProperty(&SnowOptions::m_snowFlakeCount))
                ->Property("SnowFlakeSize", BehaviorValueProperty(&SnowOptions::m_snowFlakeSize))
                ->Property("SnowFallBrightness", BehaviorValueProperty(&SnowOptions::m_snowFallBrightness))
                ->Property("SnowFallGravityScale", BehaviorValueProperty(&SnowOptions::m_snowFallGravityScale))
                ->Property("SnowFallWindScale", BehaviorValueProperty(&SnowOptions::m_snowFallWindScale))
                ->Property("SnowFallTurbulence", BehaviorValueProperty(&SnowOptions::m_snowFallTurbulence))
                ->Property("SnowFallTurbulenceFreq", BehaviorValueProperty(&SnowOptions::m_snowFallTurbulenceFreq))
                ;
        }
    }

    bool SnowOptions::VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        return true;
    }

    void UpdateSnowSettings(const AZ::Transform& worldTransform, SnowOptions options)
    {
        const AZ::Vector3& worldPos = worldTransform.GetTranslation();

        Vec3 legacyWorldPos = AZVec3ToLYVec3(worldPos);

        gEnv->p3DEngine->SetSnowSurfaceParams(
            legacyWorldPos,
            options.m_radius, 
            options.m_snowAmount, 
            options.m_frostAmount, 
            options.m_surfaceFreezing
        );
        gEnv->p3DEngine->SetSnowFallParams(
            options.m_snowFlakeCount,
            options.m_snowFlakeSize,
            options.m_snowFallBrightness, 
            options.m_snowFallGravityScale, 
            options.m_snowFallWindScale, 
            options.m_snowFallTurbulence, 
            options.m_snowFallTurbulenceFreq
        );

        //We still need to set Rain Parameters. That's how the engine knows
        //to still handle occlusion. That's all hard-coded to work with Rain and Snow was added later
        
        const AZ::Quaternion azRotation = AZ::Quaternion::CreateFromTransform(worldTransform);
        Quat rotation = AZQuaternionToLYQuaternion(azRotation);

        //Remove Z axis rotation as it, according to a comment in Rain.cpp,
        //affects occlusion quality negatively. 
        Ang3 eulRot(rotation);
        eulRot.z = 0;
        rotation.SetRotationXYZ(eulRot);
        rotation.NormalizeSafe();

        SRainParams params;

        //These are the only important options
        params.vWorldPos = legacyWorldPos;
        params.qRainRotation = rotation;
        params.bDisableOcclusion = options.m_disableOcclusion;
        params.fRadius = options.m_radius;

        gEnv->p3DEngine->SetRainParams(params);
    }

    void TurnOffSnow()
    {
        gEnv->p3DEngine->SetSnowSurfaceParams(Vec3(), 0.0f, 0.0f, 0.0f, 0.0f);
        gEnv->p3DEngine->SetSnowFallParams(0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    }

    SnowComponent::~SnowComponent()
    {
        //When the snow component is destroyed, try to free any textures that were loaded.
        //Don't do this in deactivation because we don't want to have to re-load textures if the component
        //is re-activated. Also we don't want to free textures that other snow components may want to use.
        for (size_t i = 0; i < m_Textures.size(); ++i)
        {
            ITexture* tex = m_Textures[i];
            if (tex)
            {
                tex->Release();
            }
        }
    }

    void SnowComponent::Activate()
    {
#ifndef NULL_RENDERER
        PrecacheTextures();
#endif
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        Snow::SnowComponentRequestBus::Handler::BusConnect(GetEntityId());
    
        //Get initial position
        m_currentWorldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentWorldTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        UpdateSnow();
    }

    void SnowComponent::Deactivate()
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        Snow::SnowComponentRequestBus::Handler::BusDisconnect(GetEntityId());

        //Tell the 3D engine that there is no more snow
        m_enabled = false;
        UpdateSnow();
    }
    
    void SnowComponent::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
    {
        m_currentWorldTransform = world;
        UpdateSnow();
    }

    void SnowComponent::Enable()
    {
        m_enabled = true;
        UpdateSnow();
    }

    void SnowComponent::Disable()
    {
        m_enabled = false;
        UpdateSnow();
    }

    void SnowComponent::Toggle()
    {
        m_enabled = !m_enabled;
        UpdateSnow();
    }

    void SnowComponent::SetRadius(float radius)
    {
        m_snowOptions.m_radius = radius;
        UpdateSnow();
    }

    void SnowComponent::SetSnowAmount(float snowAmount)
    {
        m_snowOptions.m_snowAmount = snowAmount;
        UpdateSnow();
    }

    void SnowComponent::SetFrostAmount(float frostAmount)
    {
        m_snowOptions.m_frostAmount = frostAmount;
        UpdateSnow();
    }

    void SnowComponent::SetSurfaceFreezing(float surfaceFreezing)
    {
        m_snowOptions.m_surfaceFreezing = surfaceFreezing;
        UpdateSnow();
    }

    void SnowComponent::SetSnowFlakeCount(AZ::u32 snowFlakeCount)
    {
        m_snowOptions.m_snowFlakeCount = snowFlakeCount;
        UpdateSnow();
    }

    void SnowComponent::SetSnowFlakeSize(float snowFlakeSize)
    {
        m_snowOptions.m_snowFlakeSize = snowFlakeSize;
        UpdateSnow();
    }

    void SnowComponent::SetSnowFallBrightness(float snowFallBrightness)
    {
        m_snowOptions.m_snowFallBrightness = snowFallBrightness;
        UpdateSnow();
    }

    void SnowComponent::SetSnowFallGravityScale(float snowFallGravityScale)
    {
        m_snowOptions.m_snowFallGravityScale = snowFallGravityScale;
        UpdateSnow();
    }

    void SnowComponent::SetSnowFallWindScale(float snowFallWindScale)
    {
        m_snowOptions.m_snowFallWindScale = snowFallWindScale;
        UpdateSnow();
    }

    void SnowComponent::SetSnowFallTurbulence(float snowFallTurbulence)
    {
        m_snowOptions.m_snowFallTurbulence = snowFallTurbulence;
        UpdateSnow();
    }

    void SnowComponent::SetSnowFallTurbulenceFreq(float snowFallTurbulenceFreq)
    {
        m_snowOptions.m_snowFallTurbulenceFreq = snowFallTurbulenceFreq;
        UpdateSnow();
    }

    void SnowComponent::SetSnowOptions(SnowOptions snowOptions)
    {
        m_snowOptions = snowOptions;
        UpdateSnow();
    }

    void SnowComponent::UpdateSnow()
    {
        //If the snow component is disabled, set the snow amount to 0
        if (!m_enabled)
        {
            TurnOffSnow();
            return;
        }

        UpdateSnowSettings(m_currentWorldTransform, m_snowOptions);
    }

#ifndef NULL_RENDERER
    void SnowComponent::PrecacheTextures()
    {
        uint32 nDefaultFlags = FT_DONT_STREAM;

        XmlNodeRef root = GetISystem()->LoadXmlFromFile("@assets@/snowtextures.xml");
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