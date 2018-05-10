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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/containers/array.h>

namespace EMotionFX
{
    class ActorInstance;
    class MotionInstance;

    namespace Integration
    {
        /**
         * EMotion FX System Request Bus
         * Used for making global requests to the EMotion FX system.
         */
        class SystemRequests
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            // Public functions
        };
        using SystemRequestBus = AZ::EBus<SystemRequests>;

        /**
         * EMotion FX System Notification Bus
         * Used for monitoring EMotion FX system-level events.
         */
        class SystemNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        };
        using SystemNotificationBus = AZ::EBus<SystemNotifications>;

        /**
         * Motion event descriptor.
         */
        struct MotionEvent
        {
        public:

            AZ_TYPE_INFO(MotionEvent, "{0C899DAC-6B19-4BDD-AD8C-8A11EF2A6729}");

            // Use fixed buffer for parameter string to avoid allocations for motion events, which can be sent at high frequency.
            enum { kMaxParameterStringLength = 64 - 1 };
            typedef AZStd::array<char, kMaxParameterStringLength + 1> ParameterStringStorage;

            ParameterStringStorage      m_parameterStorage;

            AZ::EntityId                m_entityId;         ///< EntityId associated with the originating actor
            const char*                 m_parameter;        ///< Optional string parameter
            EMotionFX::ActorInstance*   m_actorInstance;    ///< Pointer to the actor instance on which the event is playing
            EMotionFX::MotionInstance*  m_motionInstance;   ///< Pointer to the motion instance from which the event was fired
            float                       m_time;             ///< Time value of the event, in seconds
            AZ::u32                     m_eventType;        ///< Type Id of the event. m_eventTypeName stores the string representation.
            const char*                 m_eventTypeName;    ///< Event type in string form
            float                       m_globalWeight;     ///< Global weight of the event
            float                       m_localWeight;      ///< Local weight of the event
            bool                        m_isEventStart;     ///< Is this the start of a ranged event? This is always true for one-shot events.

            MotionEvent()
                : m_actorInstance(nullptr)
                , m_motionInstance(nullptr)
                , m_time(0.f)
                , m_eventType(0)
                , m_eventTypeName(nullptr)
                , m_globalWeight(0.f)
                , m_localWeight(0.f)
                , m_isEventStart(false)
            {
                m_parameterStorage[0] = '\0';
                m_parameter = m_parameterStorage.data();
            }

            MotionEvent(const MotionEvent& rhs)
                : m_actorInstance(rhs.m_actorInstance)
                , m_motionInstance(rhs.m_motionInstance)
                , m_time(rhs.m_time)
                , m_eventType(rhs.m_eventType)
                , m_eventTypeName(rhs.m_eventTypeName)
                , m_globalWeight(rhs.m_globalWeight)
                , m_localWeight(rhs.m_localWeight)
                , m_isEventStart(rhs.m_isEventStart)
            {
                m_parameterStorage = rhs.m_parameterStorage;
                m_parameter = m_parameterStorage.data();
            }

            void SetParameterString(const char* str, size_t length)
            {
                if (length)
                {
                    const size_t truncatedLength = AZStd::GetMin<size_t>(length, MotionEvent::kMaxParameterStringLength);
                    azstrncpy(m_parameterStorage.data(), m_parameterStorage.size(), str, truncatedLength);
                    m_parameterStorage[truncatedLength] = '\0';
                }
                else
                {
                    m_parameterStorage[0] = '\0';
                }
            }
        };

        /**
         * EMotion FX Actor Notification Bus
         * Used for monitoring EMotion FX per-actor events.
         */
        class ActorNotifications
            : public AZ::ComponentBus
        {
        public:

            /// Bus is accessed from job threads as well as simulation threads.
            /// This allows events to be safely queued from anywhere, and flushed from the main simulation thread.
            static const bool EnableEventQueue = true;
            typedef AZStd::recursive_mutex MutexType;

            /// A motion event has fired during playback.
            /// \param motionEvent information about the event.
            virtual void OnMotionEvent(MotionEvent /*motionEvent*/) {}

            /// A motion has looped.
            /// \param motionName name of the motion.
            virtual void OnMotionLoop(const char* /*motionName*/) {}

            /// A anim graph state is about to be entered.
            /// \param stateName name of the state.
            virtual void OnStateEntering(const char* /*stateName*/) {}
            /// A anim graph state has been entered.
            /// \param stateName name of the state.
            virtual void OnStateEntered(const char* /*stateName*/) {}
            /// A anim graph state is about to be exited.
            /// \param stateName name of the state.
            virtual void OnStateExiting(const char* /*stateName*/) {}
            /// A anim graph state has been exited.
            /// \param stateName name of the state.
            virtual void OnStateExited(const char* /*stateName*/) {}

            /// A transition between states is beginning.
            /// \param fromState name of source state.
            /// \param toState name of target state.
            virtual void OnStateTransitionStart(const char* /*fromState*/, const char* /*toState*/) {}
            /// A transition between states has completed.
            /// \param fromState name of source state.
            /// \param toState name of target state.
            virtual void OnStateTransitionEnd(const char* /*fromState*/, const char* /*toState*/) {}
        };
        using ActorNotificationBus = AZ::EBus<ActorNotifications>;
    } // namespace Integration
} // namespace EMotionFX
