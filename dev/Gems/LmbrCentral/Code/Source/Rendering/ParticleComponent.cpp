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
#include "LmbrCentral_precompiled.h"
#include "ParticleComponent.h"

#include <IParticles.h>
#include <ParticleParams.h>

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <AzFramework/Physics/PhysicsComponentBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>

#include <MathConversion.h>
#include <AzFramework/Math/MathUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Components/TransformComponent.h>


namespace LmbrCentral
{
    using AzFramework::PhysicsComponentRequestBus;
    
    bool RenameElement(AZ::SerializeContext::DataElementNode& classElement, AZ::Crc32 srcCrc, const char* newName)
    {
        AZ::SerializeContext::DataElementNode* eleNode = classElement.FindSubElement(srcCrc);

        if (!eleNode)
        {
            return false;
        }

        eleNode->SetName(newName);
        return true;
    }


    const float ParticleEmitterSettings::MaxCountScale = 1000.f;
    const float ParticleEmitterSettings::MaxTimeScale = 1000.f;
    const float ParticleEmitterSettings::MaxSpeedScale = 1000.f;
    const float ParticleEmitterSettings::MaxSizeScale = 100.f;
    const float ParticleEmitterSettings::MaxLifttimeStrength = 1.f;
    const float ParticleEmitterSettings::MinLifttimeStrength = -1.f;

    //////////////////////////////////////////////////////////////////////////
    void ParticleComponent::Reflect(AZ::ReflectContext* context)
    {
        ParticleEmitterSettings::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ParticleComponent, AZ::Component>()->
                Version(1)->
                Field("Particle", &ParticleComponent::m_settings)
            ;
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
           behaviorContext->Class<ParticleComponent>()
                ->RequestBus("ParticleComponentRequestBus");

            behaviorContext->EBus<ParticleComponentRequestBus>("ParticleComponentRequestBus")
                ->Event("SetVisibility", &ParticleComponentRequestBus::Events::SetVisibility)
                ->Event("GetVisibility", &ParticleComponentRequestBus::Events::GetVisibility)
                ->Event("Show", &ParticleComponentRequestBus::Events::Show)
                ->Event("Hide", &ParticleComponentRequestBus::Events::Hide)
                ->Event("Enable", &ParticleComponentRequestBus::Events::Enable)
                ->Event("GetEnable", &ParticleComponentRequestBus::Events::GetEnable)
                ->Event("EnablePreRoll", &ParticleComponentRequestBus::Events::EnablePreRoll)
                ->Event("SetColorTint", &ParticleComponentRequestBus::Events::SetColorTint)
                ->Event("GetColorTint", &ParticleComponentRequestBus::Events::GetColorTint)
                ->Event("SetCountScale", &ParticleComponentRequestBus::Events::SetCountScale)
                ->Event("GetCountScale", &ParticleComponentRequestBus::Events::GetCountScale)
                ->Event("SetTimeScale", &ParticleComponentRequestBus::Events::SetTimeScale)
                ->Event("GetTimeScale", &ParticleComponentRequestBus::Events::GetTimeScale)
                ->Event("SetSpeedScale", &ParticleComponentRequestBus::Events::SetSpeedScale)
                ->Event("GetSpeedScale", &ParticleComponentRequestBus::Events::GetSpeedScale)
                ->Event("SetGlobalSizeScale", &ParticleComponentRequestBus::Events::SetGlobalSizeScale)
                ->Event("GetGlobalSizeScale", &ParticleComponentRequestBus::Events::GetGlobalSizeScale)
                ->Event("SetParticleSizeScaleX", &ParticleComponentRequestBus::Events::SetParticleSizeScaleX)
                ->Event("GetParticleSizeScaleX", &ParticleComponentRequestBus::Events::GetParticleSizeScaleX)
                ->Event("SetParticleSizeScaleY", &ParticleComponentRequestBus::Events::SetParticleSizeScaleY)
                ->Event("GetParticleSizeScaleY", &ParticleComponentRequestBus::Events::GetParticleSizeScaleY)
                ->Event("SetLifetimeStrength", &ParticleComponentRequestBus::Events::SetLifetimeStrength)
                ->Event("EnableAudio", &ParticleComponentRequestBus::Events::EnableAudio)
                ->Event("SetRTPC", &ParticleComponentRequestBus::Events::SetRTPC)
                ->Event("SetViewDistMultiplier", &ParticleComponentRequestBus::Events::SetViewDistMultiplier)
                ->Event("SetUseVisArea", &ParticleComponentRequestBus::Events::SetUseVisArea)
                ->Event("GetEmitterSettings", &ParticleComponentRequestBus::Events::GetEmitterSettings)
                ->Event("Restart", &ParticleComponentRequestBus::Events::Restart)
                ->VirtualProperty("Visible", "GetVisibility", "SetVisibility")
                ->VirtualProperty("Enable", "GetEnable", "Enable")
                ->VirtualProperty("ColorTint", "GetColorTint", "SetColorTint")
                ->VirtualProperty("CountScale", "GetCountScale", "SetCountScale")
                ->VirtualProperty("TimeScale", "GetTimeScale", "SetTimeScale")
                ->VirtualProperty("SpeedScale", "GetSpeedScale", "SetSpeedScale")
                ->VirtualProperty("GlobalSizeScale", "GetGlobalSizeScale", "SetGlobalSizeScale")
                ->VirtualProperty("ParticleSizeScaleX", "GetParticleSizeScaleX", "SetParticleSizeScaleX")
                ->VirtualProperty("ParticleSizeScaleY", "GetParticleSizeScaleY", "SetParticleSizeScaleY")
                ;
        }
    }
    
    void ParticleEmitterSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ParticleEmitterSettings>()->
                Version(5, &VersionConverter)->
                
                //Particle
                Field("Visible", &ParticleEmitterSettings::m_visible)->
                Field("Enable", &ParticleEmitterSettings::m_enable)->
                Field("SelectedEmitter", &ParticleEmitterSettings::m_selectedEmitter)->
                
                //Spawn Properties
                Field("Color", &ParticleEmitterSettings::m_color)->
                Field("Pre-roll", &ParticleEmitterSettings::m_prime)->
                Field("Particle Count Scale", &ParticleEmitterSettings::m_countScale)->
                Field("Time Scale", &ParticleEmitterSettings::m_timeScale)->
                Field("Pulse Period", &ParticleEmitterSettings::m_pulsePeriod)->
                Field("GlobalSizeScale", &ParticleEmitterSettings::m_sizeScale)->
                Field("ParticleSizeX", &ParticleEmitterSettings::m_particleSizeScaleX)->
                Field("ParticleSizeY", &ParticleEmitterSettings::m_particleSizeScaleY)->
                Field("ParticleSizeRandom", &ParticleEmitterSettings::m_particleSizeScaleRandom)->
                Field("Speed Scale", &ParticleEmitterSettings::m_speedScale)->
                Field("Strength", &ParticleEmitterSettings::m_strength)->
                Field("Ignore Rotation", &ParticleEmitterSettings::m_ignoreRotation)->
                Field("Not Attached", &ParticleEmitterSettings::m_notAttached)->
                Field("Register by Bounding Box", &ParticleEmitterSettings::m_registerByBBox)->
                Field("Use LOD", &ParticleEmitterSettings::m_useLOD)->

                Field("Target Entity", &ParticleEmitterSettings::m_targetEntity)->

                //Audio
                Field("Enable Audio", &ParticleEmitterSettings::m_enableAudio)->
                Field("Audio RTPC", &ParticleEmitterSettings::m_audioRTPC)->

                //render node and misc
                Field("View Distance Multiplier", &ParticleEmitterSettings::m_viewDistMultiplier)->
                Field("Use VisArea", &ParticleEmitterSettings::m_useVisAreas)
            ;
        }
        
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<ParticleEmitterSettings>()
                ->Property("ColorTint", BehaviorValueProperty(&ParticleEmitterSettings::m_color))
                ->Property("Preroll", BehaviorValueProperty(&ParticleEmitterSettings::m_prime))
                ->Property("CountScale", BehaviorValueProperty(&ParticleEmitterSettings::m_countScale))
                ->Property("TimeScale", BehaviorValueProperty(&ParticleEmitterSettings::m_timeScale))
                ->Property("SpeedScale", BehaviorValueProperty(&ParticleEmitterSettings::m_speedScale))
                ->Property("PulsePeriod", BehaviorValueProperty(&ParticleEmitterSettings::m_pulsePeriod))
                ->Property("ParticleSizeScaleX", BehaviorValueProperty(&ParticleEmitterSettings::m_particleSizeScaleX))
                ->Property("ParticleSizeScaleY", BehaviorValueProperty(&ParticleEmitterSettings::m_particleSizeScaleY))
                ->Property("ParticleSizeRandom", BehaviorValueProperty(&ParticleEmitterSettings::m_particleSizeScaleRandom))
                ->Property("LifetimeStrength", BehaviorValueProperty(&ParticleEmitterSettings::m_strength))
                ->Property("IgnoreRotation", BehaviorValueProperty(&ParticleEmitterSettings::m_ignoreRotation))
                ->Property("NotAttached", BehaviorValueProperty(&ParticleEmitterSettings::m_notAttached))
                ->Property("RegisterByBBox", BehaviorValueProperty(&ParticleEmitterSettings::m_registerByBBox))
                ->Property("UseLOD", BehaviorValueProperty(&ParticleEmitterSettings::m_useLOD))
                ->Property("TargetEntity", BehaviorValueProperty(&ParticleEmitterSettings::m_targetEntity))
                ->Property("EnableAudio", BehaviorValueProperty(&ParticleEmitterSettings::m_enableAudio))
                ->Property("RTPC", BehaviorValueProperty(&ParticleEmitterSettings::m_audioRTPC))
                ->Property("ViewDistMultiplier", BehaviorValueProperty(&ParticleEmitterSettings::m_viewDistMultiplier))
                ->Property("UseVisAreas", BehaviorValueProperty(&ParticleEmitterSettings::m_useVisAreas));
        }
    }

    // Private Static 
    bool ParticleEmitterSettings::VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        bool re = true;
        // conversion from version 1:
        // - Need to rename Emitter Object Type to Attach Type
        // - Need to rename Emission Speed to Speed Scale
        if (classElement.GetVersion() == 1)
        {
            re &= RenameElement(classElement, AZ_CRC("Emitter Object Type", 0xc563146b), "Attach Type");
            re &= RenameElement(classElement, AZ_CRC("Emission Speed", 0xb375c0de), "Speed Scale");
        }
        // conversion from version 2:
        if (classElement.GetVersion() <= 2)
        {

            //Rename
            re &= RenameElement(classElement, AZ_CRC("Prime", 0x544b0f57), "Pre-roll");
            re &= RenameElement(classElement, AZ_CRC("Particle Size Scale", 0x533c8807), "GlobalSizeScale");
            re &= RenameElement(classElement, AZ_CRC("Size X", 0x29925f6f), "ParticleSizeX");
            re &= RenameElement(classElement, AZ_CRC("Size Y", 0x5e956ff9), "ParticleSizeY");
            re &= RenameElement(classElement, AZ_CRC("Size Random X", 0x61eb4b20), "ParticleSizeRandom");

            //Remove
            re &= classElement.RemoveElementByName(AZ_CRC("Attach Type", 0x86d39374));
            re &= classElement.RemoveElementByName(AZ_CRC("Emitter Shape", 0x2c633f81));
            re &= classElement.RemoveElementByName(AZ_CRC("Geometry", 0x95520eab));
            re &= classElement.RemoveElementByName(AZ_CRC("Count Per Unit", 0xc4969296));
            re &= classElement.RemoveElementByName(AZ_CRC("Position Offset", 0xbbc4049f));
            re &= classElement.RemoveElementByName(AZ_CRC("Random Offset", 0x53c41fee));
            re &= classElement.RemoveElementByName(AZ_CRC("Size Random Y", 0x16ec7bb6));
            re &= classElement.RemoveElementByName(AZ_CRC("Init Angles", 0x4b47ebd2));
            re &= classElement.RemoveElementByName(AZ_CRC("Rotation Rate X", 0x0356bf40));
            re &= classElement.RemoveElementByName(AZ_CRC("Rotation Rate Y", 0x74518fd6));
            re &= classElement.RemoveElementByName(AZ_CRC("Rotation Rate Z", 0xed58de6c));
            re &= classElement.RemoveElementByName(AZ_CRC("Rotation Rate Random X", 0x9d401896));
            re &= classElement.RemoveElementByName(AZ_CRC("Rotation Rate Random Y", 0xea472800));
            re &= classElement.RemoveElementByName(AZ_CRC("Rotation Rate Random Z", 0x734e79ba));
            re &= classElement.RemoveElementByName(AZ_CRC("Rotation Random Angles", 0x1d5bf41f));
        }

        // conversion from version 4:
        if (classElement.GetVersion() <= 4)
        {
            //convert color tint's type from Vector3 to Color
            int colorIndex = classElement.FindElement(AZ_CRC("Color", 0x665648e9));
            if (colorIndex == -1)
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode& color = classElement.GetSubElement(colorIndex);
            AZ::Vector3 colorVec;
            color.GetData<AZ::Vector3>(colorVec);
            AZ::Color colorVal(colorVec.GetX(), colorVec.GetY(), colorVec.GetZ(), AZ::VectorFloat(1.0f));
            color.Convert<AZ::Color>(context);
            color.SetData(context, colorVal);
        }

        return re;
    }

    //////////////////////////////////////////////////////////////////////////

    void ParticleComponent::Init()
    {
    }

    void ParticleComponent::Activate()
    {
        ParticleComponentRequestBus::Handler::BusConnect(m_entity->GetId());
        RenderNodeRequestBus::Handler::BusConnect(m_entity->GetId());

        m_emitter.AttachToEntity(m_entity->GetId());
        m_emitter.Set(m_settings.m_selectedEmitter, m_settings);
    }

    void ParticleComponent::Deactivate()
    {
        ParticleComponentRequestBus::Handler::BusDisconnect();
        RenderNodeRequestBus::Handler::BusDisconnect();

        m_emitter.Clear();
    }

    // ParticleComponentRequestBus handlers
    void ParticleComponent::Show()
    {
        m_settings.m_visible = true;
        m_emitter.Show();
    }

    void ParticleComponent::Enable(bool enable)
    {
        m_settings.m_enable = enable;
        m_emitter.Enable(enable);
    }

    void ParticleComponent::Hide()
    {
        m_settings.m_visible = false;
        m_emitter.Hide();
    }

    void ParticleComponent::SetVisibility(bool visible)
    {
        m_settings.m_visible = visible;
        m_emitter.SetVisibility(visible);
    }

    void ParticleComponent::SetupEmitter(const AZStd::string& emitterName, const ParticleEmitterSettings& settings)
    {
        m_settings = settings;
        m_settings.m_selectedEmitter = emitterName;
        m_emitter.Set(m_settings.m_selectedEmitter, m_settings);
    }

    void ParticleComponent::EnablePreRoll(bool enable)
    {
        m_settings.m_prime = enable;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    void ParticleComponent::SetColorTint(const AZ::Color& tint)
    {
        m_settings.m_color = tint;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    void ParticleComponent::SetCountScale(float scale)
    {
        if (ParticleEmitterSettings::MaxCountScale < scale || scale < 0)
        {
            return;
        }

        m_settings.m_countScale = scale;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    void ParticleComponent::SetTimeScale(float scale)
    {
        if (ParticleEmitterSettings::MaxTimeScale < scale || scale < 0)
        {
            return;
        }

        m_settings.m_timeScale = scale;
        m_emitter.ApplyEmitterSetting(m_settings);

    }

    void ParticleComponent::SetSpeedScale(float scale)
    {
        if (ParticleEmitterSettings::MaxSpeedScale < scale || scale < 0)
        {
            return;
        }

        m_settings.m_speedScale = scale;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    void ParticleComponent::SetGlobalSizeScale(float scale)
    {
        if (ParticleEmitterSettings::MaxSizeScale < scale || scale < 0)
        {
            return;
        }

        m_settings.m_sizeScale = scale;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    void ParticleComponent::SetParticleSizeScaleX(float scale)
    {
        if (ParticleEmitterSettings::MaxSizeScale < scale || scale < 0)
        {
            return;
        }

        m_settings.m_particleSizeScaleX = scale;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    void ParticleComponent::SetParticleSizeScaleY(float scale)
    {
        if (ParticleEmitterSettings::MaxSizeScale < scale || scale < 0)
        {
            return;
        }

        m_settings.m_particleSizeScaleY = scale;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    void ParticleComponent::SetPulsePeriod(float pulse)
    {
        if (pulse < 0)
        {
            return;
        }

        m_settings.m_pulsePeriod = pulse;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    void ParticleComponent::SetLifetimeStrength(float strength)
    {
        if (ParticleEmitterSettings::MinLifttimeStrength > strength
            || ParticleEmitterSettings::MaxLifttimeStrength < strength)
        {
            return;
        }

        m_settings.m_strength = strength;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    // Enable audio
    void ParticleComponent::EnableAudio(bool enable)
    {
        m_settings.m_enableAudio = enable;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    // Set audio RTPC
    void ParticleComponent::SetRTPC(const AZStd::string& rtpc)
    {
        m_settings.m_audioRTPC = rtpc;
        m_emitter.ApplyEmitterSetting(m_settings);
    }

    void ParticleComponent::SetViewDistMultiplier(float multiplier)
    {
        m_settings.m_viewDistMultiplier = multiplier;
        m_emitter.ApplyRenderFlags(m_settings);
    }

    void ParticleComponent::SetUseVisArea(bool enable)
    {
        m_settings.m_useVisAreas = enable;
        m_emitter.ApplyRenderFlags(m_settings);
    }
    

    // get emitter setting
    ParticleEmitterSettings ParticleComponent::GetEmitterSettings()
    {
        return m_settings;
    }

    void ParticleComponent::Restart()
    {
        m_emitter.Restart();
    }

    bool ParticleComponent::GetVisibility()
    {
        return m_settings.m_visible;
    }
    
    bool ParticleComponent::GetEnable()
    {
        return m_settings.m_enable;
    }

    AZ::Color ParticleComponent::GetColorTint()
    {
        return m_settings.m_color;
    }

    float ParticleComponent::GetCountScale()
    {
        return m_settings.m_countScale;
    }

    float ParticleComponent::GetTimeScale()
    {
        return m_settings.m_timeScale;
    }

    float ParticleComponent::GetSpeedScale()
    {
        return m_settings.m_speedScale;
    }

    float ParticleComponent::GetGlobalSizeScale()
    {
        return m_settings.m_sizeScale;
    }

    float ParticleComponent::GetParticleSizeScaleX()
    {
        return m_settings.m_particleSizeScaleX;
    }

    float ParticleComponent::GetParticleSizeScaleY()
    {
        return m_settings.m_particleSizeScaleY;
    }

    float ParticleComponent::GetPulsePeriod()
    {
        return m_settings.m_pulsePeriod;
    }
    //end ParticleComponentRequestBus handlers

    IRenderNode* ParticleComponent::GetRenderNode()
    {
        return m_emitter.GetRenderNode();
    }

    float ParticleComponent::GetRenderNodeRequestBusOrder() const
    {
        return s_renderNodeRequestBusOrder;
    }

    const float ParticleComponent::s_renderNodeRequestBusOrder = 800.f;

    //////////////////////////////////////////////////////////////////////////

    ParticleEmitter::ParticleEmitter()
    {
    }

    void ParticleEmitter::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        if (m_emitter)
        {
            m_emitter->SetMatrix(AZTransformToLYTransform(world));
        }
    }

    void ParticleEmitter::AttachToEntity(AZ::EntityId id)
    {
        m_attachedToEntityId = id;
    }

    void ParticleEmitter::Set(const AZStd::string& emitterName, const ParticleEmitterSettings& settings)
    {
        if (emitterName.empty())
        {
            return;
        }

        m_effect = gEnv->pParticleManager->FindEffect(emitterName.c_str());

        // Given a valid effect, we'll spawn a new emitter
        if (m_effect)
        {
            // Check if the current platform supports compute shaders for GPU Particles and Structured Buffers in the Vertex Shader.
            int featureMask = RFT_COMPUTE_SHADERS | RFT_HW_VERTEX_STRUCTUREDBUF;
            if (m_effect->GetParticleParams().eEmitterType == ParticleParams::EEmitterType::GPU && (gEnv->pRenderer->GetFeatures() & featureMask) != featureMask)
            {
                AZ_Warning("Particle Component", "GPU Particles are not supported for this platform. Emitter using GPU particles is: %s", emitterName.c_str());
                return;
            }
            //Spawn an emitter with the setting
            SpawnEmitter(settings);
        }
        else
        {
            AZ_Warning("Particle Component", "Could not find particle emitter: %s", emitterName.c_str());
        }
    }
    
    //////////////////////////////////////////////////////////////////////////

    SpawnParams ParticleEmitter::GetPropertiesAsSpawnParams(const ParticleEmitterSettings& settings) const
    {
        SpawnParams params;
        params.colorTint = Vec3(settings.m_color.GetR(), settings.m_color.GetG(), settings.m_color.GetB());
        params.bEnableAudio = settings.m_enableAudio;
        params.bRegisterByBBox = settings.m_registerByBBox;
        params.fCountScale = settings.m_countScale;
        params.fSizeScale = settings.m_sizeScale;
        params.fSpeedScale = settings.m_speedScale;
        params.fTimeScale = settings.m_timeScale;
        params.fPulsePeriod = settings.m_pulsePeriod;
        params.fStrength = settings.m_strength;
        params.sAudioRTPC = settings.m_audioRTPC.c_str();
        params.particleSizeScale.x = settings.m_particleSizeScaleX;
        params.particleSizeScale.y = settings.m_particleSizeScaleY;
        params.particleSizeScaleRandom = settings.m_particleSizeScaleRandom;
        return params;
    }

    void ParticleEmitter::SpawnEmitter(const ParticleEmitterSettings& settings)
    {
        AZ_Assert(m_effect, "Cannot spawn an emitter without an effect");
        if (!m_effect)
        {
            return;
        }

        // If we already have an emitter, remove it to spawn a new one
        if (m_emitter)
        {
            m_emitter->Kill();
            m_emitter = nullptr;
        }

        //emitter flags
        uint32 emmitterFlags = 0;
        if (settings.m_ignoreRotation)
        {
            emmitterFlags |= ePEF_IgnoreRotation;
        }

        if (settings.m_notAttached)
        {
            emmitterFlags |= ePEF_NotAttached;

            if (AZ::TransformNotificationBus::Handler::BusIsConnectedId(m_attachedToEntityId))
            {
                AZ::TransformNotificationBus::Handler::BusDisconnect(m_attachedToEntityId);
            }
        }
        else
        {
            if (!AZ::TransformNotificationBus::Handler::BusIsConnectedId(m_attachedToEntityId))
            {
                AZ::TransformNotificationBus::Handler::BusConnect(m_attachedToEntityId);
            }
        }

        // Get entity's world transforms
        AZ::Transform parentTransform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(parentTransform, m_attachedToEntityId, AZ::TransformBus, GetWorldTM);
        
        // Spawn the particle emitter
        SpawnParams spawnParams = GetPropertiesAsSpawnParams(settings);
        m_emitter = m_effect->Spawn(QuatTS(AZTransformToLYTransform(parentTransform)), emmitterFlags, &spawnParams);


        AZ::Entity* targetEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(targetEntity, &AZ::ComponentApplicationRequests::FindEntity, settings.m_targetEntity);
        if (targetEntity)
        {
            m_targetEntityHandler.Setup(m_emitter, settings.m_targetEntity);
        }
        
        //pre-roll
        if (settings.m_prime)
        {
            m_emitter->Prime();
        }

        SetVisibility(settings.m_visible);
        if (!settings.m_enable)
        {
            Enable(false);
        }
               
        ApplyRenderFlags(settings);
    }

    void ParticleEmitter::ApplyRenderFlags(const ParticleEmitterSettings& settings)
    {
        if (m_emitter == nullptr)
        {
            return;
        }

        //apply render node flags
        unsigned int flag = m_emitter->GetRndFlags();
        if (settings.m_useVisAreas)
        {
            flag &= ~ERF_OUTDOORONLY;
        }
        else
        {
            flag |= ERF_OUTDOORONLY;
        }
        m_emitter->SetRndFlags(flag);
        m_emitter->SetViewDistanceMultiplier(settings.m_viewDistMultiplier);
    }

    void ParticleEmitter::TargetEntityHandler::Setup(_smart_ptr<IParticleEmitter> emitter, AZ::EntityId targetEntity)
    {
        if (emitter == m_emitter && targetEntity == m_targetEntity)
        {
            return;
        }

        if(m_targetEntity.IsValid())
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect(m_targetEntity);
        }
        
        m_emitter = emitter;
        m_targetEntity = targetEntity;

        if (m_targetEntity.IsValid())
        {
            AZ::Transform targetEntityTransform;
            AZ::TransformBus::EventResult(targetEntityTransform, m_targetEntity, &AZ::TransformInterface::GetWorldTM);
            UpdateTargetPos(targetEntityTransform);

            AZ::TransformNotificationBus::Handler::BusConnect(m_targetEntity);
        }
        else
        {
            m_emitter->SetTarget(ParticleTarget());
        }
    }

    void ParticleEmitter::TargetEntityHandler::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        UpdateTargetPos(world);
    }

    void ParticleEmitter::TargetEntityHandler::UpdateTargetPos(const AZ::Transform& targetEntityTransform)
    {
        ParticleTarget target;
        target.bTarget = true;

        target.vTarget.Set(targetEntityTransform.GetPosition().GetX(), targetEntityTransform.GetPosition().GetY(), targetEntityTransform.GetPosition().GetZ());

        AZ::Vector3 velocity(0);
        PhysicsComponentRequestBus::EventResult(velocity, m_targetEntity, &AzFramework::PhysicsComponentRequests::GetVelocity);
        target.vVelocity.Set(velocity.GetX(), velocity.GetY(), velocity.GetZ());
        
        AZ::Aabb bounds = AZ::Aabb::CreateNull();
        LmbrCentral::MeshComponentRequestBus::EventResult(bounds, m_targetEntity, &LmbrCentral::MeshComponentRequests::GetLocalBounds);
        if (bounds.IsValid())
        {
            target.fRadius = max(bounds.GetMin().GetLength(), bounds.GetMax().GetLength());
        }
        
        m_emitter->SetTarget(target);
    }

    void ParticleEmitter::ApplyEmitterSetting(const ParticleEmitterSettings& settings)
    {
        if (m_emitter == nullptr)
            return;

        uint32 emmitterFlags = 0;
        if (settings.m_ignoreRotation)
        {
            emmitterFlags |= ePEF_IgnoreRotation;
        }

        if (settings.m_notAttached)
        {
            emmitterFlags |= ePEF_NotAttached;

            if (AZ::TransformNotificationBus::Handler::BusIsConnectedId(m_attachedToEntityId))
            {
                AZ::TransformNotificationBus::Handler::BusDisconnect(m_attachedToEntityId);
            }
        }
        else
        {
            if (!AZ::TransformNotificationBus::Handler::BusIsConnectedId(m_attachedToEntityId))
            {
                AZ::TransformNotificationBus::Handler::BusConnect(m_attachedToEntityId);
            }
        }
        
        if (!settings.m_useLOD)
        {
            emmitterFlags |= ePEF_DisableLOD; 
        }
        //save previous flags
        uint32 prevFlags = m_emitter->GetEmitterFlags();

        //set new flag
        m_emitter->SetEmitterFlags(emmitterFlags);

        //connect to the appropriate target entity
        m_targetEntityHandler.Setup(m_emitter, settings.m_targetEntity);

        SpawnParams spawnParams = GetPropertiesAsSpawnParams(settings);
        m_emitter->SetSpawnParams(spawnParams);

        //visibility and enable is not part of spawn params. set here.
        SetVisibility(settings.m_visible);
        Enable(settings.m_enable);

        //set location if changed from not attached to attached
        if (!settings.m_notAttached && (prevFlags & ePEF_NotAttached))
        {
            AZ::Transform parentTransform = AZ::Transform::CreateIdentity();
            EBUS_EVENT_ID_RESULT(parentTransform, m_attachedToEntityId, AZ::TransformBus, GetWorldTM);
            m_emitter->SetLocation(QuatTS(AZTransformToLYTransform(parentTransform)));
        }

        //if lod setting changed, restart the effect
        if ((!settings.m_useLOD) != (prevFlags & ePEF_DisableLOD))
        {
            m_emitter->Restart();
        }

        if (settings.m_prime && m_emitter->GetEmitterAge() == 0)
        {
            m_emitter->Prime();
        }

        ApplyRenderFlags(settings);
    }

    void ParticleEmitter::SetVisibility(bool visible)
    {
        if (m_emitter)
        {
            m_emitter->Hide(!visible);
        }
    }

    void ParticleEmitter::Show()
    {
        SetVisibility(true);
    }

    void ParticleEmitter::Hide()
    {
        SetVisibility(false);
    }
    
    void ParticleEmitter::Enable(bool enable)
    {
        if (m_emitter)
        {
            if (enable && !m_emitter->IsAlive())
            {
                m_emitter->Restart();
            }
            else
            {
                m_emitter->Activate(enable);
            }
        }
    }

    void ParticleEmitter::Clear()
    {
        if (m_emitter)
        {
            m_emitter->Activate(false);
            m_emitter->SetEntity(nullptr, 0);
        }

        m_emitter = nullptr;
        m_effect = nullptr;
    }

    bool ParticleEmitter::IsCreated() const
    {
        return (m_emitter != nullptr);
    }

    void ParticleEmitter::Restart()
    {
        if (m_emitter)
        {
            m_emitter->Restart();
        }
    }

    IRenderNode* ParticleEmitter::GetRenderNode()
    {
        return m_emitter;
    }
} // namespace LmbrCentral
