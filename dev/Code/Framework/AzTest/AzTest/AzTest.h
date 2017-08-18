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
#pragma once

#if defined(DARWIN)
// Notes in the Cry* code indicate that strdup may cause memory errors, and shouldn't be
// used. It's required, however, by googletest, so for test builds, un-hack the strdup removal.
#   undef strdup
#endif

#pragma warning( push )
#pragma warning(disable: 4800)  // 'int' : forcing value to bool 'true' or 'false' (performance warning)
#include <gtest/gtest.h>
#pragma warning( pop )
#include <gmock/gmock.h>

#if defined(DARWIN)
#   define AZTEST_EXPORT extern "C" __attribute__ ((visibility("default")))
#else
#   define AZTEST_EXPORT extern "C" __declspec(dllexport)
#endif

namespace AZ
{
    namespace Test
    {
        //! Forward declarations
        class Platform;

        /*!
        * Implement this interface to define the environment setup and teardown functions.
        */
        class ITestEnvironment
            : public ::testing::Environment
        {
        public:
            virtual ~ITestEnvironment()
            {}

            void SetUp() override final
            {
                SetupEnvironment();
            }

            void TearDown() override final
            {
                TeardownEnvironment();
            }

        protected:
            virtual void SetupEnvironment() = 0;
            virtual void TeardownEnvironment() = 0;
        };

        /*!
        * Empty implementation of ITestEnvironment.
        */
        class EmptyTestEnvironment final
            : public ITestEnvironment
        {
        public:
            virtual ~EmptyTestEnvironment()
            {}

        protected:
            void SetupEnvironment() override
            {}
            void TeardownEnvironment() override
            {}
        };

        void addTestEnvironment(ITestEnvironment* env);
        void addTestEnvironments(std::vector<ITestEnvironment*> envs);
        void excludeIntegTests();
        void runOnlyIntegTests();
        void printUnusedParametersWarning(int argc, char** argv);

        /*!
        * Main method for running tests from an executable.
        */
        class AzUnitTestMain final
        {
        public:
            AzUnitTestMain(std::vector<AZ::Test::ITestEnvironment*> envs)
                : m_returnCode(0)
                , m_envs(envs)
            {}

            bool Run(int argc, char** argv);
            bool Run(const char* commandLine);
            int ReturnCode() const { return m_returnCode; }

        private:
            //! Run tests in a single library by loading it dynamically and executing the exported symbol, 
            //! passing main-like parameters (argc, argv) from the (real or artificial) command line.
            int RunTestsInLib(Platform& platform, const std::string& lib, const std::string& symbol, int& argc, char** argv);

            int m_returnCode;
            std::vector<ITestEnvironment*> m_envs;
        };
    } // Test
} // AZ

#define AZ_UNIT_TEST_HOOK_NAME AzRunUnitTests
#define AZ_INTEG_TEST_HOOK_NAME AzRunIntegTests

#if !defined(AZ_MONOLITHIC_BUILD)
// Environments should be declared dynamically, framework will handle deletion of resources    
#define AZ_UNIT_TEST_HOOK(...)                                                      \
    AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)                 \
    {                                                                               \
        ::testing::InitGoogleMock(&argc, argv);                                     \
        AZ::Test::excludeIntegTests();                                              \
        AZ::Test::printUnusedParametersWarning(argc, argv);                         \
        AZ::Test::addTestEnvironments({__VA_ARGS__});                               \
        int result = RUN_ALL_TESTS();                                               \
        return result;                                                              \
    }

// Environments should be declared dynamically, framework will handle deletion of resources
#define AZ_INTEG_TEST_HOOK(...)                                                     \
    AZTEST_EXPORT int AZ_INTEG_TEST_HOOK_NAME(int argc, char** argv)                \
    {                                                                               \
        ::testing::InitGoogleMock(&argc, argv);                                     \
        AZ::Test::runOnlyIntegTests();                                              \
        AZ::Test::printUnusedParametersWarning(argc, argv);                         \
        AZ::Test::addTestEnvironments({__VA_ARGS__});                               \
        int result = RUN_ALL_TESTS();                                               \
        return result;                                                              \
    }

#else // monolithic build

// Monolithic builds do not support individual test hooks
#define AZ_UNIT_TEST_HOOK(...) ;
#define AZ_INTEG_TEST_HOOK(...) ;

#endif // AZ_MONOLITHIC_BUILD

// Declares a visible external symbol which identifies an executable as containing tests
#define DECLARE_AZ_UNIT_TEST_MAIN() AZTEST_EXPORT int ContainsAzUnitTestMain() { return 1; }

// Attempts to invoke the unit test main function if appropriate flags are present,
// otherwise simply continues launch as normal.
// Implementation depends on if command line is an array already or not.
#define INVOKE_AZ_UNIT_TEST_MAIN(...)                                               \
    do {                                                                            \
        AZ::Test::AzUnitTestMain unitTestMain({__VA_ARGS__});                       \
        if (unitTestMain.Run(argc, argv))                                           \
        {                                                                           \
            return unitTestMain.ReturnCode();                                       \
        }                                                                           \
    } while (0); // safe multi-line macro - creates a single statement

// Convenience macro for prepending Integ_ for an integration test that doesn't need a fixture
#define INTEG_TEST(test_case_name, test_name) GTEST_TEST(Integ_##test_case_name, test_name)

// Avoid accidentally being managed by CryMemory, or problems with new/delete when
// AZ allocators are not ready or properly un/initialized.
#define AZ_TEST_CLASS_ALLOCATOR(Class_)                                 \
    void* operator new (size_t size)                                    \
    {                                                                   \
        return AZ_OS_MALLOC(size, AZStd::alignment_of<Class_>::value);  \
    }                                                                   \
    void operator delete(void* ptr)                                     \
    {                                                                   \
        AZ_OS_FREE(ptr);                                                \
    }
