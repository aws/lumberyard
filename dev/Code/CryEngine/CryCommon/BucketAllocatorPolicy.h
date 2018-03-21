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

#ifndef CRYINCLUDE_CRYCOMMON_BUCKETALLOCATORPOLICY_H
#define CRYINCLUDE_CRYCOMMON_BUCKETALLOCATORPOLICY_H
#pragma once

#ifdef WIN32
#include <CryWindows.h>
#endif

#define BUCKET_ALLOCATOR_DEFAULT_MAX_SEGMENTS 8

#ifndef MEMORY_ALLOCATION_ALIGNMENT
#   error MEMORY ALLOCATION_ALIGNMENT is not defined
#endif

// Traits
#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(BucketAllocatorPolicy_h)
#else
#if defined(WIN32)
#define BUCKETALLOCATORPOLICY_H_TRAIT_USE_INTERLOCKED_FREELISTHEADER 1
#endif
#if defined(APPLE) || defined(LINUX)
#define BUCKETALLOCATORPOLICY_H_TRAIT_USE_LEGACY_FREELISTHEADER 1
#endif
#endif

namespace BucketAllocatorDetail
{
    struct AllocHeader
    {
        AllocHeader* volatile next;

#ifdef BUCKET_ALLOCATOR_TRAP_DOUBLE_DELETES
        uint32 magic;
#endif
    };

    struct SyncPolicyLocked
    {
#if defined(_WIN32)
        typedef SLIST_HEADER FreeListHeader;
#elif BUCKETALLOCATORPOLICY_H_TRAIT_USE_LEGACY_FREELISTHEADER
        typedef SLockFreeSingleLinkedListHeader FreeListHeader;
#endif

        typedef CryCriticalSectionNonRecursive Lock;

        Lock& GetRefillLock()
        {
            static Lock m_refillLock;
            return m_refillLock;
        }

        struct RefillLock
        {
            RefillLock(SyncPolicyLocked& policy)
                : m_policy(policy) { policy.GetRefillLock().Lock(); }
            ~RefillLock() { m_policy.GetRefillLock().Unlock(); }

        private:
            SyncPolicyLocked& m_policy;
        };

#if BUCKETALLOCATORPOLICY_H_TRAIT_USE_LEGACY_FREELISTHEADER

        static void PushOnto(FreeListHeader& list, AllocHeader* ptr)
        {
            CryInterlockedPushEntrySList(list, *reinterpret_cast<SLockFreeSingleLinkedListEntry*>(ptr));
        }

        static void PushListOnto(FreeListHeader& list, AllocHeader* head, AllocHeader* tail)
        {
            AllocHeader* item = head;
            AllocHeader* next;

            while (item)
            {
                next = item->next;
                item->next = NULL;

                PushOnto(list, item);

                item = next;
            }
        }

        static AllocHeader* PopOff(FreeListHeader& list)
        {
            return reinterpret_cast<AllocHeader*>(CryInterlockedPopEntrySList(list));
        }

        static AllocHeader* PopListOff(FreeListHeader& list)
        {
            return reinterpret_cast<AllocHeader*>(CryInterlockedFlushSList(list));
        }

        ILINE static bool IsFreeListEmpty(FreeListHeader& list)
        {
            return list.pNext == 0;
        }

#elif BUCKETALLOCATORPOLICY_H_TRAIT_USE_INTERLOCKED_FREELISTHEADER

        static void PushOnto(FreeListHeader& list, AllocHeader* ptr)
        {
            InterlockedPushEntrySList(&list, reinterpret_cast<PSLIST_ENTRY>(ptr));
        }

        static void PushListOnto(FreeListHeader& list, AllocHeader* head, AllocHeader* tail)
        {
            AllocHeader* item = head;
            AllocHeader* next;

            while (item)
            {
                next = item->next;
                item->next = NULL;

                PushOnto(list, item);

                item = next;
            }
        }

        static AllocHeader* PopOff(FreeListHeader& list)
        {
            return reinterpret_cast<AllocHeader*>(InterlockedPopEntrySList(&list));
        }

        static AllocHeader* PopListOff(FreeListHeader& list)
        {
            return reinterpret_cast<AllocHeader*>(InterlockedFlushSList(&list));
        }

        ILINE static bool IsFreeListEmpty(FreeListHeader& list)
        {
            return QueryDepthSList(&list) == 0;
        }
#endif
    };

    struct SyncPolicyUnlocked
    {
        typedef AllocHeader* FreeListHeader;

        struct RefillLock
        {
            RefillLock(SyncPolicyUnlocked&)
            {
            }
        };

        ILINE static void PushOnto(FreeListHeader& list, AllocHeader* ptr)
        {
            ptr->next = list;
            list = ptr;
        }

        ILINE static void PushListOnto(FreeListHeader& list, AllocHeader* head, AllocHeader* tail)
        {
            tail->next = list;
            list = head;
        }

        ILINE static AllocHeader* PopOff(FreeListHeader& list)
        {
            AllocHeader* top = list;
            if (top)
            {
                list = *(AllocHeader**)(&top->next); // cast away the volatile
            }
            return top;
        }

        ILINE static AllocHeader* PopListOff(FreeListHeader& list)
        {
            AllocHeader* pRet = list;
            list = NULL;
            return pRet;
        }

        ILINE static bool IsFreeListEmpty(FreeListHeader& list)
        {
            return list == NULL;
        }
    };

    template <size_t Size, typename SyncingPolicy, bool FallbackOnCRT = true, size_t MaxSegments = BUCKET_ALLOCATOR_DEFAULT_MAX_SEGMENTS>
    struct DefaultTraits
    {
        enum
        {
            MaxSize = 512,

#ifdef BUCKET_ALLOCATOR_PACK_SMALL_SIZES
            NumBuckets = 32 / 4 + (512 - 32) / 8,
#else
            NumBuckets = 512 / MEMORY_ALLOCATION_ALIGNMENT,
#endif

            PageLength = 64 * 1024,
            SmallBlockLength = 1024,
            SmallBlocksPerPage = 64,

            NumGenerations = 4,
            MaxNumSegments = MaxSegments,

            NumPages = Size / PageLength,

            FallbackOnCRTAllowed = FallbackOnCRT,
        };

        typedef SyncingPolicy SyncPolicy;

        static uint8 GetBucketForSize(size_t sz)
        {
#ifdef BUCKET_ALLOCATOR_PACK_SMALL_SIZES
            if (sz <= 32)
            {
                const int alignment = 4;
                size_t alignedSize = (sz + (alignment - 1)) & ~(alignment - 1);
                return alignedSize / alignment - 1;
            }
            else
            {
                const int alignment = 8;
                size_t alignedSize = (sz + (alignment - 1)) & ~(alignment - 1);
                alignedSize -= 32;
                return alignedSize / alignment + 7;
            }
#else
            const int alignment = MEMORY_ALLOCATION_ALIGNMENT;
            size_t alignedSize = (sz + (alignment - 1)) & ~(alignment - 1);
            return alignedSize / alignment - 1;
#endif
        }

        static size_t GetSizeForBucket(uint8 bucket)
        {
            size_t sz;

#ifdef BUCKET_ALLOCATOR_PACK_SMALL_SIZES
            if (bucket <= 7)
            {
                sz = (bucket + 1) * 4;
            }
            else
            {
                sz = (bucket - 7) * 8 + 32;
            }
#else
            sz = (bucket + 1) * MEMORY_ALLOCATION_ALIGNMENT;
#endif

#ifdef BUCKET_ALLOCATOR_TRAP_DOUBLE_DELETES
            return sz < sizeof(AllocHeader) ? sizeof(AllocHeader) : sz;
#else
            return sz;
#endif
        }

        static size_t GetGenerationForStability(uint8 stability)
        {
            return 3 - stability / 64;
        }
    };
}

#endif // CRYINCLUDE_CRYCOMMON_BUCKETALLOCATORPOLICY_H
