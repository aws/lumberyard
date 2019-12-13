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

#include <AzCore/Memory/AllocatorManager.h>
#include <AzFramework/CommandLine/CommandLine.h>

#include <iostream>

#if defined(AZ_TESTS_ENABLED)
#include <AzTest/AzTest.h>
DECLARE_AZ_UNIT_TEST_MAIN()
#endif

void OutputArgs(const AzFramework::CommandLine& commandLine)
{
    const AzFramework::CommandLine::ParamMap& switchList = commandLine.GetSwitchList();
    std::cout << "Switch List:" << std::endl;
    for (auto& switchPair : switchList)
    {
        // We strip white space from all of our switch names, so "flush" names will start arguments
        std::cout << switchPair.first.c_str() << std::endl;
        for (auto& switchValue : switchPair.second)
        {
            // Auto space every switch value by one space
            std::cout << " " << switchValue.c_str() << std::endl;
        }
    }
    std::cout << "End Switch List:" << std::endl;
}

int main(int argc, char** argv)
{
#if defined(AZ_TESTS_ENABLED)
    INVOKE_AZ_UNIT_TEST_MAIN();
#endif
    AZ::AllocatorInstance<AZ::OSAllocator>::Create();
    AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

    {
        AzFramework::CommandLine commandLine;

        commandLine.Parse(argc, argv);
        OutputArgs(commandLine);
    }

    AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
    return 0;
}
