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

#include <stdio.h>
#include <setjmp.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Base.h>
#include <AzCore/std/typetraits/alignment_of.h>


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define STANDALONEUNITTEST_H_SECTION_1 1
#define STANDALONEUNITTEST_H_SECTION_2 2
#define STANDALONEUNITTEST_H_SECTION_3 3
#define STANDALONEUNITTEST_H_SECTION_4 4
#endif

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID)
#   include <malloc.h>
#elif defined(AZ_PLATFORM_APPLE)
#    include <malloc/malloc.h>
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION STANDALONEUNITTEST_H_SECTION_1
#include AZ_RESTRICTED_FILE(StandaloneUnitTest_h, AZ_RESTRICTED_PLATFORM)
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// We need a debug allocator... to guarantee we will not call new or anything
// because they might me overloaded
namespace UnitTest
{
    void* DebugAlignAlloc(size_t byteSize, size_t alignment);
    void DebugAlignFree(void* ptr);

    template<class T>
    class DebugAllocatorSTL
    {
    public:
        typedef T        value_type;
        typedef T*       pointer;
        typedef const T* const_pointer;
        typedef T&       reference;
        typedef const T& const_reference;

        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;

        template<class Other>
        struct rebind
        {
            typedef DebugAllocatorSTL<Other> other;
        };

        pointer address(reference value) const { return (&value); }
        const_pointer address(const_reference value) const { return (&value); }
        DebugAllocatorSTL() {}  // construct default allocator (do nothing)
        DebugAllocatorSTL(const DebugAllocatorSTL<T>&) {}

        template<class Other>
        DebugAllocatorSTL(const DebugAllocatorSTL<Other>&) {}

        template<class Other>
        DebugAllocatorSTL<T>& operator=(const DebugAllocatorSTL<Other>&) { return *this; }

        pointer allocate(size_type count)
        {
            return (pointer)DebugAlignAlloc(count * sizeof(T), AZStd::alignment_of<T>::value);
        }
        void deallocate(pointer ptr, size_type)
        {
            DebugAlignFree(ptr);
        }
        pointer allocate(size_type count, const void*) { return allocate(count); }
        void construct(pointer ptr, const T& value) { new(ptr)    T(value); }
        void destroy(pointer ptr) { (void)ptr; ptr->~T(); }
        size_type max_size() const { return 1000000; }
    };

    template<class T>
    AZ_FORCE_INLINE bool operator==(const DebugAllocatorSTL<T>& a, const DebugAllocatorSTL<T>& b)
    {
        (void)a;
        (void)b;
        return true;
    }

    template<class T>
    AZ_FORCE_INLINE bool operator!=(const DebugAllocatorSTL<T>& a, const DebugAllocatorSTL<T>& b)
    {
        (void)a;
        (void)b;
        return false;
    }
}

#ifdef AZ_COMPILER_MSVC
#   pragma warning( push )
#   pragma warning( disable : 4275 ) // warning C4275: non dll-interface class 'stdext::exception' used as base for dll-interface class 'std::bad_cast'
#   pragma warning( disable : 4530 ) // warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
#endif
#include <vector>
#ifdef AZ_COMPILER_MSVC
#   pragma warning( pop )
#endif
#define TESTS_CONTAINER(_Type) std::vector < _Type, UnitTest::DebugAllocatorSTL<_Type> >

namespace UnitTest
{
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION STANDALONEUNITTEST_H_SECTION_2
#include AZ_RESTRICTED_FILE(StandaloneUnitTest_h, AZ_RESTRICTED_PLATFORM)
#endif


    class TestRunner
    {
        struct Assert
        {
            bool        m_isSuccess;
            const char* m_expression;
            const char* m_fileName;
            int         m_lineNumer;
        };
        struct Test
        {
            Test()
                : m_name(0)
                , m_time(0.0f)
                , m_isSuccess(false)
                , m_numAsserts(0) {}
            Test(const char* name)
                : m_name(name)
                , m_time(0.0f)
                , m_isSuccess(false)
                , m_numAsserts(0) {}
            const char*     m_name;
            float           m_time; // time to run the test in seconds
            bool            m_isSuccess;
            Assert          m_assert;
            int             m_numAsserts;
        };
        struct Suite
        {
            Suite()
                : m_name(0)
                , m_time(0.0f)
                , m_numTestsFailed(0) {}
            Suite(const char* name)
                : m_name(name)
                , m_time(0.0f)
                , m_numTestsFailed(0) {}

            const char*     m_name;
            float           m_time;
            unsigned int    m_numTestsFailed;
            TESTS_CONTAINER(Test)   m_tests;
        };

    public:
        static TestRunner& Instance()
        {
            static TestRunner runner;
            return runner;
        }

        template<class T>
        void Run(const char* testName)
        {
            m_suites.back().m_tests.push_back(Test(testName));

            T test;

#ifdef AZ_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4611)
#endif
            int status = setjmp(s_jmpBuf);
#ifdef AZ_COMPILER_MSVC
#pragma warning(pop)
#endif
            if (status == 0)
            {
                AZ_Printf("UnitTest", "   Test '%s'...", testName);
                test.run();
                m_suites.back().m_tests.back().m_isSuccess = true;
                AZ_Printf("UnitTest", " passed\n");
            }
            else
            {
                m_suites.back().m_numTestsFailed++;
            }
        }

        void Cleanup()
        {
            // this is a static/singleton class, so we need to explicitly clean up tests when we're done.
            m_suites.clear();
        }

        bool GetAndPrintStatus()
        {
            int numSuitesFailed = 0;
            for (int iSuite = 0; iSuite < (int)m_suites.size(); ++iSuite)
            {
                numSuitesFailed += m_suites[iSuite].m_numTestsFailed;
            }

            if (numSuitesFailed > 0)
            {
                AZ_Printf("UnitTest", "===========================================================\n");
                AZ_Printf("UnitTest", "               !!! TESTS FAILED !!!\n");
                AZ_Printf("UnitTest", "===========================================================\n");
                return false;
            }
            else
            {
                int numSuitesPassed = (int)m_suites.size();
                (void)numSuitesPassed;
                int numTestsPassed = 0;
                int numAssertsPassed = 0;
                for (int iSuite = 0; iSuite < (int)m_suites.size(); ++iSuite)
                {
                    int numTests = (int)m_suites[iSuite].m_tests.size();
                    numTestsPassed += numTests;
                    for (int iTest = 0; iTest < numTests; ++iTest)
                    {
                        numAssertsPassed += (int)m_suites[iSuite].m_tests[iTest].m_numAsserts;
                    }
                }
                AZ_Printf("UnitTest", "Total of %d suites, %d tests and %d asserts passed!\n", numSuitesPassed, numTestsPassed, numAssertsPassed);
            }

            return true;
        }

        //=========================================================================
        // Output results to a file compatible with NUnit format.
        // [6/22/2009]
        //=========================================================================
        void StoreXMLResults(const char* testProjectName, const char* xmlFileName)
        {
#ifdef AZ_PLATFORM_WINDOWS
#pragma warning( disable : 4996 )
            char buffer[1024];
            char nameBuffer[512];
            FILE* file = fopen(xmlFileName, "wb");

            int numTests = 0;
            int numTestsFailed = 0;
            for (int iSuite = 0; iSuite < (int)m_suites.size(); ++iSuite)
            {
                numTests += (int)m_suites[iSuite].m_tests.size();
                numTestsFailed += (int)m_suites[iSuite].m_numTestsFailed;
            }
            sprintf(buffer, "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"no\"?>\r\n");
            XMLWriteToOutput(buffer, file);
            sprintf(buffer, "<test-results name=\"%s\" total=\"%d\" errors=\"0\" failures=\"%d\" not-run=\"0\" ignored=\"0\" skipped=\"0\" invalid=\"0\" date=\"%s\" time=\"%s\">\r\n",
                testProjectName, numTests, numTestsFailed, __DATE__, __TIME__); /* for date and time we use the build time */
            XMLWriteToOutput(buffer, file);

            for (int iSuite = 0; iSuite < (int)m_suites.size(); ++iSuite)
            {
                Suite& s = m_suites[iSuite];
                XMLFixTemplateName(s.m_name, nameBuffer);
                sprintf(buffer, "<test-suite name=\"%s\" executed=\"True\" success=\"%s\" time=\"%.3f\" asserts=\"0\">\r\n", nameBuffer, s.m_numTestsFailed ? "False" : "True", s.m_time);
                XMLWriteToOutput(buffer, file);
                sprintf(buffer, "<results>\r\n");
                XMLWriteToOutput(buffer, file);
                for (int iTest = 0; iTest < (int)s.m_tests.size(); ++iTest)
                {
                    Test& t = s.m_tests[iTest];
                    XMLFixTemplateName(t.m_name, nameBuffer);
                    if (!t.m_isSuccess)
                    {
                        sprintf(buffer, "<test-case name=\"%s\" executed=\"True\" success=\"%s\" time=\"%.3f\" asserts=\"%d\">\r\n", nameBuffer, t.m_isSuccess ? "True" : "False", t.m_time, t.m_numAsserts);
                        XMLWriteToOutput(buffer, file);

                        sprintf(buffer, "<failure>\r\n<message>\r\n");
                        XMLWriteToOutput(buffer, file);

                        sprintf(buffer, "<![CDATA[%s]]>\r\n", t.m_assert.m_expression);
                        XMLWriteToOutput(buffer, file);

                        sprintf(buffer, "</message>\r\n<stack-trace>\r\n");
                        XMLWriteToOutput(buffer, file);

                        sprintf(buffer, "<![CDATA[%s(%d) : error]]>\r\n", t.m_assert.m_fileName, t.m_assert.m_lineNumer);
                        XMLWriteToOutput(buffer, file);

                        sprintf(buffer, "</stack-trace>\r\n</failure>\r\n</test-case>");
                        XMLWriteToOutput(buffer, file);
                    }
                    else
                    {
                        sprintf(buffer, "<test-case name=\"%s\" executed=\"True\" success=\"%s\" time=\"%.3f\" asserts=\"%d\"/>\r\n", nameBuffer, t.m_isSuccess ? "True" : "False", t.m_time, t.m_numAsserts);
                        XMLWriteToOutput(buffer, file);
                    }
                }
                sprintf(buffer, "</results>\r\n</test-suite>\r\n");
                XMLWriteToOutput(buffer, file);
            }

            sprintf(buffer, "</test-results>\r\n");
            XMLWriteToOutput(buffer, file);

            if (file)
            {
                fclose(file);
            }
#pragma warning( default : 4996 )
#else
            (void)testProjectName;
            (void)xmlFileName;
#endif
        }

        void ProcessAssert(const char* expression, const char* file, int line, bool expressionTest)
        {
            // since we pipe normal asserts trough the test framework (if we want to test them too)
            // make sure we handle the case for static objects with asserts (outside any tests)
            // This generally should not happen but on a very rare occasion is possible.
            if (m_suites.empty())
            {
                if (!expressionTest)
                {
                    AZ_Printf("UnitTest", " FAILED\n");
                    AZ_Printf("UnitTest", "     Assertion failed: %s\n", expression);
                    AZ_Printf("UnitTest", "     %s(%d) : error\n", file, line);

                    AZ::Debug::Trace::Break();
                }
                return;
            }

            Test& t = m_suites.back().m_tests.back();
            ++t.m_numAsserts;

            if (!expressionTest)
            {
                if (m_isAssertTest)
                {
                    ++m_numAssertsFailed;
                }
                else
                {
                    t.m_assert.m_expression = expression;
                    t.m_assert.m_fileName = file;
                    t.m_assert.m_lineNumer = line;
                    t.m_assert.m_isSuccess = expressionTest;

                    AZ_Printf("UnitTest", " FAILED\n");
                    AZ_Printf("UnitTest", "     Assertion failed: %s\n", expression);
                    //write formatted error message which visual studio will recognize
                    AZ_Printf("UnitTest", "     %s(%d) : error\n", file, line);
                    longjmp(s_jmpBuf, 1);
                }
            }
        }

        void StartSuite(const char* name)
        {
            m_suites.push_back(Suite(name));
        }

        void StartAssertTests()
        {
            m_isAssertTest = true;
            m_numAssertsFailed = 0;
        }
        int  StopAssertTests()
        {
            m_isAssertTest = false;
            return m_numAssertsFailed;
        }

    private:
        // Templated names (test or suite) need to be encoded to work in XML.
        void XMLFixTemplateName(const char* src, char* dst)
        {
            for (size_t i = 0; i < strlen(src); ++i)
            {
                if (src[i] == '<')
                {
                    *dst++ = '&';
                    *dst++ = 'l';
                    *dst++ = 't';
                    *dst++ = ';';
                }
                else if (src[i] == '>')
                {
                    *dst++ = '&';
                    *dst++ = 'g';
                    *dst++ = 't';
                    *dst++ = ';';
                }
                else
                {
                    *dst++ = src[i];
                }
            }
            *dst = '\0';
        }

        void XMLWriteToOutput(const char* data, FILE* file)
        {
#ifdef AZ_PLATFORM_WINDOWS
            if (file)
            {
                fwrite(data, 1, strlen(data), file);
            }
            else
            {
                AZ_Printf("UnitTest", "%s", data);
            }
#else
            (void)data;
            (void)file;
#endif
        }

        jmp_buf s_jmpBuf;

        TESTS_CONTAINER(Suite)  m_suites;

        bool m_isAssertTest;
        int  m_numAssertsFailed;
    };

    inline void* DebugAlignAlloc(size_t byteSize, size_t alignment)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        return _aligned_offset_malloc(byteSize, alignment, 0);
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION STANDALONEUNITTEST_H_SECTION_3
#include AZ_RESTRICTED_FILE(StandaloneUnitTest_h, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        return memalign(alignment, byteSize);
#endif
    }

    inline void DebugAlignFree(void* ptr)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        _aligned_free(ptr);
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION STANDALONEUNITTEST_H_SECTION_4
#include AZ_RESTRICTED_FILE(StandaloneUnitTest_h, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        free(ptr);
#endif
    }
}

/**
* Create a function that will run all declared tests.
* void  RR_TEST_RUN_XXXXX()
*/
#define AZ_TEST_SUITE(_suiteName)                        \
    void AZ_TEST_RUNFUNC_##_suiteName()                  \
    {                                                    \
        const char* suiteName = #_suiteName;             \
        AZ_Printf("UnitTest", "Suite: %s\n", suiteName); \
        UnitTest::TestRunner::Instance().StartSuite(suiteName);

#define AZ_TEST(_testName) \
    UnitTest::TestRunner::Instance().Run< _testName >(#_testName);
#define AZ_TEST_SUITE_END \
    }

/// Run a test suite.
#define AZ_TEST_RUN_SUITE(_suiteName)    \
    void AZ_TEST_RUNFUNC_##_suiteName(); \
    AZ_TEST_RUNFUNC_##_suiteName();

/// Return and print test status.
#define AZ_TEST_GET_STATUS      UnitTest::TestRunner::Instance().GetAndPrintStatus()
#define AZ_TEST_OUTPUT_RESULTS(_ProjectName, _FileName) UnitTest::TestRunner::Instance().StoreXMLResults(_ProjectName, _FileName)

#define AZ_TEST_ASSERT(exp) { UnitTest::TestRunner::Instance().ProcessAssert(#exp, __FILE__, __LINE__, (exp) ? true : false); }
#define AZ_TEST_ASSERT_MSG(exp, _msg)    { if (!(exp)) { UnitTest::TestRunner::Instance().AssertFailed(_msg, __FILE__, __LINE__); } \
}
#define AZ_TEST_ASSERT_CLOSE(_exp, _value, _eps) { UnitTest::TestRunner::Instance().ProcessAssert(#_exp " close to "#_value, __FILE__, __LINE__, std::abs((long)(_exp) - (_value)) >= (long)_eps ? false : true); }
#define AZ_TEST_ASSERT_FLOAT_CLOSE(_exp, _value) { UnitTest::TestRunner::Instance().ProcessAssert(#_exp " close to "#_value, __FILE__, __LINE__, fabsf((_exp) - (_value)) >= 0.002f ? false : true); }

#define AZ_TEST_STATIC_ASSERT(_Exp)                         AZ_STATIC_ASSERT(_Exp, "Test Static Assert")
#ifdef AZ_DEBUG_BUILD
#   define AZ_TEST_START_ASSERTTEST                     UnitTest::TestRunner::Instance().StartAssertTests()
#   define AZ_TEST_STOP_ASSERTTEST(_NumTriggeredAsserts)    AZ_TEST_ASSERT(UnitTest::TestRunner::Instance().StopAssertTests() == _NumTriggeredAsserts)
#else
// we can't test asserts, since they are not enabled for non debug builds!
#   define AZ_TEST_START_ASSERTTEST
#   define AZ_TEST_STOP_ASSERTTEST(_NumTriggeredAsserts)
#endif

