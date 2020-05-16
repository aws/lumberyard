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
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>

#include <QApplication>

AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    // NOTE - these lines are what make this code different from AZ_UNIT_TEST_HOOK()
    AzQtComponents::PrepareQtPaths();
    QApplication app(argc, argv);
    // end
    AZ::Test::excludeIntegTests();
    AZ::Test::ApplyGlobalParameters(&argc, argv);
    AZ::Test::printUnusedParametersWarning(argc, argv);
    AZ::Test::addTestEnvironments({});
    int result = RUN_ALL_TESTS();
    return result;
}