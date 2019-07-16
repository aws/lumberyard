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


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define MAIN_CPP_SECTION_1 1
#endif

#if defined(AZ_TESTS_ENABLED)
DECLARE_AZ_UNIT_TEST_MAIN()
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MAIN_CPP_SECTION_1
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/Main_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/Main_cpp_provo.inl"
    #endif
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

AZ_UNIT_TEST_HOOK(new UnitTest::TraceBusHook());

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
