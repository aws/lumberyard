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
#include <stdafx.h>
#include <QFile>
#include "ResourceCompilerImageUnitTests.h"
#include "ExportSettings.h"
#include "IRCLog.h"
#include "IUnitTestHelper.h"

namespace
{
    void ImageExportSettingsUnitTest(IUnitTestHelper* unitTestHelper)
    {
        const char* testFileName = "myTestFile.txt";
        const char* testExportSettingsFileName = "myTestFile.txt.exportsettings";
        QFile testExportSettingsFile {
            testExportSettingsFileName
        };

        // Ensure the file is gone to start
        if (testExportSettingsFile.exists())
        {
            if (!unitTestHelper->TEST_BOOL(testExportSettingsFile.remove()))
            {
                RCLogError("Unable to remove %s for unit test purposes, unable to unit test ImageExportSettings", testExportSettingsFileName);
                return;
            }
        }

        // Try loading settings that do not exist (this should return false)
        string settings;
        unitTestHelper->TEST_BOOL(!ImageExportSettings::LoadSettings(testFileName, settings));

        // Save some blank settings
        settings = "";
        unitTestHelper->TEST_BOOL(ImageExportSettings::SaveSettings(testFileName, settings));

        // Load the blank settings (won't work because we don't save blank settings files)
        unitTestHelper->TEST_BOOL(!ImageExportSettings::LoadSettings(testFileName, settings));

        // Save some actual settings
        string testSettings = "Some random settings test values";
        unitTestHelper->TEST_BOOL(ImageExportSettings::SaveSettings(testFileName, testSettings));

        // Load the actual settings
        unitTestHelper->TEST_BOOL(ImageExportSettings::LoadSettings(testFileName, settings));
        unitTestHelper->TEST_BOOL(settings.compare(testSettings) == 0);

        // Cleanup
        if (!unitTestHelper->TEST_BOOL(testExportSettingsFile.remove()))
        {
            RCLogWarning("Unable to clean up %s after unit testing.", testExportSettingsFileName);
        }
    }
}

void ResourceCompilerImageUnitTests::RunAllUnitTests(IUnitTestHelper* unitTestHelper)
{
    // Run Image Export Settings Test
    ImageExportSettingsUnitTest(unitTestHelper);

    // ADD MORE TESTS HERE
    // Example: unitTestHelper->TEST_BOOL(1 == 1);
}
