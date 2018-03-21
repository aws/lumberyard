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

#include "StarterGameGem_precompiled.h"
#include "StarterGameTimeOfDayUtility.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include <ITimeOfDay.h>


namespace StarterGameGem
{
    void StarterGameTimeOfDayUtility::SetTimeOfDayPaused(bool pause)
    {
        gEnv->p3DEngine->GetTimeOfDay()->SetPaused(pause);
    }

    void StarterGameTimeOfDayUtility::SetTimeOfDaySpeed(float speed)
    {
        ITimeOfDay::SAdvancedInfo info;
        gEnv->p3DEngine->GetTimeOfDay()->GetAdvancedInfo(info);
        
        info.fAnimSpeed = speed;
        gEnv->p3DEngine->GetTimeOfDay()->SetAdvancedInfo(info);
    }

    float StarterGameTimeOfDayUtility::GetTimeOfDay()
    {
        return gEnv->p3DEngine->GetTimeOfDay()->GetTime();
    }

    void StarterGameTimeOfDayUtility::SetTimeOfDay(float hour, bool forceUpdate /*= false*/, bool forceEnvUpdate /*= true*/)
    {
        gEnv->p3DEngine->GetTimeOfDay()->SetTime(hour, forceUpdate, forceEnvUpdate);
    }

    float StarterGameTimeOfDayUtility::GetSunLatitude()
    {
        return gEnv->p3DEngine->GetTimeOfDay()->GetSunLatitude();
    }

    float StarterGameTimeOfDayUtility::GetSunLongitude()
    {
        return gEnv->p3DEngine->GetTimeOfDay()->GetSunLongitude();
    }

    void StarterGameTimeOfDayUtility::SetSunPosition(float longitude, float latitude)
    {
        gEnv->p3DEngine->GetTimeOfDay()->SetSunPos(longitude, latitude);
    }

    void StarterGameTimeOfDayUtility::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->Class<StarterGameTimeOfDayUtility>("StarterGameTimeOfDayUtility")
                ->Method("SetTimeOfDayPaused", &SetTimeOfDayPaused)

                ->Method("SetTimeOfDaySpeed", &SetTimeOfDaySpeed)

                ->Method("GetTimeOfDay", &GetTimeOfDay)
                ->Method("SetTimeOfDay", &SetTimeOfDay)

                ->Method("GetSunLatitude", &GetSunLatitude)
                ->Method("GetSunLongitude", &GetSunLongitude)
                ->Method("SetSunPosition", &SetSunPosition)
            ;
        }

    }

}
