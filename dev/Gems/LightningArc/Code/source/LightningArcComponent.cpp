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

#include "LightningArc_precompiled.h"
#include "LightningArcComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include "LightningGameEffectAz.h"

namespace Lightning
{
    // BehaviorContext LightningArcComponentNotificationBus forwarder
    class BehaviorLightningArcComponentNotificationBusHandler 
        : public LightningArcComponentNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorLightningArcComponentNotificationBusHandler, "{1FFAE4AC-14E6-4294-82DF-6D9B27211DA8}", AZ::SystemAllocator,
            OnSpark);

        void OnSpark() override
        {
            Call(FN_OnSpark);
        }
    };

    void LightningArcConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LightningArcConfiguration>()
                ->Version(1)
                ->Field("Enabled", &LightningArcConfiguration::m_enabled)
                ->Field("Targets", &LightningArcConfiguration::m_targets)
                ->Field("Material", &LightningArcConfiguration::m_materialAsset)
                ->Field("ArcParams", &LightningArcConfiguration::m_arcParams)
                ->Field("Delay", &LightningArcConfiguration::m_delay)
                ->Field("DelayVariation", &LightningArcConfiguration::m_delayVariation)
            ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<LightningArcComponentRequestBus>("LightningArcComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)

                ->Event("Enable", &LightningArcComponentRequestBus::Events::Enable)
                ->Event("Disable", &LightningArcComponentRequestBus::Events::Disable)
                ->Event("Toggle", &LightningArcComponentRequestBus::Events::Toggle)
                ->Event("IsEnabled", &LightningArcComponentRequestBus::Events::IsEnabled)

                //SetTargets is not exposed to Lua because creation of AZStd::vectors is not available yet
                ->Event("GetTargets", &LightningArcComponentRequestBus::Events::GetTargets)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)

                ->Event("SetDelay", &LightningArcComponentRequestBus::Events::SetDelay)
                ->Event("GetDelay", &LightningArcComponentRequestBus::Events::GetDelay)
                ->VirtualProperty("Delay", "GetDelay", "SetDelay")

                ->Event("SetDelayVariation", &LightningArcComponentRequestBus::Events::SetDelayVariation)
                ->Event("GetDelayVariation", &LightningArcComponentRequestBus::Events::GetDelayVariation)
                ->VirtualProperty("DelayVariation", "GetDelayVariation", "SetDelayVariation")

                ->Event("SetStrikeTimeMin", &LightningArcComponentRequestBus::Events::SetStrikeTimeMin)
                ->Event("GetStrikeTimeMin", &LightningArcComponentRequestBus::Events::GetStrikeTimeMin)
                ->VirtualProperty("StrikeTimeMin", "GetStrikeTimeMin", "SetStrikeTimeMin")

                ->Event("SetStrikeTimeMax", &LightningArcComponentRequestBus::Events::SetStrikeTimeMax)
                ->Event("GetStrikeTimeMax", &LightningArcComponentRequestBus::Events::GetStrikeTimeMax)
                ->VirtualProperty("StrikeTimeMax", "GetStrikeTimeMax", "SetStrikeTimeMax")

                ->Event("SetStrikeFadeOut", &LightningArcComponentRequestBus::Events::SetStrikeFadeOut)
                ->Event("GetStrikeFadeOut", &LightningArcComponentRequestBus::Events::GetStrikeFadeOut)
                ->VirtualProperty("StrikeFadeOut", "GetStrikeFadeOut", "SetStrikeFadeOut")

                ->Event("SetStrikeSegmentCount", &LightningArcComponentRequestBus::Events::SetStrikeSegmentCount)
                ->Event("GetStrikeSegmentCount", &LightningArcComponentRequestBus::Events::GetStrikeSegmentCount)
                ->VirtualProperty("StrikeSegmentCount", "GetStrikeSegmentCount", "SetStrikeSegmentCount")

                ->Event("SetStrikePointCount", &LightningArcComponentRequestBus::Events::SetStrikePointCount)
                ->Event("GetStrikePointCount", &LightningArcComponentRequestBus::Events::GetStrikePointCount)
                ->VirtualProperty("StrikePointCount", "GetStrikePointCount", "SetStrikePointCount")

                ->Event("SetLightningDeviation", &LightningArcComponentRequestBus::Events::SetLightningDeviation)
                ->Event("GetLightningDeviation", &LightningArcComponentRequestBus::Events::GetLightningDeviation)
                ->VirtualProperty("LightningDeviation", "GetLightningDeviation", "SetLightningDeviation")

                ->Event("SetLightningFuzziness", &LightningArcComponentRequestBus::Events::SetLightningFuzziness)
                ->Event("GetLightningFuzziness", &LightningArcComponentRequestBus::Events::GetLightningFuzziness)
                ->VirtualProperty("LightningFuzziness", "GetLightningFuzziness", "SetLightningFuzziness")

                ->Event("SetLightningVelocity", &LightningArcComponentRequestBus::Events::SetLightningVelocity)
                ->Event("GetLightningVelocity", &LightningArcComponentRequestBus::Events::GetLightningVelocity)
                ->VirtualProperty("LightningVelocity", "GetLightningVelocity", "SetLightningVelocity")

                ->Event("SetBranchProbablity", &LightningArcComponentRequestBus::Events::SetBranchProbablity)
                ->Event("GetBranchProbablity", &LightningArcComponentRequestBus::Events::GetBranchProbablity)
                ->VirtualProperty("BranchProbablity", "GetBranchProbablity", "SetBranchProbablity")

                ->Event("SetBranchMaxLevel", &LightningArcComponentRequestBus::Events::SetBranchMaxLevel)
                ->Event("GetBranchMaxLevel", &LightningArcComponentRequestBus::Events::GetBranchMaxLevel)
                ->VirtualProperty("BranchMaxLevel", "GetBranchMaxLevel", "SetBranchMaxLevel")

                ->Event("SetMaxStrikeCount", &LightningArcComponentRequestBus::Events::SetMaxStrikeCount)
                ->Event("GetMaxStrikeCount", &LightningArcComponentRequestBus::Events::GetMaxStrikeCount)
                ->VirtualProperty("MaxStrikeCount", "GetMaxStrikeCount", "SetMaxStrikeCount")

                ->Event("SetBeamSize", &LightningArcComponentRequestBus::Events::SetBeamSize)
                ->Event("GetBeamSize", &LightningArcComponentRequestBus::Events::GetBeamSize)
                ->VirtualProperty("BeamSize", "GetBeamSize", "SetBeamSize")

                ->Event("SetBeamTexTiling", &LightningArcComponentRequestBus::Events::SetBeamTexTiling)
                ->Event("GetBeamTexTiling", &LightningArcComponentRequestBus::Events::GetBeamTexTiling)
                ->VirtualProperty("BeamTexTiling", "GetBeamTexTiling", "SetBeamTexTiling")

                ->Event("SetBeamTexShift", &LightningArcComponentRequestBus::Events::SetBeamTexShift)
                ->Event("GetBeamTexShift", &LightningArcComponentRequestBus::Events::GetBeamTexShift)
                ->VirtualProperty("BeamTexShift", "GetBeamTexShift", "SetBeamTexShift")

                ->Event("SetBeamTexFrames", &LightningArcComponentRequestBus::Events::SetBeamTexFrames)
                ->Event("GetBeamTexFrames", &LightningArcComponentRequestBus::Events::GetBeamTexFrames)
                ->VirtualProperty("BeamTexFrames", "GetBeamTexFrames", "SetBeamTexFrames")

                ->Event("SetBeamTexFPS", &LightningArcComponentRequestBus::Events::SetBeamTexFPS)
                ->Event("GetBeamTexFPS", &LightningArcComponentRequestBus::Events::GetBeamTexFPS)
                ->VirtualProperty("BeamTexFPS", "GetBeamTexFPS", "SetBeamTexFPS")

                ;

            behaviorContext->EBus<LightningArcComponentNotificationBus>("LightningArcComponentNotificationBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Handler<BehaviorLightningArcComponentNotificationBusHandler>()
                ;

            behaviorContext->Class<LightningArcComponent>()
                ->RequestBus("LightningArcComponentRequestBus")
                ->NotificationBus("LightningArcComponentNotificationBus")
                ;
        }
    }

    void LightningArcComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ_CRC("LightningArcService"));
    }

    void LightningArcComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LightningArcService"));
    }

    void LightningArcComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires)
    {
        requires.push_back(AZ_CRC("TransformService"));
    }

    void LightningArcComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LightningArcComponent, AZ::Component>()
                ->Version(1)
                ->Field("m_config", &LightningArcComponent::m_config)
                ;
        }

        LightningArcParams::Reflect(context);
        LightningArcConfiguration::Reflect(context);
    }

    LightningArcComponent::LightningArcComponent()
        : m_material(nullptr)
        , m_currentTimePoint(0.0)
        , m_sparkTimePoint(0.0)
        , m_currentWorldPos(AZ::Vector3::CreateZero())
        , m_rand(time(0))
        , m_gameEffect(nullptr)
    {

    }

    void LightningArcComponent::Activate()
    {
        //Retrieve material from asset
        const AZStd::string& materialPath = m_config.m_materialAsset.GetAssetPath();
        if (!materialPath.empty())
        {
            m_material = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(materialPath.c_str());
        }

        m_currentTimePoint = 0.0;

        AZ::TransformBus::EventResult(m_currentWorldPos, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

        LightningArcRequestBus::BroadcastResult(m_gameEffect, &LightningArcRequestBus::Events::GetGameEffectAZ);
        
        LightningArcComponentRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());

        if (m_config.m_enabled)
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void LightningArcComponent::Deactivate()
    {
        m_material = nullptr;

        m_gameEffect = nullptr;

        AZ::TickBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        LightningArcComponentRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    void LightningArcComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentWorldPos = world.GetPosition();
    }

    void LightningArcComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        m_currentTimePoint = time.GetSeconds();

        if (m_currentTimePoint >= m_sparkTimePoint)
        {
            TriggerSpark();
            CalculateNextSparkTime();
        }
    }

    void LightningArcComponent::Enable() 
    {
        m_config.m_enabled = true;
        AZ::TickBus::Handler::BusConnect();
    }
    void LightningArcComponent::Disable() 
    {
        m_config.m_enabled = false;
        AZ::TickBus::Handler::BusDisconnect();
    }
    void LightningArcComponent::Toggle() 
    {
        if (m_config.m_enabled)
        {
            Disable();
        }
        else
        {
            Enable();
        }
    }

    void LightningArcComponent::SetStrikeSegmentCount(AZ::u32 strikeSegmentCount)
    {
        //Cannot have less than 1 segment
        if (strikeSegmentCount == 0)
        {
            AZ_Warning("LightningArcComponent", false, "SetStrikeSegmentCount cannot take 0 as a vaild parameter. A lightning arc cannot have 0 segments. Using 1 instead.");
            strikeSegmentCount = 1;
        }
        else if (strikeSegmentCount > MAX_STRIKE_SEGMENT_COUNT)
        {
            AZ_Warning("LightningArcComponent", false, "SetStrikeSegmentCount cannot take %d as a vaild parameter since it is greater than the allowed maximum. Using %d instead.", strikeSegmentCount, MAX_STRIKE_SEGMENT_COUNT);
            strikeSegmentCount = MAX_STRIKE_SEGMENT_COUNT;
        }
        m_config.m_arcParams.m_strikeNumSegments = strikeSegmentCount;
    }

    void LightningArcComponent::SetStrikePointCount(AZ::u32 strikePointCount)
    {
        //Cannot have less than 1 point
        if (strikePointCount == 0)
        {
            AZ_Warning("LightningArcComponent", false, "SetStrikePointCount cannot take 0 as a vaild parameter. A lightning arc cannot have 0 points. Using 1 instead.");
            strikePointCount = 1;
        }
        else if (strikePointCount > MAX_STRIKE_POINT_COUNT)
        {
            AZ_Warning("LightningArcComponent", false, "SetStrikePointCount cannot take %d as a vaild parameter since it is greater than the allowed maximum. Using %d instead.", strikePointCount, MAX_STRIKE_POINT_COUNT);
            strikePointCount = MAX_STRIKE_POINT_COUNT;
        }
        m_config.m_arcParams.m_strikeNumPoints = strikePointCount;
    }

    void LightningArcComponent::CalculateNextSparkTime()
    {
        double variation = static_cast<double>(m_rand.GetRandomFloat() * 0.5f + 0.5f) * m_config.m_delayVariation;

        m_sparkTimePoint = m_currentTimePoint + m_config.m_delay + variation;
    }

    void LightningArcComponent::TriggerSpark()
    {
        if (m_material == nullptr || m_gameEffect == nullptr)
        {
            return;
        }

        if (m_config.m_targets.size() == 0)
        {
            return;
        }

        //Pick a random target entity. 
        AZ::u32 size = static_cast<AZ::u32>(m_config.m_targets.size());
        AZ::u32 targetIndex = m_rand.GetRandom() % size;

        AZ::EntityId targetId = m_config.m_targets[targetIndex];

        AZ::s32 id = m_gameEffect->TriggerSpark(m_config.m_arcParams, m_material,
            LightningGameEffectAZ::Target(m_currentWorldPos),
            LightningGameEffectAZ::Target(targetId));

        //Trigger OnSpark event
        LightningArcComponentNotificationBus::Event(GetEntityId(), &LightningArcComponentNotificationBus::Events::OnSpark);
    }

} //namespace Lightning