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

#include "AnimGraphEventBuffer.h"
#include "EventManager.h"
#include "AnimGraphInstance.h"


namespace EMotionFX
{
    AnimGraphEventBuffer::AnimGraphEventBuffer()
    {
        mEvents.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_EVENTBUFFERS);
    }


    AnimGraphEventBuffer::~AnimGraphEventBuffer()
    {
    }


    // set the emitter pointers
    void AnimGraphEventBuffer::UpdateEmitters(AnimGraphNode* emitterNode)
    {
        const uint32 numEvents = mEvents.GetLength();
        for (uint32 i = 0; i < numEvents; ++i)
        {
            mEvents[i].mEmitter = emitterNode;
        }
    }


    // update the weights
    void AnimGraphEventBuffer::UpdateWeights(AnimGraphInstance* animGraphInstance)
    {
        const uint32 numEvents = mEvents.GetLength();
        for (uint32 i = 0; i < numEvents; ++i)
        {
            EventInfo&              curEvent            = mEvents[i];
            AnimGraphNodeData*     emitterUniqueData   = curEvent.mEmitter->FindUniqueNodeData(animGraphInstance);
            curEvent.mGlobalWeight  = emitterUniqueData->GetGlobalWeight();
            curEvent.mLocalWeight   = emitterUniqueData->GetLocalWeight();
        }
    }


    // log details of all events
    void AnimGraphEventBuffer::Log() const
    {
        const uint32 numEvents = mEvents.GetLength();
        for (uint32 i = 0; i < numEvents; ++i)
        {
            MCore::LogInfo("Event #%d: (time=%f) (type=%s) (param=%s) (emitter=%s) (locWeight=%.4f  globWeight=%.4f)", i,
                mEvents[i].mTimeValue,
                mEvents[i].mTypeString->c_str(),
                mEvents[i].mParameters->c_str(),
                mEvents[i].mEmitter->GetName(),
                mEvents[i].mLocalWeight,
                mEvents[i].mGlobalWeight);
        }
    }


    // trigger the events
    void AnimGraphEventBuffer::TriggerEvents() const
    {
        const uint32 numEvents = mEvents.GetLength();
        for (uint32 i = 0; i < numEvents; ++i)
        {
            GetEventManager().OnEvent(mEvents[i]);
        }
    }


    void AnimGraphEventBuffer::Reserve(uint32 numEvents)
    {
        mEvents.Reserve(numEvents);
    }


    void AnimGraphEventBuffer::Resize(uint32 numEvents)
    {
        mEvents.Resize(numEvents);
    }


    void AnimGraphEventBuffer::AddEvent(const EventInfo& newEvent)
    {
        mEvents.Add(newEvent);
    }


    void AnimGraphEventBuffer::Clear()
    {
        mEvents.Clear(false);
    }


    void AnimGraphEventBuffer::SetEvent(uint32 index, const EventInfo& eventInfo)
    {
        mEvents[index] = eventInfo;
    }
}   // namespace EMotionFX
