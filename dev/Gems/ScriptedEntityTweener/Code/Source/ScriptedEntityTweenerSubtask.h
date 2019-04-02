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

#include <AzCore/std/any.h>
#include <AzCore/std/containers/set.h>
#include <Maestro/Bus/SequenceAgentComponentBus.h>
#include <ScriptedEntityTweener/ScriptedEntityTweenerEnums.h>

namespace ScriptedEntityTweener
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Each subtask performs operations on a virtual address.
    class ScriptedEntityTweenerSubtask
    {
    public:
        ScriptedEntityTweenerSubtask(const Maestro::SequenceAgentEventBusId* agentBusId)
            : m_sequenceAgentBusId(agentBusId)
        {
            AZ_Assert(m_sequenceAgentBusId, "m_sequenceAgentBusId should never be null");
            Reset();
        }

        bool Initialize(const AnimationParameterAddressData& addressData, const AZStd::any& targetValue, const AnimationProperties& properties);
        
        //! Update virtual property based on animation properties, fill out callbacks vector with any callback information that needs to be called this update.
        void Update(float deltaTime, AZStd::set<CallbackData>& callbacks);

        //! True if active and animating a virtual property
        bool IsActive() { return m_isActive; }

        //! Clear any callbacks stored in lua for this subtask
        void ClearCallbacks();

        void SetPaused(int timelineId, bool isPaused)
        {
            if (m_animationProperties.m_timelineId == timelineId)
            {
                m_isPaused = isPaused;
            }
        }
        void SetPlayDirectionReversed(int timelineId, bool isPlayingBackward)
        {
            if (m_animationProperties.m_timelineId == timelineId)
            {
                m_animationProperties.m_isPlayingBackward = isPlayingBackward;
            }
        }
        void SetSpeed(int timelineId, float speed)
        {
            if (m_animationProperties.m_timelineId == timelineId)
            {
                m_animationProperties.m_playbackSpeedMultiplier = speed;
            }
        }

        void SetInitialValue(const AZ::Uuid& animationId, const AZStd::any& initialValue)
        {
            if (m_animationProperties.m_animationId == animationId)
            {
                GetValueFromAny(m_valueInitial, initialValue);
            }
        }

        const int GetTimelineId() const
        {
            return m_animationProperties.m_timelineId;
        }

        bool GetVirtualPropertyValue(AZStd::any& returnVal, const AnimationParameterAddressData& addressData);

    private:
        //! EntityAnimatedValue exists to simplify logic relating to using Maestro's AnimatedValues
        struct EntityAnimatedValue
        {
        public:
            EntityAnimatedValue()
                : floatVal(AnimationProperties::UninitializedParamFloat), vectorVal(AZ::Vector3::CreateZero()), quatVal(AZ::Quaternion::CreateIdentity())
            {

            }
            void GetValue(float& outVal) const
            {
                outVal = floatVal;
            }
            void GetValue(AZ::Vector3& outVal) const
            {
                outVal = vectorVal;
            }
            void GetValue(AZ::Quaternion& outVal) const
            {
                outVal = quatVal;
            }

            void SetValue(float val)
            {
                floatVal = val;
            }
            void SetValue(AZ::Vector3 val)
            {
                vectorVal = val;
            }
            void SetValue(AZ::Quaternion val)
            {
                quatVal = val;
            }
        private:
            float floatVal;
            AZ::Vector3 vectorVal;
            AZ::Quaternion quatVal;
        };

        const Maestro::SequenceAgentEventBusId* const m_sequenceAgentBusId;

        AnimationProperties m_animationProperties;

        //! Address to specific parameter/component/bus and virtual property being modified.
        Maestro::SequenceComponentRequests::AnimatablePropertyAddress m_animatableAddress;

        //! RTTI type info of the AnimatablePropertyAddress
        AZ::Uuid m_propertyTypeId;

        bool m_isActive;
        bool m_isPaused;
        float m_timeSinceStart;
        int m_timesPlayed;

        EntityAnimatedValue m_valueInitial;
        EntityAnimatedValue m_valueTarget;


        void Reset()
        {
            m_isActive = false;
            m_isPaused = false;
            m_timeSinceStart = 0.0f;

            m_valueInitial = EntityAnimatedValue();
            m_valueTarget = EntityAnimatedValue();

            m_timesPlayed = 0;

            m_animationProperties.Reset();
            m_animatableAddress = Maestro::SequenceComponentRequests::AnimatablePropertyAddress();
            m_propertyTypeId = AZ::Uuid::CreateNull();
        }

        bool GetAnimatablePropertyAddress(Maestro::SequenceComponentRequests::AnimatablePropertyAddress& outAddress, const AZStd::string& componentName, const AZStd::string& virtualPropertyName);

        //! Set value from an AZStd::any object
        bool GetValueFromAny(EntityAnimatedValue& value, const AZStd::any& anyValue);

        //! Set value to an AZStd::any object
        bool GetValueAsAny(AZStd::any& anyValue, const EntityAnimatedValue& value);

        //! Get value from the virtual address's value
        bool GetVirtualValue(EntityAnimatedValue& animatedValue);

        //! Set the virtual address's value
        bool SetVirtualValue(const EntityAnimatedValue& value);
    };

}