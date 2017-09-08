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
#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE)
        return _aligned_malloc(byteSize, alignment);
#else
        return memalign((size_t)alignment, (size_t)byteSize);
#endif
    }

    inline void AZFree(void* ptr, AZStd::size_t byteSize = 0, AZStd::size_t alignment = 0)
    {
        (void)byteSize;
        (void)alignment;
#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE)
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

    virtual AZ::Debug::Result OnPreAssert(const AZ::Debug::TraceMessageParameters& parameters) override
    { 
        UnitTest::TestRunner::Instance().ProcessAssert(parameters.message, parameters.file, parameters.line, false);
        return AZ::Debug::Result::Handled;
    }
    virtual AZ::Debug::Result OnAssert(const AZ::Debug::TraceMessageParameters& parameters) override
    {
        return AZ::Debug::Result::Handled; // stop processing
    }
    virtual AZ::Debug::Result OnPreError(const AZ::Debug::TraceMessageParameters& parameters) override
    {
        UnitTest::TestRunner::Instance().ProcessAssert(parameters.message, parameters.file, parameters.line, false);
        return AZ::Debug::Result::Handled;
    }
    virtual AZ::Debug::Result OnError(const AZ::Debug::TraceMessageParameters& parameters) override
    {
        if (UnitTest::TestRunner::Instance().m_isAssertTest)
        {
            UnitTest::TestRunner::Instance().ProcessAssert(parameters.message, __FILE__, __LINE__, UnitTest::AssertionExpr(false));
        }
        else
        {
            GTEST_TEST_BOOLEAN_(false, parameters.message, false, true, GTEST_NONFATAL_FAILURE_);
        }
        return AZ::Debug::Result::Handled; // stop processing
    }
    virtual AZ::Debug::Result OnPreWarning(const AZ::Debug::TraceMessageParameters& parameters) override 
    { 
        return AZ::Debug::Result::Continue;
    }
    virtual AZ::Debug::Result OnWarning(const AZ::Debug::TraceMessageParameters& parameters) override
    { 
        return AZ::Debug::Result::Continue;
    }
    virtual AZ::Debug::Result OnOutput(const AZ::Debug::TraceMessageParameters& parameters) override
    {
        return AZ::Debug::Result::Handled;
    }
};

AZ_UNIT_TEST_HOOK(new TraceDrillerHook());
