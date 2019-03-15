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

// include the required headers
#include "MotionInstance.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "PlayBackInfo.h"
#include "Pose.h"
#include "EventManager.h"
#include "EventHandler.h"
#include "Node.h"
#include "TransformData.h"
#include "MotionEventTable.h"
#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/IDGenerator.h>
#include <MCore/Source/Compare.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionInstance, MotionAllocator, 0)


    // constructor
    MotionInstance::MotionInstance(Motion* motion, ActorInstance* actorInstance, uint32 startNodeIndex)
        : BaseObject()
    {
        MCORE_ASSERT(actorInstance);

        mCurrentTime            = 0.0f;
        mLastCurTime            = 0.0f;
        mPassedTime             = 0.0f;
        mClipStartTime          = 0.0f;
        mClipEndTime            = motion ? motion->GetMaxTime() : 0.0f;
        mPlaySpeed              = 1.0f;
        mTargetWeight           = 1.0f;
        mBlendInTime            = 0.0f;
        mFreezeAtTime           = -1.0f;
        mMotion                 = motion;
        mPlayMode               = PLAYMODE_FORWARD;
        mBlendMode              = BLENDMODE_OVERWRITE;
        mFadeTime               = 0.3f;
        mLastLoops              = 0;
        mMaxLoops               = EMFX_LOOPFOREVER;
        mCurLoops               = 0;
        mPriorityLevel          = 0;
        mWeight                 = 0.0f;
        mWeightDelta            = 0.0f;
        mEventWeightThreshold   = 0.0f;
        mTotalPlayTime          = 0.0f;
        mMaxPlayTime            = 0.0f;// disable the maximum play time by setting it to 0 or a negative value
        mTimeDifToEnd           = 0.0f;
        mActorInstance          = actorInstance;
        mStartNodeIndex         = startNodeIndex;
        mBoolFlags              = 0;// disable all boolean settings on default
        mCustomData             = nullptr;
        mID                     = MCore::GetIDGenerator().GenerateID();
        mCachedKeys             = nullptr;
        mMotionGroup            = nullptr;
        mSubPool                = nullptr;

        // enable certain boolean settings
        SetDeleteOnZeroWeight(true);
        SetCanOverwrite(true);
        SetMotionEventsEnabled(true);
        EnableFlag(BOOL_ISACTIVE);
        EnableFlag(BOOL_ISFIRSTREPOSUPDATE);
        EnableFlag(BOOL_BLENDBEFOREENDED);
        EnableFlag(BOOL_USEMOTIONEXTRACTION);

#if defined(EMFX_DEVELOPMENT_BUILD)
        if (actorInstance->GetIsOwnedByRuntime())
        {
            EnableFlag(BOOL_ISOWNEDBYRUNTIME);
        }
#endif // EMFX_DEVELOPMENT_BUILD

        // reset the cache hit counters
        ResetCacheHitCounters();

        // set the memory categories
        mMotionLinks.SetMemoryCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONLINKS);
        mNodeWeights.SetMemoryCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONINSTANCES);
        m_eventHandlersByEventType.resize(EVENT_TYPE_MOTION_INSTANCE_LAST_EVENT - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT + 1);

        GetEventManager().OnCreateMotionInstance(this);
    }


    // destructor
    MotionInstance::~MotionInstance()
    {
        // trigger the OnDeleteMotionInstance event
        GetEventManager().OnDeleteMotionInstance(this);

        // remove all event handlers
        RemoveAllEventHandlers();

        // release the allocated cached key memory
        MCore::Free(mCachedKeys);

        mMotionLinks.Clear();
        mNodeWeights.Clear();

        if (mMotionGroup)
        {
            mMotionGroup->Destroy();
        }

        mSubPool = nullptr;
    }


    // create
    MotionInstance* MotionInstance::Create(Motion* motion, ActorInstance* actorInstance, uint32 startNodeIndex)
    {
        return aznew MotionInstance(motion, actorInstance, startNodeIndex);
    }


    // create the motion instance in place
    MotionInstance* MotionInstance::Create(void* memLocation, Motion* motion, ActorInstance* actorInstance, uint32 startNodeIndex)
    {
        return new(memLocation) MotionInstance(motion, actorInstance, startNodeIndex);
    }


    // allocate the motion group if needed
    void MotionInstance::InitMotionGroup()
    {
        if (mMotionGroup)
        {
            return;
        }

        mMotionGroup = MotionGroup::Create(this);
    }


    // remove the motion group
    void MotionInstance::RemoveMotionGroup()
    {
        if (mMotionGroup)
        {
            mMotionGroup->Destroy();
            mMotionGroup = nullptr;
        }
    }


    // init the instance from a given playback info
    void MotionInstance::InitFromPlayBackInfo(const PlayBackInfo& info, bool resetCurrentPlaytime)
    {
        SetFadeTime             (info.mBlendOutTime);
        SetMixMode              (info.mMix);
        SetMaxLoops             (info.mNumLoops);
        SetBlendMode            (info.mBlendMode);
        SetPlaySpeed            (info.mPlaySpeed);
        SetWeight               (info.mTargetWeight, info.mBlendInTime);
        SetPriorityLevel        (info.mPriorityLevel);
        SetPlayMode             (info.mPlayMode);
        SetRetargetingEnabled   (info.mRetarget);
        SetMotionExtractionEnabled(info.mMotionExtractionEnabled);
        SetFreezeAtLastFrame    (info.mFreezeAtLastFrame);
        SetMotionEventsEnabled  (info.mEnableMotionEvents);
        SetMaxPlayTime          (info.mMaxPlayTime);
        SetEventWeightThreshold (info.mEventWeightThreshold);
        SetBlendOutBeforeEnded  (info.mBlendOutBeforeEnded);
        SetCanOverwrite         (info.mCanOverwrite);
        SetDeleteOnZeroWeight   (info.mDeleteOnZeroWeight);
        SetMirrorMotion         (info.mMirrorMotion);
        SetClipStartTime        (info.mClipStartTime);
        SetClipEndTime          (info.mClipEndTime);
        SetFreezeAtTime         (info.mFreezeAtTime);
        SetIsInPlace            (info.mInPlace);

        if (info.mClipEndTime <= MCore::Math::epsilon && mMotion)
        {
            SetClipEndTime(mMotion->GetMaxTime());
        }

        // if we're playing backwards
        if (resetCurrentPlaytime)
        {
            if (info.mPlayMode == PLAYMODE_BACKWARD)
            {
                if (mMotion)
                {
                    mLastCurTime = mClipEndTime;//mMotion->GetMaxTime();
                }
                else
                {
                    mLastCurTime = mClipStartTime;//0.0f;
                }
                mCurrentTime = mLastCurTime;
            }
            else
            {
                mCurrentTime = mClipStartTime;
            }

            // the time from the current play time till the end of the motion
            if (mMotion)
            {
                mTimeDifToEnd = mClipEndTime - mClipStartTime;
            }
            else
            {
                mTimeDifToEnd = 0.0f;
            }
        }
    }


    // initialize the motion instance for sampling
    void MotionInstance::InitForSampling()
    {
        // resize the number of motion links array to the number of nodes in the actor
        const uint32 numNodes = mActorInstance->GetActor()->GetNumNodes();

        // init the cached keys
        if (!mCachedKeys)
        {
            mCachedKeys = (uint32*)MCore::Allocate(sizeof(uint32) * numNodes, EMFX_MEMCATEGORY_MOTIONS_MOTIONINSTANCES);
            for (uint32 i = 0; i < numNodes; ++i)
            {
                mCachedKeys[i] = MCORE_INVALIDINDEX32;
            }
        }
        else
        {
            mCachedKeys = (uint32*)MCore::Realloc(mCachedKeys, sizeof(uint32) * numNodes, EMFX_MEMCATEGORY_MOTIONS_MOTIONINSTANCES);
            for (uint32 i = 0; i < numNodes; ++i)
            {
                mCachedKeys[i] = MCORE_INVALIDINDEX32;
            }
        }

        // create the motion links
        mMotionLinks.Resize(numNodes);
        mMotion->CreateMotionLinks(mActorInstance->GetActor(), this);
    }


    // change the current time
    void MotionInstance::SetCurrentTime(float time, bool resetLastAndPastTime)
    {
        mCurrentTime = time;
        if (resetLastAndPastTime)
        {
            mLastCurTime    = time;
            mPassedTime     = 0.0f;
        }
    }


    // reset the playback times and current position
    void MotionInstance::ResetTimes()
    {
        if (mPlayMode == PLAYMODE_FORWARD)
        {
            mCurrentTime = mClipStartTime;
        }
        else
        {
            mCurrentTime = mClipEndTime;
        }

        mPassedTime     = 0.0f;
        mLastCurTime    = mCurrentTime;
        mTotalPlayTime  = 0.0f;
        mCurLoops       = 0;
        mLastLoops      = 0;
        mTimeDifToEnd   = mClipEndTime - mClipStartTime;

        SetIsFrozen(false);
        EnableFlag(MotionInstance::BOOL_ISFIRSTREPOSUPDATE);
    }



    // update the time values based on the motion playback settings
    void MotionInstance::CalcNewTimeAfterUpdate(float timePassed, float* outNewTime)
    {
        // init defaults
        *outNewTime         = mCurrentTime;

        // if we're dealing with a grouped motion
        if (!mMotion)
        {
            return;
        }

        // if we are playing forward
        if (mPlayMode == PLAYMODE_FORWARD)
        {
            float currentTime = mCurrentTime + timePassed * mPlaySpeed;

            float maxTime = mClipEndTime;////mMotion->GetMaxTime()
            if (maxTime > mMotion->GetMaxTime())
            {
                maxTime = mMotion->GetMaxTime();
                mClipEndTime = maxTime;
            }

            // check and handle the bounds of the current playtime, to make it loop etc
            if (mMaxLoops == EMFX_LOOPFOREVER) // if it's looping forever
            {
                if (currentTime >= maxTime)
                {
                    if (maxTime > 0.0f)
                    {
                        currentTime = mClipStartTime + MCore::Math::SafeFMod(currentTime - mClipStartTime, maxTime - mClipStartTime);
                    }
                    else
                    {
                        currentTime = 0.0f;
                    }
                }

                if (currentTime < 0.0f)
                {
                    while (currentTime < 0.0f)
                    {
                        currentTime = maxTime + currentTime;
                    }
                }
            }
            else
            {
                // if we passed the end of the motion, keep it there
                if (currentTime >= maxTime)
                {
                    if ((mCurLoops + 1) >= mMaxLoops)
                    {
                        if (!GetFreezeAtLastFrame())
                        {
                            if (maxTime > 0.0f)
                            {
                                currentTime = mClipStartTime + MCore::Math::SafeFMod(currentTime - mClipStartTime, maxTime - mClipStartTime);
                            }
                            else
                            {
                                currentTime = 0.0f;
                            }
                        }
                        else
                        {
                            currentTime = maxTime;
                        }
                    }
                    else
                    {
                        //DisableFlag( BOOL_ISFROZENATLASTFRAME );

                        if (maxTime > 0.0f)
                        {
                            currentTime = mClipStartTime + MCore::Math::SafeFMod(currentTime - mClipStartTime, maxTime - mClipStartTime);
                        }
                        else
                        {
                            currentTime = 0.0f;
                        }
                    }
                }
            }

            // if we use the freeze at time setting
            if (mFreezeAtTime >= 0.0f)
            {
                // if we past that freeze time, keep it there
                if (currentTime > mFreezeAtTime)
                {
                    currentTime = mFreezeAtTime;
                }
            }

            // if the current time is before the start of the motion, set it to the start of the motion
            if (currentTime < mClipStartTime)
            {
                currentTime = mClipStartTime;
            }

            // set updated, validated values again
            *outNewTime = currentTime;
        }
        else // backward playback mode
        {
            float currentTime   = mCurrentTime - (timePassed * mPlaySpeed);
            float maxTime       = mClipEndTime;
            if (maxTime > mMotion->GetMaxTime())
            {
                maxTime = mMotion->GetMaxTime();
                mClipEndTime = maxTime;
            }

            // check and handle the bounds of the current playtime, to make it loop etc
            if (mMaxLoops == EMFX_LOOPFOREVER) // if it's looping forever
            {
                if (currentTime < mClipStartTime)
                {
                    currentTime = maxTime + (currentTime - mClipStartTime);
                }

                if (currentTime > maxTime)
                {
                    while (currentTime > maxTime)
                    {
                        currentTime = 2.0f * maxTime - currentTime;
                    }
                }
            }
            else
            {
                // if we passed the end of the motion
                if (currentTime <= mClipStartTime)
                {
                    if ((mCurLoops + 1) >= mMaxLoops)
                    {
                        if (!GetFreezeAtLastFrame())
                        {
                            currentTime = maxTime + (currentTime - mClipStartTime);
                        }
                        else
                        {
                            currentTime = mClipStartTime;
                        }
                    }
                    else
                    {
                        //DisableFlag( BOOL_ISFROZENATLASTFRAME );
                        currentTime = maxTime + currentTime;
                    }
                }
            }

            // if we use the freeze at time setting
            if (mFreezeAtTime >= 0.0f)
            {
                // if we past that freeze time, keep it there
                if (currentTime < mFreezeAtTime)
                {
                    currentTime = mFreezeAtTime;
                }
            }

            // if the current time is before the start of the motion, set it to the start of the motion
            if (currentTime < mClipStartTime)
            {
                currentTime = mClipStartTime;
            }

            // copy over the current time
            *outNewTime = currentTime;
        }
    }



    // update the time values based on the motion playback settings
    void MotionInstance::UpdateTime(float timePassed)
    {
        // if we're dealing with a grouped motion
        if (!mMotion)
        {
            mPassedTime      = timePassed * mPlaySpeed;
            mTotalPlayTime  += MCore::Math::Abs(timePassed);
            return;
        }

        bool hasLooped = false;

        // if we are playing forward
        if (mPlayMode == PLAYMODE_FORWARD)
        {
            mPassedTime          = timePassed * mPlaySpeed;
            float currentTime    = mCurrentTime + mPassedTime;

            float maxTime        = mClipEndTime;////mMotion->GetMaxTime()
            if (maxTime > mMotion->GetMaxTime())
            {
                maxTime = mMotion->GetMaxTime();
                mClipEndTime = maxTime;
            }

            mTotalPlayTime += MCore::Math::Abs(timePassed);

            // check and handle the bounds of the current playtime, to make it loop etc
            if (mMaxLoops == EMFX_LOOPFOREVER) // if it's looping forever
            {
                if (currentTime >= maxTime)
                {
                    mCurLoops++;
                    hasLooped = true;

                    if (maxTime > 0.0f)
                    {
                        currentTime = mClipStartTime + MCore::Math::SafeFMod(currentTime - mClipStartTime, maxTime - mClipStartTime);
                    }
                    else
                    {
                        currentTime = 0.0f;
                    }
                }

                if (currentTime < 0.0f)
                {
                    while (currentTime < 0.0f)
                    {
                        currentTime = maxTime + currentTime;
                    }
                }
            }
            else
            {
                // if we passed the end of the motion, keep it there
                if (currentTime >= maxTime)
                {
                    mCurLoops++;
                    hasLooped = true;

                    if (mCurLoops >= mMaxLoops)
                    {
                        //if (mActorInstance->GetActor()->GetRepositioningNodeIndex() == MCORE_INVALIDINDEX32)
                        //currentTime = maxTime;
                        //else
                        {
                            if (!GetFreezeAtLastFrame())
                            {
                                if (maxTime > 0.0f)
                                {
                                    currentTime = mClipStartTime + MCore::Math::SafeFMod(currentTime - mClipStartTime, maxTime - mClipStartTime);
                                }
                                else
                                {
                                    currentTime = 0.0f;
                                }
                            }
                            else
                            {
                                mCurLoops = mMaxLoops - 1;
                                currentTime = maxTime;
                                mPassedTime = 0.0f;
                                hasLooped = false;

                                if (!GetIsFrozen())
                                {
                                    mMotion->GetEventTable()->ProcessEvents(mCurrentTime, maxTime + 0.00001f, this);
                                    EnableFlag(BOOL_ISFROZENATLASTFRAME);
                                    GetEventManager().OnIsFrozenAtLastFrame(this);
                                }
                            }
                        }
                    }
                    else
                    {
                        DisableFlag(BOOL_ISFROZENATLASTFRAME);

                        if (maxTime > 0.0f)
                        {
                            currentTime = mClipStartTime + MCore::Math::SafeFMod(currentTime - mClipStartTime, maxTime - mClipStartTime);
                        }
                        else
                        {
                            currentTime = 0.0f;
                        }
                    }
                }
            }

            // if we use the freeze at time setting
            if (mFreezeAtTime >= 0.0f)
            {
                // if we past that freeze time, keep it there
                if (currentTime > mFreezeAtTime)
                {
                    currentTime = mFreezeAtTime;
                }
            }

            // if the current time is before the start of the motion, set it to the start of the motion
            if (currentTime < mClipStartTime)
            {
                currentTime = mClipStartTime;
            }

            // set updated, validated values again
            mCurrentTime = currentTime;

            // time difference till the loop point
            mTimeDifToEnd = mClipEndTime - mCurrentTime;
        }
        else // backward playback mode
        {
            mPassedTime         = -(timePassed * mPlaySpeed);
            float currentTime   = mCurrentTime + mPassedTime;
            float maxTime        = mClipEndTime;
            if (maxTime > mMotion->GetMaxTime())
            {
                maxTime = mMotion->GetMaxTime();
                mClipEndTime = maxTime;
            }

            mTotalPlayTime += MCore::Math::Abs(timePassed);

            // check and handle the bounds of the current playtime, to make it loop etc
            if (mMaxLoops == EMFX_LOOPFOREVER) // if it's looping forever
            {
                if (currentTime <= mClipStartTime)
                {
                    hasLooped = true;
                    mCurLoops++;
                    currentTime = maxTime + (currentTime - mClipStartTime);
                }

                if (currentTime > maxTime)
                {
                    while (currentTime > maxTime)
                    {
                        currentTime = 2.0f * maxTime - currentTime;
                    }
                }
            }
            else
            {
                // if we passed the end of the motion
                if (currentTime <= mClipStartTime)
                {
                    mCurLoops++;
                    hasLooped = true;

                    if (mCurLoops >= mMaxLoops)
                    {
                        if (!GetFreezeAtLastFrame())
                        {
                            //if (mActorInstance->GetActor()->GetRepositioningNodeIndex() == MCORE_INVALIDINDEX32)
                            //currentTime = 0.0f;
                            //else
                            {
                                currentTime = maxTime + (currentTime - mClipStartTime);
                            }
                        }
                        else
                        {
                            mCurLoops = mMaxLoops - 1;
                            currentTime = mClipStartTime;
                            mPassedTime = 0.0f;
                            hasLooped = false;

                            if (!GetIsFrozen())
                            {
                                mMotion->GetEventTable()->ProcessEvents(mClipStartTime, mCurrentTime, this);
                                EnableFlag(BOOL_ISFROZENATLASTFRAME);
                                GetEventManager().OnIsFrozenAtLastFrame(this);
                            }
                        }
                    }
                    else
                    {
                        DisableFlag(BOOL_ISFROZENATLASTFRAME);
                        currentTime = maxTime + currentTime;
                    }
                }
            }

            // if we use the freeze at time setting
            if (mFreezeAtTime >= 0.0f)
            {
                // if we past that freeze time, keep it there
                if (currentTime < mFreezeAtTime)
                {
                    currentTime = mFreezeAtTime;
                }
            }

            // if the current time is before the start of the motion, set it to the start of the motion
            if (currentTime < mClipStartTime)
            {
                currentTime = mClipStartTime;
            }

            // copy over the current time
            mCurrentTime = currentTime;

            // time difference to the loop point
            mTimeDifToEnd = mCurrentTime - mClipStartTime;
        }

        // trigger the OnMotionInstanceHasLooped event
        if (hasLooped)
        {
            GetEventManager().OnHasLooped(this);
        }
    }


    // update the motion instance information
    void MotionInstance::Update(float timePassed)
    {
        // don't do anything when we're not active
        if (!GetIsActive())
        {
            return;
        }

        // update the motion group
        if (mMotionGroup)
        {
            mMotionGroup->Update(timePassed);
        }

        // update the last number of loops
        mLastLoops = mCurLoops;

        // if we're paused, don't update the time values and process motion events
        if (GetIsPaused() || AZ::IsClose(timePassed, 0, AZ::g_fltEps))
        {
            mPassedTime     = 0.0f;
            mLastCurTime    = mCurrentTime;
            return;
        }

        // update the current time value
        const float currentTimePreUpdate = mCurrentTime;
        UpdateTime(timePassed);

        // If UpdateTime() did not advance mCurrentTime we can skip over ProcessEvents()
        if (!AZ::IsClose(mLastCurTime, mCurrentTime, AZ::g_fltEps))
        {
            // if we are blending towards the destination motion or layer.
            // Do this after UpdateTime(timePassed) and use (mCurrentTime - mLastCurTime)
            // as the elapsed time. This will function for Updates that use SetCurrentTime(time, false)
            // like Simple Motion component does with Track View. This will also work for motions that
            // have mPlaySpeed that is not 1.0f.
            if (GetIsBlending())
            {
                float deltaTime = 0.0f;

                if (mPlayMode == PLAYMODE_FORWARD)
                {
                    // Playing forward, if the motion looped, need to consider the wrapped delta time
                    if (mLastCurTime > mCurrentTime)
                    {
                        // Need to add the last time up to the end of the motion, and the cur time from the start of the motion.
                        // That will give us the full wrap around time.
                        deltaTime = (mClipEndTime - mLastCurTime) + (mCurrentTime - mClipStartTime);
                    }
                    else
                    {
                        // No looping, simple time passed calc.
                        deltaTime = (mCurrentTime - mLastCurTime);
                    }
                }
                else
                {
                    // Playing in reverse, if the motion looped, need to consider the wrapped delta time
                    if (mLastCurTime < mCurrentTime)
                    {
                        // Need to add the last time up to the start of the motion, and the cur time from the end of the motion.
                        // That will give us the full wrap around time.
                        deltaTime = (mClipStartTime - mLastCurTime) + (mCurrentTime - mClipEndTime);
                    }
                    else
                    {
                        // No looping, simple time passed calc.
                        deltaTime = (mLastCurTime - mCurrentTime);
                    }
                }

                // update the weight
                mWeight += mWeightDelta * deltaTime;

                // if we're increasing the weight
                if (mWeightDelta >= 0)
                {
                    // if we reached our target weight, don't go past that
                    if (mWeight >= mTargetWeight)
                    {
                        mWeight = mTargetWeight;
                        DisableFlag(BOOL_ISBLENDING);
                        GetEventManager().OnStopBlending(this);
                    }
                }
                else // if we're decreasing the weight
                {
                    // if we reached our target weight, don't let it go lower than that
                    if (mWeight <= mTargetWeight)
                    {
                        mWeight = mTargetWeight;
                        DisableFlag(BOOL_ISBLENDING);
                        GetEventManager().OnStopBlending(this);
                    }
                }
            }

            // process events
            ProcessEvents(mLastCurTime, mCurrentTime);
        }

        mLastCurTime = currentTimePreUpdate;
    }


    // process events between the previous and current time
    void MotionInstance::ProcessEvents(float oldTime, float newTime)
    {
        // calculate the actual real time passed in the animation playback
        const float realTimePassed = newTime - oldTime;

        // process motion events
        if (GetMotionEventsEnabled() && !GetIsPaused() && !MCore::Compare<float>::CheckIfIsClose(realTimePassed, 0.0f, MCore::Math::epsilon) && mWeight >= mEventWeightThreshold)
        {
            float curTimeValue = mCurrentTime;
            float oldTimeValue = mLastCurTime;

            // if a loop has happened we need to do some extra work
            if (GetHasLooped())
            {
                // if we're playing forward
                if (mPlayMode == PLAYMODE_FORWARD)
                {
                    mMotion->GetEventTable()->ProcessEvents(oldTimeValue, mClipEndTime + 0.0001f, this);
                    oldTimeValue = mClipStartTime;
                }
                else // we're playing backward
                {
                    mMotion->GetEventTable()->ProcessEvents(mClipStartTime - 0.0001f, oldTimeValue, this);
                    oldTimeValue = mClipEndTime;
                }
            }

            // process the remaining part
            if (!GetHasEnded())
            {
                // if forward playback
                if (oldTimeValue < curTimeValue)
                {
                    // if we end up exactly on the max time, include it
                    if (MCore::Math::Abs(curTimeValue - mClipEndTime) < 0.00001f)
                    {
                        curTimeValue = mClipEndTime + 0.0001f;
                    }
                }
                else // backward playback
                {
                    // if the end time is exactly at 0 seconds, include events on 0.0
                    if (curTimeValue < mClipStartTime + 0.00001f)
                    {
                        curTimeValue = mClipStartTime - 0.0001f;
                    }
                }

                // process the events
                mMotion->GetEventTable()->ProcessEvents(oldTimeValue, curTimeValue, this);
            }
        }
    }


    // gather events that get triggered between two given time values
    void MotionInstance::ExtractEvents(float oldTime, float newTime, AnimGraphEventBuffer* outBuffer)
    {
        // calculate the actual real time passed in the animation playback
        const float realTimePassed = newTime - oldTime;

        // process motion events
        if (GetMotionEventsEnabled() && !GetIsPaused() && !MCore::Compare<float>::CheckIfIsClose(realTimePassed, 0.0f, MCore::Math::epsilon) && mWeight >= mEventWeightThreshold)
        {
            float curTimeValue = mCurrentTime;
            float oldTimeValue = mLastCurTime;

            // if a loop has happened we need to do some extra work
            if (GetHasLooped())
            {
                // if we're playing forward
                if (mPlayMode == PLAYMODE_FORWARD)
                {
                    mMotion->GetEventTable()->ExtractEvents(oldTimeValue, mClipEndTime + 0.0001f, this, outBuffer);
                    oldTimeValue = mClipStartTime - 0.00001f;
                }
                else // we're playing backward
                {
                    mMotion->GetEventTable()->ExtractEvents(oldTimeValue, mClipStartTime - 0.0001f, this, outBuffer);
                    oldTimeValue = mClipEndTime + 0.00001f;
                }
            }

            // process the remaining part
            if (!GetHasEnded())
            {
                // if forward playback
                if (oldTimeValue < curTimeValue)
                {
                    // if we end up exactly on the max time, include it
                    if (MCore::Math::Abs(curTimeValue - mClipEndTime) < 0.00001f)
                    {
                        curTimeValue = mClipEndTime + 0.0001f;
                    }
                }
                else // backward playback
                {
                    // if the end time is exactly at 0 seconds, include events on 0.0
                    if (curTimeValue < mClipStartTime + 0.00001f)
                    {
                        curTimeValue = mClipStartTime - 0.0001f;
                    }
                }

                // process the events
                mMotion->GetEventTable()->ExtractEvents(oldTimeValue, curTimeValue, this, outBuffer);
            }
        }
    }



    // gather events that get triggered between two given time values
    void MotionInstance::ExtractEventsNonLoop(float oldTime, float newTime, AnimGraphEventBuffer* outBuffer)
    {
        // calculate the actual real time passed in the animation playback
        const float realTimePassed = newTime - oldTime;

        // if there is no real time delta
        if (MCore::Math::Abs(realTimePassed) < MCore::Math::epsilon)
        {
            return;
        }

        // extract the events
        mMotion->GetEventTable()->ExtractEvents(oldTime, newTime, this, outBuffer);
    }



    // update the motion by an old and new time value
    void MotionInstance::UpdateByTimeValues(float oldTime, float newTime, AnimGraphEventBuffer* outEventBuffer)
    {
        mLastCurTime    = oldTime;
        mLastLoops      = mCurLoops;

        // if we are blending towards the destination motion or layer
        const float timePassed = newTime - oldTime;
        if (MCore::Math::Abs(timePassed) < MCore::Math::epsilon)
        {
            mCurrentTime = newTime;
            return;
        }

        if (GetIsBlending())
        {
            // update the weight
            mWeight += mWeightDelta * timePassed;

            // if we're increasing the weight
            if (mWeightDelta >= 0)
            {
                // if we reached our target weight, don't go past that
                if (mWeight >= mTargetWeight)
                {
                    mWeight = mTargetWeight;
                    DisableFlag(BOOL_ISBLENDING);
                    GetEventManager().OnStopBlending(this);
                }
            }
            else // if we're decreasing the weight
            {
                // if we reached our target weight, don't let it go lower than that
                if (mWeight <= mTargetWeight)
                {
                    mWeight = mTargetWeight;
                    DisableFlag(BOOL_ISBLENDING);
                    GetEventManager().OnStopBlending(this);
                }
            }
        }

        // if we're dealing with a grouped motion
        if (!mMotion)
        {
            mPassedTime      = timePassed;
            mTotalPlayTime  += MCore::Math::Abs(timePassed);
            return;
        }

        bool hasLooped = false;

        //if (Math::Abs(timePassed) > 0.000001f)
        //MCore::LogInfo("%s passed = %f     mode=%d", mMotion->GetFileName(), timePassed, mPlayMode);

        // if we are playing forward
        if (mPlayMode == PLAYMODE_FORWARD)
        {
            mPassedTime          = timePassed;
            float currentTime    = newTime;

            float maxTime        = mClipEndTime;////mMotion->GetMaxTime()
            if (maxTime > mMotion->GetMaxTime())
            {
                maxTime = mMotion->GetMaxTime();
                mClipEndTime = maxTime;
            }

            mTotalPlayTime += MCore::Math::Abs(timePassed);

            // check and handle the bounds of the current playtime, to make it loop etc
            if (mMaxLoops == EMFX_LOOPFOREVER) // if it's looping forever
            {
                if (oldTime > newTime)
                {
                    mCurLoops++;
                    hasLooped = true;

                    if (maxTime > 0.0f)
                    {
                        currentTime = newTime;//mClipStartTime + MCore::Math::SafeFMod(currentTime-mClipStartTime, maxTime - mClipStartTime);
                    }
                    else
                    {
                        currentTime = 0.0f;
                    }
                }

                /*          if (currentTime < 0.0f)
                            {
                                while (currentTime < 0.0f)
                                    currentTime = maxTime + currentTime;
                            }*/
            }
            else // not looping forever
            {
                // if we passed the end of the motion, keep it there
                if (oldTime > newTime)
                {
                    mCurLoops++;
                    hasLooped = true;

                    if (mCurLoops >= mMaxLoops)
                    {
                        //if (mActorInstance->GetActor()->GetRepositioningNodeIndex() == MCORE_INVALIDINDEX32)
                        //currentTime = maxTime;
                        //else
                        {
                            if (!GetFreezeAtLastFrame())
                            {
                                if (maxTime > 0.0f)
                                {
                                    currentTime = newTime; // mClipStartTime + MCore::Math::SafeFMod(currentTime-mClipStartTime, maxTime - mClipStartTime);
                                }
                                else
                                {
                                    currentTime = 0.0f;
                                }
                            }
                            else
                            {
                                mCurLoops = mMaxLoops - 1;
                                currentTime = maxTime;
                                mPassedTime = 0.0f;
                                hasLooped = false;

                                if (!GetIsFrozen())
                                {
                                    if (outEventBuffer)
                                    {
                                        mMotion->GetEventTable()->ExtractEvents(mCurrentTime, maxTime + 0.00001f, this, outEventBuffer);
                                    }
                                    EnableFlag(BOOL_ISFROZENATLASTFRAME);
                                    GetEventManager().OnIsFrozenAtLastFrame(this);
                                }
                            }
                        }
                    }
                    else // we didn't reach max loops yet
                    {
                        DisableFlag(BOOL_ISFROZENATLASTFRAME);

                        if (maxTime > 0.0f)
                        {
                            currentTime = newTime; // mClipStartTime + MCore::Math::SafeFMod(currentTime-mClipStartTime, maxTime - mClipStartTime);
                        }
                        else
                        {
                            currentTime = 0.0f;
                        }
                    }
                }
                // special case for non looping motions when we reached the end of the motion
                else if (!GetIsPlayingForever() && MCore::Math::IsFloatEqual(newTime, mClipEndTime))
                {
                    if (!GetIsFrozen())
                    {
                        if (outEventBuffer)
                        {
                            mMotion->GetEventTable()->ExtractEvents(mCurrentTime, maxTime + 0.00001f, this, outEventBuffer);
                        }

                        EnableFlag(BOOL_ISFROZENATLASTFRAME);
                        GetEventManager().OnIsFrozenAtLastFrame(this);
                    }
                }
            }

            // if we use the freeze at time setting
            if (mFreezeAtTime >= 0.0f)
            {
                // if we past that freeze time, keep it there
                if (currentTime > mFreezeAtTime)
                {
                    currentTime = mFreezeAtTime;
                }
            }

            // if the current time is before the start of the motion, set it to the start of the motion
            if (currentTime < mClipStartTime)
            {
                currentTime = mClipStartTime;
            }

            // set updated, validated values again
            mCurrentTime = currentTime;

            // time difference till the loop point
            mTimeDifToEnd = mClipEndTime - mCurrentTime;
        }
        else // backward playback mode
        {
            mPassedTime         = timePassed;
            float currentTime   = newTime;//mCurrentTime + mPassedTime;
            float maxTime       = mClipEndTime;
            if (maxTime > mMotion->GetMaxTime())
            {
                maxTime = mMotion->GetMaxTime();
                mClipEndTime = maxTime;
            }

            mTotalPlayTime += MCore::Math::Abs(timePassed);

            // check and handle the bounds of the current playtime, to make it loop etc
            if (mMaxLoops == EMFX_LOOPFOREVER) // if it's looping forever
            {
                if (newTime > oldTime)
                {
                    hasLooped = true;
                    mCurLoops++;
                    currentTime = newTime;//maxTime + (currentTime - mClipStartTime);
                }

                if (currentTime > maxTime)
                {
                    while (currentTime > maxTime)
                    {
                        currentTime = 2.0f * maxTime - currentTime;
                    }
                }
            }
            else
            {
                // if we passed the end of the motion
                if (oldTime < newTime)
                {
                    mCurLoops++;
                    hasLooped = true;

                    if (mCurLoops >= mMaxLoops)
                    {
                        if (!GetFreezeAtLastFrame())
                        {
                            //if (mActorInstance->GetActor()->GetRepositioningNodeIndex() == MCORE_INVALIDINDEX32)
                            //currentTime = 0.0f;
                            //else
                            {
                                currentTime = newTime;//maxTime + (currentTime - mClipStartTime);
                            }
                        }
                        else
                        {
                            mCurLoops = mMaxLoops - 1;
                            currentTime = mClipStartTime;
                            mPassedTime = 0.0f;
                            hasLooped = false;

                            if (!GetIsFrozen())
                            {
                                if (outEventBuffer)
                                {
                                    mMotion->GetEventTable()->ExtractEvents(mClipStartTime, mCurrentTime, this, outEventBuffer);
                                }
                                EnableFlag(BOOL_ISFROZENATLASTFRAME);
                                GetEventManager().OnIsFrozenAtLastFrame(this);
                            }
                        }
                    }
                    else // didn't reach the max loops yet
                    {
                        DisableFlag(BOOL_ISFROZENATLASTFRAME);
                        currentTime = newTime;//maxTime + currentTime;
                    }
                }
            }

            // if we use the freeze at time setting
            if (mFreezeAtTime >= 0.0f)
            {
                // if we past that freeze time, keep it there
                if (currentTime < mFreezeAtTime)
                {
                    currentTime = mFreezeAtTime;
                }
            }

            // if the current time is before the start of the motion, set it to the start of the motion
            if (currentTime < mClipStartTime)
            {
                currentTime = mClipStartTime;
            }

            // copy over the current time
            mCurrentTime = currentTime;

            // time difference to the loop point
            mTimeDifToEnd = mCurrentTime - mClipStartTime;
        }

        // trigger the OnMotionInstanceHasLooped event
        if (hasLooped)
        {
            GetEventManager().OnHasLooped(this);
        }

        // extract the events if desired
        if (outEventBuffer)
        {
            ExtractEvents(mLastCurTime, mCurrentTime, outEventBuffer);
        }
    }



    // start a blend to the new weight
    void MotionInstance::SetWeight(float targetWeight, float blendTimeInSeconds)
    {
        // make sure the inputs are valid
        MCORE_ASSERT(blendTimeInSeconds >= 0);
        MCORE_ASSERT(targetWeight >= 0 && targetWeight <= 1);
        mTargetWeight = MCore::Clamp<float>(targetWeight, 0.0f, 1.0f);

        if (blendTimeInSeconds > 0.0f)
        {
            // calculate the rate of change of the weight value, so it goes towards the target weight
            mWeightDelta = (mTargetWeight - mWeight) / blendTimeInSeconds;

            // update the blendin time
            if (mTargetWeight > mWeight)
            {
                mBlendInTime = blendTimeInSeconds;
            }

            if (!GetIsBlending())
            {
                EnableFlag(BOOL_ISBLENDING);
                GetEventManager().OnStartBlending(this);
            }
        }
        else // blendtime is zero
        {
            if (mTargetWeight > mWeight)
            {
                mBlendInTime = 0.0f;
            }

            mWeight         = mTargetWeight;
            mWeightDelta    = 0.0f;

            if (GetIsBlending())
            {
                DisableFlag(BOOL_ISBLENDING);
                GetEventManager().OnStopBlending(this);
            }
        }
    }


    // setup a weight for a given node
    void MotionInstance::SetNodeWeight(uint32 nodeIndex, float weight)
    {
        MCORE_ASSERT(weight >= 0.0f && weight <= 1.0f);

        // if we haven't allocated the node weights yet, do so
        if (mNodeWeights.GetLength() == 0)
        {
            const uint32 numNodes = mMotionLinks.GetLength();
            mNodeWeights.Resize(numNodes);

            // init them on weight 1.0f
            for (uint32 i = 0; i < numNodes; ++i)
            {
                mNodeWeights[i].FromFloat(1.0f, 0.0f, 1.0f);
            }

            // setup that this motion cannot overwrite another motion, since we use a blend mask
            DisableFlag(BOOL_CANOVERWRITE);
        }

        // adjust the weight we want
        mNodeWeights[nodeIndex].FromFloat(weight, 0.0f, 1.0f);
    }


    // stop the motion with a new fadeout time
    void MotionInstance::Stop(float fadeOutTime)
    {
        // trigger a motion event
        GetEventManager().OnStop(this);

        // update the fade out time
        SetFadeTime(fadeOutTime);

        // fade out
        SetWeight(0.0f, mFadeTime);

        // we are stopping the motion
        //if (mFadeTime > 0)    // if the blendtime would be 0, the motion would already have been deleted now
        EnableFlag(BOOL_ISSTOPPING);
    }


    // stop the motion using the currently setup fadeout time
    void MotionInstance::Stop()
    {
        // trigger a motion event
        GetEventManager().OnStop(this);

        // fade out
        SetWeight(0.0f, mFadeTime);

        // we are stopping the motion
        //  if (mFadeTime > 0)  // if the blendtime would be 0, the motion would already have been deleted now
        EnableFlag(BOOL_ISSTOPPING);
    }


    void MotionInstance::SetIsActive(bool enabled)
    {
        if (GetIsActive() != enabled)
        {
            SetFlag(BOOL_ISACTIVE, enabled);
            GetEventManager().OnChangedActiveState(this);
        }
    }


    // pause
    void MotionInstance::Pause()
    {
        if (GetIsPaused() == false)
        {
            EnableFlag(BOOL_ISPAUSED);
            GetEventManager().OnChangedPauseState(this);
        }
    }


    // unpause
    void MotionInstance::UnPause()
    {
        if (GetIsPaused())
        {
            DisableFlag(BOOL_ISPAUSED);
            GetEventManager().OnChangedPauseState(this);
        }
    }


    // enable or disable pause state
    void MotionInstance::SetPause(bool pauseEnabled)
    {
        if (GetIsPaused() != pauseEnabled)
        {
            SetFlag(BOOL_ISPAUSED, pauseEnabled);
            GetEventManager().OnChangedPauseState(this);
        }
    }


    // add a given event handler
    void MotionInstance::AddEventHandler(MotionInstanceEventHandler* eventHandler)
    {
        AZ_Assert(eventHandler, "Expected non-null event handler");
        eventHandler->SetMotionInstance(this);

        for (const EventTypes eventType : eventHandler->GetHandledEventTypes())
        {
            AZ_Assert(AZStd::find(m_eventHandlersByEventType[eventType - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT].begin(), m_eventHandlersByEventType[eventType - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT].end(), eventHandler) == m_eventHandlersByEventType[eventType - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT].end(),
                "Event handler already added to manager");
            m_eventHandlersByEventType[eventType - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT].emplace_back(eventHandler);
        }
    }


    // remove a given handler from memory
    void MotionInstance::RemoveEventHandler(MotionInstanceEventHandler* eventHandler)
    {
        for (const EventTypes eventType : eventHandler->GetHandledEventTypes())
        {
            EventHandlerVector& eventHandlers = m_eventHandlersByEventType[eventType - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
            eventHandlers.erase(AZStd::remove(eventHandlers.begin(), eventHandlers.end(), eventHandler), eventHandlers.end());
        }
    }


    //  remove all event handlers
    void MotionInstance::RemoveAllEventHandlers()
    {
#ifdef DEBUG
        for (const EventHandlerVector& eventHandlers : m_eventHandlersByEventType)
        {
            AZ_Assert(eventHandlers.empty(), "Expected all events to be removed");
        }
#endif
        m_eventHandlersByEventType.clear();
    }

    //--------------------

    // on a motion event
    void MotionInstance::OnEvent(const EventInfo& eventInfo)
    {
        EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_EVENT - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnEvent(eventInfo);
        }
    }


    // when this motion instance starts
    void MotionInstance::OnStartMotionInstance(PlayBackInfo* info)
    {
        EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_START_MOTION_INSTANCE - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStartMotionInstance(info);
        }
    }


    // when this motion instance gets deleted
    void MotionInstance::OnDeleteMotionInstance()
    {
        EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_DELETE_MOTION_INSTANCE - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnDeleteMotionInstance();
        }
    }


    // when this motion instance is stopped
    void MotionInstance::OnStop()
    {
        EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STOP - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStop();
        }
    }


    // when it has looped
    void MotionInstance::OnHasLooped()
    {
        EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_HAS_LOOPED - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnHasLooped();
        }
    }


    // when it reached the maximimum number of loops
    void MotionInstance::OnHasReachedMaxNumLoops()
    {
        EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_HAS_REACHED_MAX_NUM_LOOPS - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnHasReachedMaxNumLoops();
        }
    }


    // when it has reached the maximimum playback time
    void MotionInstance::OnHasReachedMaxPlayTime()
    {
        EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_HAS_REACHED_MAX_PLAY_TIME - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnHasReachedMaxPlayTime();
        }
    }


    // when it is frozen in the last frame
    void MotionInstance::OnIsFrozenAtLastFrame()
    {
        EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_IS_FROZEN_AT_LAST_FRAME - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnIsFrozenAtLastFrame();
        }
    }


    // when the pause state changes
    void MotionInstance::OnChangedPauseState()
    {
        EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_CHANGED_PAUSE_STATE - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnChangedPauseState();
        }
    }


    // when the active state changes
    void MotionInstance::OnChangedActiveState()
    {
        EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_CHANGED_ACTIVE_STATE - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnChangedActiveState();
        }
    }


    // when it starts blending in or out
    void MotionInstance::OnStartBlending()
    {
        EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_START_BLENDING - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStartBlending();
        }
    }


    // when it stops blending
    void MotionInstance::OnStopBlending()
    {
        EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STOP_BLENDING - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStopBlending();
        }
    }


    // when the motion instance is queued
    void MotionInstance::OnQueueMotionInstance(PlayBackInfo* info)
    {
        EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_QUEUE_MOTION_INSTANCE - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnQueueMotionInstance(info);
        }
    }

    void MotionInstance::SetIsOwnedByRuntime(bool isOwnedByRuntime)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        SetFlag(BOOL_ISOWNEDBYRUNTIME, isOwnedByRuntime);
#endif
    }


    bool MotionInstance::GetIsOwnedByRuntime() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return (mBoolFlags & BOOL_ISOWNEDBYRUNTIME) != 0;
#else
        return true;
#endif
    }


    // calculate a world space transformation for a given node by sampling the motion at a given time
    void MotionInstance::CalcGlobalTransform(const MCore::Array<uint32>& hierarchyPath, float timeValue, Transform* outTransform) const
    {
        Actor*      actor = mActorInstance->GetActor();
        Skeleton*   skeleton = actor->GetSkeleton();

        // start with an identity transform
        Transform subMotionTransform;
        outTransform->Identity();

        // iterate from root towards the node (so backwards in the array)
        for (int32 i = hierarchyPath.GetLength() - 1; i >= 0; --i)
        {
            // get the current node index
            const uint32 nodeIndex = hierarchyPath[i];

            // sample the motion if this is supported by the type of motion
            if (mMotion->GetSupportsCalcNodeTransform())
            {
                mMotion->CalcNodeTransform(this, &subMotionTransform, actor, skeleton->GetNode(nodeIndex), timeValue, GetRetargetingEnabled());
            }
            else
            {
                subMotionTransform.Identity();
            }

            // multiply parent transform with the current node's transform
            outTransform->Multiply(subMotionTransform);
        }
    }


    // get relative movements between two non-looping time values
    void MotionInstance::CalcRelativeTransform(Node* rootNode, float curTime, float oldTime, Transform* outTransform) const
    {
        // get the facial setup of the actor
        Actor* actor = mActorInstance->GetActor();

        // check if we support calculating of the node transform
        if (mMotion->GetSupportsCalcNodeTransform())
        {
            // calculate the two node transformations
            Transform curNodeTransform;
            mMotion->CalcNodeTransform(this, &curNodeTransform, actor, rootNode, curTime, GetRetargetingEnabled());

            Transform oldNodeTransform;
            mMotion->CalcNodeTransform(this, &oldNodeTransform, actor, rootNode, oldTime, GetRetargetingEnabled());

            // calculate the relative transforms
            outTransform->mPosition = curNodeTransform.mPosition - oldNodeTransform.mPosition;
            outTransform->mRotation = curNodeTransform.mRotation * oldNodeTransform.mRotation.Conjugated();
            outTransform->mRotation.Normalize();
        }
        else // doesn't support CalcNodeTranform
        {
            outTransform->mPosition.Set(0.0f, 0.0f, 0.0f);
            outTransform->mRotation.Identity();
        }
    }


    // extract the motion delta transform
    bool MotionInstance::ExtractMotion(Transform& outTrajectoryDelta)
    {
        // start clean
        outTrajectoryDelta.ZeroWithIdentityQuaternion();
        if (!GetMotionExtractionEnabled())
        {
            return true;
        }

        Actor*  actor               = mActorInstance->GetActor();
        Node*   motionExtractNode   = actor->GetMotionExtractionNode();

        // check if we have enough data to actually calculate the results
        if (!motionExtractNode)
        {
            return false;
        }

        // get the motion extraction node index
        const uint32 motionExtractionNodeIndex = motionExtractNode->GetNodeIndex();

        // check if this motion influences the node
        const MotionLink* motionLink = GetMotionLink(motionExtractionNodeIndex);
        if (!motionLink->GetIsActive() || MCore::Compare<float>::CheckIfIsClose(GetPassedTime(), 0.0f, MCore::Math::epsilon))
        {
            return false;
        }

        // get the current and previous time value from the motion instance
        float curTimeValue = GetCurrentTime();
        float oldTimeValue = GetLastCurrentTime();

        Transform trajectoryDelta;
        trajectoryDelta.ZeroWithIdentityQuaternion();

        // if the motion isn't paused
        if (!GetIsPaused())
        {
            // the relative transformations that we sample
            Transform relativeTrajectoryTransform;
            relativeTrajectoryTransform.Identity();

            // prevent looping from moving the character back to the origin if this is desired
            if (GetHasLooped())
            {
                // if we're playing forward
                if (mPlayMode == PLAYMODE_FORWARD)
                {
                    CalcRelativeTransform(motionExtractNode, mClipEndTime, oldTimeValue, &relativeTrajectoryTransform);
                    oldTimeValue = 0.0f;
                }
                else // we're playing backward
                {
                    CalcRelativeTransform(motionExtractNode, mClipStartTime, oldTimeValue, &relativeTrajectoryTransform);
                    oldTimeValue = mMotion->GetMaxTime();
                }

                // add the relative transform to the final values
                trajectoryDelta.mPosition += relativeTrajectoryTransform.mPosition;
                trajectoryDelta.mRotation  = relativeTrajectoryTransform.mRotation * trajectoryDelta.mRotation;
            }

            // calculate the relative movement
            relativeTrajectoryTransform.Identity();
            CalcRelativeTransform(motionExtractNode, curTimeValue, oldTimeValue, &relativeTrajectoryTransform);

            // add the relative transform to the final values
            trajectoryDelta.mPosition += relativeTrajectoryTransform.mPosition;
            trajectoryDelta.mRotation  = relativeTrajectoryTransform.mRotation * trajectoryDelta.mRotation;
        } // if not paused


        // Calculate the first frame's transform.
        Transform firstFrameTransform;
        mMotion->CalcNodeTransform(this, &firstFrameTransform, actor, motionExtractNode, GetClipStartTime(), GetRetargetingEnabled());

        // Calculate the current frame's transform.
        Transform currentFrameTransform;
        mMotion->CalcNodeTransform(this, &currentFrameTransform, actor, motionExtractNode, GetCurrentTime(), GetRetargetingEnabled());

        // Calculate the difference between the first frame of the motion and the bind pose transform.
        TransformData* transformData = mActorInstance->GetTransformData();
        const Pose* bindPose = transformData->GetBindPose();
        MCore::Quaternion permBindPoseRotDiff = firstFrameTransform.mRotation * bindPose->GetLocalSpaceTransform(motionExtractionNodeIndex).mRotation.Conjugated();
        AZ::Vector3 permBindPosePosDiff = bindPose->GetLocalSpaceTransform(motionExtractionNodeIndex).mPosition - firstFrameTransform.mPosition;
        permBindPoseRotDiff.x = 0.0f;
        permBindPoseRotDiff.y = 0.0f;
        permBindPoseRotDiff.Normalize();

        if (!(mMotion->GetMotionExtractionFlags() & MOTIONEXTRACT_CAPTURE_Z))
        {
            permBindPosePosDiff.SetZ(0.0f);
        }

        // If this is the first frame's motion extraction switch turn that flag off.
        MCore::Quaternion bindPoseRotDiff(0.0f, 0.0f, 0.0f, 1.0f);
        AZ::Vector3 bindPosePosDiff(0.0f, 0.0f, 0.0f);
        if (mBoolFlags & MotionInstance::BOOL_ISFIRSTREPOSUPDATE)
        {
            bindPoseRotDiff = permBindPoseRotDiff;
            bindPosePosDiff = permBindPosePosDiff;
        }

        // Capture rotation around the up axis only.
        trajectoryDelta.ApplyMotionExtractionFlags(mMotion->GetMotionExtractionFlags());

        MCore::Quaternion removeRot = currentFrameTransform.mRotation * firstFrameTransform.mRotation.Conjugated();
        removeRot.x = 0.0f;
        removeRot.y = 0.0f;
        removeRot.Normalize();

        MCore::Quaternion rotation = removeRot.Conjugated() * trajectoryDelta.mRotation * permBindPoseRotDiff.Conjugated();
        rotation.x = 0.0f;
        rotation.y = 0.0f;
        rotation.Normalize();

        AZ::Vector3 rotatedPos = rotation * (trajectoryDelta.mPosition - bindPosePosDiff);

        // Calculate the real trajectory delta, taking into account the actor instance rotation.
        outTrajectoryDelta.mPosition = mActorInstance->GetLocalSpaceTransform().mRotation * rotatedPos;
        outTrajectoryDelta.mRotation = trajectoryDelta.mRotation * bindPoseRotDiff;

        if (mBoolFlags & MotionInstance::BOOL_ISFIRSTREPOSUPDATE)
        {
            DisableFlag(MotionInstance::BOOL_ISFIRSTREPOSUPDATE);
        }

        return true;
    }


    // get the sub pool
    MotionInstancePool::SubPool* MotionInstance::GetSubPool() const
    {
        return mSubPool;
    }


    // set the subpool
    void MotionInstance::SetSubPool(MotionInstancePool::SubPool* subPool)
    {
        mSubPool = subPool;
    }


    bool MotionInstance::GetIsReadyForSampling() const                          { return (mMotionLinks.GetLength() > 0); }
    void MotionInstance::SetCustomData(void* customDataPointer)                 { mCustomData = customDataPointer; }
    void* MotionInstance::GetCustomData() const                                 { return mCustomData; }
    float MotionInstance::GetBlendInTime() const                                { return mBlendInTime; }
    float MotionInstance::GetPassedTime() const                                 { return mPassedTime; }
    float MotionInstance::GetCurrentTime() const                                { return mCurrentTime; }
    float MotionInstance::GetMaxTime() const                                    { return mClipEndTime; }
    float MotionInstance::GetDuration() const                                   { return (mClipEndTime - mClipStartTime); }
    float MotionInstance::GetPlaySpeed() const                                  { return mPlaySpeed; }
    Motion* MotionInstance::GetMotion() const                                   { return mMotion; }
    MotionGroup* MotionInstance::GetMotionGroup() const                         { return mMotionGroup; }
    bool MotionInstance::GetIsGroupedMotion() const                             { return (mMotionGroup && mMotionGroup->GetNumMotionInstances() > 0); }
    void MotionInstance::SetCurrentTimeNormalized(float normalizedTimeValue)    { mCurrentTime = mClipStartTime + normalizedTimeValue * (mClipEndTime - mClipStartTime); }
    float MotionInstance::GetCurrentTimeNormalized() const
    {
        float maxTime = mClipEndTime;
        if (maxTime > 0.0f)
        {
            return (mCurrentTime - mClipStartTime) / (maxTime - mClipStartTime);
        }
        else
        {
            return 0.0f;
        }
    }
    float MotionInstance::GetLastCurrentTime() const                            { return mLastCurTime; }
    void MotionInstance::SetMotion(Motion* motion)                              { mMotion = motion; }
    void MotionInstance::SetPassedTime(float timePassed)                        { mPassedTime = timePassed; }
    void MotionInstance::SetPlaySpeed(float speed)                              { MCORE_ASSERT(speed >= 0.0f); mPlaySpeed = speed; }
    void MotionInstance::SetPlayMode(EPlayMode mode)                            { mPlayMode = mode; }
    EPlayMode MotionInstance::GetPlayMode() const                               { return mPlayMode; }
    void MotionInstance::SetFadeTime(float fadeTime)                            { mFadeTime = fadeTime; }
    float MotionInstance::GetFadeTime() const                                   { return mFadeTime; }
    EMotionBlendMode MotionInstance::GetBlendMode() const                       { return mBlendMode; }
    float MotionInstance::GetWeight() const                                     { return mWeight; }
    float MotionInstance::GetTargetWeight() const                               { return mTargetWeight; }
    void MotionInstance::SetBlendMode(EMotionBlendMode mode)                    { mBlendMode = mode; }
    void MotionInstance::SetMirrorMotion(bool enabled)                          { SetFlag(BOOL_MIRRORMOTION, enabled); }
    bool MotionInstance::GetMirrorMotion() const                                { return (mBoolFlags & BOOL_MIRRORMOTION) != 0; }
    void MotionInstance::Rewind()                                               { SetCurrentTime(0.0f); }
    bool MotionInstance::GetHasEnded() const                                    { return (((mMaxLoops != EMFX_LOOPFOREVER) && (mCurLoops >= mMaxLoops)) || ((mMaxPlayTime > 0.0f) && (mCurrentTime >= mMaxPlayTime))); }
    void MotionInstance::SetMixMode(bool mixModeEnabled)                        { SetFlag(BOOL_ISMIXING, mixModeEnabled); }
    bool MotionInstance::GetIsStopping() const                                  { return (mBoolFlags & BOOL_ISSTOPPING) != 0; }
    bool MotionInstance::GetIsPlaying() const                                   { return (!GetHasEnded() && !GetIsPaused()); }
    bool MotionInstance::GetIsMixing() const                                    { return (mBoolFlags & BOOL_ISMIXING) != 0; }
    bool MotionInstance::GetIsBlending() const                                  { return (mBoolFlags & BOOL_ISBLENDING) != 0; }
    bool MotionInstance::GetIsPaused() const                                    { return (mBoolFlags & BOOL_ISPAUSED); }
    void MotionInstance::SetMaxLoops(uint32 numLoops)                           { mMaxLoops = numLoops; }
    uint32 MotionInstance::GetMaxLoops() const                                  { return mMaxLoops; }
    bool MotionInstance::GetHasLooped() const                                   { return (mCurLoops != mLastLoops); }
    void MotionInstance::SetNumCurrentLoops(uint32 numCurrentLoops)             { mCurLoops = numCurrentLoops; }
    void MotionInstance::SetNumLastLoops(uint32 numCurrentLoops)                { mLastLoops = numCurrentLoops; }
    uint32 MotionInstance::GetNumLastLoops() const                              { return mLastLoops; }
    uint32 MotionInstance::GetNumCurrentLoops() const                           { return mCurLoops; }
    bool MotionInstance::GetIsPlayingForever() const                            { return (mMaxLoops == EMFX_LOOPFOREVER); }
    ActorInstance* MotionInstance::GetActorInstance() const                     { return mActorInstance; }
    uint32 MotionInstance::GetPriorityLevel() const                             { return mPriorityLevel; }
    void MotionInstance::SetPriorityLevel(uint32 priorityLevel)                 { mPriorityLevel = priorityLevel; }
    bool MotionInstance::GetMotionExtractionEnabled() const                     { return (mBoolFlags & BOOL_USEMOTIONEXTRACTION) != 0; }
    void MotionInstance::SetMotionExtractionEnabled(bool enable)                { SetFlag(BOOL_USEMOTIONEXTRACTION, enable); }
    bool MotionInstance::GetCanOverwrite() const                                { return (mBoolFlags & BOOL_CANOVERWRITE) != 0; }
    void MotionInstance::SetCanOverwrite(bool canOverwrite)                     { SetFlag(BOOL_CANOVERWRITE, canOverwrite); }
    bool MotionInstance::GetDeleteOnZeroWeight() const                          { return (mBoolFlags & BOOL_DELETEONZEROWEIGHT) != 0; }
    void MotionInstance::SetDeleteOnZeroWeight(bool deleteOnZeroWeight)         { SetFlag(BOOL_DELETEONZEROWEIGHT, deleteOnZeroWeight); }
    bool MotionInstance::GetHasBlendMask() const                                { return (mNodeWeights.GetLength() > 0); }
    uint32 MotionInstance::GetStartNodeIndex() const                            { return mStartNodeIndex; }
    bool MotionInstance::GetRetargetingEnabled() const                          { return (mBoolFlags & BOOL_RETARGET) != 0; }
    void MotionInstance::SetRetargetingEnabled(bool enabled)                    { SetFlag(BOOL_RETARGET, enabled); }
    bool MotionInstance::GetIsActive() const                                    { return (mBoolFlags & BOOL_ISACTIVE) != 0; }
    bool MotionInstance::GetIsFrozen() const                                    { return (mBoolFlags & BOOL_ISFROZENATLASTFRAME) != 0; }
    void MotionInstance::SetIsFrozen(bool isFrozen)
    {
        if (isFrozen)
        {
            EnableFlag(BOOL_ISFROZENATLASTFRAME);
        }
        else
        {
            DisableFlag(BOOL_ISFROZENATLASTFRAME);
        }
    }
    bool MotionInstance::GetMotionEventsEnabled() const                         { return (mBoolFlags & BOOL_ENABLEMOTIONEVENTS) != 0; }
    void MotionInstance::SetMotionEventsEnabled(bool enabled)                   { SetFlag(BOOL_ENABLEMOTIONEVENTS, enabled); }
    void MotionInstance::SetEventWeightThreshold(float weightThreshold)         { mEventWeightThreshold = weightThreshold; }
    float MotionInstance::GetEventWeightThreshold() const                       { return mEventWeightThreshold; }
    bool MotionInstance::GetFreezeAtLastFrame() const                           { return (mBoolFlags & BOOL_FREEZEATLASTFRAME) != 0; }
    void MotionInstance::SetBlendOutBeforeEnded(bool enabled)                   { SetFlag(BOOL_BLENDBEFOREENDED, enabled); }
    bool MotionInstance::GetBlendOutBeforeEnded() const                         { return (mBoolFlags & BOOL_BLENDBEFOREENDED) != 0; }
    void MotionInstance::SetFreezeAtLastFrame(bool enabled)                     { SetFlag(BOOL_FREEZEATLASTFRAME, enabled); }
    float MotionInstance::GetTotalPlayTime() const                              { return mTotalPlayTime; }
    void MotionInstance::SetTotalPlayTime(float playTime)                       { mTotalPlayTime = playTime; }
    float MotionInstance::GetMaxPlayTime() const                                { return mMaxPlayTime; }
    void MotionInstance::SetMaxPlayTime(float playTime)                         { mMaxPlayTime = playTime; }
    uint32 MotionInstance::GetNumMotionLinks() const                            { return mMotionLinks.GetLength(); }
    MotionLink* MotionInstance::GetMotionLink(uint32 nr)                        { return &mMotionLinks[nr]; }
    const MotionLink* MotionInstance::GetMotionLink(uint32 nr)  const           { return &mMotionLinks[nr]; }
    void MotionInstance::SetMotionLink(uint32 nr, const MotionLink& link)       { mMotionLinks[nr] = link; }
    void MotionInstance::DisableNodeWeights()                                   { EnableFlag(BOOL_CANOVERWRITE); mNodeWeights.Clear(); }
    void MotionInstance::ResetCacheHitCounters()                                { mCacheHits = 0; mCacheMisses = 0; }
    void MotionInstance::ProcessCacheHitResult(uint8 wasHitResult)
    {
        if (wasHitResult == 0)
        {
            mCacheMisses++;
        }
        else
        {
            mCacheHits++;
        }
    }
    uint32 MotionInstance::GetNumCacheHits() const                              { return mCacheHits; }
    uint32 MotionInstance::GetNumCacheMisses() const                            { return mCacheMisses; }
    float MotionInstance::CalcCacheHitPercentage() const                        { return (mCacheHits / (float)(mCacheHits + mCacheMisses)) * 100.0f; }
    float MotionInstance::GetTimeDifToLoopPoint() const                         { return mTimeDifToEnd; }
    float MotionInstance::GetClipStartTime() const                              { return mClipStartTime; }
    void MotionInstance::SetClipStartTime(float timeInSeconds)                  { mClipStartTime = timeInSeconds; }
    float MotionInstance::GetClipEndTime() const                                { return mClipEndTime; }
    void MotionInstance::SetClipEndTime(float timeInSeconds)                    { mClipEndTime = timeInSeconds; }
    MotionLink* MotionInstance::GetMotionLinks() const                          { return mMotionLinks.GetPtr(); }
    float MotionInstance::GetFreezeAtTime() const                               { return mFreezeAtTime; }
    void MotionInstance::SetFreezeAtTime(float timeInSeconds)                   { mFreezeAtTime = timeInSeconds; }
    bool MotionInstance::GetIsInPlace() const                                   { return (mBoolFlags & BOOL_INPLACE) != 0; }
    void MotionInstance::SetIsInPlace(bool inPlace)                             { SetFlag(BOOL_INPLACE, inPlace); }

    float MotionInstance::GetNodeWeight(uint32 nodeIndex) const
    {
        if (mNodeWeights.GetLength())
        {
            return mNodeWeights[nodeIndex].ToFloat(0.0f, 1.0f);
        }
        else
        {
            return 1.0f;
        }
    }
} // namespace EMotionFX
