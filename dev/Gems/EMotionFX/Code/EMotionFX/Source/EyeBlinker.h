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

// include core system
#include "EMotionFXConfig.h"
#include "BaseObject.h"
#include "MorphSetupInstance.h"
#include <MCore/Source/Array.h>


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;

    /**
     * The procedural eye blinker class.
     * This can be used to automate eyeblinks by modifying the weight value of a morph target.
     */
    class EMFX_API EyeBlinker
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        static EyeBlinker* Create(ActorInstance* actorInstance);

        bool AddMorphTarget(const char* name);
        void ClearMorphTargets();
        void RemoveMorphTarget(const char* name);
        void Update(float timeDelta);

        void SetMinWeight(float weight)                     { mMinWeight = weight; }
        void SetMaxWeight(float weight)                     { mMaxWeight = weight; }
        void SetMinOpenedTime(float seconds)                { mMinOpenedTime = seconds; }
        void SetMaxOpenedTime(float seconds)                { mMaxOpenedTime = seconds; }
        void SetMinClosedTime(float seconds)                { mMinClosedTime = seconds; }
        void SetMaxClosedTime(float seconds)                { mMaxClosedTime = seconds; }
        void SetMinBlinkTime(float seconds)                 { mMinBlinkTime = seconds; }
        void SetMaxBlinkTime(float seconds)                 { mMaxBlinkTime = seconds; }

        float GetMinWeight() const                          { return mMinWeight; }
        float GetMaxWeight() const                          { return mMaxWeight; }
        float GetMinOpenedTime() const                      { return mMinOpenedTime; }
        float GetMaxOpenedTime() const                      { return mMaxOpenedTime; }
        float GetMinClosedTime() const                      { return mMinClosedTime; }
        float GetMaxClosedTime() const                      { return mMaxClosedTime; }
        float GetMinBlinkTime() const                       { return mMinBlinkTime; }
        float GetMaxBlinkTime() const                       { return mMaxBlinkTime; }

    private:
        struct EMFX_API MorphInfo
        {
            MorphSetupInstance::MorphTarget*    mMorphTarget;
            float                               mBlendTime;
            float                               mTimeInBlend;
            float                               mIdleTime;
            bool                                mIsBlending;
            bool                                mIsOpened;
        };

        MCore::Array<MorphInfo> mMorphTargets;
        ActorInstance*          mActorInstance;
        float                   mMinWeight;
        float                   mMaxWeight;
        float                   mMaxOpenedTime;
        float                   mMinOpenedTime;
        float                   mMaxClosedTime;
        float                   mMinClosedTime;
        float                   mMinBlinkTime;
        float                   mMaxBlinkTime;
        float                   mCurMaxOpenedTime;
        float                   mCurMaxClosedTime;
        float                   mCurMaxBlinkTime;

        EyeBlinker(ActorInstance* actorInstance);
        ~EyeBlinker();
    };
} // namespace EMotionFX
