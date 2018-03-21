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
#include "TestTypes.h"

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/string.h>

using namespace AZ;
using namespace AZ::IO;
using namespace AZ::Debug;

#if   defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE_OSX)
#   define AZ_ROOT_TEST_FOLDER  "./"
#elif defined(AZ_PLATFORM_ANDROID)
#   define AZ_ROOT_TEST_FOLDER  "/sdcard/"
#elif defined(AZ_PLATFORM_APPLE_IOS)
#   define AZ_ROOT_TEST_FOLDER "/Documents/"
#elif defined(AZ_PLATFORM_APPLE_TV)
#   define AZ_ROOT_TEST_FOLDER "/Library/Caches/"
#else
#   define AZ_ROOT_TEST_FOLDER  ""
#endif

namespace // anonymous
{
#if defined(AZ_PLATFORM_APPLE_IOS) || defined(AZ_PLATFORM_APPLE_TV)
    AZStd::string GetTestFolderPath()
    {
        return AZStd::string(getenv("HOME")) + AZ_ROOT_TEST_FOLDER;
    }
#else
    AZStd::string GetTestFolderPath()
    {
        return AZ_ROOT_TEST_FOLDER;
    }
#endif
} // anonymous namespace

namespace UnitTest
{
    /**
     * systemFile test
     */
    class SystemFileTest
        : public AllocatorsFixture
    {
    public:
        void run()
        {
            // Test Deleting a non-existent file
            AZStd::string invalidFileName = GetTestFolderPath() + "InvalidFileName";
            AZ_TEST_ASSERT(SystemFile::Exists(invalidFileName.c_str()) == false);
            AZ_TEST_ASSERT(SystemFile::Delete(invalidFileName.c_str()) == false);

            //Files that don't exist are not considered writable
            AZ_TEST_ASSERT(SystemFile::IsWritable(invalidFileName.c_str()) == false);

            AZStd::string testFile = GetTestFolderPath() + "SystemFileTest.txt";
            AZStd::string testString = "Hello";

            // If file exists start by deleting the file
            SystemFile::Delete(testFile.c_str());
            AZ_TEST_ASSERT(!SystemFile::Exists(testFile.c_str()));

            // Test Writing a file
            {
                SystemFile oFile;
                oFile.Open(testFile.c_str(), SystemFile::SF_OPEN_CREATE | SystemFile::SF_OPEN_WRITE_ONLY);
                oFile.Write(testString.c_str(), testString.length() + 1);
                AZ_TEST_ASSERT(oFile.Tell() == testString.length() + 1);
                AZ_TEST_ASSERT(oFile.Eof());
                oFile.Flush();
                oFile.Close();
                AZ_TEST_ASSERT(SystemFile::Exists(testFile.c_str()));
            }
            // Test Reading from the file
            {
                SystemFile iFile;
                iFile.Open(testFile.c_str(), SystemFile::SF_OPEN_READ_ONLY);
                AZ_TEST_ASSERT(iFile.IsOpen());
                char* buffer = (char*)azmalloc(testString.length() + 1);
                SystemFile::SizeType nRead = iFile.Read(testString.length() + 1, buffer);
                AZ_TEST_ASSERT(nRead == testString.length() + 1);
                AZ_TEST_ASSERT(strcmp(testString.c_str(), buffer) == 0);
                AZ_TEST_ASSERT(iFile.Tell() == testString.length() + 1);
                AZ_TEST_ASSERT(iFile.Eof());
                iFile.Close();
                AZ_TEST_ASSERT(!iFile.IsOpen());
                azfree(buffer);
            }

            //Test Appending to the file
            {
                SystemFile oFile;
                oFile.Open(testFile.c_str(), SystemFile::SF_OPEN_APPEND | SystemFile::SF_OPEN_WRITE_ONLY);
                SystemFile::SizeType initialSize = oFile.Length();
                oFile.Write(testString.c_str(), testString.length() + 1);
                oFile.Write(testString.c_str(), testString.length() + 1);
                AZ_TEST_ASSERT(oFile.Tell() == initialSize + (testString.length() + 1) * 2);
                AZ_TEST_ASSERT(oFile.Eof());
                oFile.Flush();
                oFile.Close();
                AZ_TEST_ASSERT(SystemFile::Exists(testFile.c_str()));
            }

            // Test Reading from the file
            {
                SystemFile iFile;
                iFile.Open(testFile.c_str(), SystemFile::SF_OPEN_READ_ONLY);
                AZ_TEST_ASSERT(iFile.IsOpen());
                size_t len = testString.length() + 1;
                char* buffer = (char*)azmalloc(len * 3);
                SystemFile::SizeType nRead = iFile.Read(len * 3, buffer);
                AZ_TEST_ASSERT(nRead == len * 3);
                AZ_TEST_ASSERT(strcmp(testString.c_str(), buffer) == 0);
                AZ_TEST_ASSERT(strcmp(testString.c_str(), buffer + len) == 0);
                AZ_TEST_ASSERT(strcmp(testString.c_str(), buffer + (len * 2)) == 0);
                AZ_TEST_ASSERT(iFile.Tell() == nRead);
                AZ_TEST_ASSERT(iFile.Eof());
                iFile.Close();
                AZ_TEST_ASSERT(!iFile.IsOpen());
                azfree(buffer);
            }


            //File should exist now, and be writable
            AZ_TEST_ASSERT(SystemFile::IsWritable(testFile.c_str()) == true);
#if AZ_TRAIT_OS_CAN_SET_FILES_WRITABLE
            SystemFile::SetWritable(testFile.c_str(), false);
            AZ_TEST_ASSERT(SystemFile::IsWritable(testFile.c_str()) == false);
            SystemFile::SetWritable(testFile.c_str(), true);
            AZ_TEST_ASSERT(SystemFile::IsWritable(testFile.c_str()) == true);
#endif

            // Now that the file exists, verify that we can delete it
            bool deleted = SystemFile::Delete(testFile.c_str());
            AZ_TEST_ASSERT(deleted);
            AZ_TEST_ASSERT(!SystemFile::Exists(testFile.c_str()));
        }
    };

    TEST_F(SystemFileTest, Test)
    {
        run();
    }
}   // namespace UnitTest
