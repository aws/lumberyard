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
#include <AzCore/std/string/string.h>

namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class MotionInstance;
    class AnimGraphNode;
    class MotionEvent;


    /**
     * Triggered event info.
     * This class holds the information for each event that gets triggered.
     */
    class EMFX_API EventInfo
    {
        MCORE_MEMORYOBJECTCATEGORY(EventInfo, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_EVENTS);

    public:
        float           mTimeValue;         /**< The time value of the event, in seconds. */
        uint32          mTypeID;            /**< The type ID of the event. */
        AZStd::string*  mParameters;        /**< The parameter string */
        AZStd::string*  mTypeString;        /**< The type string. */
        ActorInstance*  mActorInstance;     /**< The actor instance that triggered this event. */
        MotionInstance* mMotionInstance;    /**< The motion instance which triggered this event, can be nullptr. */
        AnimGraphNode*  mEmitter;           /**< The animgraph node which originally did emit this event. This parameter can be nullptr. */
        MotionEvent*    mEvent;             /**< The event itself. */
        float           mGlobalWeight;      /**< The global weight of the event. */
        float           mLocalWeight;       /**< The local weight of the event. */
        bool            mIsEventStart;      /**< Is this the start of a ranged event? Ticked events will always have this set to true. */

        EventInfo()
        {
            mTimeValue          = 0.0f;
            mTypeID             = MCORE_INVALIDINDEX32;
            mParameters         = nullptr;
            mTypeString         = nullptr;
            mActorInstance      = nullptr;
            mMotionInstance     = nullptr;
            mEmitter            = nullptr;
            mEvent              = nullptr;
            mLocalWeight        = 1.0f;
            mGlobalWeight       = 1.0f;
            mIsEventStart       = true;
        }
    };
}   // namespace EMotionFX

