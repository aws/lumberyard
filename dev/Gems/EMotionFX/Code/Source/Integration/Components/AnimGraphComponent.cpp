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


#include "EMotionFX_precompiled.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <Integration/Components/AnimGraphComponent.h>

#include <MCore/Source/AttributeString.h>

//#pragma optimize("",off)

namespace EMotionFX
{
    namespace Integration
    {
        //////////////////////////////////////////////////////////////////////////
        AnimGraphComponent::ParameterDefaults::ParameterDefaults()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        AnimGraphComponent::ParameterDefaults::~ParameterDefaults()
        {
            Reset();
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::ParameterDefaults::Reset()
        {
            for (AZ::ScriptProperty* p : m_parameters)
            {
                delete p;
            }
            m_parameters.clear();
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::ParameterDefaults::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<ParameterDefaults>()
                    ->Version(1)
                    ->Field("Parameters", &ParameterDefaults::m_parameters)
                ;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::Configuration::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<Configuration>()
                    ->Version(1)
                    ->Field("AnimGraphAsset", &Configuration::m_animGraphAsset)
                    ->Field("MotionSetAsset", &Configuration::m_motionSetAsset)
                    ->Field("ParameterDefaults", &Configuration::m_parameterDefaults)
                ;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::Reflect(AZ::ReflectContext* context)
        {
            ParameterDefaults::Reflect(context);
            Configuration::Reflect(context);

            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<AnimGraphComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("Configuration", &AnimGraphComponent::m_configuration)
                ;
            }

            auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Constant("InvalidParameterIndex", BehaviorConstant(static_cast<AZ::u32>(MCORE_INVALIDINDEX32)));

                behaviorContext->EBus<AnimGraphComponentRequestBus>("AnimGraphComponentRequestBus")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                // General API
                    ->Event("FindParameterIndex", &AnimGraphComponentRequestBus::Events::FindParameterIndex)
                // Parameter setters
                    ->Event("SetParameterFloat", &AnimGraphComponentRequestBus::Events::SetParameterFloat)
                    ->Event("SetParameterBool", &AnimGraphComponentRequestBus::Events::SetParameterBool)
                    ->Event("SetParameterString", &AnimGraphComponentRequestBus::Events::SetParameterString)
                    ->Event("SetParameterVector2", &AnimGraphComponentRequestBus::Events::SetParameterVector2)
                    ->Event("SetParameterVector3", &AnimGraphComponentRequestBus::Events::SetParameterVector3)
                    ->Event("SetParameterRotationEuler", &AnimGraphComponentRequestBus::Events::SetParameterRotationEuler)
                    ->Event("SetParameterRotation", &AnimGraphComponentRequestBus::Events::SetNamedParameterRotation)
                    ->Event("SetNamedParameterFloat", &AnimGraphComponentRequestBus::Events::SetNamedParameterFloat)
                    ->Event("SetNamedParameterBool", &AnimGraphComponentRequestBus::Events::SetNamedParameterBool)
                    ->Event("SetNamedParameterString", &AnimGraphComponentRequestBus::Events::SetNamedParameterString)
                    ->Event("SetNamedParameterVector2", &AnimGraphComponentRequestBus::Events::SetNamedParameterVector2)
                    ->Event("SetNamedParameterVector3", &AnimGraphComponentRequestBus::Events::SetNamedParameterVector3)
                    ->Event("SetNamedParameterRotationEuler", &AnimGraphComponentRequestBus::Events::SetNamedParameterRotationEuler)
                    ->Event("SetNamedParameterRotation", &AnimGraphComponentRequestBus::Events::SetNamedParameterRotation)
                // Parameter getters
                    ->Event("GetParameterFloat", &AnimGraphComponentRequestBus::Events::GetParameterFloat)
                    ->Event("GetParameterBool", &AnimGraphComponentRequestBus::Events::GetParameterBool)
                    ->Event("GetParameterString", &AnimGraphComponentRequestBus::Events::GetParameterString)
                    ->Event("GetParameterVector2", &AnimGraphComponentRequestBus::Events::GetParameterVector2)
                    ->Event("GetParameterVector3", &AnimGraphComponentRequestBus::Events::GetParameterVector3)
                    ->Event("GetParameterRotationEuler", &AnimGraphComponentRequestBus::Events::GetParameterRotationEuler)
                    ->Event("GetParameterRotation", &AnimGraphComponentRequestBus::Events::GetNamedParameterRotation)
                    ->Event("GetNamedParameterFloat", &AnimGraphComponentRequestBus::Events::GetNamedParameterFloat)
                    ->Event("GetNamedParameterBool", &AnimGraphComponentRequestBus::Events::GetNamedParameterBool)
                    ->Event("GetNamedParameterString", &AnimGraphComponentRequestBus::Events::GetNamedParameterString)
                    ->Event("GetNamedParameterVector2", &AnimGraphComponentRequestBus::Events::GetNamedParameterVector2)
                    ->Event("GetNamedParameterVector3", &AnimGraphComponentRequestBus::Events::GetNamedParameterVector3)
                    ->Event("GetNamedParameterRotationEuler", &AnimGraphComponentRequestBus::Events::GetNamedParameterRotationEuler)
                    ->Event("GetNamedParameterRotation", &AnimGraphComponentRequestBus::Events::GetNamedParameterRotation)
                ;

                behaviorContext->EBus<AnimGraphComponentNotificationBus>("AnimGraphComponentNotificationBus")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                    ->Event("OnAnimGraphInstanceCreated", &AnimGraphComponentNotificationBus::Events::OnAnimGraphInstanceCreated)
                    ->Event("OnAnimGraphInstanceDestroyed", &AnimGraphComponentNotificationBus::Events::OnAnimGraphInstanceDestroyed)
                    ->Event("OnAnimGraphFloatParameterChanged", &AnimGraphComponentNotificationBus::Events::OnAnimGraphFloatParameterChanged)
                    ->Event("OnAnimGraphBoolParameterChanged", &AnimGraphComponentNotificationBus::Events::OnAnimGraphBoolParameterChanged)
                    ->Event("OnAnimGraphStringParameterChanged", &AnimGraphComponentNotificationBus::Events::OnAnimGraphStringParameterChanged)
                    ->Event("OnAnimGraphVector2ParameterChanged", &AnimGraphComponentNotificationBus::Events::OnAnimGraphVector2ParameterChanged)
                    ->Event("OnAnimGraphVector3ParameterChanged", &AnimGraphComponentNotificationBus::Events::OnAnimGraphVector3ParameterChanged)
                    ->Event("OnAnimGraphRotationParameterChanged", &AnimGraphComponentNotificationBus::Events::OnAnimGraphRotationParameterChanged)
                ;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        AnimGraphComponent::Configuration::Configuration()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        AnimGraphComponent::AnimGraphComponent(const Configuration* config)
        {
            if (config)
            {
                m_configuration = *config;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        AnimGraphComponent::~AnimGraphComponent()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::Init()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::Activate()
        {
            m_animGraphInstance.reset();

            AZ::Data::AssetBus::MultiHandler::BusDisconnect();

            auto& cfg = m_configuration;

            if (cfg.m_animGraphAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(cfg.m_animGraphAsset.GetId());
                cfg.m_animGraphAsset.QueueLoad();

                if (cfg.m_motionSetAsset.GetId().IsValid())
                {
                    AZ::Data::AssetBus::MultiHandler::BusConnect(cfg.m_motionSetAsset.GetId());
                    cfg.m_motionSetAsset.QueueLoad();
                }
            }

            ActorComponentNotificationBus::Handler::BusConnect(GetEntityId());
            AnimGraphComponentRequestBus::Handler::BusConnect(GetEntityId());
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::Deactivate()
        {
            AnimGraphComponentRequestBus::Handler::BusDisconnect();
            ActorComponentNotificationBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();

            DestroyAnimGraphInstance();
            m_configuration.m_animGraphAsset.Release();
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            OnAssetReady(asset);
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            auto& cfg = m_configuration;

            if (asset == cfg.m_animGraphAsset)
            {
                cfg.m_animGraphAsset = asset;
            }
            else if (asset == cfg.m_motionSetAsset)
            {
                cfg.m_motionSetAsset = asset;
            }

            CheckCreateAnimGraphInstance();
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance)
        {
            m_actorInstance = actorInstance;

            CheckCreateAnimGraphInstance();
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::OnActorInstanceDestroyed(EMotionFX::ActorInstance* /*actorInstance*/)
        {
            DestroyAnimGraphInstance();

            m_actorInstance.reset();
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::CheckCreateAnimGraphInstance()
        {
            auto& cfg = m_configuration;

            if (m_actorInstance && cfg.m_animGraphAsset.IsReady() && cfg.m_motionSetAsset.IsReady())
            {
                DestroyAnimGraphInstance();

                m_animGraphInstance = cfg.m_animGraphAsset.Get()->CreateInstance(m_actorInstance, cfg.m_motionSetAsset.Get()->m_emfxMotionSet);
                if (!m_animGraphInstance)
                {
                    AZ_Error("EMotionFX", false, "Failed to create anim graph instance for entity \"%s\" %s.",
                        GetEntity()->GetName().c_str(),
                        GetEntityId().ToString().c_str());
                    return;
                }

                m_actorInstance->SetAnimGraphInstance(m_animGraphInstance.get());

                AnimGraphInstancePostCreate();

                // Apply parameter defaults.
                EMotionFXPtr<EMotionFX::AnimGraph> animGraph = cfg.m_animGraphAsset.Get()->GetAnimGraph();
                for (AZ::ScriptProperty* parameter : cfg.m_parameterDefaults.m_parameters)
                {
                    const char* paramName = parameter->m_name.c_str();

                    if (azrtti_istypeof<AZ::ScriptPropertyNumber>(parameter))
                    {
                        SetNamedParameterFloat(paramName, aznumeric_caster(static_cast<AZ::ScriptPropertyNumber*>(parameter)->m_value));
                    }
                    else if (azrtti_istypeof<AZ::ScriptPropertyBoolean>(parameter))
                    {
                        SetNamedParameterBool(paramName, static_cast<AZ::ScriptPropertyBoolean*>(parameter)->m_value);
                    }
                    else
                    {
                        AZ_Warning("EMotionFX", false, "Invalid type for anim graph parameter \"%s\".", paramName);
                    }
                }

                // Notify listeners that the anim graph is ready.
                AnimGraphComponentNotificationBus::Event(
                    GetEntityId(),
                    &AnimGraphComponentNotificationBus::Events::OnAnimGraphInstanceCreated,
                    m_animGraphInstance.get());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::DestroyAnimGraphInstance()
        {
            if (m_animGraphInstance)
            {
                AnimGraphComponentNotificationBus::Event(
                    GetEntityId(),
                    &AnimGraphComponentNotificationBus::Events::OnAnimGraphInstanceDestroyed,
                    m_animGraphInstance.get());

                AnimGraphInstancePreDestroy();

                m_animGraphInstance.reset();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::AnimGraphInstancePostCreate()
        {
            // Reference is not incremented when the instance is assigned to the actor, but is
            // decremented when actor is destroyed. Add a ref here to account for this.
            m_animGraphInstance.get()->IncreaseReferenceCount();
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::AnimGraphInstancePreDestroy()
        {
            // If the anim graph is still active on the actor, deactivate it.
            // Also remove the extra reference we added to account for the actor's ownership
            // over it (see corresponding logic in OnAnimGraphInstanceCreated()), since we're
            // relinquishing that ownership.
            if (m_actorInstance && m_animGraphInstance && m_actorInstance->GetAnimGraphInstance() == m_animGraphInstance.get())
            {
                m_actorInstance->SetAnimGraphInstance(nullptr);
                m_animGraphInstance->DecreaseReferenceCount();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::u32 AnimGraphComponent::FindParameterIndex(const char* parameterName)
        {
            if (m_animGraphInstance)
            {
                return m_animGraphInstance->FindParameterIndex(parameterName);
            }

            return MCORE_INVALIDINDEX32;
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetParameterFloat(AZ::u32 parameterIndex, float value)
        {
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return;
            }

            if (m_animGraphInstance)
            {
                MCore::AttributeFloat* param = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeFloat>(parameterIndex);
                if (param)
                {
                    const float previousValue = param->GetValue();
                    param->SetValue(value);

                    // Notify listeners about the parameter change
                    AnimGraphComponentNotificationBus::Event(
                        GetEntityId(),
                        &AnimGraphComponentNotificationBus::Events::OnAnimGraphFloatParameterChanged,
                        m_animGraphInstance.get(),
                        parameterIndex,
                        previousValue,
                        value);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetParameterBool(AZ::u32 parameterIndex, bool value)
        {
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return;
            }

            if (m_animGraphInstance)
            {
                MCore::AttributeFloat* param = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeFloat>(parameterIndex);
                if (param)
                {
                    const bool previousValue = AZ::IsClose(param->GetValue(), 1.f, MCore::Math::epsilon);
                    param->SetValue(value ? 1.0f : 0.0f);

                    // Notify listeners about the parameter change
                    AnimGraphComponentNotificationBus::Event(
                        GetEntityId(),
                        &AnimGraphComponentNotificationBus::Events::OnAnimGraphBoolParameterChanged,
                        m_animGraphInstance.get(),
                        parameterIndex,
                        previousValue,
                        value);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetParameterString(AZ::u32 parameterIndex, const char* value)
        {
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return;
            }

            if (m_animGraphInstance)
            {
                MCore::AttributeString* param = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeString>(parameterIndex);
                if (param)
                {
                    // Since the event is sent out synchronously we just need to keep a copy of the
                    // previous value. The new value can be reused from "value"
                    // If the event were to change to a queued event, the parameters should be changed
                    // to AZStd::string for safety.
                    const AZStd::string previousValue(param->GetValue());
                    param->SetValue(value);

                    // Notify listeners about the parameter change
                    AnimGraphComponentNotificationBus::Event(
                        GetEntityId(),
                        &AnimGraphComponentNotificationBus::Events::OnAnimGraphStringParameterChanged,
                        m_animGraphInstance.get(),
                        parameterIndex,
                        previousValue.c_str(),
                        value);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetParameterVector2(AZ::u32 parameterIndex, const AZ::Vector2& value)
        {
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return;
            }

            if (m_animGraphInstance)
            {
                MCore::AttributeVector2* param = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeVector2>(parameterIndex);
                if (param)
                {
                    const AZ::Vector2 previousValue = param->GetValue();
                    param->SetValue(value);

                    // Notify listeners about the parameter change
                    AnimGraphComponentNotificationBus::Event(
                        GetEntityId(),
                        &AnimGraphComponentNotificationBus::Events::OnAnimGraphVector2ParameterChanged,
                        m_animGraphInstance.get(),
                        parameterIndex,
                        previousValue,
                        value);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetParameterVector3(AZ::u32 parameterIndex, const AZ::Vector3& value)
        {
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return;
            }

            if (m_animGraphInstance)
            {
                MCore::AttributeVector3* param = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeVector3>(parameterIndex);
                if (param)
                {
                    const AZ::Vector3 previousValue = AZ::Vector3(param->GetValue());
                    param->SetValue(AZ::PackedVector3f(value));

                    // Notify listeners about the parameter change
                    AnimGraphComponentNotificationBus::Event(
                        GetEntityId(),
                        &AnimGraphComponentNotificationBus::Events::OnAnimGraphVector3ParameterChanged,
                        m_animGraphInstance.get(),
                        parameterIndex,
                        previousValue,
                        value);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetParameterRotationEuler(AZ::u32 parameterIndex, const AZ::Vector3& value)
        {
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return;
            }

            if (m_animGraphInstance)
            {
                MCore::AttributeQuaternion* param = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeQuaternion>(parameterIndex);
                if (param)
                {
                    const AZ::Quaternion previousValue = MCore::EmfxQuatToAzQuat(param->GetValue());
                    param->SetValue(MCore::AzEulerAnglesToEmfxQuat(value));
                    const AZ::Quaternion newValue = MCore::EmfxQuatToAzQuat(param->GetValue());

                    // Notify listeners about the parameter change
                    AnimGraphComponentNotificationBus::Event(
                        GetEntityId(),
                        &AnimGraphComponentNotificationBus::Events::OnAnimGraphRotationParameterChanged,
                        m_animGraphInstance.get(),
                        parameterIndex,
                        previousValue,
                        newValue);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetParameterRotation(AZ::u32 parameterIndex, const AZ::Quaternion& value)
        {
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return;
            }

            if (m_animGraphInstance)
            {
                MCore::AttributeQuaternion* param = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeQuaternion>(parameterIndex);
                if (param)
                {
                    const AZ::Quaternion previousValue = MCore::EmfxQuatToAzQuat(param->GetValue());
                    param->SetValue(MCore::AzQuatToEmfxQuat(value));

                    // Notify listeners about the parameter change
                    AnimGraphComponentNotificationBus::Event(
                        GetEntityId(),
                        &AnimGraphComponentNotificationBus::Events::OnAnimGraphRotationParameterChanged,
                        m_animGraphInstance.get(),
                        parameterIndex,
                        previousValue,
                        value);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetNamedParameterFloat(const char* parameterName, float value)
        {
            if (m_animGraphInstance)
            {
                const uint32 parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (parameterIndex == MCORE_INVALIDINDEX32)
                {
                    AZ_Warning("EmotionFX", false, "Invalid anim graph parameter name: %s", parameterName);
                    return;
                }
                SetParameterFloat(parameterIndex, value);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetNamedParameterBool(const char* parameterName, bool value)
        {
            if (m_animGraphInstance)
            {
                const uint32 parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (parameterIndex == MCORE_INVALIDINDEX32)
                {
                    AZ_Warning("EmotionFX", false, "Invalid anim graph parameter name: %s", parameterName);
                    return;
                }
                SetParameterBool(parameterIndex, value);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetNamedParameterString(const char* parameterName, const char* value)
        {
            if (m_animGraphInstance)
            {
                const uint32 parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (parameterIndex == MCORE_INVALIDINDEX32)
                {
                    AZ_Warning("EmotionFX", false, "Invalid anim graph parameter name: %s", parameterName);
                    return;
                }
                SetParameterString(parameterIndex, value);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetNamedParameterVector2(const char* parameterName, const AZ::Vector2& value)
        {
            if (m_animGraphInstance)
            {
                const uint32 parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (parameterIndex == MCORE_INVALIDINDEX32)
                {
                    AZ_Warning("EmotionFX", false, "Invalid anim graph parameter name: %s", parameterName);
                    return;
                }
                SetParameterVector2(parameterIndex, value);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetNamedParameterVector3(const char* parameterName, const AZ::Vector3& value)
        {
            if (m_animGraphInstance)
            {
                const uint32 parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (parameterIndex == MCORE_INVALIDINDEX32)
                {
                    AZ_Warning("EmotionFX", false, "Invalid anim graph parameter name: %s", parameterName);
                    return;
                }
                SetParameterVector3(parameterIndex, value);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetNamedParameterRotationEuler(const char* parameterName, const AZ::Vector3& value)
        {
            if (m_animGraphInstance)
            {
                const uint32 parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (parameterIndex == MCORE_INVALIDINDEX32)
                {
                    AZ_Warning("EmotionFX", false, "Invalid anim graph parameter name: %s", parameterName);
                    return;
                }
                SetParameterRotationEuler(parameterIndex, value);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphComponent::SetNamedParameterRotation(const char* parameterName, const AZ::Quaternion& value)
        {
            if (m_animGraphInstance)
            {
                const uint32 parameterIndex = m_animGraphInstance->FindParameterIndex(parameterName);
                if (parameterIndex == MCORE_INVALIDINDEX32)
                {
                    AZ_Warning("EmotionFX", false, "Invalid anim graph parameter name: %s", parameterName);
                    return;
                }
                SetParameterRotation(parameterIndex, value);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        float AnimGraphComponent::GetParameterFloat(AZ::u32 parameterIndex)
        {
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return 0.f;
            }

            float value = 0.f;
            if (m_animGraphInstance)
            {
                m_animGraphInstance->GetFloatParameterValue(parameterIndex, &value);
            }
            return value;
        }

        //////////////////////////////////////////////////////////////////////////
        bool AnimGraphComponent::GetParameterBool(AZ::u32 parameterIndex)
        {
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return false;
            }

            bool value = false;
            if (m_animGraphInstance)
            {
                m_animGraphInstance->GetFloatParameterValueAsBool(parameterIndex, &value);
            }
            return value;
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string AnimGraphComponent::GetParameterString(AZ::u32 parameterIndex)
        {
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return AZStd::string();
            }

            if (m_animGraphInstance)
            {
                MCore::AttributeString* param = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeString>(parameterIndex);
                if (param)
                {
                    return AZStd::string(param->GetValue().AsChar());
                }
            }
            return AZStd::string();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Vector2 AnimGraphComponent::GetParameterVector2(AZ::u32 parameterIndex)
        {
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return AZ::Vector2::CreateZero();
            }

            if (m_animGraphInstance)
            {
                AZ::Vector2 value;
                m_animGraphInstance->GetVector2ParameterValue(parameterIndex, &value);
                return value;
            }
            return AZ::Vector2::CreateZero();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Vector3 AnimGraphComponent::GetParameterVector3(AZ::u32 parameterIndex)
        {
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return AZ::Vector3::CreateZero();
            }

            if (m_animGraphInstance)
            {
                AZ::Vector3 value;
                m_animGraphInstance->GetVector3ParameterValue(parameterIndex, &value);
                return value;
            }
            return AZ::Vector3::CreateZero();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Vector3 AnimGraphComponent::GetParameterRotationEuler(AZ::u32 parameterIndex)
        {
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return AZ::Vector3::CreateZero();
            }

            if (m_animGraphInstance)
            {
                MCore::Quaternion value;
                m_animGraphInstance->GetRotationParameterValue(parameterIndex, &value);
                return value.ToEuler();
            }
            return AZ::Vector3::CreateZero();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Quaternion AnimGraphComponent::GetParameterRotation(AZ::u32 parameterIndex)
        {
            if (parameterIndex == MCORE_INVALIDINDEX32)
            {
                AZ_Warning("EMotionFX", false, "Invalid anim graph parameter index: %u", parameterIndex);
                return AZ::Quaternion::CreateZero();
            }

            if (m_animGraphInstance)
            {
                MCore::Quaternion value;
                m_animGraphInstance->GetRotationParameterValue(parameterIndex, &value);
                return EmfxQuatToAzQuat(value);
            }
            return AZ::Quaternion::CreateIdentity();
        }

        //////////////////////////////////////////////////////////////////////////
        float AnimGraphComponent::GetNamedParameterFloat(const char* parameterName)
        {
            if (m_animGraphInstance)
            {
                return GetParameterFloat(m_animGraphInstance->FindParameterIndex(parameterName));
            }
            return 0.f;
        }

        //////////////////////////////////////////////////////////////////////////
        bool AnimGraphComponent::GetNamedParameterBool(const char* parameterName)
        {
            if (m_animGraphInstance)
            {
                return GetParameterBool(m_animGraphInstance->FindParameterIndex(parameterName));
            }
            return false;
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string AnimGraphComponent::GetNamedParameterString(const char* parameterName)
        {
            if (m_animGraphInstance)
            {
                return GetParameterString(m_animGraphInstance->FindParameterIndex(parameterName));
            }
            return AZStd::string();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Vector2 AnimGraphComponent::GetNamedParameterVector2(const char* parameterName)
        {
            if (m_animGraphInstance)
            {
                return GetParameterVector2(m_animGraphInstance->FindParameterIndex(parameterName));
            }
            return AZ::Vector2::CreateZero();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Vector3 AnimGraphComponent::GetNamedParameterVector3(const char* parameterName)
        {
            if (m_animGraphInstance)
            {
                return GetParameterVector3(m_animGraphInstance->FindParameterIndex(parameterName));
            }
            return AZ::Vector3::CreateZero();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Vector3 AnimGraphComponent::GetNamedParameterRotationEuler(const char* parameterName)
        {
            if (m_animGraphInstance)
            {
                return GetParameterRotationEuler(m_animGraphInstance->FindParameterIndex(parameterName));
            }
            return AZ::Vector3::CreateZero();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Quaternion AnimGraphComponent::GetNamedParameterRotation(const char* parameterName)
        {
            if (m_animGraphInstance)
            {
                return GetParameterRotation(m_animGraphInstance->FindParameterIndex(parameterName));
            }
            return AZ::Quaternion::CreateIdentity();
        }
    } // namespace Integration
} // namespace EMotionFXAnimation

