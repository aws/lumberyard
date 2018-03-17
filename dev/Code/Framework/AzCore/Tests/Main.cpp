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

#include <AzCore/Debug/Timer.h>
#include <AzCore/Debug/TraceMessageBus.h>

#include <AzCore/std/typetraits/typetraits.h>

#if defined(AZ_TESTS_ENABLED)
DECLARE_AZ_UNIT_TEST_MAIN()
#endif


namespace AZ
{
    inline void* AZMemAlloc(AZStd::size_t byteSize, AZStd::size_t alignment, const char* name = "No name allocation")
    {
        (void)name;
#if AZ_TRAIT_SUPPORT_WINDOWS_ALIGNED_MALLOC
        return _aligned_malloc(byteSize, alignment);
#else
        return memalign((size_t)alignment, (size_t)byteSize);
#endif
    }

    inline void AZFree(void* ptr, AZStd::size_t byteSize = 0, AZStd::size_t alignment = 0)
    {
        (void)byteSize;
        (void)alignment;
#if AZ_TRAIT_SUPPORT_WINDOWS_ALIGNED_MALLOC
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }
}
// END OF TEMP MEMORY ALLOCATIONS
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using namespace AZ;

// Handle asserts
class TraceDrillerHook
    : public AZ::Test::ITestEnvironment
    , public AZ::Debug::TraceMessageBus::Handler
{
public:
    void SetupEnvironment() override
    {
        AllocatorInstance<OSAllocator>::Create(); // used by the bus


        BusConnect();
    }

    void TeardownEnvironment() override
    {
        BusDisconnect();


        AllocatorInstance<OSAllocator>::Destroy(); // used by the bus
    }

    bool OnPreAssert(const char* file, int line, const char* /* func */, const char* message) override 
    { 
        if (UnitTest::TestRunner::Instance().m_isAssertTest)
        {
            UnitTest::TestRunner::Instance().ProcessAssert(message, file, line, false);
        }
        else
        {
            GTEST_TEST_BOOLEAN_(false, message, false, true, GTEST_NONFATAL_FAILURE_);
        }
        return true;
    }
    bool OnAssert(const char* /*message*/) override
    {
        return true; // stop processing
    }
    bool OnPreError(const char* /*window*/, const char* file, int line, const char* /*func*/, const char* message) override
    {
        if (UnitTest::TestRunner::Instance().m_isAssertTest)
        {
            UnitTest::TestRunner::Instance().ProcessAssert(message, file, line, false);
            return true;
        }
        return false;
    }
    bool OnError(const char* /*window*/, const char* message) override
    {
        if (UnitTest::TestRunner::Instance().m_isAssertTest)
        {
            UnitTest::TestRunner::Instance().ProcessAssert(message, __FILE__, __LINE__, UnitTest::AssertionExpr(false));
        }
        else
        {
            GTEST_TEST_BOOLEAN_(false, message, false, true, GTEST_NONFATAL_FAILURE_);
        }
        return true; // stop processing
    }
    bool OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) 
    { 
        return false; 
    }
    bool OnWarning(const char* /*window*/, const char* /*message*/) override
    { 
        return false; 
    }

    bool OnOutput(const char* /*window*/, const char* /*message*/) override
    {
        return true;
    }
};

AZ_UNIT_TEST_HOOK(new TraceDrillerHook());

#if defined(HAVE_BENCHMARK)
AZTEST_EXPORT size_t AzRunBenchmarks(int argc, char** argv)
{
    AZ::AllocatorInstance<AZ::OSAllocator>::Create();
    AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();

    AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();

    return 0;
}
#endif // HAVE_BENCHMARK
