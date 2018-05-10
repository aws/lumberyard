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

// Description : Defines functions for CryEngine custom memory manager.
//               See also CryMemoryManager_impl h  it must be included only once per module
//               CryMemoryManager_impl h is included by platform_impl h


#pragma once

#if !defined(_RELEASE)
// Enable this to check for heap corruption on windows systems by walking the
// crt.
#define MEMORY_ENABLE_DEBUG_HEAP 0
#endif

// Section dictionary
#if defined(AZ_RESTRICTED_PLATFORM)
#define CRYMEMORYMANAGER_H_SECTION_TRAITS 1
#define CRYMEMORYMANAGER_H_SECTION_ALLOCPOLICY 2
#define CRYMEMORYMANAGER_H_SECTION_THROW 3
#endif

// Traits
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CRYMEMORYMANAGER_H_SECTION_TRAITS
#include AZ_RESTRICTED_FILE(CryMemoryManager_h, AZ_RESTRICTED_PLATFORM)
#else
#if !defined(APPLE)
#define CRYMEMORYMANAGER_H_TRAIT_INCLUDE_MALLOC_H 1
#endif
#if defined(LINUX) || defined(APPLE)
#define CRYMEMORYMANAGER_H_TRAIT_INCLUDE_NEW_NOT_NEW_H 1
#endif
#if !defined(LINUX) && !defined(APPLE)
#define CRYMEMORYMANAGER_H_TRAIT_INCLUDE_CRTDBG_H 1
#endif
#if !defined(APPLE)
#define CRYMEMORYMANAGER_H_TRAIT_USE_CRTCHECKMEMORY 1
#endif
#endif

// Throw
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CRYMEMORYMANAGER_H_SECTION_THROW
#include AZ_RESTRICTED_FILE(CryMemoryManager_h, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
#define CRYMM_THROW throw()
#endif

#if !defined(RELEASE)
#define CRYMM_THROW_BAD_ALLOC throw(std::bad_alloc)
#else
#define CRYMM_THROW_BAD_ALLOC throw()
#endif

// Scope based heap checker macro
#if (defined(WIN32) || defined(WIN64)) && MEMORY_ENABLE_DEBUG_HEAP
# include <crtdbg.h>
# define MEMORY_CHECK_HEAP() do { if (!_CrtCheckMemory()) {__debugbreak(); } \
} while (0);
struct _HeapChecker
{
    _HeapChecker() { MEMORY_CHECK_HEAP(); }
    ~_HeapChecker() { MEMORY_CHECK_HEAP(); }
};
# define MEMORY_SCOPE_CHECK_HEAP_NAME_EVAL(x, y) x ## y
# define MEMORY_SCOPE_CHECK_HEAP_NAME MEMORY_SCOPE_CHECK_HEAP_NAME_EVAL(__heap_checker__, __LINE__)
# define MEMORY_SCOPE_CHECK_HEAP() _HeapChecker MMRM_SCOPE_CHECK_HEAP_NAME
#endif

#if !defined(MEMORY_CHECK_HEAP)
# define MEMORY_CHECK_HEAP() void(NULL)
#endif
#if !defined(MEMORY_SCOPE_CHECK_HEAP)
# define MEMORY_SCOPE_CHECK_HEAP() void(NULL)
#endif

//////////////////////////////////////////////////////////////////////////
// Define this if you want to use slow debug memory manager in any config.
//////////////////////////////////////////////////////////////////////////
//#define DEBUG_MEMORY_MANAGER
//////////////////////////////////////////////////////////////////////////

// That mean we use node_allocator for all small allocations

#include "platform.h"

#include <stdarg.h>
#include <algorithm>

#if defined(APPLE) || defined(ANDROID)
    #include <AzCore/Memory/OSAllocator.h> // memalign
#endif // defined(APPLE)

#ifndef STLALLOCATOR_CLEANUP
#define STLALLOCATOR_CLEANUP
#endif

#define _CRY_DEFAULT_MALLOC_ALIGNMENT 4

#if CRYMEMORYMANAGER_H_TRAIT_INCLUDE_MALLOC_H
    #include <malloc.h>
#endif

#if defined(__cplusplus)
#if CRYMEMORYMANAGER_H_TRAIT_INCLUDE_NEW_NOT_NEW_H
    #include <new>
#else
    #include <new.h>
#endif
#endif

    #ifdef CRYSYSTEM_EXPORTS
        #define CRYMEMORYMANAGER_API DLL_EXPORT
    #else
        #define CRYMEMORYMANAGER_API DLL_IMPORT
    #endif

#ifdef __cplusplus
//////////////////////////////////////////////////////////////////////////
#ifdef DEBUG_MEMORY_MANAGER
    #ifdef _DEBUG
        #define _DEBUG_MODE
    #endif
#endif //DEBUG_MEMORY_MANAGER

#if defined(_DEBUG) && CRYMEMORYMANAGER_H_TRAIT_INCLUDE_CRTDBG_H
    #include <crtdbg.h>
#endif //_DEBUG

namespace CryMemory
{
    // checks if the heap is valid in debug; in release, this function shouldn't be called
    // returns non-0 if it's valid and 0 if not valid
    ILINE int IsHeapValid()
    {
    #if (defined(_DEBUG) && !defined(RELEASE_RUNTIME) && CRYMEMORYMANAGER_H_TRAIT_USE_CRTCHECKMEMORY) || (defined(DEBUG_MEMORY_MANAGER))
        return _CrtCheckMemory();
    #else
        return true;
    #endif
    }
}

#ifdef DEBUG_MEMORY_MANAGER
// Restore debug mode define
    #ifndef _DEBUG_MODE
        #undef _DEBUG
    #endif //_DEBUG_MODE
#endif //DEBUG_MEMORY_MANAGER
//////////////////////////////////////////////////////////////////////////

#endif //__cplusplus

struct ICustomMemoryHeap;
class IGeneralMemoryHeap;
class IPageMappingHeap;
class IDefragAllocator;
class IMemoryAddressRange;

// Description:
//   Interfaces that allow access to the CryEngine memory manager.
struct IMemoryManager
{
    typedef unsigned char HeapHandle;
    enum
    {
        BAD_HEAP_HANDLE = 0xFF
    };

    struct SProcessMemInfo
    {
        uint64 PageFaultCount;
        uint64 PeakWorkingSetSize;
        uint64 WorkingSetSize;
        uint64 QuotaPeakPagedPoolUsage;
        uint64 QuotaPagedPoolUsage;
        uint64 QuotaPeakNonPagedPoolUsage;
        uint64 QuotaNonPagedPoolUsage;
        uint64 PagefileUsage;
        uint64 PeakPagefileUsage;

        uint64 TotalPhysicalMemory;
        int64  FreePhysicalMemory;

        uint64 TotalVideoMemory;
        int64 FreeVideoMemory;
    };

    enum EAllocPolicy
    {
        eapDefaultAllocator,
        eapPageMapped,
        eapCustomAlignment,
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CRYMEMORYMANAGER_H_SECTION_ALLOCPOLICY
#include AZ_RESTRICTED_FILE(CryMemoryManager_h, AZ_RESTRICTED_PLATFORM)
#endif
    };

    virtual ~IMemoryManager(){}

    virtual bool GetProcessMemInfo(SProcessMemInfo& minfo) = 0;

    // Description:
    //   Used to add memory block size allocated directly from the crt or OS to the memory manager statistics.
    virtual void FakeAllocation(long size) = 0;

    // Initialise the level heap.
    virtual void InitialiseLevelHeap() = 0;

    // Switch the default heap to the level heap.
    virtual void SwitchToLevelHeap() = 0;

    // Switch the default heap to the global heap.
    virtual void SwitchToGlobalHeap() = 0;

    // Enable the global heap for this thread only. Returns previous heap selection, which must be passed to LocalSwitchToHeap.
    virtual int LocalSwitchToGlobalHeap() = 0;

    // Enable the level heap for this thread only. Returns previous heap selection, which must be passed to LocalSwitchToHeap.
    virtual int LocalSwitchToLevelHeap() = 0;

    // Switch to a specific heap for this thread only. Usually used to undo a previous LocalSwitchToGlobalHeap
    virtual void LocalSwitchToHeap(int heap) = 0;

    // Fetch the violation status of the level heap
    virtual bool GetLevelHeapViolationState(bool& usingLevelHeap, size_t& numAllocs, size_t& allocSize) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Heap Tracing API
    virtual HeapHandle TraceDefineHeap(const char* heapName, size_t size, const void* pBase) = 0;
    virtual void TraceHeapAlloc(HeapHandle heap, void* mem, size_t size, size_t blockSize, const char* sUsage, const char* sNameHint = 0) = 0;
    virtual void TraceHeapFree(HeapHandle heap, void* mem, size_t blockSize) = 0;
    virtual void TraceHeapSetColor(uint32 color) = 0;
    virtual uint32 TraceHeapGetColor() = 0;
    virtual void TraceHeapSetLabel(const char* sLabel) = 0;
    //////////////////////////////////////////////////////////////////////////

    // Retrieve access to the MemReplay implementation class.
    virtual struct IMemReplay* GetIMemReplay() = 0;

    // Create an instance of ICustomMemoryHeap
    virtual ICustomMemoryHeap* const CreateCustomMemoryHeapInstance(EAllocPolicy const eAllocPolicy) = 0;
    virtual IGeneralMemoryHeap* CreateGeneralExpandingMemoryHeap(size_t upperLimit, size_t reserveSize, const char* sUsage) = 0;
    virtual IGeneralMemoryHeap* CreateGeneralMemoryHeap(void* base, size_t sz, const char* sUsage) = 0;

    virtual IMemoryAddressRange* ReserveAddressRange(size_t capacity, const char* sName) = 0;
    virtual IPageMappingHeap* CreatePageMappingHeap(size_t addressSpace, const char* sName) = 0;

    virtual IDefragAllocator* CreateDefragAllocator() = 0;

    virtual void* AllocPages(size_t size) = 0;
    virtual void FreePages(void* p, size_t size) = 0;
};


// Global function implemented in CryMemoryManager_impl.h
IMemoryManager* CryGetIMemoryManager();

class STraceHeapAllocatorAutoColor
{
public:
    explicit STraceHeapAllocatorAutoColor(uint32 color) { m_color = CryGetIMemoryManager()->TraceHeapGetColor(); CryGetIMemoryManager()->TraceHeapSetColor(color); }
    ~STraceHeapAllocatorAutoColor() { CryGetIMemoryManager()->TraceHeapSetColor(m_color); };
protected:
    uint32 m_color;
    STraceHeapAllocatorAutoColor() {};
};

#define TRACEHEAP_AUTOCOLOR(color) STraceHeapAllocatorAutoColor _auto_color_(color);


// Summary:
//      Structure filled by call to CryModuleGetMemoryInfo().
struct CryModuleMemoryInfo
{
    uint64 requested;
    // Total Ammount of memory allocated.
    uint64 allocated;
    // Total Ammount of memory freed.
    uint64 freed;
    // Total number of memory allocations.
    int num_allocations;
    // Allocated in CryString.
    uint64 CryString_allocated;
    // Allocated in STL.
    uint64 STL_allocated;
    // Amount of memory wasted in pools in stl (not usefull allocations).
    uint64 STL_wasted;
};

struct CryReplayInfo
{
    uint64 uncompressedLength;
    uint64 writtenLength;
    uint32 trackingSize;
    const char* filename;
};

//////////////////////////////////////////////////////////////////////////
// Extern declarations of globals inside CrySystem.
//////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


CRYMEMORYMANAGER_API void* CryMalloc(size_t size, size_t& allocated, size_t alignment);
CRYMEMORYMANAGER_API void* CryRealloc(void* memblock, size_t size, size_t& allocated, size_t& oldsize, size_t alignment);
CRYMEMORYMANAGER_API size_t CryFree(void* p, size_t alignment);
CRYMEMORYMANAGER_API size_t CryGetMemSize(void* p, size_t size);
CRYMEMORYMANAGER_API int  CryStats(char* buf);
CRYMEMORYMANAGER_API void CryFlushAll();
CRYMEMORYMANAGER_API void CryCleanup();
CRYMEMORYMANAGER_API int  CryGetUsedHeapSize();
CRYMEMORYMANAGER_API int  CryGetWastedHeapSize();
CRYMEMORYMANAGER_API void* CrySystemCrtMalloc(size_t size);
CRYMEMORYMANAGER_API void* CrySystemCrtRealloc(void* p, size_t size);
CRYMEMORYMANAGER_API size_t CrySystemCrtFree(void* p);
CRYMEMORYMANAGER_API size_t CrySystemCrtSize(void* p);
CRYMEMORYMANAGER_API size_t CrySystemCrtGetUsedSpace();
CRYMEMORYMANAGER_API void CryGetIMemoryManagerInterface(void** pIMemoryManager);

// This function is local in every module
/*CRYMEMORYMANAGER_API*/
void CryGetMemoryInfoForModule(CryModuleMemoryInfo* pInfo);

#ifdef __cplusplus
}
#endif //__cplusplus



//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Cry Memory Manager accessible in all build modes.
//////////////////////////////////////////////////////////////////////////
#if !defined(USING_CRY_MEMORY_MANAGER)
    #define USING_CRY_MEMORY_MANAGER
#endif

#ifndef AZ_MONOLITHIC_BUILD
#define CRY_MEM_USAGE_API extern "C" DLL_EXPORT
#else // AZ_MONOLITHIC_BUILD
#define CRY_MEM_USAGE_API
#endif // AZ_MONOLITHIC_BUILD

#include "CryMemReplay.h"

#if CAPTURE_REPLAY_LOG
#define CRYMM_INLINE inline
#else
#define CRYMM_INLINE ILINE
#endif

#if defined(NOT_USE_CRY_MEMORY_MANAGER)
CRYMM_INLINE void* CryModuleMalloc(size_t size)
{
    MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CryMalloc);
    void* ptr = malloc(size);
    MEMREPLAY_SCOPE_ALLOC(ptr, size, 0);
    return ptr;
}

CRYMM_INLINE void* CryModuleRealloc(void* memblock, size_t size)
{
    MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CryMalloc);
    void* ret = realloc(memblock, size);
    MEMREPLAY_SCOPE_REALLOC(memblock, ret, size, 0);
    return ret;
}

CRYMM_INLINE void  CryModuleFree(void* memblock)
{
    MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CryMalloc);
    free(memblock);
    MEMREPLAY_SCOPE_FREE(memblock);
}

CRYMM_INLINE void  CryModuleMemalignFree(void* memblock)
{
    MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CryMalloc);
    free(memblock);
    MEMREPLAY_SCOPE_FREE(memblock);
}

CRYMM_INLINE void* CryModuleReallocAlign(void* memblock, size_t size, size_t alignment)
{
    MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CryMalloc);
    void* ret = realloc(memblock, size);
    MEMREPLAY_SCOPE_REALLOC(memblock, ret, size, alignment);
    return ret;
}

CRYMM_INLINE void* CryModuleMemalign(size_t size, size_t alignment)
{
    MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CryMalloc);
            #if defined(__GNUC__) && !defined(APPLE)
    void* ret = memalign(alignment, size);
            #else
    void* ret = malloc(size);
            #endif
    MEMREPLAY_SCOPE_ALLOC(ret, size, alignment);
    return ret;
}

#else //NOT_USE_CRY_MEMORY_MANAGER

/////////////////////////////////////////////////////////////////////////
// Extern declarations,used by overrided new and delete operators.
//////////////////////////////////////////////////////////////////////////
extern "C"
{
void* CryModuleMalloc(size_t size) throw();
void* CryModuleRealloc(void* memblock, size_t size) throw();
void  CryModuleFree(void* ptr) throw();
void* CryModuleMemalign(size_t size, size_t alignment) throw();
void* CryModuleReallocAlign(void* memblock, size_t size, size_t alignment) throw();
void  CryModuleMemalignFree(void*) throw();
void* CryModuleCalloc(size_t a, size_t b) throw();
}

CRYMM_INLINE void* CryModuleCRTMalloc(size_t s)
{
    MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CrtMalloc);
    void* ret = CryModuleMalloc(s);
    MEMREPLAY_SCOPE_ALLOC(ret, s, 0);
    return ret;
}

CRYMM_INLINE void* CryModuleCRTRealloc(void* p, size_t s)
{
    MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CrtMalloc);
    void* ret = CryModuleRealloc(p, s);
    MEMREPLAY_SCOPE_REALLOC(p, ret, s, 0);
    return ret;
}

CRYMM_INLINE void CryModuleCRTFree(void* p)
{
    MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CrtMalloc);
    CryModuleFree(p);
    MEMREPLAY_SCOPE_FREE(p);
}

#define malloc   CryModuleCRTMalloc
#define realloc  CryModuleCRTRealloc
#define free     CryModuleCRTFree
    #define calloc  CryModuleCalloc

#endif //NOT_USE_CRY_MEMORY_MANAGER

CRY_MEM_USAGE_API void CryModuleGetMemoryInfo(CryModuleMemoryInfo* pMemInfo);

// Need for our allocator to avoid deadlock in cleanup
void*  CryCrtMalloc(size_t size);
size_t CryCrtFree(void* p);
// wrapper for _msize on PC
size_t CryCrtSize(void* p);

#if !defined(NOT_USE_CRY_MEMORY_MANAGER)
#include "CryMemoryAllocator.h"
#endif

//////////////////////////////////////////////////////////////////////////
class ScopedSwitchToGlobalHeap
{
#if USE_LEVEL_HEAP
public:
    ILINE ScopedSwitchToGlobalHeap()
        : m_old(CryGetIMemoryManager()->LocalSwitchToGlobalHeap())
    {
    }

    ILINE ~ScopedSwitchToGlobalHeap()
    {
        CryGetIMemoryManager()->LocalSwitchToHeap(m_old);
    }

private:
    ScopedSwitchToGlobalHeap(const ScopedSwitchToGlobalHeap&);
    ScopedSwitchToGlobalHeap& operator = (const ScopedSwitchToGlobalHeap&);

private:
    int m_old;
#else
public:
    ILINE ScopedSwitchToGlobalHeap() {}
#endif
};

class CondScopedSwitchToGlobalHeap
{
#if USE_LEVEL_HEAP
public:
    ILINE CondScopedSwitchToGlobalHeap(bool cond)
        : m_old(0)
        , m_cond(cond)
    {
        IF (cond, 0)
        {
            m_old = CryGetIMemoryManager()->LocalSwitchToGlobalHeap();
        }
    }

    ILINE ~CondScopedSwitchToGlobalHeap()
    {
        IF (m_cond, 0)
        {
            CryGetIMemoryManager()->LocalSwitchToHeap(m_old);
        }
    }

private:
    CondScopedSwitchToGlobalHeap(const CondScopedSwitchToGlobalHeap&);
    CondScopedSwitchToGlobalHeap& operator = (const CondScopedSwitchToGlobalHeap&);

private:
    int m_old;
    bool m_cond;
#else
public:
    ILINE CondScopedSwitchToGlobalHeap(bool) {}
#endif
};

class ScopedSwitchToLevelHeap
{
#if USE_LEVEL_HEAP
public:
    ILINE ScopedSwitchToLevelHeap()
        : m_old(CryGetIMemoryManager()->LocalSwitchToLevelHeap())
    {
    }

    ILINE ~ScopedSwitchToLevelHeap()
    {
        CryGetIMemoryManager()->LocalSwitchToHeap(m_old);
    }

private:
    ScopedSwitchToLevelHeap(const ScopedSwitchToLevelHeap&);
    ScopedSwitchToLevelHeap& operator = (const ScopedSwitchToLevelHeap&);

private:
    int m_old;
#else
public:
    ILINE ScopedSwitchToLevelHeap() {}
#endif
};

// These utility functions should be used for allocating objects with specific alignment requirements on the heap.
// Note: On MSVC before 2013, only zero and one argument are supported, because C++11 support is not complete.
#if !defined(_MSC_VER) || _MSC_VER >= 1800

template<typename T, typename ... Args>
inline T* CryAlignedNew(Args&& ... args)
{
    void* pAlignedMemory = CryModuleMemalign(sizeof(T), std::alignment_of<T>::value);
    return new(pAlignedMemory) T(std::forward<Args>(args) ...);
}

#else

template<typename T>
inline T* CryAlignedNew()
{
    void* pAlignedMemory = CryModuleMemalign(sizeof(T), std::alignment_of<T>::value);
    return new(pAlignedMemory) T();
}

template<typename T, typename A1>
inline T* CryAlignedNew(A1&& a1)
{
    void* pAlignedMemory = CryModuleMemalign(sizeof(T), std::alignment_of<T>::value);
    return new(pAlignedMemory) T(std::forward<A1>(a1));
}

#endif

// This utility function should be used for allocating arrays of objects with specific alignment requirements on the heap.
// Note: The caller must remember the number of items in the array, since CryAlignedDeleteArray needs this information.
template<typename T>
inline T* CryAlignedNewArray(size_t count)
{
    T* const pAlignedMemory = reinterpret_cast<T*>(CryModuleMemalign(sizeof(T) * count, std::alignment_of<T>::value));
    T* pCurrentItem = pAlignedMemory;
    for (size_t i = 0; i < count; ++i, ++pCurrentItem)
    {
        new(static_cast<void*>(pCurrentItem))T();
    }
    return pAlignedMemory;
}

// Utility function that frees an object previously allocated with CryAlignedNew.
template<typename T>
inline void CryAlignedDelete(T* pObject)
{
    if (pObject)
    {
        pObject->~T();
        CryModuleMemalignFree(pObject);
    }
}

// Utility function that frees an array of objects previously allocated with CryAlignedNewArray.
// The same count used to allocate the array must be passed to this function.
template<typename T>
inline void CryAlignedDeleteArray(T* pObject, size_t count)
{
    if (pObject)
    {
        for (size_t i = 0; i < count; ++i)
        {
            (pObject + i)->~T();
        }
        CryModuleMemalignFree(pObject);
    }
}
