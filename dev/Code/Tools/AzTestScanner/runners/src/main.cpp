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
#include <iostream>
#include <string>
#include <memory>

#define VERBOSE 1

#if AZ_TESTS_ENABLED
#   include <AzTest/AzTest.h>
#   include <AzTest/Platform.h>
#   include <AzCore/IO/SystemFile.h>
#else
#   define AZ_MAX_PATH_LEN 1024
#endif // AZ_TESTS_ENABLED

#if defined(AZ_PLATFORM_WINDOWS)
#    define getcwd _getcwd // stupid MSFT "deprecation" warning
#endif

namespace
{
    const int INCORRECT_USAGE = 101;
    const int LIB_NOT_FOUND = 102;
    const int SYMBOL_NOT_FOUND = 103;
    const char* INTEG_BOOTSTRAP = "AzTestIntegBootstrap";
}

#if AZ_TESTS_ENABLED
DECLARE_AZ_UNIT_TEST_MAIN()
#endif // AZ_TESTS_ENABLED

//! display proper usage of the application
void usage(AZ::Test::Platform& platform)
{
    std::cerr << "AzTestRunner" << std::endl;
    std::cerr << "Runs AZ unit and integration tests. Exit code is the result from GoogleTest." << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage:" << std::endl;
    std::cerr << "   AzTestRunner.exe <lib> <symbol> [--integ] [--wait-for-debugger] [google-test-args]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options:" << std::endl;
    std::cerr << "   --integ: tells runner to bootstrap the engine for integration tests" << std::endl;
    if (platform.SupportsWaitForDebugger())
    {
        std::cerr << "   --wait-for-debugger: tells runner to wait for debugger to attach to process" << std::endl;
    }
    std::cerr << "   --pause-on-completion: tells the runner to pause after running the tests" << std::endl;
    std::cerr << std::endl;
    std::cerr << "exit codes:" << std::endl;
    std::cerr << "   0 - all tests pass" << std::endl;
    std::cerr << "   1 - test failure" << std::endl;
    std::cerr << "   " << INCORRECT_USAGE << " - incorrect usage (see above)" << std::endl;
    std::cerr << "   " << LIB_NOT_FOUND << " - library/dll could not be loaded" << std::endl;
    std::cerr << "   " << SYMBOL_NOT_FOUND << " - export symbol not found" << std::endl;
    std::cerr << std::endl;
}

//! attempt to run the int X() method exported by the specified library
int main(int argc, char** argv)
{
#if AZ_TESTS_ENABLED
    INVOKE_AZ_UNIT_TEST_MAIN();
#endif

    AZ::Test::Platform& platform = AZ::Test::GetPlatform();

    if (argc < 3)
    {
        usage(platform);
        return INCORRECT_USAGE;
    }
    
#if VERBOSE
    char cwd[AZ_MAX_PATH_LEN] = { '\0' };
    getcwd(cwd, sizeof(cwd));
    std::cout << "cwd = " << cwd << std::endl;

    for (int i=0; i<argc; i++)
    {
        std::cout << "arg[" << i << "] " << argv[i] << std::endl;
    }
#endif

    // capture positional arguments
    // [0] is the program name
    std::string lib = argv[1];
    std::string symbol = argv[2];

    // shift argv parameters down as we don't need lib or symbol anymore
    AZ::Test::RemoveParameters(argc, argv, 1, 2);

    // capture optional arguments
    bool waitForDebugger = false;
    bool isInteg = false;
    bool pauseOnCompletion = false;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--wait-for-debugger") == 0)
        {
            waitForDebugger = true;
            AZ::Test::RemoveParameters(argc, argv, i, i);
            i--;
        }
        else if (strcmp(argv[i], "--integ") == 0)
        {
            isInteg = true;
            AZ::Test::RemoveParameters(argc, argv, i, i);
            i--;
        }
        else if (strcmp(argv[i], "--pause-on-completion") == 0)
        {
            pauseOnCompletion = true;
            AZ::Test::RemoveParameters(argc, argv, i, i);
            i--;
        }
    }

    std::cout << "LIB: " << lib << std::endl;

    // Wait for debugger
    if (waitForDebugger)
    {
        if (platform.SupportsWaitForDebugger())
        {
            std::cout << "Waiting for debugger..." << std::endl;
            platform.WaitForDebugger();
        }
        else
        {
            std::cerr << "Warning - platform does not support --wait-for-debugger feature" << std::endl;
        }
    }

    // Grab a bootstrapper library if requested
    std::shared_ptr<AZ::Test::IModuleHandle> bootstrap;
    if (isInteg)
    {
        bootstrap = platform.GetModule(INTEG_BOOTSTRAP);
        if (!bootstrap->IsValid())
        {
            std::cerr << "FAILED to load bootstrapper" << std::endl;
            return LIB_NOT_FOUND;
        }

        // Initialize the bootstrapper
        auto init = bootstrap->GetFunction("Initialize");
        if (init->IsValid())
        {
            int result = (*init)();
            if (result != 0)
            {
                std::cerr << "Could not initialize bootstrapper, exiting" << std::endl;
                return result;
            }
        }
    }

    platform.SuppressPopupWindows();
    
    int result = 0;
    {
        std::shared_ptr<AZ::Test::IModuleHandle> module = platform.GetModule(lib);
        if (module->IsValid())
        {
            std::cout << "OKAY Library loaded: " << lib << std::endl;
            
            auto fn = module->GetFunction(symbol);
            if (fn->IsValid())
            {
                std::cout << "OKAY Symbol found: " << symbol << std::endl;
                
                result = (*fn)(argc, argv);
                
                std::cout << "OKAY " << symbol << "() returned " << result << std::endl;
            }
            else
            {
                std::cerr << "FAILED to find symbol: " << symbol << std::endl;
                result = SYMBOL_NOT_FOUND;
            }
        }
        else
        {
            std::cerr << "FAILED to load library: " << lib << std::endl;
            result = LIB_NOT_FOUND;
        }
    }

    // Shutdown the bootstrapper
    if (bootstrap)
    {
        auto shutdown = bootstrap->GetFunction("Shutdown");
        if (shutdown->IsValid())
        {
            int result = (*shutdown)();
            if (result != 0)
            {
                std::cerr << "Bootstrapper shutdown failed, exiting" << std::endl;
                return result;
            }
        }
        bootstrap.reset();
    }

    std::cout << "Returning code: " << result << std::endl;

    if (pauseOnCompletion)
    {
        system("pause");
    }

    return result;
}
