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
#include "Tests.h"
#include <GridMate/Achievements/AchievementMgr.h>
#include <AzCore/std/parallel/thread.h>


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define ACHIEVEMENTMGR_CPP_SECTION_1 1
#define ACHIEVEMENTMGR_CPP_SECTION_2 2
#define ACHIEVEMENTMGR_CPP_SECTION_3 3
#define ACHIEVEMENTMGR_CPP_SECTION_4 4
#define ACHIEVEMENTMGR_CPP_SECTION_5 5
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION ACHIEVEMENTMGR_CPP_SECTION_1
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/AchievementMgr_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/AchievementMgr_cpp_provo.inl"
    #endif
#endif

using namespace GridMate;

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION ACHIEVEMENTMGR_CPP_SECTION_2
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/AchievementMgr_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/AchievementMgr_cpp_provo.inl"
    #endif
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION ACHIEVEMENTMGR_CPP_SECTION_3
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/AchievementMgr_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/AchievementMgr_cpp_provo.inl"
    #endif
#endif

AZ_TEST_SUITE(AchievementSuite)

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION ACHIEVEMENTMGR_CPP_SECTION_4
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/AchievementMgr_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/AchievementMgr_cpp_provo.inl"
    #endif
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION ACHIEVEMENTMGR_CPP_SECTION_5
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/AchievementMgr_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/AchievementMgr_cpp_provo.inl"
    #endif
#endif

AZ_TEST_SUITE_END
