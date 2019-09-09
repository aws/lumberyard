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

#include "EMotionFX_precompiled.h"

#include <AzTest/AzTest.h>

AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    AZ::Test::excludeIntegTests();
    AZ::Test::printUnusedParametersWarning(argc, argv);
    AZ::Test::addTestEnvironments({});
    int result = RUN_ALL_TESTS();
#if defined(WIN32) || defined(WIN64)
    TerminateProcess(GetCurrentProcess(), result);
#else
    _exit(result);
#endif

#ifdef WIN32
    //Post a WM_QUIT message to the Win32 api which causes the message loop to END
    //This is not the same as handling a WM_DESTROY event which destroys a window
    //but keeps the message loop alive.
    PostQuitMessage(0);
#endif
    return result;
}
