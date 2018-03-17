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

#ifndef CRYINCLUDE_CRYSYSTEM_MEMREPLAY_H
#define CRYINCLUDE_CRYSYSTEM_MEMREPLAY_H
#pragma once


#define REPLAY_RECORD_FREECS 1
#define REPLAY_RECORD_USAGE_CHANGES 0
#define REPLAY_RECORD_THREADED 1
#define REPLAY_RECORD_CONTAINERS 0

#if CAPTURE_REPLAY_LOG || ENABLE_STATOSCOPE

#include <AzCore/Socket/AzSocket_fwd.h>

#if !defined(LINUX) && !defined(APPLE)
#define REPLAY_SOCKET_WRITER
#endif

class IReplayWriter
{
public:
    virtual ~IReplayWriter() {}

    virtual bool Open() = 0;
    virtual void Close() = 0;

    virtual const char* GetFilename() const = 0;
    virtual uint64 GetWrittenLength() const = 0;

    virtual int Write(const void* data, size_t sz, size_t n) = 0;
    virtual void Flush() = 0;
};

class ReplayDiskWriter
    : public IReplayWriter
{
public:
    explicit ReplayDiskWriter(const char* pSuffix);

    bool Open();
    void Close();

    const char* GetFilename() const { return m_filename; }
    uint64 GetWrittenLength() const { return m_written; }

    int Write(const void* data, size_t sz, size_t n);
    void Flush();

private:
    char m_filename[MAX_PATH];

    FILE* m_fp;
    uint64 m_written;
};

class ReplaySocketWriter
    : public IReplayWriter
{
public:
    explicit ReplaySocketWriter(const char* address);

    bool Open();
    void Close();

    const char* GetFilename() const { return "socket"; }
    uint64 GetWrittenLength() const { return m_written; }

    int Write(const void* data, size_t sz, size_t n);
    void Flush();

private:
    char m_address[128];
    uint16 m_port;

    AZSOCKET m_sock;
    uint64 m_written;
};

#endif

#if CAPTURE_REPLAY_LOG

#include "ICryPak.h"
#include "HeapAllocator.h"
#include "CryZlib.h"
#include "CryThread.h"

namespace MemReplayEventIds
{
    enum Ids
    {
        RE_Alloc,
        RE_Free,
        RE_Callstack,
        RE_FrameStart,
        RE_Label,
        RE_ModuleRef,
        RE_AllocVerbose,
        RE_FreeVerbose,
        RE_Info,
        RE_PushContext,
        RE_PopContext,
        RE_Alloc3,
        RE_Free3,
        RE_PushContext2,
        RE_ModuleShortRef,
        RE_AddressProfile,
        RE_PushContext3,
        RE_Free4,
        RE_AllocUsage,
        RE_Info2,
        RE_Screenshot,
        RE_SizerPush,
        RE_SizerPop,
        RE_SizerAddRange,
        RE_AddressProfile2,
        RE_Alloc64,
        RE_Free64,
        RE_BucketMark,
        RE_BucketMark2,
        RE_BucketUnMark,
        RE_BucketUnMark2,
        RE_PoolMark,
        RE_PoolUnMark,
        RE_TextureAllocContext,
        RE_PageFault,
        RE_AddAllocReference,
        RE_RemoveAllocReference,
        RE_PoolMark2,
        RE_TextureAllocContext2,
        RE_BucketCleanupEnabled,
        RE_Info3,
        RE_Alloc4,
        RE_Realloc,
        RE_RegisterContainer,
        RE_UnregisterContainer,
        RE_BindToContainer,
        RE_UnbindFromContainer,
        RE_RegisterFixedAddressRange,
        RE_SwapContainers,
        RE_MapPage,
        RE_UnMapPage,
        RE_ModuleUnRef,
        RE_Alloc5,
        RE_Free5,
        RE_Realloc2,
        RE_AllocUsage2,
        RE_Alloc6,
        RE_Free6,
        RE_Realloc3,
        RE_UnregisterAddressRange,
        RE_MapPage2,
    };
}

namespace MemReplayPlatformIds
{
    enum Ids
    {
        PI_Unknown = 0,
        PI_PC,
        PI_Durango, // ACCEPTED_USE
        PI_Orbis // ACCEPTED_USE
    };
};

#pragma pack(push)
#pragma pack(1)

struct MemReplayLogHeader
{
    MemReplayLogHeader(uint32 _tag, uint32 _platform, uint32 _pointerWidth)
        : tag(_tag)
        , platform(_platform)
        , pointerWidth(_pointerWidth)
    {
    }

    uint32 tag;
    uint32 platform;
    uint32 pointerWidth;
} __PACKED;

struct MemReplayEventHeader
{
    MemReplayEventHeader(int _id, size_t _size, uint8 _sequenceCheck)
        : sequenceCheck(_sequenceCheck)
        , eventId(static_cast<uint8>(_id))
        , eventLength(static_cast<uint16>(_size))
    {
    }

    uint8 sequenceCheck;
    uint8 eventId;
    uint16 eventLength;
} __PACKED;

struct MemReplayFrameStartEvent
{
    static const int EventId = MemReplayEventIds::RE_FrameStart;

    uint32 frameId;

    MemReplayFrameStartEvent(uint32 _frameId)
        : frameId(_frameId)
    {}
} __PACKED;

struct MemReplayLabelEvent
{
    static const int EventId = MemReplayEventIds::RE_Label;

    char label[1];

    MemReplayLabelEvent(const char* _label)
    {
        // Assume there is room beyond this instance.
        strcpy(this->label, _label); // we're intentionally writing beyond the end of this array, so don't use cry_strcpy()
    }
} __PACKED;

struct MemReplayPushContextEvent
{
    static const int EventId = MemReplayEventIds::RE_PushContext3;

    uint32 threadId;
    uint32 contextType;
    uint32 flags;

    // This field must be the last in the structure, and enough memory should be allocated
    // for the structure to hold the required name.
    char name[1];

    MemReplayPushContextEvent(uint32 _threadId, const char* _name, EMemStatContextTypes::Type _type, uint32 _flags)
    {
        // We're going to assume that there actually is enough space to store the name directly in the struct.

        this->threadId = _threadId;
        this->contextType = static_cast<uint32>(_type);
        this->flags = _flags;
        strcpy(this->name, _name); // we're intentionally writing beyond the end of this array, so don't use cry_strcpy()
    }
} __PACKED;

struct MemReplayPopContextEvent
{
    static const int EventId = MemReplayEventIds::RE_PopContext;

    uint32 threadId;

    explicit MemReplayPopContextEvent(uint32 _threadId)
    {
        this->threadId = _threadId;
    }
} __PACKED;

struct MemReplayModuleRefEvent
{
    static const int EventId = MemReplayEventIds::RE_ModuleRef;

    char name[256];
    char path[256];
    char sig[512];

    MemReplayModuleRefEvent(const char* _name, const char* _path, const char* _sig)
    {
        cry_strcpy(this->name, _name);
        cry_strcpy(this->path, _path);
        cry_strcpy(this->sig, _sig);
    }
} __PACKED;

struct MemReplayModuleUnRefEvent
{
    static const int EventId = MemReplayEventIds::RE_ModuleUnRef;

    UINT_PTR address;

    MemReplayModuleUnRefEvent(UINT_PTR _address)
        : address(_address) {}
} __PACKED;

struct MemReplayModuleRefShortEvent
{
    static const int EventId = MemReplayEventIds::RE_ModuleShortRef;

    char name[256];

    MemReplayModuleRefShortEvent(const char* _name)
    {
        cry_strcpy(this->name, _name);
    }
} __PACKED;

struct MemReplayAllocEvent
{
    static const int EventId = MemReplayEventIds::RE_Alloc6;

    uint32 threadId;
    UINT_PTR id;
    uint32 alignment;
    uint32 sizeRequested;
    uint32 sizeConsumed;
    int32 sizeGlobal;           //  Inferred from changes in global memory status

    uint16 moduleId;
    uint16 allocClass;
    uint16 allocSubClass;
    uint16 callstackLength;
    UINT_PTR callstack[1];  // Must be last.

    MemReplayAllocEvent(uint32 _threadId, uint16 _moduleId, uint16 _allocClass, uint16 _allocSubClass, UINT_PTR _id, uint32 _alignment, uint32 _sizeReq, uint32 _sizeCon, int32 _sizeGlobal)
        : threadId(_threadId)
        , id(_id)
        , alignment(_alignment)
        , sizeRequested(_sizeReq)
        , sizeConsumed(_sizeCon)
        , sizeGlobal(_sizeGlobal)
        , moduleId(_moduleId)
        , allocClass(_allocClass)
        , allocSubClass(_allocSubClass)
        , callstackLength(0)
    {
    }
} __PACKED;

struct MemReplayFreeEvent
{
    static const int EventId = MemReplayEventIds::RE_Free6;

    uint32 threadId;
    UINT_PTR id;
    int32 sizeGlobal;           //  Inferred from changes in global memory status

    uint16 moduleId;
    uint16 allocClass;
    uint16 allocSubClass;

    uint16 callstackLength;
    UINT_PTR callstack[1];  // Must be last.

    MemReplayFreeEvent(uint32 _threadId, uint16 _moduleId, uint16 _allocClass, uint16 _allocSubClass, UINT_PTR _id, int32 _sizeGlobal)
        : threadId(_threadId)
        , id(_id)
        , sizeGlobal(_sizeGlobal)
        , moduleId(_moduleId)
        , allocClass(_allocClass)
        , allocSubClass(_allocSubClass)
        , callstackLength(0)
    {
    }
} __PACKED;

struct MemReplayInfoEvent
{
    static const int EventId = MemReplayEventIds::RE_Info3;

    uint32 executableSize;
    uint32 initialGlobalSize;
    uint32 bucketsFree;

    MemReplayInfoEvent(uint32 _executableSize, uint32 _initialGlobalSize, uint32 _bucketsFree)
        : executableSize(_executableSize)
        , initialGlobalSize(_initialGlobalSize)
        , bucketsFree(_bucketsFree)
    {
    }
} __PACKED;

struct MemReplayAddressProfileEvent
{
    static const int EventId = MemReplayEventIds::RE_AddressProfile2;

    UINT_PTR rsxStart;
    uint32 rsxLength;

    MemReplayAddressProfileEvent(UINT_PTR _rsxStart, uint32 _rsxLength)
        : rsxStart(_rsxStart)
        , rsxLength(_rsxLength)
    {
    }
} __PACKED;

struct MemReplayAllocUsageEvent
{
    static const int EventId = MemReplayEventIds::RE_AllocUsage2;

    uint32 allocClass;
    UINT_PTR id;
    uint32 used;

    MemReplayAllocUsageEvent(uint16 _allocClass, UINT_PTR _id, uint32 _used)
        : allocClass(_allocClass)
        , id(_id)
        , used(_used)
    {
    }
} __PACKED;

struct MemReplayScreenshotEvent
{
    static const int EventId = MemReplayEventIds::RE_Screenshot;

    uint8 bmp[1];

    MemReplayScreenshotEvent()
    {
    }
} __PACKED;

struct MemReplaySizerPushEvent
{
    static const int EventId = MemReplayEventIds::RE_SizerPush;

    char name[1];

    MemReplaySizerPushEvent(const char* _name)
    {
        strcpy(this->name, _name);
    }
} __PACKED;

struct MemReplaySizerPopEvent
{
    static const int EventId = MemReplayEventIds::RE_SizerPop;
} __PACKED;

struct MemReplaySizerAddRangeEvent
{
    static const int EventId = MemReplayEventIds::RE_SizerAddRange;

    UINT_PTR address;
    uint32 size;
    int32 count;

    MemReplaySizerAddRangeEvent(const UINT_PTR _address, uint32 _size, int32 _count)
        : address(_address)
        , size(_size)
        , count(_count)
    {}
} __PACKED;

struct MemReplayBucketMarkEvent
{
    static const int EventId = MemReplayEventIds::RE_BucketMark2;

    UINT_PTR address;
    uint32 length;
    int32 index;
    uint32 alignment;

    MemReplayBucketMarkEvent(UINT_PTR _address, uint32 _length, int32 _index, uint32 _alignment)
        : address(_address)
        , length(_length)
        , index(_index)
        , alignment(_alignment)
    {}
} __PACKED;

struct MemReplayBucketUnMarkEvent
{
    static const int EventId = MemReplayEventIds::RE_BucketUnMark2;

    UINT_PTR address;
    int32 index;

    MemReplayBucketUnMarkEvent(UINT_PTR _address, int32 _index)
        : address(_address)
        , index(_index)
    {}
} __PACKED;

struct MemReplayAddAllocReferenceEvent
{
    static const int EventId = MemReplayEventIds::RE_AddAllocReference;

    UINT_PTR address;
    UINT_PTR referenceId;
    uint32 callstackLength;
    UINT_PTR callstack[1];

    MemReplayAddAllocReferenceEvent(UINT_PTR _address, UINT_PTR _referenceId)
        : address(_address)
        , referenceId(_referenceId)
        , callstackLength(0)
    {
    }
} __PACKED;

struct MemReplayRemoveAllocReferenceEvent
{
    static const int EventId = MemReplayEventIds::RE_RemoveAllocReference;

    UINT_PTR referenceId;

    MemReplayRemoveAllocReferenceEvent(UINT_PTR _referenceId)
        : referenceId(_referenceId)
    {
    }
} __PACKED;

struct MemReplayPoolMarkEvent
{
    static const int EventId = MemReplayEventIds::RE_PoolMark2;

    UINT_PTR address;
    uint32 length;
    int32 index;
    uint32 alignment;
    char name[1];

    MemReplayPoolMarkEvent(UINT_PTR _address, uint32 _length, int32 _index, uint32 _alignment, const char* _name)
        : address(_address)
        , length(_length)
        , index(_index)
        , alignment(_alignment)
    {
        strcpy(this->name, _name);
    }
} __PACKED;

struct MemReplayPoolUnMarkEvent
{
    static const int EventId = MemReplayEventIds::RE_PoolUnMark;

    UINT_PTR address;
    int32 index;

    MemReplayPoolUnMarkEvent(UINT_PTR _address, int32 _index)
        : address(_address)
        , index(_index)
    {}
} __PACKED;

struct MemReplayTextureAllocContextEvent
{
    static const int EventId = MemReplayEventIds::RE_TextureAllocContext2;

    UINT_PTR address;
    uint32 mip;
    uint32 width;
    uint32 height;
    uint32 flags;
    char name[1];

    MemReplayTextureAllocContextEvent(UINT_PTR _ptr, uint32 _mip, uint32 _width, uint32 _height, uint32 _flags, const char* _name)
        : address(_ptr)
        , mip(_mip)
        , width(_width)
        , height(_height)
        , flags(_flags)
    {
        strcpy(this->name, _name);
    }
} __PACKED;

struct MemReplayBucketCleanupEnabledEvent
{
    static const int EventId = MemReplayEventIds::RE_BucketCleanupEnabled;

    UINT_PTR allocatorBaseAddress;
    uint32 cleanupsEnabled;

    MemReplayBucketCleanupEnabledEvent(UINT_PTR _allocatorBaseAddress, bool _cleanupsEnabled)
        : allocatorBaseAddress(_allocatorBaseAddress)
        , cleanupsEnabled(_cleanupsEnabled ? 1 : 0)
    {
    }
} __PACKED;

struct MemReplayReallocEvent
{
    static const int EventId = MemReplayEventIds::RE_Realloc3;

    uint32 threadId;
    UINT_PTR oldId;
    UINT_PTR newId;
    uint32 alignment;
    uint32 newSizeRequested;
    uint32 newSizeConsumed;
    int32 sizeGlobal;           //  Inferred from changes in global memory status

    uint16 moduleId;
    uint16 allocClass;
    uint16 allocSubClass;

    uint16 callstackLength;
    UINT_PTR callstack[1];  // Must be last.

    MemReplayReallocEvent(uint32 _threadId, uint16 _moduleId, uint16 _allocClass, uint16 _allocSubClass, UINT_PTR _oldId, UINT_PTR _newId, uint32 _alignment, uint32 _newSizeReq, uint32 _newSizeCon, int32 _sizeGlobal)
        : threadId(_threadId)
        , oldId(_oldId)
        , newId(_newId)
        , alignment(_alignment)
        , newSizeRequested(_newSizeReq)
        , newSizeConsumed(_newSizeCon)
        , sizeGlobal(_sizeGlobal)
        , moduleId(_moduleId)
        , allocClass(_allocClass)
        , allocSubClass(_allocSubClass)
        , callstackLength(0)
    {
    }
} __PACKED;

struct MemReplayRegisterContainerEvent
{
    static const int EventId = MemReplayEventIds::RE_RegisterContainer;

    UINT_PTR key;
    uint32 type;

    uint32 callstackLength;
    UINT_PTR callstack[1]; // Must be last

    MemReplayRegisterContainerEvent(UINT_PTR _key, uint32 _type)
        : key(_key)
        , type(_type)
        , callstackLength(0)
    {
    }
} __PACKED;

struct MemReplayUnregisterContainerEvent
{
    static const int EventId = MemReplayEventIds::RE_UnregisterContainer;

    UINT_PTR key;

    explicit MemReplayUnregisterContainerEvent(UINT_PTR _key)
        : key(_key)
    {
    }
} __PACKED;

struct MemReplayBindToContainerEvent
{
    static const int EventId = MemReplayEventIds::RE_BindToContainer;

    UINT_PTR key;
    UINT_PTR ptr;

    MemReplayBindToContainerEvent(UINT_PTR _key, UINT_PTR _ptr)
        : key(_key)
        , ptr(_ptr)
    {
    }
} __PACKED;

struct MemReplayUnbindFromContainerEvent
{
    static const int EventId = MemReplayEventIds::RE_UnbindFromContainer;

    UINT_PTR key;
    UINT_PTR ptr;

    MemReplayUnbindFromContainerEvent(UINT_PTR _key, UINT_PTR _ptr)
        : key(_key)
        , ptr(_ptr)
    {
    }
} __PACKED;

struct MemReplayRegisterFixedAddressRangeEvent
{
    static const int EventId = MemReplayEventIds::RE_RegisterFixedAddressRange;

    UINT_PTR address;
    uint32 length;

    char name[1];

    MemReplayRegisterFixedAddressRangeEvent(UINT_PTR _address, uint32 _length, const char* _name)
        : address(_address)
        , length(_length)
    {
        strcpy(this->name, _name);
    }
} __PACKED;

struct MemReplaySwapContainersEvent
{
    static const int EventId = MemReplayEventIds::RE_SwapContainers;

    UINT_PTR keyA;
    UINT_PTR keyB;

    MemReplaySwapContainersEvent(UINT_PTR _keyA, UINT_PTR _keyB)
        : keyA(_keyA)
        , keyB(_keyB)
    {
    }
} __PACKED;

struct MemReplayMapPageEvent
{
    static const int EventId = MemReplayEventIds::RE_MapPage2;

    UINT_PTR address;
    uint32 length;
    uint32 callstackLength;
    UINT_PTR callstack[1];

    MemReplayMapPageEvent(UINT_PTR _address, uint32 _length)
        : address(_address)
        , length(_length)
        , callstackLength(0)
    {
    }
} __PACKED;

struct MemReplayUnMapPageEvent
{
    static const int EventId = MemReplayEventIds::RE_UnMapPage;

    UINT_PTR address;
    uint32 length;

    MemReplayUnMapPageEvent(UINT_PTR _address, uint32 _length)
        : address(_address)
        , length(_length)
    {
    }
} __PACKED;

#pragma pack(pop)

#if defined(_WIN32) || defined(LINUX) || defined(APPLE)

class ReplayAllocatorBase
{
public:
    enum
    {
        PageSize = 4096
    };

public:
    void* ReserveAddressSpace(size_t sz);
    void UnreserveAddressSpace(void* base, void* committedEnd);

    void* MapAddressSpace(void* addr, size_t sz);
};

#endif

class ReplayAllocator
    : private ReplayAllocatorBase
{
public:
    ReplayAllocator();
    ~ReplayAllocator();

    void Reserve(size_t sz);

    void* Allocate(size_t sz);
    void Free();

    size_t GetCommittedSize() const { return static_cast<size_t>(reinterpret_cast<INT_PTR>(m_commitEnd) - reinterpret_cast<INT_PTR>(m_heap)); }

private:
    ReplayAllocator(const ReplayAllocator&);
    ReplayAllocator& operator = (const ReplayAllocator&);

private:
    enum
    {
        MaxSize = 6 * 1024 * 1024
    };

private:
    CryCriticalSectionNonRecursive m_lock;

    LPVOID m_heap;
    LPVOID m_commitEnd;
    LPVOID m_allocEnd;
};

class ReplayCompressor
{
public:
    ReplayCompressor(ReplayAllocator& allocator, IReplayWriter* writer);
    ~ReplayCompressor();

    void Flush();
    void Write(const uint8* data, size_t len);

private:
    enum
    {
        CompressTargetSize = 64 * 1024
    };

private:
    static voidpf zAlloc (voidpf opaque, uInt items, uInt size);
    static void   zFree  (voidpf opaque, voidpf address);

private:
    ReplayCompressor(const ReplayCompressor&);
    ReplayCompressor& operator = (const ReplayCompressor&);

private:
    ReplayAllocator* m_allocator;
    IReplayWriter* m_writer;

    uint8* m_compressTarget;
    z_stream m_compressStream;
    int m_zSize;
};

#if REPLAY_RECORD_THREADED

void CryMemStatIgnoreThread(DWORD threadId);

class ReplayRecordThread
    : public CryThread<>
{
public:
    ReplayRecordThread(ReplayAllocator& allocator, IReplayWriter* writer);

    void Flush();
    void Write(const uint8* data, size_t len);

public:
    void Run();
    void Cancel();

private:
    enum Command
    {
        CMD_Idle = 0,
        CMD_Write,
        CMD_Flush,
        CMD_Stop
    };

private:

    ReplayCompressor m_compressor;
    volatile Command m_nextCommand;
    const uint8* volatile m_nextData;
    long volatile m_nextLength;
};
#endif

class ReplayLogStream
{
public:
    ReplayLogStream();
    ~ReplayLogStream();

    bool Open(const char* openString);
    void Close();

    bool IsOpen() const { return m_isOpen != 0; }
    const char* const GetFilename() { assert(m_writer); return m_writer->GetFilename(); }

    template <typename T>
    ILINE T* BeginAllocateRawEvent(size_t evAdditionalReserveLength)
    {
        return reinterpret_cast<T*>(BeginAllocateRaw(sizeof(T) + evAdditionalReserveLength));
    }

    template <typename T>
    ILINE void EndAllocateRawEvent(size_t evAdditionalLength)
    {
        EndAllocateRaw(T::EventId, sizeof(T) + evAdditionalLength);
    }

    template <typename T>
    ILINE T* AllocateRawEvent(size_t evAdditionalLength)
    {
        return reinterpret_cast<T*>(AllocateRaw(T::EventId, sizeof(T) + evAdditionalLength));
    }

    template <typename T>
    ILINE T* AllocateRawEvent()
    {
        return AllocateRaw(T::EventId, sizeof(T));
    }

    void WriteRawEvent(uint8 id, const void* event, uint eventSize)
    {
        void* ev = AllocateRaw(id, eventSize);
        memcpy(ev, event, eventSize);
    }

    template <typename T>
    ILINE void WriteEvent(const T* ev, size_t evAdditionalLength)
    {
        WriteRawEvent(T::EventId, ev, sizeof(T) + evAdditionalLength);
    }

    template <typename T>
    ILINE void WriteEvent(const T& ev)
    {
        WriteRawEvent(T::EventId, &ev, sizeof(T));
    }

    void Flush();
    void FullFlush();

    size_t GetSize() const;
    uint64 GetWrittenLength() const;
    uint64 GetUncompressedLength() const;

private:
    ReplayLogStream(const ReplayLogStream&);
    ReplayLogStream& operator = (const ReplayLogStream&);

private:
    void* BeginAllocateRaw(size_t evReserveLength)
    {
        uint32 size = sizeof(MemReplayEventHeader) + evReserveLength;
        size = (size + 3) & ~3;
        evReserveLength = size - sizeof(MemReplayEventHeader);

        if (((&m_buffer[m_bufferSize]) - m_bufferEnd) < (int)size)
        {
            Flush();
        }

        return reinterpret_cast<MemReplayEventHeader*>(m_bufferEnd) + 1;
    }

    void EndAllocateRaw(uint8 id, size_t evLength)
    {
        uint32 size = sizeof(MemReplayEventHeader) + evLength;
        size = (size + 3) & ~3;
        evLength = size - sizeof(MemReplayEventHeader);

        MemReplayEventHeader* header = reinterpret_cast<MemReplayEventHeader*>(m_bufferEnd);
        new (header) MemReplayEventHeader(id, evLength, m_sequenceCheck);

        ++m_sequenceCheck;
        m_bufferEnd += size;
    }

    void* AllocateRaw(uint8 id, uint eventSize)
    {
        void* result = BeginAllocateRaw(eventSize);
        EndAllocateRaw(id, eventSize);
        return result;
    }

private:
    ReplayAllocator m_allocator;

    volatile int m_isOpen;

    uint8* m_buffer;
    size_t m_bufferSize;
    uint8* m_bufferEnd;

    uint64 m_uncompressedLen;
    uint8 m_sequenceCheck;

#if REPLAY_RECORD_THREADED
    uint8* m_bufferA;
    uint8* m_bufferB;
    ReplayRecordThread* m_recordThread;
#else
    uint8* m_bufferA;
    ReplayCompressor* m_compressor;
#endif

    IReplayWriter* m_writer;
};

class MemReplayCrySizer
    : public ICrySizer
{
public:
    MemReplayCrySizer(ReplayLogStream& stream, const char* name);
    ~MemReplayCrySizer();

    virtual void Release() { delete this; }

    virtual size_t GetTotalSize();
    virtual size_t GetObjectCount();
    virtual void Reset();
    virtual void End() {}

    virtual bool AddObject (const void* pIdentifier, size_t nSizeBytes, int nCount = 1);

    virtual IResourceCollector* GetResourceCollector();
    virtual void SetResourceCollector(IResourceCollector*);

    virtual void Push (const char* szComponentName);
    virtual void PushSubcomponent (const char* szSubcomponentName);
    virtual void Pop ();

private:
    ReplayLogStream* m_stream;

    size_t m_totalSize;
    size_t m_totalCount;
    std::set<const void*> m_addedObjects;
};

extern int GetPageBucketAlloc_wasted_in_allocation();
extern int GetPageBucketAlloc_get_free();

class CReplayModules
{
public:
    struct ModuleLoadDesc
    {
        UINT_PTR address;
        UINT_PTR size;
        char name[256];
        char path[256];
        char sig[512];
    };

    struct ModuleUnloadDesc
    {
        UINT_PTR address;
    };

    typedef void (* FModuleLoad)(void*, const ModuleLoadDesc& desc);
    typedef void (* FModuleUnload)(void*, const ModuleUnloadDesc& desc);

public:
    CReplayModules()
        : m_numKnownModules(0)
    {
    }

    void RefreshModules(FModuleLoad onLoad, FModuleUnload onUnload, void* pUser);

private:
    struct Module
    {
        UINT_PTR baseAddress;
        UINT_PTR size;
        UINT timestamp;

        Module(){}
        Module(UINT_PTR ba, UINT_PTR s, UINT t)
            : baseAddress(ba)
            , size(s)
            , timestamp(t) {}

        friend bool operator < (Module const& a, Module const& b)
        {
            if (a.baseAddress != b.baseAddress)
            {
                return a.baseAddress < b.baseAddress;
            }
            if (a.size != b.size)
            {
                return a.size < b.size;
            }
            return a.timestamp < b.timestamp;
        }
    };
#if AZ_LEGACY_CRYSYSTEM_TRAIT_MEMREPLAY_MODULE_STATE
    struct EnumModuleState
    {
        CReplayModules* pSelf;
        HANDLE hProcess;
        FModuleLoad onLoad;
        void* pOnLoadUser;
    };
#endif

private:

#if defined(LINUX)
    static int EnumModules_Linux(struct dl_phdr_info* info, size_t size, void* userContext);
#endif

private:
    Module m_knownModules[128];
    size_t m_numKnownModules;
};

//////////////////////////////////////////////////////////////////////////
class CMemReplay
    : public IMemReplay
{
public:
    static CMemReplay* GetInstance();

public:
    CMemReplay();
    ~CMemReplay();

    //////////////////////////////////////////////////////////////////////////
    // IMemReplay interface implementation
    //////////////////////////////////////////////////////////////////////////
    void DumpStats();
    void DumpSymbols();

    void StartOnCommandLine(const char* cmdLine);
    void Start(bool bPaused, const char* openString);
    void Stop();
    void Flush();

    void GetInfo(CryReplayInfo& infoOut);

    // Call to begin a new allocation scope.
    bool EnterScope(EMemReplayAllocClass::Class cls, uint16 subCls, int moduleId);

    // Records an event against the currently active scope and exits it.
    void ExitScope_Alloc(UINT_PTR id, UINT_PTR sz, UINT_PTR alignment = 0);
    void ExitScope_Realloc(UINT_PTR originalId, UINT_PTR newId, UINT_PTR sz, UINT_PTR alignment = 0);
    void ExitScope_Free(UINT_PTR id);
    void ExitScope();

    bool EnterLockScope();
    void LeaveLockScope();

    void AllocUsage(EMemReplayAllocClass::Class allocClass, UINT_PTR id, UINT_PTR used);

    void AddAllocReference(void* ptr, void* ref);
    void RemoveAllocReference(void* ref);

    void AddLabel(const char* label);
    void AddLabelFmt(const char* labelFmt, ...);
    void AddFrameStart();
    void AddScreenshot();

    void AddContext(int type, uint32 flags, const char* str);
    void AddContextV(int type, uint32 flags, const char* format, va_list args);
    void RemoveContext();

    void OwnMemory (void* p, size_t size);

    void MapPage(void* base, size_t size);
    void UnMapPage(void* base, size_t size);

    void RegisterFixedAddressRange(void* base, size_t size, const char* name);
    void MarkBucket(int bucket, size_t alignment, void* base, size_t length);
    void UnMarkBucket(int bucket, void* base);
    void BucketEnableCleanups(void* allocatorBase, bool enabled);

    void MarkPool(int pool, size_t alignment, void* base, size_t length, const char* name);
    void UnMarkPool(int pool, void* base);
    void AddTexturePoolContext(void* ptr, int mip, int width, int height, const char* name, uint32 flags);

    void AddSizerTree(const char* name);

    void RegisterContainer(const void* key, int type);
    void UnregisterContainer(const void* key);
    void BindToContainer(const void* key, const void* alloc);
    void UnbindFromContainer(const void* key, const void* alloc);
    void SwapContainers(const void* keyA, const void* keyB);
    //////////////////////////////////////////////////////////////////////////

private:
    static void RecordModuleLoad(void* pSelf, const CReplayModules::ModuleLoadDesc& mld);
    static void RecordModuleUnload(void* pSelf, const CReplayModules::ModuleUnloadDesc& mld);

private:
    CMemReplay(const CMemReplay&);
    CMemReplay& operator = (const CMemReplay&);

private:
    void RecordAlloc(EMemReplayAllocClass::Class cls, uint16 subCls, int moduleId, UINT_PTR p, UINT_PTR alignment, UINT_PTR sizeRequested, UINT_PTR sizeConsumed, INT_PTR sizeGlobal);
    void RecordRealloc(EMemReplayAllocClass::Class cls, uint16 subCls, int moduleId, UINT_PTR op, UINT_PTR p, UINT_PTR alignment, UINT_PTR sizeRequested, UINT_PTR sizeConsumed, INT_PTR sizeGlobal);
    void RecordFree(EMemReplayAllocClass::Class cls, uint16 subCls, int moduleId, UINT_PTR p, INT_PTR sizeGlobal, bool captureCallstack);
    void RecordModules();

private:
    volatile uint32 m_allocReference;
    ReplayLogStream m_stream;
    CReplayModules m_modules;

    CryCriticalSection m_scope;

    int m_scopeDepth;
    EMemReplayAllocClass::Class m_scopeClass;
    uint16 m_scopeSubClass;
    int m_scopeModuleId;
};
//////////////////////////////////////////////////////////////////////////

#endif

#endif // CRYINCLUDE_CRYSYSTEM_MEMREPLAY_H
