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
#ifndef AZ_ALLOCATOR_MANAGER_H
#define AZ_ALLOCATOR_MANAGER_H 1

#include <AzCore/base.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/parallel/mutex.h>

#include <AzCore/std/functional.h> // for callbacks

namespace AZ
{
    class IAllocator;

    /**
    * Global allocation manager. It has access to all
    * created allocators IAllocator interface. And control
    * some global allocations. This manager is NOT thread safe, so all allocators
    * should be created on the same thread (we can change that if needed, we just need a good reason).
    */
    class AllocatorManager
    {
        friend IAllocator;
        friend class Debug::AllocationRecords;
    public:
        typedef AZStd::function<void (IAllocator* allocator, size_t /*byteSize*/, size_t /*alignment*/, int/* flags*/, const char* /*name*/, const char* /*fileName*/, int lineNum /*=0*/)>    OutOfMemoryCBType;

        AZ_FORCE_INLINE static AllocatorManager& Instance()         { return g_allocMgr; }
        AZ_FORCE_INLINE  int            GetNumAllocators() const    { return m_numAllocators; }
        AZ_FORCE_INLINE  IAllocator*    GetAllocator(int index)
        {
            AZ_Assert(index < m_numAllocators, "Invalid allocator index %d [0,%d]!", index, m_numAllocators - 1);
            return m_allocators[index];
        }
        /// Calls all registered allocators garbage collect function.
        void    GarbageCollect();
        /// Add out of memory listener. True if add was successful, false if listener was already added.
        bool    AddOutOfMemoryListener(OutOfMemoryCBType& cb);
        /// Remove out of memory listener.
        void    RemoveOutOfMemoryListener();

        /// Set default memory track mode for all allocators created after this point.
        void    SetDefaultTrackingMode(AZ::Debug::AllocationRecords::Mode mode) { m_defaultTrackingRecordMode = mode; }
        AZ::Debug::AllocationRecords::Mode  GetDefaultTrackingMode() const      { return m_defaultTrackingRecordMode; }

        /// Set memory track mode for all allocators already created.
        void    SetTrackingMode(AZ::Debug::AllocationRecords::Mode mode);

        /// Especially for great code and engines...
        void    SetAllocatorLeaking(bool allowLeaking)  { m_isAllocatorLeaking = allowLeaking; }

        //////////////////////////////////////////////////////////////////////////
        // Debug support
        static const int MaxNumMemoryBreaks = 5;
        struct MemoryBreak
        {
            MemoryBreak();
            void*           addressStart;
            void*           addressEnd;
            size_t          byteSize;
            unsigned int    alignment;
            const char*     name;

            const char*     fileName;
            int             lineNum;
        };
        /// Installs a memory break on a specific slot (0 to MaxNumMemoryBreaks). Code will trigger and DebugBreak.
        void SetMemoryBreak(int slot, const MemoryBreak& mb);
        /// Reset a memory break. -1 all slots, otherwise (0 to MaxNumMemoryBreaks)
        void ResetMemoryBreak(int slot = -1);
        //////////////////////////////////////////////////////////////////////////
    private:
        void DebugBreak(void* address, const Debug::AllocationInfo& info);

        AllocatorManager(const AllocatorManager&);
        AllocatorManager& operator=(const AllocatorManager&);

        // Called from IAllocator
        void RegisterAllocator(IAllocator* alloc);
        void UnRegisterAllocator(IAllocator* alloc);

        static const int m_maxNumAllocators = 100;
        IAllocator*         m_allocators[m_maxNumAllocators];
        volatile int        m_numAllocators;
        OutOfMemoryCBType   m_outOfMemoryListener;
        bool                m_isAllocatorLeaking;
        MemoryBreak         m_memoryBreak[MaxNumMemoryBreaks];
        char                m_activeBreaks;
        AZStd::mutex        m_allocatorListMutex;

        AZ::Debug::AllocationRecords::Mode m_defaultTrackingRecordMode;

        AllocatorManager();
        ~AllocatorManager();

        static AllocatorManager g_allocMgr;    ///< The single instance of the allocator manager
    };
}

#endif // AZ_ALLOCATOR_MANAGER_H
#pragma once


