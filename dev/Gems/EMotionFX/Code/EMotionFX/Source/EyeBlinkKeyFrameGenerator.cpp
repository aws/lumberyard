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
#include "EMotionFXConfig.h"
#include "EyeBlinkKeyFrameGenerator.h"
#include "KeyTrackLinear.h"
#include <MCore/Source/Random.h>


namespace EMotionFX
{
    // constructor
    EyeBlinkKeyFrameGenerator::EyeBlinkKeyFrameGenerator()
        : KeyFrameGenerator<float, MCore::Compressed16BitFloat>()
    {
        mStartTime  = 0;
        mEndTime    = 30.0f;
        mInterval   = 2.0f;
        mRandomness = 1.5f;
        mBlinkSpeed = 0.15f;
    }


    // destructor
    EyeBlinkKeyFrameGenerator::~EyeBlinkKeyFrameGenerator()
    {
    }


    // the creation method
    EyeBlinkKeyFrameGenerator* EyeBlinkKeyFrameGenerator::Create()
    {
        return new EyeBlinkKeyFrameGenerator();
    }


    float EyeBlinkKeyFrameGenerator::AlignToFPS(float timeValue, uint32 framesPerSecond)
    {
        const float dif = MCore::Math::SafeFMod(timeValue, 1.0f / framesPerSecond);
        return timeValue + dif;
    }


    // generate method
    void EyeBlinkKeyFrameGenerator::Generate(KeyTrackLinear<float, MCore::Compressed16BitFloat>* outTrack)
    {
        // precalc some values
        const float halfBlinkSpeed = mBlinkSpeed * 0.5f;

        // start a bit after the first time
        float curTime = mStartTime;

        // add a first key, with eyes open
        outTrack->AddKeySorted(AlignToFPS(mStartTime, 30), 0.0f);

        // while we can still add blinks in the range
        const float lastTime = mEndTime - mInterval - mRandomness;
        while (curTime < lastTime)
        {
            // calculate the displacement on the interval
            const float displacement = MCore::Random::RandF(-mRandomness, mRandomness);

            // calculate the time of the eyblink, when the eyes should be closed
            curTime += mInterval + displacement;

            const float startBlinkTime  = AlignToFPS(curTime - halfBlinkSpeed, 30);
            const float blinkTime       = AlignToFPS(curTime, 30);
            const float endBlinkTime    = AlignToFPS(curTime + halfBlinkSpeed, 30);

            // add the keys to generate a blink
            outTrack->AddKeySorted(startBlinkTime, 0.0f);
            outTrack->AddKeySorted(blinkTime, 1.0f);
            outTrack->AddKeySorted(endBlinkTime, 0.0f);
        }

        // add the last key, with eyes open
        outTrack->AddKeySorted(AlignToFPS(mEndTime, 30), 0.0f);
    }


    // get the description
    const char* EyeBlinkKeyFrameGenerator::GetDescription() const
    {
        return "EyeBlinkKeyFrameGenerator";
    }


    // get the generator ID
    uint32 EyeBlinkKeyFrameGenerator::GetType() const
    {
        return EyeBlinkKeyFrameGenerator::TYPE_ID;
    }


    // setup the properties of the system
    void EyeBlinkKeyFrameGenerator::SetProperties(float startTime, float endTime, float interval, float randomness, float blinkSpeed)
    {
        mStartTime  = startTime;
        mEndTime    = endTime;
        mInterval   = interval;
        mRandomness = randomness;
        mBlinkSpeed = blinkSpeed;
    }


    // get the start time
    float EyeBlinkKeyFrameGenerator::GetStartTime() const
    {
        return mStartTime;
    }


    // get the end time
    float EyeBlinkKeyFrameGenerator::GetEndTime() const
    {
        return mEndTime;
    }


    // get the randomness
    float EyeBlinkKeyFrameGenerator::GetRandomness() const
    {
        return mRandomness;
    }


    // get the interval
    float EyeBlinkKeyFrameGenerator::GetInterval() const
    {
        return mInterval;
    }


    // get the blink speed
    float EyeBlinkKeyFrameGenerator::GetBlinkSpeed() const
    {
        return mBlinkSpeed;
    }
} // namespace EMotionFX
