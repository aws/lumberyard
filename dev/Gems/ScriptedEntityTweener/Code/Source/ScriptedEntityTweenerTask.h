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

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/list.h>
#include <ScriptedEntityTweener/ScriptedEntityTweenerEnums.h>
#include <Maestro/Bus/SequenceAgentComponentBus.h>
#include "ScriptedEntityTweenerSubtask.h"


namespace ScriptedEntityTweener
{
    //! One task per entity id, contains a collection of subtasks that are unique per virtual property.
    class ScriptedEntityTweenerTask
    {
    public: // member functions
        ScriptedEntityTweenerTask(AZ::EntityId id, bool createSequenceAgent = false);
        ~ScriptedEntityTweenerTask();

        void AddAnimation(const AnimationParameters& params, bool overwriteQueued = true);

        void Update(float deltaTime);

        bool GetIsActive();

        void SetPaused(const AnimationParameterAddressData& addressData, int timelineId, bool isPaused);

        void SetPlayDirectionReversed(const AnimationParameterAddressData& addressData, int timelineId, bool isPlayingBackward);

        void SetSpeed(const AnimationParameterAddressData& addressData, int timelineId, float speed);

        void SetInitialValue(const AnimationParameterAddressData& addressData, const AZ::Uuid& timelineId, const AZStd::any& initialValue);

        void GetVirtualPropertyValue(AZStd::any& returnVal, const AnimationParameterAddressData& addressData);

        bool operator<(const ScriptedEntityTweenerTask& other) const
        {
            return m_entityId < other.m_entityId;
        }
        bool operator>(const ScriptedEntityTweenerTask& other) const
        {
            return other.m_entityId < m_entityId;
        }
        bool operator==(const ScriptedEntityTweenerTask& other) const
        {
            return m_entityId == other.m_entityId;
        }

    private: // member functions
        AZ::EntityId m_entityId;
        Maestro::SequenceAgentEventBusId m_sequenceAgentBusId;

        //! Unique(per address data) active subtasks being updated
        AZStd::unordered_map<AnimationParameterAddressData, ScriptedEntityTweenerSubtask> m_subtasks;

        class QueuedSubtaskInfo
        {
        public:
            QueuedSubtaskInfo(AnimationParameters params, float delayTime)
                : m_params(params)
                , m_currentDelayTime(delayTime)
                , m_isPaused(false)
                , m_hasInitialValue(false)
            {
            }

            bool UpdateUntilReady(float deltaTime)
            {
                m_currentDelayTime -= deltaTime * m_params.m_animationProperties.m_playbackSpeedMultiplier;
                if (m_currentDelayTime <= .0f)
                {
                    m_params.m_animationProperties.m_timeToDelayAnim = .0f;
                    return true;
                }
                return false;
            }

            int GetTimelineId()
            {
                return m_params.m_animationProperties.m_timelineId;
            }

            const AZ::Uuid& GetAnimationId()
            {
                return m_params.m_animationProperties.m_animationId;
            }

            void SetPlaybackSpeed(float speed)
            {
                m_params.m_animationProperties.m_playbackSpeedMultiplier = speed;
            }

            AnimationParameters& GetParameters()
            {
                return m_params;
            }

            void SetPaused(bool isPaused)
            {
                m_isPaused = isPaused;
            }

            void SetInitialValue(const AZStd::any initialValue)
            {
                m_hasInitialValue = true;
                m_initialValue = initialValue;
            }

            bool HasInitialValue()
            {
                return m_hasInitialValue;
            }

            const AZStd::any& GetInitialValue()
            {
                return m_initialValue;
            }

        private:
            QueuedSubtaskInfo();

            float m_currentDelayTime;
            bool m_isPaused;
            
            bool m_hasInitialValue;
            AZStd::any m_initialValue;

            AnimationParameters m_params;
        };

        //! List of AnimationsParameters that need to be delayed before being added to m_subtasks, possibly overriding an animation.
        AZStd::list<QueuedSubtaskInfo> m_queuedSubtasks;

        AZStd::set<CallbackData> m_callbacks;

        bool IsTimelineIdValid(int timelineId)
        {
            return timelineId != AnimationProperties::InvalidTimelineId;
        }
    };
}