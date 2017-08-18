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

// we only want to do the tests for the dll version of our lib. Otherwise, they'd get run twice
#if defined(AZ_QT_COMPONENTS_EXPORT_SYMBOLS)

#include <AzTest/AzTest.h>

AZ_UNIT_TEST_HOOK();

TEST(AzQtComponentsTest, Sanity)
{
    EXPECT_EQ(1, 1);
}

#endif // defined(AZ_QT_COMPONENTS_EXPORT_SYMBOLS)