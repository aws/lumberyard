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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CryLegacy_precompiled.h"
#include <ICryMannequin.h>
#include <Mannequin/Serialization.h>

#include <AzTest/AzTest.h>

namespace mannequin
{
    //////////////////////////////////////////////////////////////////////////
    TEST(ProceduralParamsTests, IProceduralParams_StringWrapper_EmptyConstructor)
    {
        const IProceduralParams::StringWrapper s;
        EXPECT_TRUE(string("") == s.c_str());
    }

    TEST(ProceduralParamsTests, IProceduralParams_StringWrapper_Constructor)
    {
        const IProceduralParams::StringWrapper s("test");
        EXPECT_TRUE(string("test") == s.c_str());
    }

    TEST(ProceduralParamsTests, IProceduralParams_StringWrapper_OperatorEqual)
    {
        IProceduralParams::StringWrapper s("test");
        EXPECT_TRUE(string("test") == s.c_str());

        s = "t";
        EXPECT_TRUE(string("t") == s.c_str());

        s = "test";
        EXPECT_TRUE(string("test") == s.c_str());

        IProceduralParams::StringWrapper s2;
        EXPECT_TRUE(string("") == s2.c_str());

        s2 = s = IProceduralParams::StringWrapper("test2");
        EXPECT_TRUE(string("test2") == s.c_str());
        EXPECT_TRUE(string("test2") == s2.c_str());
    }

    TEST(ProceduralParamsTests, IProceduralParams_StringWrapper_GetLength)
    {
        IProceduralParams::StringWrapper s("test");
        EXPECT_TRUE(4 == s.GetLength());

        s = "t";
        EXPECT_TRUE(1 == s.GetLength());

        s = "test";
        EXPECT_TRUE(4 == s.GetLength());

        const IProceduralParams::StringWrapper s2("test2");
        EXPECT_TRUE(5 == s2.GetLength());
    }

    TEST(ProceduralParamsTests, IProceduralParams_StringWrapper_IsEmpty)
    {
        {
            IProceduralParams::StringWrapper s("test");
            EXPECT_TRUE(!s.IsEmpty());

            s = "";
            EXPECT_TRUE(s.IsEmpty());

            s = "test";
            EXPECT_TRUE(!s.IsEmpty());
        }

        {
            const IProceduralParams::StringWrapper s("test");
            EXPECT_TRUE(!s.IsEmpty());
        }

        {
            const IProceduralParams::StringWrapper s("");
            EXPECT_TRUE(s.IsEmpty());
        }

        {
            const IProceduralParams::StringWrapper s;
            EXPECT_TRUE(s.IsEmpty());
        }
    }
}