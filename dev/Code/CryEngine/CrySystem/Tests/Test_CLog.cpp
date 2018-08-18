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
#include "StdAfx.h"

#include <AzTest/AzTest.h>
#include <Log.h>

#include <AzCore/IO/SystemFile.h> // for max path decl
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <CryPakFileIO.h>
#include <AzCore/std/functional.h> // for function<> in the find files callback.

namespace CLogUnitTests
{
    // for fuzzing test, how much work to do?
    static int s_numTrialsToPerform = 65535; // as much we can do in about a second.

    class Integ_CLogUnitTests
        : public ::testing::Test
        , public UnitTest::TraceBusRedirector
    {
        void SetUp() override
        {
            BusConnect();
        }
        
        void TearDown() override
        {
            BusDisconnect();
        }
    };

    TEST_F(Integ_CLogUnitTests, LogAlways_InvalidString_Asserts)
    {
        AZ_TEST_START_ASSERTTEST;
        CLog testLog(gEnv->pSystem);
        testLog.LogAlways(nullptr);
        AZ_TEST_STOP_ASSERTTEST(1);
    }

    TEST_F(Integ_CLogUnitTests, LogAlways_EmptyString_IgnoresWithoutCrashing)
    {
        CLog testLog(gEnv->pSystem);
        testLog.LogAlways("");
    }

    TEST_F(Integ_CLogUnitTests, LogAlways_NormalString_NoFileName_DoesNotCrash)
    {
        CLog testLog(gEnv->pSystem);
        testLog.LogAlways("test");
    }

    TEST_F(Integ_CLogUnitTests, LogAlways_SetFileName_Empty_DoesNotCrash)
    {
        CLog testLog(gEnv->pSystem);
        testLog.SetFileName("", false);
        testLog.LogAlways("test");
    }

    TEST_F(Integ_CLogUnitTests, LogAlways_FuzzTest)
    {
        CLog testLog(gEnv->pSystem);
        AZStd::string randomJunkName;

        randomJunkName.resize(128, '\0');

        for (int trialNumber = 0; trialNumber < s_numTrialsToPerform; ++trialNumber)
        {
            for (int randomChar = 0; randomChar < randomJunkName.size(); ++randomChar)
            {
                // note that this is intentionally allowing null characters to generate.
                // note that this also puts characters AFTER the null, if a null appears in the mddle.
                // so that if there are off by one errors they could include cruft afterwards.

                if (randomChar > trialNumber % randomJunkName.size())
                {
                    // choose this point for the nulls to begin.  It makes sure we test every size of string.
                    randomJunkName[randomChar] = 0;
                }
                else
                {
                    randomJunkName[randomChar] = (char)(rand() % 256); // this will trigger invalid UTF8 decoding too
                }
            }
            testLog.LogAlways("%s", randomJunkName.c_str());
        }
    }

} // end namespace CLogUnitTests


