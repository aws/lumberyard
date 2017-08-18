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
#include "SequenceComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <LmbrCentral/Cinematics/SequenceAgentComponentBus.h>

namespace LmbrCentral
{
    /**
    * Reflect the SequenceComponentNotificationBus to the Behavior Context
    */
    class BehaviorSequenceComponentNotificationBusHandler : public SequenceComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorSequenceComponentNotificationBusHandler, "{3EC0FB38-4649-41E7-8409-0D351FE99A64}", AZ::SystemAllocator
            , OnStart
            , OnStop
            , OnPause
            , OnResume
            , OnAbort
            , OnUpdate
            , OnTrackEventTriggered
            );

        void OnStart(float startTime) override
        {
            Call(FN_OnStart, startTime);
        }
        void OnStop(float stopTime) override
        {
            Call(FN_OnStop, stopTime);
        }
        void OnPause() override
        {
            Call(FN_OnPause);
        }
        void OnResume() override
        {
            Call(FN_OnResume);
        }
        void OnAbort(float abortTime) override
        {
            Call(FN_OnAbort, abortTime);
        }
        void OnUpdate(float updateTime) override
        {
            Call(FN_OnUpdate, updateTime);
        }
        void OnTrackEventTriggered(const char* eventName, const char* eventValue) override
        {
            Call(FN_OnTrackEventTriggered, eventName, eventValue);
        }
    };

    SequenceComponent::SequenceComponent()
        : m_sequence(nullptr)
    {
    }

    /*static*/ void SequenceComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        
        if (serializeContext)
        {
            serializeContext->Class<SequenceComponent>()
                ->Version(1);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<SequenceComponentRequestBus>("SequenceComponentRequestBus")
                ->Event("Play", &SequenceComponentRequestBus::Events::Play)
                ->Event("PlayBetweenTimes", &SequenceComponentRequestBus::Events::PlayBetweenTimes)
                ->Event("Stop", &SequenceComponentRequestBus::Events::Stop)
                ->Event("Pause", &SequenceComponentRequestBus::Events::Pause)
                ->Event("Resume", &SequenceComponentRequestBus::Events::Resume)
                ->Event("SetPlaySpeed", &SequenceComponentRequestBus::Events::SetPlaySpeed)
                ->Event("JumpToTime", &SequenceComponentRequestBus::Events::JumpToTime)
                ->Event("JumpToBeginning", &SequenceComponentRequestBus::Events::JumpToBeginning)
                ->Event("JumpToEnd", &SequenceComponentRequestBus::Events::JumpToEnd)
                ->Event("GetCurrentPlayTime", &SequenceComponentRequestBus::Events::GetCurrentPlayTime)
                ->Event("GetPlaySpeed", &SequenceComponentRequestBus::Events::GetPlaySpeed)
                ;
            behaviorContext->Class<SequenceComponent>()->RequestBus("SequenceComponentRequestBus");

            behaviorContext->EBus<SequenceComponentNotificationBus>("SequenceComponentNotificationBus")->
                Handler<BehaviorSequenceComponentNotificationBusHandler>();
        }
    }

    void SequenceComponent::Init()
    {
        // TODO: we create the IAnimSequence object and connect to it here in the Editor. How is this done in pure Game mode?
    }

    void SequenceComponent::Activate()
    {
        LmbrCentral::SequenceComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void SequenceComponent::Deactivate()
    {
        LmbrCentral::SequenceComponentRequestBus::Handler::BusDisconnect();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    bool SequenceComponent::SetAnimatedPropertyValue(const AZ::EntityId& animatedEntityId, const AnimatablePropertyAddress& animatableAddress, const AnimatedValue& value)
    {
        const LmbrCentral::SequenceAgentEventBusId busId(GetEntityId(), animatedEntityId);
        bool changed = false;
        
        EBUS_EVENT_ID_RESULT(changed, busId, LmbrCentral::SequenceAgentComponentRequestBus, SetAnimatedPropertyValue, animatableAddress, value);
        
        return changed;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void SequenceComponent::GetAnimatedPropertyValue(AnimatedValue& returnValue, const AZ::EntityId& animatedEntityId, const AnimatablePropertyAddress& animatableAddress)
    {
        const LmbrCentral::SequenceAgentEventBusId busId(GetEntityId(), animatedEntityId);
        float retVal = .0f;

        EBUS_EVENT_ID(busId, LmbrCentral::SequenceAgentComponentRequestBus, GetAnimatedPropertyValue, returnValue, animatableAddress);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Uuid SequenceComponent::GetAnimatedAddressTypeId(const AZ::EntityId& animatedEntityId, const LmbrCentral::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress)
    {
        AZ::Uuid typeId = AZ::Uuid::CreateNull();
        const LmbrCentral::SequenceAgentEventBusId ebusId(GetEntityId(), animatedEntityId);

        LmbrCentral::SequenceAgentComponentRequestBus::EventResult(typeId, ebusId, &LmbrCentral::SequenceAgentComponentRequestBus::Events::GetAnimatedAddressTypeId, animatableAddress);

        return typeId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    IAnimSequence* SequenceComponent::GetSequence(bool forceRefresh) const
    {
        if (!m_sequence || forceRefresh)
        {
            m_sequence = gEnv->pMovieSystem->FindSequence(GetEntityId());
        }
        return m_sequence;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void SequenceComponent::Play()
    {
        IAnimSequence* sequence = GetSequence();
        if (sequence)
        {
            gEnv->pMovieSystem->PlaySequence(sequence, /*parentSeq =*/ nullptr, /*bResetFX =*/ true,/*bTrackedSequence =*/ false, /*float startTime =*/ -FLT_MAX, /*float endTime =*/ -FLT_MAX);
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void SequenceComponent::PlayBetweenTimes(float startTime, float endTime)
    {
        IAnimSequence* sequence = GetSequence();
        if (sequence)
        {
            gEnv->pMovieSystem->PlaySequence(sequence, /*parentSeq =*/ nullptr, /*bResetFX =*/ true,/*bTrackedSequence =*/ false, startTime, endTime);
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void SequenceComponent::Stop()
    {
        IAnimSequence* sequence = GetSequence();
        if (sequence)
        {
            gEnv->pMovieSystem->StopSequence(sequence);
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void SequenceComponent::Pause()
    {
        IAnimSequence* sequence = GetSequence();
        if (sequence)
        {
            sequence->Pause();
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void SequenceComponent::Resume()
    {
        IAnimSequence* sequence = GetSequence();
        if (sequence)
        {
            sequence->Resume();
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void SequenceComponent::SetPlaySpeed(float newSpeed)
    {
        IAnimSequence* sequence = GetSequence();
        if (sequence)
        {
            gEnv->pMovieSystem->SetPlayingSpeed(sequence, newSpeed);
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void SequenceComponent::JumpToTime(float newTime)
    {
        IAnimSequence* sequence = GetSequence();
        if (sequence)
        {
            newTime = clamp_tpl(newTime, sequence->GetTimeRange().start, sequence->GetTimeRange().end);
            gEnv->pMovieSystem->SetPlayingTime(sequence, newTime);
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void SequenceComponent::JumpToEnd()
    {
        IAnimSequence* sequence = GetSequence();
        if (sequence)
        {
            gEnv->pMovieSystem->SetPlayingTime(sequence, sequence->GetTimeRange().end);
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void SequenceComponent::JumpToBeginning()
    {
        IAnimSequence* sequence = GetSequence();
        if (sequence)
        {
            gEnv->pMovieSystem->SetPlayingTime(sequence, sequence->GetTimeRange().start);
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    float SequenceComponent::GetCurrentPlayTime()
    {
        IAnimSequence* sequence = GetSequence();
        if (sequence)
        {
            return gEnv->pMovieSystem->GetPlayingTime(sequence);
        }
        return .0f;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    float SequenceComponent::GetPlaySpeed()
    {
        IAnimSequence* sequence = GetSequence();
        if (sequence)
        {
            return gEnv->pMovieSystem->GetPlayingSpeed(sequence);
        }
        return 1.0f;
    }
}// namespace LmbrCentral
