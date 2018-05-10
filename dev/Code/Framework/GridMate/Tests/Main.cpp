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
#include <GridMate/GridMate.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/Debug/TraceMessageBus.h>

#include <stdlib.h>

// Notes about the Dec 2016 version of this testing app:
// . these tests have not been added to WAF, requires a separate EXE to run with '--unittest' on the command line
// . AzTest only works in on the Windows platform
// . The GoogleTest framework requires making memory allocations between tests; thus turned off OVERLOAD_GLOBAL_ALLOCATIONS testing


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define MAIN_CPP_SECTION_1 1
#define MAIN_CPP_SECTION_2 2
#define MAIN_CPP_SECTION_3 3
#define MAIN_CPP_SECTION_4 4
#endif

#if defined(AZ_TESTS_ENABLED)

#if !defined(AZ_PLATFORM_WINDOWS)
#pragma error "AzTest only supported on Windows platforms."
#else

DECLARE_AZ_UNIT_TEST_MAIN()

// Handle asserts
class TraceDrillerHook
    : public AZ::Debug::TraceMessageBus::Handler
{
public:
    TraceDrillerHook()
    {
        // used by the bus
        if (!AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
        {
            AZ::AllocatorInstance<AZ::OSAllocator>::Create();
        }
        BusConnect();
    }

    ~TraceDrillerHook() override
    {
        BusDisconnect();
        AZ::AllocatorInstance<AZ::OSAllocator>::Destroy(); // used by the bus
    }

    bool OnAssert(const char* message) override
    {
        (void)message;
        AZ_TEST_ASSERT(false); // just forward
        return true;
    }
};

int main(int argc, char** argv)
{
    TraceDrillerHook traceDrillerHook;

    INVOKE_AZ_UNIT_TEST_MAIN();
}


#endif // IS defined(AZ_PLATFORM_WINDOWS)

#else // NOT defined(AZ_TESTS_ENABLED)

using namespace GridMate;

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MAIN_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(Main_cpp, AZ_RESTRICTED_PLATFORM)
#endif

/**
 * Default AZStd memory allocations implementations. Just so we can assert if we use them
 */
namespace AZStd
{
    void* Default_Alloc(AZStd::size_t byteSize, AZStd::size_t alignment, AZStd::size_t offset, const char* name)
    {
        (void)byteSize;
        (void)alignment;
        (void)offset;
        (void)name;
        AZ_Assert(byteSize > 0, "We are lib, we should NOT use generic memory! Use GridMateAllocator!");
        return 0;
    }

    void Default_Free(void* ptr)
    {
        (void)ptr;
        AZ_Assert(ptr != NULL, "We are lib, we should NOT use generic memory! Use GridMateAllocator!");
    }
}

//////////////////////////////////////////////////////////////////////////
// Overload global new and delete to make sure we don't new anything
#if AZ_TRAIT_DEFINE_STUBBED_OPERATOR_NEW_OVERRIDES
void* operator new(std::size_t size, const char* fileName, int lineNum, const AZ::Internal::AllocatorDummy&)
{
    (void)size;
    (void)fileName;
    (void)lineNum;
    AZ_Assert(size > 0, "We are lib, we should NOT use generic memory! Use GridMateAllocator!");
    return (void*)0x0badf00d;
}
void* operator new[](std::size_t size, const char* fileName, int lineNum, const AZ::Internal::AllocatorDummy&)
{
    (void)size;
    (void)fileName;
    (void)lineNum;
    AZ_Assert(size > 0, "We are lib, we should NOT use generic memory! Use GridMateAllocator!");
    return (void*)0x0badf00d;
}
void* operator new(std::size_t size)
{
    (void)size;
    AZ_Assert(size > 0, "We are lib, we should NOT use generic memory! Use GridMateAllocator!");
    return (void*)0x0badf00d;
}
void* operator new[](std::size_t size)
{
    (void)size;
    AZ_Assert(size > 0, "We are lib, we should NOT use generic memory! Use GridMateAllocator!");
    return (void*)0x0badf00d;
}
void operator delete(void* ptr)
#if defined(AZ_PLATFORM_LINUX)
_GLIBCXX_USE_NOEXCEPT
#elif defined(AZ_PLATFORM_APPLE)
_NOEXCEPT
#endif // AZ_PLATFORM_LINUX
{
    (void)ptr;
    AZ_Assert(ptr != NULL, "We are lib, we should NOT use generic memory! Use GridMateAllocator!");
}
void operator delete[](void* ptr)
#if defined(AZ_PLATFORM_LINUX)
_GLIBCXX_USE_NOEXCEPT
#elif defined(AZ_PLATFORM_APPLE)
_NOEXCEPT
#endif // AZ_PLATFORM_LINUX
{
    (void)ptr;
    AZ_Assert(ptr != NULL, "We are lib, we should NOT use generic memory! Use GridMateAllocator!");
}
#endif
//////////////////////////////////////////////////////////////////////////

// Handle asserts
class TraceDrillerHook
    : public AZ::Debug::TraceMessageBus::Handler
{
public:
    TraceDrillerHook()
    {
        AZ::AllocatorInstance<AZ::OSAllocator>::Create(); // used by the bus
        BusConnect();
    }

    ~TraceDrillerHook() override
    {
        BusDisconnect();
        AZ::AllocatorInstance<AZ::OSAllocator>::Destroy(); // used by the bus
    }
    
    bool OnAssert(const char* message) override
    {
        (void)message;
        AZ_TEST_ASSERT(false); // just forward
        return true;
    }
};

//=========================================================================
//
//
//=========================================================================
int DoTests(const char* projectName = 0, const char* resultsFileName = 0)
{
    // TODO: console unit tests are broken as of Dec 2016
    (void)projectName;
    (void)resultsFileName;
    return 0;
//
//    TraceDrillerHook traceHook;
//
//    AZ_TEST_RUN_SUITE(SerializeSuite);
//    AZ_TEST_RUN_SUITE(CarrierSuite);
//    AZ_TEST_RUN_SUITE(ReplicaSmallSuite);
//    AZ_TEST_RUN_SUITE(ReplicaMediumSuite);
//    AZ_TEST_RUN_SUITE(ReplicaSuite);
//    AZ_TEST_RUN_SUITE(OnlineSuite);
//    AZ_TEST_RUN_SUITE(SessionSuite);
//
//#if defined(GM_INTEREST_MANAGER)
//    AZ_TEST_RUN_SUITE(InterestManagementSuite);
//#endif
//
//    // Special tests enabled manually
//
//    //AZ_TEST_RUN_SUITE(LeaderboardSuite);
//    //AZ_TEST_RUN_SUITE(AchievementSuite);
//    //AZ_TEST_RUN_SUITE(StorageSuite);
//
//    if (resultsFileName)
//    {
//        AZ_TEST_OUTPUT_RESULTS(projectName, resultsFileName);
//    }
//    return AZ_TEST_GET_STATUS ? 0 : 1;
}

//=========================================================================
//
//
//=========================================================================
namespace Render {

    void Flip()
    {
    }

    void Init()
    {
    }

    void Destroy()
    {
    }
}   // namespace Render

//=========================================================================
// Setup
// [9/8/2010]
//=========================================================================
void Setup()
{
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MAIN_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(Main_cpp, AZ_RESTRICTED_PLATFORM)
#endif
    Render::Init();
}

//=========================================================================
// Destroy
// [9/8/2010]
//=========================================================================
void Destroy()
{
    Render::Destroy();

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MAIN_CPP_SECTION_3
#include AZ_RESTRICTED_FILE(Main_cpp, AZ_RESTRICTED_PLATFORM)
#endif
}

#if defined(AZ_PLATFORM_APPLE_IOS) || defined(AZ_PLATFORM_APPLE_TV)
extern "C"
{
void RunTests()
{
    Setup();
    DoTests();
    Destroy();
}
}
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MAIN_CPP_SECTION_4
#include AZ_RESTRICTED_FILE(Main_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
int main(int argc, char* argv[])
{
    const char* projectName = 0;
    const char* resultsFileName = 0;
    for (int i = 0; i < argc; ++i)
    {
        char* pos;
        if ((pos = strstr(argv[i], "-xml:")) != 0)
        {
            pos += 5;
            resultsFileName = pos;
            if (projectName == 0)
            {
                projectName = "GridMates";
            }
        }
        else if ((pos = strstr(argv[i], "-name:")) != 0)
        {
            pos += 6;
            projectName = pos;
        }
    }

    Setup();
    int ret = DoTests(projectName, resultsFileName);
    Destroy();
    return ret;
}
#endif

#endif // AZ_TESTS_ENABLED
