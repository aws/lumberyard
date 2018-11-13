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


namespace AZ
{
    class ReflectContext;
}

namespace StarterGameGem
{
    /*!
    * Wrapper for time of day utility functions exposed to Lua for StarterGame.
    */
    class StarterGameTimeOfDayUtility
    {
    public:
        AZ_TYPE_INFO(StarterGameTimeOfDayUtility, "{A54E9CC3-D351-44A9-86FD-0065346A0FAC}");
        AZ_CLASS_ALLOCATOR(StarterGameTimeOfDayUtility, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflection);

        static void SetTimeOfDayPaused(bool pause);

        static void SetTimeOfDaySpeed(float speed);

        static float GetTimeOfDay();
        static void SetTimeOfDay(float hour, bool forceUpdate = false, bool forceEnvUpdate = true);

        static float GetSunLatitude();
        static float GetSunLongitude();
        static void SetSunPosition(float longitude, float latitude);
    };
} // namespace StarterGameGem
