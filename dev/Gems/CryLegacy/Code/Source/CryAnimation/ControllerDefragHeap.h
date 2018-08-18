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

#ifndef CRYINCLUDE_CRYANIMATION_CONTROLLERDEFRAGHEAP_H
#define CRYINCLUDE_CRYANIMATION_CONTROLLERDEFRAGHEAP_H
#pragma once

class IControllerRelocatableChain;

class CControllerDefragHdl
{
public:
    CControllerDefragHdl()
        : m_hdlAndFlags(0)
    {
    }

    explicit CControllerDefragHdl(void* pFixed)
        : m_hdlAndFlags((UINT_PTR)pFixed)
    {
    }

    explicit CControllerDefragHdl(IDefragAllocator::Hdl hdl)
        : m_hdlAndFlags((hdl << 1) | IsHandleMask)
    {
    }

    bool IsFixed() const { return (m_hdlAndFlags & IsHandleMask) == 0; }
    IDefragAllocator::Hdl AsHdl() const { return (IDefragAllocator::Hdl)(m_hdlAndFlags >> 1); }
    void* AsFixed() const { return (void*)m_hdlAndFlags; }

    bool IsValid() const { return m_hdlAndFlags != 0; }

    friend bool operator == (CControllerDefragHdl a, CControllerDefragHdl b)
    {
        return a.m_hdlAndFlags == b.m_hdlAndFlags;
    }

    friend bool operator != (CControllerDefragHdl a, CControllerDefragHdl b)
    {
        return a.m_hdlAndFlags != b.m_hdlAndFlags;
    }

private:
    enum
    {
        IsHandleMask = 0x1
    };

private:
    UINT_PTR m_hdlAndFlags;
};

class CControllerDefragHeap
    : private IDefragAllocatorPolicy
{
public:
    struct Stats
    {
        IDefragAllocatorStats defragStats;
        size_t bytesInFixedAllocs;
    };

public:
    CControllerDefragHeap();
    ~CControllerDefragHeap();

    void Init(size_t capacity);
    bool IsInitialised() const { return m_pAddressRange != NULL; }

    Stats GetStats();

    void Update();

    CControllerDefragHdl AllocPinned(size_t sz, IControllerRelocatableChain* pContext);
    void Free(CControllerDefragHdl hdl);

    void ChangeContext(CControllerDefragHdl hdl, IControllerRelocatableChain* pContext)
    {
        if (!hdl.IsFixed())
        {
            m_pAllocator->ChangeContext(hdl.AsHdl(), pContext);
        }
    }

    size_t UsableSize(CControllerDefragHdl hdl);

    void* WeakPin(CControllerDefragHdl hdl)
    {
        if (!hdl.IsFixed())
        {
            return m_pBaseAddress + m_pAllocator->WeakPin(hdl.AsHdl());
        }
        else
        {
            return hdl.AsFixed();
        }
    }

    void* Pin(CControllerDefragHdl hdl)
    {
        if (!hdl.IsFixed())
        {
            return m_pBaseAddress + m_pAllocator->Pin(hdl.AsHdl());
        }
        else
        {
            return hdl.AsFixed();
        }
    }

    void Unpin(CControllerDefragHdl hdl)
    {
        if (!hdl.IsFixed())
        {
            m_pAllocator->Unpin(hdl.AsHdl());
        }
    }

private:
    union FixedHdr
    {
        char pad[16];
        struct
        {
            uint32 size;
            bool bFromGPH;
        };
    };

    struct Copy
    {
        Copy()
        {
            memset(this, 0, sizeof(*this));
        }

        Copy(void* _pContext, UINT_PTR _dstOffset, UINT_PTR _srcOffset, UINT_PTR _size, IDefragAllocatorCopyNotification* _pNotification)
            : inUse(true)
            , pContext(_pContext)
            , dstOffset(_dstOffset)
            , srcOffset(_srcOffset)
            , size(_size)
            , pNotification(_pNotification)
            , jobState(-1)
            , relocateFrameId(0)
            , cancelled(false)
            , relocated(false)
        {
        }

        bool inUse;

        void* pContext;
        UINT_PTR dstOffset;
        UINT_PTR srcOffset;
        UINT_PTR size;
        IDefragAllocatorCopyNotification* pNotification;

        volatile int jobState;
        uint32 relocateFrameId;
        bool cancelled;
        bool relocated;
    };

    enum
    {
        CompletionLatencyFrames = 2,
        MaxInFlightCopies = 32,
        MaxScheduledCopiesPerUpdate = 4,
        MaxScheduledBytesPerUpdate = 128 * 1024,
        MinJobCopySize = 4096,
        MinFixedAllocSize = 384 * 1024,
        MinAlignment = 16,
        MaxAllocs = 4 * 1024,
        AllowGPHFallback = 1,
    };

private:
    virtual uint32 BeginCopy(void* context, UINT_PTR dstOffset, UINT_PTR srcOffset, UINT_PTR size, IDefragAllocatorCopyNotification* pNotification);
    virtual void Relocate(uint32 userMoveId, void* pContext, UINT_PTR newOffset, UINT_PTR oldOffset, UINT_PTR size);
    virtual void CancelCopy(uint32 userMoveId, void* pContext, bool bSync);

    virtual void SyncCopy(void* context, UINT_PTR dstOffset, UINT_PTR srcOffset, UINT_PTR size);

private:
    CControllerDefragHeap(const CControllerDefragHeap&);
    CControllerDefragHeap& operator = (const CControllerDefragHeap&);

private:
    bool IncreaseRange(UINT_PTR offs, UINT_PTR sz);
    void DecreaseRange(UINT_PTR offs, UINT_PTR sz);
    void DecreaseRange_Locked(UINT_PTR offs, UINT_PTR sz);

    void* FixedAlloc(size_t sz, bool bFromGPH);
    void FixedFree(void* p);

    void UpdateInflight(int frameId);

private:
    IMemoryAddressRange* m_pAddressRange;
    IDefragAllocator* m_pAllocator;

    char* m_pBaseAddress;
    uint32 m_nPageSize;
    uint32 m_nLogPageSize;

    CryCriticalSectionNonRecursive m_pagesLock;
    std::vector<int> m_numAllocsPerPage;

    volatile int m_nBytesInFixedAllocs;

    Copy m_copiesInFlight[MaxInFlightCopies];
    size_t m_numAvailableCopiesInFlight;
    size_t m_bytesInFlight;
    int m_tickId;
    uint32 m_nLastCopyIdx;

    CryCriticalSectionNonRecursive m_queuedCancelLock;
    int m_numQueuedCancels;
    uint32 m_queuedCancels[MaxInFlightCopies];
};

#endif // CRYINCLUDE_CRYANIMATION_CONTROLLERDEFRAGHEAP_H
