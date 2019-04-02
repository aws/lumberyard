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

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define AZTEST_H_SECTION_1 1
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION AZTEST_H_SECTION_1
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/AzTest_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/AzTest_h_provo.inl"
    #endif
#elif defined(DARWIN) || defined(ANDROID) || defined(LINUX)
#define AZTEST_H_TRAITS_UNDEF_STRDUP 1
#endif

#if AZTEST_H_TRAITS_UNDEF_STRDUP
// Notes in the Cry* code indicate that strdup may cause memory errors, and shouldn't be
// used. It's required, however, by googletest, so for test builds, un-hack the strdup removal.
#   undef strdup
#endif // AZTEST_H_TRAITS_UNDEF_STRDUP

#pragma warning( push )
#pragma warning(disable: 4800)  // 'int' : forcing value to bool 'true' or 'false' (performance warning)
#include <gtest/gtest.h>
#pragma warning( pop )
#include <gmock/gmock.h>

#include <AzCore/Memory/OSAllocator.h>

#if defined(DARWIN) || defined(ANDROID) || defined(LINUX)
#   define AZTEST_DLL_PUBLIC __attribute__ ((visibility ("default")))
#else
#   define AZTEST_DLL_PUBLIC
#endif


#if defined(DARWIN) || defined(ANDROID) || defined(LINUX)
#   define AZTEST_EXPORT extern "C" AZTEST_DLL_PUBLIC
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
        * Monolithic builds will have all the environments available.  Keep a mapping to run the desired envs.
        */
        class TestEnvironmentRegistry
        {
        public:
            TestEnvironmentRegistry(std::vector<ITestEnvironment*> a_envs, const std::string& a_module_name, bool a_unit)
                : m_module_name(a_module_name)
                , m_envs(a_envs)
                , m_unit(a_unit)
            {
                s_envs.push_back(this);
            }

            const std::string m_module_name;
            std::vector<ITestEnvironment*> m_envs;
            bool m_unit;

            static std::vector<TestEnvironmentRegistry*> s_envs;

        private:
            TestEnvironmentRegistry& operator=(const TestEnvironmentRegistry& tmp);
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
            int m_returnCode;
            std::vector<ITestEnvironment*> m_envs;
        };

        //! Run tests in a single library by loading it dynamically and executing the exported symbol,
        //! passing main-like parameters (argc, argv) from the (real or artificial) command line.
        int RunTestsInLib(Platform& platform, const std::string& lib, const std::string& symbol, int& argc, char** argv);

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

#undef GTEST_MODULE_NAME_
#define GTEST_MODULE_NAME_ AZ_MODULE_NAME
#define AZTEST_CONCAT_(a, b) a ## _ ## b
#define AZTEST_CONCAT(a, b) AZTEST_CONCAT_(a, b)

#define AZ_UNIT_TEST_HOOK_REGISTRY_NAME AZTEST_CONCAT(AZ_UNIT_TEST_HOOK_NAME, Registry)
#define AZ_INTEG_TEST_HOOK_REGISTRY_NAME AZTEST_CONCAT(AZ_INTEG_TEST_HOOK_NAME, Registry)

#define AZ_UNIT_TEST_HOOK(...)                                                      \
            static AZ::Test::TestEnvironmentRegistry* AZ_UNIT_TEST_HOOK_REGISTRY_NAME =\
                new( AZ_OS_MALLOC(sizeof(AZ::Test::TestEnvironmentRegistry),           \
                                  alignof(AZ::Test::TestEnvironmentRegistry)))         \
              AZ::Test::TestEnvironmentRegistry({ __VA_ARGS__ }, AZ_MODULE_NAME, true);         

#define AZ_INTEG_TEST_HOOK(...)                                                     \
            static AZ::Test::TestEnvironmentRegistry* AZ_INTEG_TEST_HOOK_REGISTRY_NAME =\
                new( AZ_OS_MALLOC(sizeof(AZ::Test::TestEnvironmentRegistry),           \
                                  alignof(AZ::Test::TestEnvironmentRegistry)))         \
              AZ::Test::TestEnvironmentRegistry({ __VA_ARGS__ }, AZ_MODULE_NAME, false); 
#endif // AZ_MONOLITHIC_BUILD

// Declares a visible external symbol which identifies an executable as containing tests
#define DECLARE_AZ_UNIT_TEST_MAIN() AZTEST_EXPORT int ContainsAzUnitTestMain() { return 1; }

// Attempts to invoke the unit test main function if appropriate flags are present,
// otherwise simply continues launch as normal.
#define INVOKE_AZ_UNIT_TEST_MAIN(...)                                               \
    do {                                                                            \
        AZ::Test::AzUnitTestMain unitTestMain({__VA_ARGS__});                       \
        if (unitTestMain.Run(argc, argv))                                           \
        {                                                                           \
            return unitTestMain.ReturnCode();                                       \
        }                                                                           \
    } while (0); // safe multi-line macro - creates a single statement

// Some implementations use a commandLine rather than argc/argv
#define INVOKE_AZ_UNIT_TEST_MAIN_COMMAND_LINE(...)                                  \
    do {                                                                            \
        AZ::Test::AzUnitTestMain unitTestMain({__VA_ARGS__});                       \
        if (unitTestMain.Run(commandLine))                                          \
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
