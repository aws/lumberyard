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

#if defined(AZ_TESTS_ENABLED)
#include <AzTest/AzTest.h>
#endif

#include <stdio.h>


#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/base.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/typetraits/has_member_function.h>
#include <AzCore/Debug/Trace.h>

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID)
#   include <malloc.h>
#elif defined(AZ_PLATFORM_APPLE)
#    include <malloc/malloc.h>
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

        pointer address(reference value) const              { return (&value); }
        const_pointer address(const_reference value) const  { return (&value); }
        DebugAllocatorSTL() {}  // construct default allocator (do nothing)
        DebugAllocatorSTL(const DebugAllocatorSTL<T>&)  {}

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
        pointer allocate(size_type count, const void*)  {   return allocate(count); }
        void construct(pointer ptr, const T& value)     {   new(ptr)    T(value); }
        void destroy(pointer ptr)                       {   (void)ptr; ptr->~T(); }
        size_type max_size() const                      { return 1000000; }
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


    class TestRunner
    {
    public:
        static TestRunner& Instance()
        {
            static TestRunner runner;
            return runner;
        }

        void ProcessAssert(const char* /*expression*/, const char* /*file*/, int /*line*/, bool expressionTest)
        {
#if defined (AZ_TESTS_ENABLED)
            ASSERT_TRUE(m_isAssertTest);
#endif

            if (m_isAssertTest)
            {
                if (!expressionTest)
                {
                    ++m_numAssertsFailed;
                }
            }
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

        bool m_isAssertTest;
        int  m_numAssertsFailed;
    };

    inline void* DebugAlignAlloc(size_t byteSize, size_t alignment)
    {
#if AZ_TRAIT_SUPPORT_WINDOWS_ALIGNED_MALLOC
        return _aligned_offset_malloc(byteSize, alignment, 0);
#else
        return memalign(alignment, byteSize);
#endif
    }

    inline void DebugAlignFree(void* ptr)
    {
#if AZ_TRAIT_SUPPORT_WINDOWS_ALIGNED_MALLOC
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }

    AZ_HAS_MEMBER(OperatorBool, operator bool, bool, ());

    class AssertionExpr
    {
        bool m_value;
    public:
        explicit AssertionExpr(bool b)
            : m_value(b)
        {}

        template <class T>
        explicit AssertionExpr(const T& t)
        {
            m_value = Evaluate(t);
        }

        operator bool() { return m_value; }

        template <class T>
        typename AZStd::enable_if<AZStd::is_integral<T>::value, bool>::type
        Evaluate(const T& t)
        {
            return t != 0;
        }

        template <class T>
        typename AZStd::enable_if<AZStd::is_class<T>::value, bool>::type
        Evaluate(const T& t)
        {
            return static_cast<bool>(t);
        }

        template <class T>
        typename AZStd::enable_if<AZStd::is_pointer<T>::value, bool>::type
        Evaluate(const T& t)
        {
            return t != nullptr;
        }
    };
}

#if defined(AZ_TESTS_ENABLED)

#define AZ_TEST_ASSERT(exp) { \
    if (UnitTest::TestRunner::Instance().m_isAssertTest) \
        UnitTest::TestRunner::Instance().ProcessAssert(#exp, __FILE__, __LINE__, UnitTest::AssertionExpr(exp)); \
    else GTEST_TEST_BOOLEAN_(exp, #exp, false, true, GTEST_NONFATAL_FAILURE_); }

#define AZ_TEST_ASSERT_CLOSE(_exp, _value, _eps) EXPECT_NEAR((double)_exp, (double)_value, (double)_eps)
#define AZ_TEST_ASSERT_FLOAT_CLOSE(_exp, _value) EXPECT_NEAR(_exp, _value, 0.002f)

#define AZ_TEST_STATIC_ASSERT(_Exp)                         AZ_STATIC_ASSERT(_Exp, "Test Static Assert")
#ifdef AZ_ENABLE_TRACING
#   define AZ_TEST_START_ASSERTTEST                         UnitTest::TestRunner::Instance().StartAssertTests()
#   define AZ_TEST_STOP_ASSERTTEST(_NumTriggeredAsserts)    GTEST_ASSERT_EQ(_NumTriggeredAsserts, UnitTest::TestRunner::Instance().StopAssertTests())
#else
// we can't test asserts, since they are not enabled for non trace-enabled builds!
#   define AZ_TEST_START_ASSERTTEST
#   define AZ_TEST_STOP_ASSERTTEST(_NumTriggeredAsserts)
#endif

#else

#define AZ_TEST_ASSERT(...)
#define AZ_TEST_ASSERT_CLOSE(...)
#define AZ_TEST_ASSERT_FLOAT_CLOSE(...)
#define AZ_TEST_STATIC_ASSERT(...)
#define AZ_TEST_START_ASSERTTEST
#define AZ_TEST_STOP_ASSERTTEST(...)

#endif // AZ_TESTS_ENABLED

#define AZ_TEST(...)
#define AZ_TEST_SUITE(...)
#define AZ_TEST_SUITE_END
