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
#include <ICryMannequinDefs.h>

#include <Serialization/IClassFactory.h>
#include <Serialization/IArchiveHost.h>
#include "Serialization/CRCRef.h"

#include <AzTest/AzTest.h>

namespace mannequin
{
    //////////////////////////////////////////////////////////////////////////
    TEST(CRCRefTests, SCRCRef_Operator_LessThan_NonStoreString)
    {
        {
            const SCRCRef< 0 > src0("A");
            const SCRCRef< 0 > src1("B");

            EXPECT_TRUE(src0 < src1 || src1 < src0);
        }

        {
            const SCRCRef< 0 > src0("A");
            const SCRCRef< 0 > src1("A");

            EXPECT_TRUE(!(src0 < src1) && !(src1 < src0));
        }
    }


    //////////////////////////////////////////////////////////////////////////
    TEST(CRCRefTests, SCRCRef_Operator_LessThan_StoreString)
    {
        {
            const SCRCRef< 1 > src0("A");
            const SCRCRef< 1 > src1("B");

            EXPECT_TRUE(src0 < src1 || src1 < src0);
        }

        {
            const SCRCRef< 1 > src0("A");
            const SCRCRef< 1 > src1("A");

            EXPECT_TRUE(!(src0 < src1) && !(src1 < src0));
        }
    }


    //////////////////////////////////////////////////////////////////////////
    TEST(CRCRefTests, SCRCRef_Operator_LessThan_Mixed)
    {
        {
            const SCRCRef< 1 > src0("A");
            const SCRCRef< 0 > src1("B");

            EXPECT_TRUE(src0 < src1 || src1 < src0);
        }

        {
            const SCRCRef< 0 > src0("A");
            const SCRCRef< 1 > src1("B");

            EXPECT_TRUE(src0 < src1 || src1 < src0);
        }

        {
            const SCRCRef< 0 > src0("A");
            const SCRCRef< 1 > src1("A");

            EXPECT_TRUE(!(src0 < src1) && !(src1 < src0));
        }

        {
            const SCRCRef< 1 > src0("A");
            const SCRCRef< 0 > src1("A");

            EXPECT_TRUE(!(src0 < src1) && !(src1 < src0));
        }
    }


    //////////////////////////////////////////////////////////////////////////
    TEST(CRCRefTests, SCRCRef_Operator_Equal_NonStoreString)
    {
        {
            const SCRCRef< 0 > src0("A");
            const SCRCRef< 0 > src1("B");

            EXPECT_TRUE(!(src0 == src1));
        }

        {
            const SCRCRef< 0 > src0("A");
            const SCRCRef< 0 > src1("A");

            EXPECT_TRUE(src0 == src1);
        }
    }


    //////////////////////////////////////////////////////////////////////////
    TEST(CRCRefTests, SCRCRef_Operator_Equal_StoreString)
    {
        {
            const SCRCRef< 1 > src0("A");
            const SCRCRef< 1 > src1("B");

            EXPECT_TRUE(!(src0 == src1));
        }

        {
            const SCRCRef< 1 > src0("A");
            const SCRCRef< 1 > src1("A");

            EXPECT_TRUE(src0 == src1);
        }
    }


    //////////////////////////////////////////////////////////////////////////
    TEST(CRCRefTests, SCRCRef_Operator_Equal_Mixed)
    {
        {
            const SCRCRef< 0 > src0("A");
            const SCRCRef< 1 > src1("B");

            EXPECT_TRUE(!(src0 == src1));
        }

        {
            const SCRCRef< 1 > src0("A");
            const SCRCRef< 0 > src1("B");

            EXPECT_TRUE(!(src0 == src1));
        }

        {
            const SCRCRef< 0 > src0("A");
            const SCRCRef< 1 > src1("A");

            EXPECT_TRUE(src0 == src1);
        }

        {
            const SCRCRef< 1 > src0("A");
            const SCRCRef< 0 > src1("A");

            EXPECT_TRUE(src0 == src1);
        }
    }


    //////////////////////////////////////////////////////////////////////////
    TEST(CRCRefTests, SCRCRef_Operator_NotEqual_NonStoreString)
    {
        {
            const SCRCRef< 0 > src0("A");
            const SCRCRef< 0 > src1("B");

            EXPECT_TRUE(src0 != src1);
        }

        {
            const SCRCRef< 0 > src0("A");
            const SCRCRef< 0 > src1("A");

            EXPECT_TRUE(!(src0 != src1));
        }
    }


    //////////////////////////////////////////////////////////////////////////
    TEST(CRCRefTests, SCRCRef_Operator_NotEqual_StoreString)
    {
        {
            const SCRCRef< 1 > src0("A");
            const SCRCRef< 1 > src1("B");

            EXPECT_TRUE(src0 != src1);
        }

        {
            const SCRCRef< 1 > src0("A");
            const SCRCRef< 1 > src1("A");

            EXPECT_TRUE(!(src0 != src1));
        }
    }


    //////////////////////////////////////////////////////////////////////////
    TEST(CRCRefTests, SCRCRef_Operator_NotEqual_Mixed)
    {
        {
            const SCRCRef< 0 > src0("A");
            const SCRCRef< 1 > src1("B");

            EXPECT_TRUE(src0 != src1);
        }

        {
            const SCRCRef< 1 > src0("A");
            const SCRCRef< 0 > src1("B");

            EXPECT_TRUE(src0 != src1);
        }

        {
            const SCRCRef< 0 > src0("A");
            const SCRCRef< 1 > src1("A");

            EXPECT_TRUE(!(src0 != src1));
        }

        {
            const SCRCRef< 1 > src0("A");
            const SCRCRef< 0 > src1("A");

            EXPECT_TRUE(!(src0 != src1));
        }
    }
}
