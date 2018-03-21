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
#include "ScriptedEntityTweener_precompiled.h"

#include <AzCore/Math/Color.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <ScriptedEntityTweener/ScriptedEntityTweenerBus.h>

#include "ScriptedEntityTweenerSubtask.h"
#include "ScriptedEntityTweenerMath.h"

namespace ScriptedEntityTweener
{
    bool ScriptedEntityTweenerSubtask::Initialize(const AnimationParameterAddressData& addressData, const AZStd::any& targetValue, const AnimationProperties& properties)
    {
        Reset();
        if (GetAnimatablePropertyAddress(m_animatableAddress, addressData.m_componentName, addressData.m_virtualPropertyName))
        {
            Maestro::SequenceAgentComponentRequestBus::EventResult(m_propertyTypeId, *m_sequenceAgentBusId, &Maestro::SequenceAgentComponentRequestBus::Events::GetAnimatedAddressTypeId, m_animatableAddress);
            if (!m_propertyTypeId.IsNull())
            {
                if (GetVirtualValue(m_valueInitial))
                {
                    if (GetValueFromAny(m_valueTarget, targetValue))
                    {
                        m_isActive = true;
                        m_animationProperties = properties;

                        if (m_animationProperties.m_isFrom)
                        {
                            EntityAnimatedValue initialValue = m_valueInitial;
                            m_valueInitial = m_valueTarget;
                            m_valueTarget = initialValue;
                        }
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void ScriptedEntityTweenerSubtask::Update(float deltaTime, AZStd::set<CallbackData>& callbacks)
    {
        if (m_isPaused || !m_isActive)
        {
            return;
        }
        //TODO: Use m_animationProperties.m_amplitudeOverride
        
        float timeAnimationActive = AZ::GetClamp(m_timeSinceStart + m_animationProperties.m_timeIntoAnimation, .0f, m_animationProperties.m_timeDuration);

        //If animation is meant to complete instantly, set timeToComplete and timeAnimationActive to the same non-zero value, so GetEasingResult will return m_valueTarget
        if (m_animationProperties.m_timeDuration == 0)
        {
            m_animationProperties.m_timeDuration = timeAnimationActive = 1.0f;
        }

        EntityAnimatedValue currentValue;
        if (m_propertyTypeId == AZ::AzTypeInfo<float>::Uuid())
        {
            float initialValue;
            m_valueInitial.GetValue(initialValue);

            float targetValue;
            m_valueTarget.GetValue(targetValue);

            currentValue.SetValue(EasingEquations::GetEasingResult(m_animationProperties.m_easeMethod, m_animationProperties.m_easeType, timeAnimationActive, m_animationProperties.m_timeDuration, initialValue, targetValue));

            SetVirtualValue(currentValue);
        }
        else if (m_propertyTypeId == AZ::AzTypeInfo<AZ::Vector3>::Uuid() || m_propertyTypeId == AZ::AzTypeInfo<AZ::Color>::Uuid())
        {
            AZ::Vector3 initialValue;
            m_valueInitial.GetValue(initialValue);

            AZ::Vector3 targetValue;
            m_valueTarget.GetValue(targetValue);

            currentValue.SetValue(EasingEquations::GetEasingResult(m_animationProperties.m_easeMethod, m_animationProperties.m_easeType, timeAnimationActive, m_animationProperties.m_timeDuration, initialValue, targetValue));

            SetVirtualValue(currentValue);
        }
        else if (m_propertyTypeId == AZ::AzTypeInfo<AZ::Quaternion>::Uuid())
        {
            AZ::Quaternion initialValue;
            m_valueInitial.GetValue(initialValue);

            AZ::Quaternion targetValue;
            m_valueTarget.GetValue(targetValue);

            currentValue.SetValue(EasingEquations::GetEasingResult(m_animationProperties.m_easeMethod, m_animationProperties.m_easeType, timeAnimationActive, m_animationProperties.m_timeDuration, initialValue, targetValue));

            SetVirtualValue(currentValue);
        }

        float progressPercent = timeAnimationActive / m_animationProperties.m_timeDuration;
        if (m_animationProperties.m_isPlayingBackward)
        {
            progressPercent = 1.0f - progressPercent;
        }

        if (progressPercent >= 1.0f)
        {
            m_timesPlayed = m_timesPlayed + 1;
            if (m_timesPlayed >= m_animationProperties.m_timesToPlay && m_animationProperties.m_timesToPlay != -1)
            {
                m_isActive = false;
                if (m_animationProperties.m_onCompleteCallbackId != AnimationProperties::InvalidCallbackId)
                {
                    callbacks.insert(CallbackData(CallbackTypes::OnComplete, m_animationProperties.m_onCompleteCallbackId));
                }
                if (m_animationProperties.m_onLoopCallbackId != AnimationProperties::InvalidCallbackId)
                {
                    callbacks.insert(CallbackData(CallbackTypes::RemoveCallback, m_animationProperties.m_onLoopCallbackId));
                }
                if (m_animationProperties.m_onUpdateCallbackId != AnimationProperties::InvalidCallbackId)
                {
                    callbacks.insert(CallbackData(CallbackTypes::RemoveCallback, m_animationProperties.m_onUpdateCallbackId));
                }
            }
            else
            {
                m_timeSinceStart = .0f;
                if (m_animationProperties.m_onLoopCallbackId != AnimationProperties::InvalidCallbackId)
                {
                    callbacks.insert(CallbackData(CallbackTypes::OnLoop, m_animationProperties.m_onLoopCallbackId));
                }
            }
        }

        if (m_animationProperties.m_onUpdateCallbackId != AnimationProperties::InvalidCallbackId)
        {
            CallbackData updateCallback(CallbackTypes::OnUpdate, m_animationProperties.m_onUpdateCallbackId);
            GetValueAsAny(updateCallback.m_callbackData, currentValue);
            updateCallback.m_progressPercent = progressPercent;
            callbacks.insert(updateCallback);
        }

        if (m_animationProperties.m_isPlayingBackward)
        {
            deltaTime *= -1.0f;
        }
        m_timeSinceStart += (deltaTime * m_animationProperties.m_playbackSpeedMultiplier);
    }

    bool ScriptedEntityTweenerSubtask::GetAnimatablePropertyAddress(Maestro::SequenceComponentRequests::AnimatablePropertyAddress& outAddress, const AZStd::string& componentName, const AZStd::string& virtualPropertyName)
    {
        /*
        Relies on some behavior context definitions for lookup

        behaviorContext->EBus<UiFaderBus>("UiFaderBus")
            ->Event("GetFadeValue", &UiFaderBus::Events::GetFadeValue)
            ->Event("SetFadeValue", &UiFaderBus::Events::SetFadeValue)
            ->VirtualProperty("Fade", "GetFadeValue", "SetFadeValue");

        behaviorContext->Class<UiFaderComponent>()->RequestBus("UiFaderBus");

        behaviorContext->EBus<UiFaderNotificationBus>("UiFaderNotificationBus")
            ->Handler<BehaviorUiFaderNotificationBusHandler>();
        */
        AZ::BehaviorContext* behaviorContext = nullptr;
        EBUS_EVENT_RESULT(behaviorContext, AZ::ComponentApplicationBus, GetBehaviorContext);

        AZ::Entity* entity = nullptr;
        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, m_sequenceAgentBusId->first);
        if (entity && behaviorContext)
        {
            const AZ::Entity::ComponentArrayType& entityComponents = entity->GetComponents();
            for (AZ::Component* component : entityComponents)
            {
                const AZ::Uuid& componentId = (*component).RTTI_GetType();
                auto findClassIter = behaviorContext->m_typeToClassMap.find(componentId);
                if (findClassIter != behaviorContext->m_typeToClassMap.end())
                {
                    AZ::BehaviorClass* behaviorClass = findClassIter->second;

                    if (behaviorClass->m_name == componentName)
                    {
                        for (auto reqBusName = behaviorClass->m_requestBuses.begin(); reqBusName != behaviorClass->m_requestBuses.end(); reqBusName++)
                        {
                            auto findBusIter = behaviorContext->m_ebuses.find(*reqBusName);
                            if (findBusIter != behaviorContext->m_ebuses.end())
                            {
                                AZ::BehaviorEBus* behaviorEbus = findBusIter->second;
                                if (behaviorEbus->m_virtualProperties.find(virtualPropertyName) != behaviorEbus->m_virtualProperties.end())
                                {
                                    outAddress = Maestro::SequenceComponentRequests::AnimatablePropertyAddress(component->GetId(), virtualPropertyName);
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
        AZ_Warning("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::GetAnimatablePropertyAddress - failed");
        return false;
    }

    bool ScriptedEntityTweenerSubtask::GetValueFromAny(EntityAnimatedValue& value, const AZStd::any& anyValue)
    {
        if (m_propertyTypeId == AZ::AzTypeInfo<float>::Uuid())
        {
            float floatVal;
            if (any_numeric_cast(&anyValue, floatVal))
            {
                value.SetValue(floatVal);
            }
            else
            {
                AZ_Warning("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::GetValueFromAny - numeric cast to float failed [%s]", m_animatableAddress.GetVirtualPropertyName().c_str());
                return false;
            }
        }
        else if (m_propertyTypeId == AZ::AzTypeInfo<AZ::Vector3>::Uuid() && anyValue.is<AZ::Vector3>())
        {
            value.SetValue(AZStd::any_cast<AZ::Vector3>(anyValue));
        }
        else if (m_propertyTypeId == AZ::AzTypeInfo<AZ::Color>::Uuid() && anyValue.is<AZ::Color>())
        {
            AZ::Color color = AZStd::any_cast<AZ::Color>(anyValue);
            value.SetValue(color.GetAsVector3());
        }
        else if (m_propertyTypeId == AZ::AzTypeInfo<AZ::Quaternion>::Uuid() && anyValue.is<AZ::Quaternion>())
        {
            value.SetValue(AZStd::any_cast<AZ::Quaternion>(anyValue));
        }
        else
        {
            AZ_Warning("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::GetValueFromAny - Virtual property type unsupported [%s]", m_animatableAddress.GetVirtualPropertyName().c_str());
            return false;
        }
        return true;
    }

    bool ScriptedEntityTweenerSubtask::GetValueAsAny(AZStd::any& anyValue, const EntityAnimatedValue& value)
    {
        if (m_propertyTypeId == AZ::AzTypeInfo<float>::Uuid())
        {
            float floatVal;
            value.GetValue(floatVal);
            anyValue = floatVal;
        }
        else if (m_propertyTypeId == AZ::AzTypeInfo<AZ::Vector3>::Uuid())
        {
            AZ::Vector3 vectorValue;
            value.GetValue(vectorValue);
            anyValue = vectorValue;
        }
        else if (m_propertyTypeId == AZ::AzTypeInfo<AZ::Color>::Uuid())
        {
            AZ::Vector3 vectorValue;
            value.GetValue(vectorValue);
            anyValue = AZ::Color::CreateFromVector3(vectorValue);
        }
        else if (m_propertyTypeId == AZ::AzTypeInfo<AZ::Quaternion>::Uuid())
        {
            AZ::Quaternion quatValue;
            value.GetValue(quatValue);
            anyValue = quatValue;
        }
        else
        {
            AZ_Warning("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::GetValueAsAny - Virtual property type unsupported [%s]", m_animatableAddress.GetVirtualPropertyName().c_str());
            return false;
        }
        return true;
    }

    bool ScriptedEntityTweenerSubtask::GetVirtualValue(EntityAnimatedValue& animatedValue)
    {
        if (m_propertyTypeId == AZ::AzTypeInfo<float>::Uuid())
        {
            Maestro::SequenceComponentRequests::AnimatedFloatValue toReturnFloat;
            Maestro::SequenceAgentComponentRequestBus::Event(*m_sequenceAgentBusId, &Maestro::SequenceAgentComponentRequestBus::Events::GetAnimatedPropertyValue, toReturnFloat, m_animatableAddress);
            animatedValue.SetValue(toReturnFloat.GetFloatValue());
        }
        else if (m_propertyTypeId == AZ::AzTypeInfo<AZ::Vector3>::Uuid() || m_propertyTypeId == AZ::AzTypeInfo<AZ::Color>::Uuid())
        {
            Maestro::SequenceComponentRequests::AnimatedVector3Value toReturnVector(AZ::Vector3::CreateZero());
            Maestro::SequenceAgentComponentRequestBus::Event(*m_sequenceAgentBusId, &Maestro::SequenceAgentComponentRequestBus::Events::GetAnimatedPropertyValue, toReturnVector, m_animatableAddress);
            animatedValue.SetValue(toReturnVector.GetVector3Value());
        }
        else if (m_propertyTypeId == AZ::AzTypeInfo<AZ::Quaternion>::Uuid())
        {
            Maestro::SequenceComponentRequests::AnimatedQuaternionValue toReturnQuat(AZ::Quaternion::CreateIdentity());
            Maestro::SequenceAgentComponentRequestBus::Event(*m_sequenceAgentBusId, &Maestro::SequenceAgentComponentRequestBus::Events::GetAnimatedPropertyValue, toReturnQuat, m_animatableAddress);
            animatedValue.SetValue(toReturnQuat.GetQuaternionValue());
        }
        else
        {
            AZ_Warning("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::GetVirtualValue - Trying to get unsupported parameter type for [%s]", m_animatableAddress.GetVirtualPropertyName().c_str());
            return false;
        }
        return true;
    }

    bool ScriptedEntityTweenerSubtask::SetVirtualValue(const EntityAnimatedValue& value)
    {
        bool toReturn = false;
        if (m_propertyTypeId == AZ::AzTypeInfo<float>::Uuid())
        {
            float floatVal;
            value.GetValue(floatVal);
            Maestro::SequenceComponentRequests::AnimatedFloatValue animatedValue;
            animatedValue.SetValue(floatVal);
            Maestro::SequenceAgentComponentRequestBus::EventResult(toReturn, *m_sequenceAgentBusId, &Maestro::SequenceAgentComponentRequestBus::Events::SetAnimatedPropertyValue, m_animatableAddress, animatedValue);
        }
        else if (m_propertyTypeId == AZ::AzTypeInfo<AZ::Vector3>::Uuid() || m_propertyTypeId == AZ::AzTypeInfo<AZ::Color>::Uuid())
        {
            AZ::Vector3 vectorValue;
            value.GetValue(vectorValue);
            Maestro::SequenceComponentRequests::AnimatedVector3Value animatedValue(AZ::Vector3::CreateZero());
            animatedValue.SetValue(vectorValue);
            Maestro::SequenceAgentComponentRequestBus::EventResult(toReturn, *m_sequenceAgentBusId, &Maestro::SequenceAgentComponentRequestBus::Events::SetAnimatedPropertyValue, m_animatableAddress, animatedValue);
        }
        else if (m_propertyTypeId == AZ::AzTypeInfo<AZ::Quaternion>::Uuid())
        {
            AZ::Quaternion quatValue;
            value.GetValue(quatValue);
            Maestro::SequenceComponentRequests::AnimatedQuaternionValue animatedValue(AZ::Quaternion::CreateIdentity());
            animatedValue.SetValue(quatValue);
            Maestro::SequenceAgentComponentRequestBus::EventResult(toReturn, *m_sequenceAgentBusId, &Maestro::SequenceAgentComponentRequestBus::Events::SetAnimatedPropertyValue, m_animatableAddress, animatedValue);
        }
        else
        {
            AZ_Warning("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::SetVirtualValue - Trying to set unsupported parameter type");
            return false;
        }
        return toReturn;
    }

    void ScriptedEntityTweenerSubtask::ClearCallbacks()
    {
        AZStd::vector<int> callbackIds = { m_animationProperties.m_onCompleteCallbackId, m_animationProperties.m_onLoopCallbackId, m_animationProperties.m_onUpdateCallbackId };

        for (const auto& callbackId : callbackIds)
        {
            if (callbackId != AnimationProperties::InvalidCallbackId)
            {
                ScriptedEntityTweenerNotificationsBus::Broadcast(&ScriptedEntityTweenerNotificationsBus::Events::RemoveCallback, callbackId);
            }
        }
    }

    bool ScriptedEntityTweenerSubtask::GetVirtualPropertyValue(AZStd::any& returnVal, const AnimationParameterAddressData& addressData)
    {
        if (m_animatableAddress.GetComponentId() == AZ::InvalidComponentId)
        {
            if (GetAnimatablePropertyAddress(m_animatableAddress, addressData.m_componentName, addressData.m_virtualPropertyName))
            {
                Maestro::SequenceAgentComponentRequestBus::EventResult(m_propertyTypeId, *m_sequenceAgentBusId, &Maestro::SequenceAgentComponentRequestBus::Events::GetAnimatedAddressTypeId, m_animatableAddress);
                if (!m_propertyTypeId.IsNull())
                {
                    EntityAnimatedValue tempVal;
                    if (GetVirtualValue(tempVal))
                    {
                        return GetValueAsAny(returnVal, tempVal);
                    }
                }
            }
        }
        else
        {
            EntityAnimatedValue tempVal;
            if (GetVirtualValue(tempVal))
            {
                return GetValueAsAny(returnVal, tempVal);
            }
        }
        AZ_Warning("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::GetVirtualPropertyValue - failed for [%s, %s]", addressData.m_componentName.c_str(), addressData.m_virtualPropertyName.c_str());
        return false;
    }
}