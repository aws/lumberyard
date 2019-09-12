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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Provides implementation for CryMemoryManager globally defined functions.
//               This file included only by platform_impl.h, do not include it directly in code!


#pragma once

#ifdef AZ_MONOLITHIC_BUILD
    #include <ISystem.h> // <> required for Interfuscator
#endif // AZ_MONOLITHIC_BUILD

#include "CryLibrary.h"

#include <AzCore/Module/Environment.h>

#define DLL_ENTRY_GETMEMMANAGER "CryGetIMemoryManagerInterface"

// Resolve IMemoryManager by looking in this DLL, then loading and rummaging through
// CrySystem. Cache the result per DLL, because this is not quick.
IMemoryManager* CryGetIMemoryManager()
{
    static AZ::EnvironmentVariable<IMemoryManager*> memMan = nullptr;
    if (!memMan)
    {
        memMan = AZ::Environment::FindVariable<IMemoryManager*>("CryIMemoryManagerInterface");
        AZ_Assert(memMan, "Unable to find CryIMemoryManagerInterface via AZ::Environment");
    }
    return *memMan;
}

// Redefine new & delete for entire module.
// For any module which defines USE_CRY_NEW_AND_DELETE, these versions of new and delete
// will be linked into that module instead of the default CRT version
// On Mac/Linux, RC/editor DLLs must not override operator new, because new/delete are forced public symbols
// and any conflict between new/delete will result in mismatched memory management and thus crashes
#if defined(USE_CRY_NEW_AND_DELETE) && !defined(NEW_OVERRIDEN) && !defined(AZ_TESTS_ENABLED) && !defined(AZ_PLATFORM_MAC) && !defined(AZ_PLATFORM_LINUX)

#if defined(CRY_FORCE_MALLOC_NEW_ALIGN)
#    define ModuleMalloc(size) CryModuleMemalign(size, TARGET_DEFAULT_ALIGN)
#    define ModuleFree(ptr) CryModuleMemalignFree(ptr)
#else
#    define ModuleMalloc(size) CryModuleMalloc(size)
#    define ModuleFree(ptr) CryModuleFree(ptr)
#endif // defined(CRY_FORCE_MALLOC_NEW_ALIGN)

//This code causes a seg fault in clang in profile builds if optimizations are turned on. In XCode 6.4
//TODO: Try to remove in a future version of XCode.
#if defined(AZ_MONOLITHIC_BUILD)&& (defined(IOS) || defined(APPLETV))
#pragma clang optimize off
#endif

PREFAST_SUPPRESS_WARNING(28251)
void* __cdecl operator new(size_t size)
{
    void* ret = ModuleMalloc(size);
    return ret;
}
PREFAST_SUPPRESS_WARNING(28251)
void* __cdecl operator new[](size_t size)
{
    void* ret = ModuleMalloc(size);
    return ret;
}

void* __cdecl operator new(size_t size, const std::nothrow_t& nothrow) throw()
{
    void* ret = ModuleMalloc(size);
    return ret;
}

void* __cdecl operator new[](size_t size, const std::nothrow_t& nothrow) throw()
{
    void* ret = ModuleMalloc(size);
    return ret;
}

void __cdecl operator delete(void* p) throw()
{
    ModuleFree(p);
}
void __cdecl operator delete[](void* p) throw()
{
    ModuleFree(p);
}

void __cdecl operator delete(void* p, const std::nothrow_t&) throw()
{
    ModuleFree(p);
}

void __cdecl operator delete[](void* p, const std::nothrow_t&) throw()
{
    ModuleFree(p);
}

#if defined(AZ_MONOLITHIC_BUILD)&& (defined(IOS) || defined(APPLETV))
#pragma clang optimize on
#endif

#endif // defined(USE_CRY_NEW_AND_DELETE) && !defined(NEW_OVERRIDEN)
