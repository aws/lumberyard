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

// include the required headers
#include "EMotionFXConfig.h"
#include <MCore/Source/Array.h>
#include "EventInfo.h"


namespace EMotionFX
{
    // forward declarations
    class MotionEventTrack;
    class AnimGraphNode;
    class AnimGraphInstance;


    /**
     * The anim graph event buffer class, which holds a collection of events which later have to be triggered.
     * This buffer is passed around the animgraph when processing it. The event buffer emitted by the root state machine of the animgraph will get triggered.
     */
    class EMFX_API AnimGraphEventBuffer
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphEventBuffer, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_EVENTBUFFERS);

    public:
        AnimGraphEventBuffer();
        ~AnimGraphEventBuffer();

        void Reserve(uint32 numEvents);
        void Resize(uint32 numEvents);
        void AddEvent(const EventInfo& newEvent);
        void SetEvent(uint32 index, const EventInfo& eventInfo);
        void Clear();

        MCORE_INLINE uint32 GetNumEvents() const                    { return mEvents.GetLength(); }
        MCORE_INLINE const EventInfo& GetEvent(uint32 index) const  { return mEvents[index]; }

        void TriggerEvents() const;
        void UpdateWeights(AnimGraphInstance* animGraphInstance);
        void UpdateEmitters(AnimGraphNode* emitterNode);
        void Log() const;

    private:
        MCore::Array<EventInfo>     mEvents;    /**< The collection of events inside this buffer. */
    };
}   // namespace EMotionFX
