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

// include required headers
#include "EyeBlinker.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "MorphSetup.h"
#include <MCore/Source/Random.h>
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(EyeBlinker, EyeBlinkerAllocator, 0)


    EyeBlinker::EyeBlinker(ActorInstance* actorInstance)
        : BaseObject()
    {
        mActorInstance      = actorInstance;
        mMinOpenedTime      = 1.0f;
        mMaxOpenedTime      = 5.0f;
        mMinClosedTime      = 0.05f;
        mMaxClosedTime      = 0.1f;
        mMinBlinkTime       = 0.1f;
        mMaxBlinkTime       = 0.2f;
        mMinWeight          = 0.0f;
        mMaxWeight          = 1.0f;
        mCurMaxOpenedTime   = MCore::Random::RandF(mMinOpenedTime, mMaxOpenedTime);
        mCurMaxClosedTime   = MCore::Random::RandF(mMinClosedTime, mMaxClosedTime);
        mCurMaxBlinkTime    = MCore::Random::RandF(mMinBlinkTime, mMaxBlinkTime);
    }


    // destructor
    EyeBlinker::~EyeBlinker()
    {
    }


    // creation method
    EyeBlinker* EyeBlinker::Create(ActorInstance* actorInstance)
    {
        return aznew EyeBlinker(actorInstance);
    }


    // add a morph target that it controls
    bool EyeBlinker::AddMorphTarget(const char* name)
    {
        // get the morph setup etc
        EMotionFX::Actor*               actor           = mActorInstance->GetActor();
        EMotionFX::MorphSetup*          morphSetup      = actor->GetMorphSetup(0);
        EMotionFX::MorphSetupInstance*  targetSetup     = mActorInstance->GetMorphSetupInstance();
        if (morphSetup == nullptr || targetSetup == nullptr)
        {
            return false;
        }

        // find the morph target
        EMotionFX::MorphTarget* morphTarget = morphSetup->FindMorphTargetByNameNoCase(name);
        if (morphTarget == nullptr)
        {
            MCore::LogWarning("EMotionFX::EyeBlinker::AddMorphTarget() - There is no morph target with name '%s' found in the first LOD level's morph setup!", name);
            return false;
        }

        // find the morph target instance
        EMotionFX::MorphSetupInstance::MorphTarget* morphTargetInstance = targetSetup->FindMorphTargetByID(morphTarget->GetID());
        if (morphTargetInstance == nullptr)
        {
            MCore::LogWarning("EMotionFX::EyeBlinker::AddMorphTarget() - Failed to locate the morph target named '%s' inside the actor instance!", name);
            return false;
        }

        // enable manual mode and reset its weight
        morphTargetInstance->SetManualMode(true);
        morphTargetInstance->SetWeight(0.0f);

        // add it to the morph info list
        MorphInfo info;
        info.mMorphTarget   = morphTargetInstance;
        info.mIdleTime      = 0.0f;
        info.mBlendTime     = mMinBlinkTime / 2.0f;
        info.mTimeInBlend   = 0.0f;
        info.mIsBlending    = false;
        info.mIsOpened      = true;
        mMorphTargets.Add(info);

        return true;
    }


    // clear all morph targets it controls
    void EyeBlinker::ClearMorphTargets()
    {
        mMorphTargets.Clear();
    }


    // remove a given morph target from the control
    void EyeBlinker::RemoveMorphTarget(const char* name)
    {
        // get the morph setup etc
        EMotionFX::Actor*               actor           = mActorInstance->GetActor();
        EMotionFX::MorphSetup*          morphSetup      = actor->GetMorphSetup(0);
        EMotionFX::MorphSetupInstance*  targetSetup     = mActorInstance->GetMorphSetupInstance();
        if (morphSetup == nullptr || targetSetup == nullptr)
        {
            return;
        }

        // find the morph target
        EMotionFX::MorphTarget* morphTarget = morphSetup->FindMorphTargetByNameNoCase(name);
        if (morphTarget == nullptr)
        {
            return;
        }

        // find the morph target instance
        EMotionFX::MorphSetupInstance::MorphTarget* morphTargetInstance = targetSetup->FindMorphTargetByID(morphTarget->GetID());
        if (morphTargetInstance == nullptr)
        {
            return;
        }

        // remove it from the list
        const uint32 numMorphs = mMorphTargets.GetLength();
        for (uint32 i = 0; i < numMorphs; ++i)
        {
            if (mMorphTargets[i].mMorphTarget == morphTargetInstance)
            {
                mMorphTargets.Remove(i);
                return;
            }
        }
    }


    // update
    void EyeBlinker::Update(float timeDelta)
    {
        // update all morph targets this blinker controls
        const uint32 numMorphs = mMorphTargets.GetLength();
        for (uint32 i = 0; i < numMorphs; ++i)
        {
            MorphInfo&  morphInfo = mMorphTargets[i];

            // if we're not in progress of opening or closing the eye
            if (morphInfo.mIsBlending == false)
            {
                // if the eye is open
                if (morphInfo.mIsOpened)
                {
                    morphInfo.mIdleTime += timeDelta;

                    // if we reached the maximum time we are allowed to have the eye opened
                    if (morphInfo.mIdleTime > mCurMaxOpenedTime)
                    {
                        morphInfo.mIdleTime     = 0.0f;
                        morphInfo.mIsBlending   = true;
                    }
                }
                else // it is in a closed state
                {
                    morphInfo.mIdleTime += timeDelta;

                    // if we reached the maximum time we are allowed to have the eye closed
                    if (morphInfo.mIdleTime > mCurMaxClosedTime)
                    {
                        morphInfo.mIdleTime     = 0.0f;
                        morphInfo.mIsBlending   = true;
                    }
                }
            }
            else // we are opening or closing the eye
            {
                morphInfo.mTimeInBlend += timeDelta;
                if (morphInfo.mTimeInBlend >= morphInfo.mBlendTime)
                {
                    morphInfo.mTimeInBlend = morphInfo.mBlendTime;
                }

                const float t = morphInfo.mTimeInBlend / morphInfo.mBlendTime;

                // if we are closing it
                if (morphInfo.mIsOpened)
                {
                    const float weight = MCore::EaseInOutInterpolate<float>(mMinWeight, mMaxWeight, t);
                    morphInfo.mMorphTarget->SetWeight(weight);

                    // if we reached the closed state
                    if (morphInfo.mMorphTarget->GetWeight() >= mMaxWeight - MCore::Math::epsilon)
                    {
                        morphInfo.mIsBlending   = false;
                        morphInfo.mIsOpened     = false;
                        morphInfo.mIdleTime     = 0.0f;
                        morphInfo.mBlendTime    = mCurMaxBlinkTime / 2.0f;
                        morphInfo.mTimeInBlend  = 0.0f;
                    }
                }
                else // we are opening it
                {
                    const float weight = MCore::EaseInOutInterpolate<float>(mMaxWeight, mMinWeight, t);
                    morphInfo.mMorphTarget->SetWeight(weight);

                    // if we reached the opened state
                    if (morphInfo.mMorphTarget->GetWeight() <= mMinWeight + MCore::Math::epsilon)
                    {
                        mCurMaxOpenedTime       = MCore::Random::RandF(mMinOpenedTime, mMaxOpenedTime);
                        mCurMaxClosedTime       = MCore::Random::RandF(mMinClosedTime, mMaxClosedTime);
                        mCurMaxBlinkTime        = MCore::Random::RandF(mMinBlinkTime, mMaxBlinkTime);
                        morphInfo.mIsBlending   = false;
                        morphInfo.mIsOpened     = true;
                        morphInfo.mIdleTime     = 0.0f;
                        morphInfo.mBlendTime    = mCurMaxBlinkTime / 2.0f;
                        morphInfo.mTimeInBlend  = 0.0f;
                    }
                }
            }
        }
    }
}   // namespace EMotionFX
