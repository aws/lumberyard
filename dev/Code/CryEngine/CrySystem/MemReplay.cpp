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

#include "StdAfx.h"
#include "BitFiddling.h"

#include <stdio.h>
#include <algorithm>  // std::min()
#include <ISystem.h>
#include <platform.h>

#include "MemoryManager.h"

#include "MemReplay.h"

#include "System.h"

#include <AzCore/Debug/StackTracer.h>
#include <AzCore/Socket/AzSocket.h>

//#pragma optimize("",off)


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define MEMREPLAY_CPP_SECTION_1 1
#define MEMREPLAY_CPP_SECTION_2 2
#define MEMREPLAY_CPP_SECTION_3 3
#define MEMREPLAY_CPP_SECTION_4 4
#define MEMREPLAY_CPP_SECTION_5 5
#define MEMREPLAY_CPP_SECTION_6 6
#define MEMREPLAY_CPP_SECTION_7 7
#define MEMREPLAY_CPP_SECTION_8 8
#define MEMREPLAY_CPP_SECTION_9 9
#define MEMREPLAY_CPP_SECTION_10 10
#define MEMREPLAY_CPP_SECTION_11 11
#define MEMREPLAY_CPP_SECTION_12 12
#define MEMREPLAY_CPP_SECTION_13 13
#define MEMREPLAY_CPP_SECTION_14 14
#define MEMREPLAY_CPP_SECTION_15 15
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MEMREPLAY_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(MemReplay_cpp, AZ_RESTRICTED_PLATFORM)

#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MEMREPLAY_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(MemReplay_cpp, AZ_RESTRICTED_PLATFORM)
#endif

#if defined(APPLE) || defined(LINUX)
#include <sys/mman.h>
#endif

#if CAPTURE_REPLAY_LOG || ENABLE_STATOSCOPE

ReplayDiskWriter::ReplayDiskWriter(const char* pSuffix)
    : m_fp(NULL)
    , m_written(0)
{
    if (pSuffix == NULL || strcmp(pSuffix, "") == 0)
    {
        time_t curTime;
        time(&curTime);
#ifdef AZ_COMPILER_MSVC
        struct tm lt;
        localtime_s(&lt, &curTime);
        strftime(m_filename, sizeof(m_filename) / sizeof(m_filename[0]), "memlog-%Y%m%d-%H%M%S.zmrl", &lt);
#else
        auto lt = localtime(&curTime);
        strftime(m_filename, sizeof(m_filename) / sizeof(m_filename[0]), "memlog-%Y%m%d-%H%M%S.zmrl", lt);
#endif
    }
    else
    {
        sprintf_s(m_filename, "memlog-%s.zmrl", pSuffix);
    }
}

bool ReplayDiskWriter::Open()
{
#ifdef fopen
#undef fopen
#endif
    char fn[MAX_PATH] = "";
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MEMREPLAY_CPP_SECTION_3
#include AZ_RESTRICTED_FILE(MemReplay_cpp, AZ_RESTRICTED_PLATFORM)
#endif
    cry_strcat(fn, m_filename);

    m_fp = nullptr;
    azfopen(&m_fp, fn, "wb");

    return m_fp != NULL;

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MEMREPLAY_CPP_SECTION_4
#include AZ_RESTRICTED_FILE(MemReplay_cpp, AZ_RESTRICTED_PLATFORM)
#endif
}

void ReplayDiskWriter::Close()
{
#ifdef fclose
#undef fclose
#endif

    if (m_fp)
    {
        ::fclose(m_fp);

        m_fp = NULL;
    }

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MEMREPLAY_CPP_SECTION_5
#include AZ_RESTRICTED_FILE(MemReplay_cpp, AZ_RESTRICTED_PLATFORM)
#endif
}

int ReplayDiskWriter::Write(const void* data, size_t sz, size_t n)
{
#ifdef fwrite
#undef fwrite
#endif
    int res = ::fwrite(data, sz, n, m_fp);
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MEMREPLAY_CPP_SECTION_6
#include AZ_RESTRICTED_FILE(MemReplay_cpp, AZ_RESTRICTED_PLATFORM)
#endif

    m_written += res * sz;

    return res;
}

void ReplayDiskWriter::Flush()
{
#ifdef fflush
#undef fflush
#endif

    ::fflush(m_fp);

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MEMREPLAY_CPP_SECTION_7
#include AZ_RESTRICTED_FILE(MemReplay_cpp, AZ_RESTRICTED_PLATFORM)
#endif
}

ReplaySocketWriter::ReplaySocketWriter(const char* address)
    : m_sock(AZ_SOCKET_INVALID)
    , m_written(0)
    , m_port(29494)
{
#if defined (REPLAY_SOCKET_WRITER)
    const char* portStart = strchr(address, ':');
    if (portStart)
    {
        m_port = atoi(portStart + 1);
    }
    else
    {
        portStart = address + strlen(address);
    }

    cry_strcpy(m_address, address, (size_t)(portStart - address));
#endif //#if defined (REPLAY_SOCKET_WRITER)
}

bool ReplaySocketWriter::Open()
{
#if defined (REPLAY_SOCKET_WRITER)
    uint32 addr32 = 0;

    AZ::AzSock::Startup();
    AZ::AzSock::AzSocketAddress socketAddress;
    if (!socketAddress.SetAddress(m_address, 0))
    {
        return false;
    }

    m_sock = AZ::AzSock::Socket();
    if (!AZ::AzSock::IsAzSocketValid(m_sock))
    {
        return false;
    }

    AZ::s32 connectResult = static_cast<AZ::s32>(AZ::AzSock::AzSockError::eASE_MISC_ERROR);
    for (uint16 port = m_port; (connectResult < 0) && port < (29494 + 32); ++port)
    {
        socketAddress.SetAddrPort(port);
        connectResult = AZ::AzSock::Connect(m_sock, socketAddress);
    }

    if (AZ::AzSock::SocketErrorOccured(connectResult))
    {
        AZ::AzSock::Shutdown(m_sock, SD_BOTH);
        AZ::AzSock::CloseSocket(m_sock);
        m_sock = AZ_SOCKET_INVALID;
        return false;
    }
#endif //#if defined (REPLAY_SOCKET_WRITER)
    return true;
}

void ReplaySocketWriter::Close()
{
#if defined (REPLAY_SOCKET_WRITER)
    if (m_sock != AZ_SOCKET_INVALID)
    {
        AZ::AzSock::Shutdown(m_sock, SD_BOTH);
        AZ::AzSock::CloseSocket(m_sock);
        m_sock = AZ_SOCKET_INVALID;
    }

    AZ::AzSock::Cleanup();
#endif //#if defined (REPLAY_SOCKET_WRITER)
}

int ReplaySocketWriter::Write(const void* data, size_t sz, size_t n)
{
#if defined (REPLAY_SOCKET_WRITER)
    if (m_sock != AZ_SOCKET_INVALID)
    {
        const char* bdata = reinterpret_cast<const char*>(data);
        size_t toSend = sz * n;
        while (toSend > 0)
        {
            int sent = AZ::AzSock::Send(m_sock, bdata, toSend, 0);
            if (AZ::AzSock::SocketErrorOccured(sent))
            {
                __debugbreak();
                break;
            }

            toSend -= sent;
            bdata += sent;
        }

        m_written += sz * n;
    }
#endif //#if defined (REPLAY_SOCKET_WRITER)

    return n;
}

void ReplaySocketWriter::Flush()
{
}

#endif

#if CAPTURE_REPLAY_LOG

#include "CryThread.h"
typedef CryLockT<CRYLOCK_RECURSIVE> MemFastMutex;
static MemFastMutex& GetLogMutex()
{
    static MemFastMutex logmutex;
    return logmutex;
}
static volatile DWORD s_ignoreThreadId = (DWORD)-1;

static uint32 g_memReplayFrameCount;
const int k_maxCallStackDepth = 256;
extern volatile bool g_replayCleanedUp;

static volatile UINT_PTR s_replayLastGlobal = 0;
static volatile int s_replayStartingFree = 0;


#if defined(_WIN32)

void* ReplayAllocatorBase::ReserveAddressSpace(size_t sz)
{
    return VirtualAlloc(NULL, sz, MEM_RESERVE, PAGE_READWRITE);
}

void ReplayAllocatorBase::UnreserveAddressSpace(void* base, void* committedEnd)
{
    VirtualFree(base, 0, MEM_RELEASE);
}

void* ReplayAllocatorBase::MapAddressSpace(void* addr, size_t sz)
{
    return VirtualAlloc(addr, sz, MEM_COMMIT, PAGE_READWRITE);
}

#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MEMREPLAY_CPP_SECTION_8
#include AZ_RESTRICTED_FILE(MemReplay_cpp, AZ_RESTRICTED_PLATFORM)
#elif defined(APPLE) || defined(LINUX)
void* ReplayAllocatorBase::ReserveAddressSpace(size_t sz)
{
    return mmap(NULL, sz, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
}

void ReplayAllocatorBase::UnreserveAddressSpace(void* base, void* committedEnd)
{
    int res = munmap(base, reinterpret_cast<char*>(committedEnd) - reinterpret_cast<char*>(base));
    (void) res;
    assert(res == 0);
}

void* ReplayAllocatorBase::MapAddressSpace(void* addr, size_t sz)
{
    int res = mprotect(addr, sz, PROT_READ | PROT_WRITE);
    return addr;
}
#elif defined(LINUX)

void* ReplayAllocatorBase::ReserveAddressSpace(size_t sz)
{
    return mmap(NULL, sz, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
}

void ReplayAllocatorBase::UnreserveAddressSpace(void* base, void* committedEnd)
{
    int res = munmap(base, reinterpret_cast<char*>(committedEnd) - reinterpret_cast<char*>(base));
}

void* ReplayAllocatorBase::MapAddressSpace(void* addr, size_t sz)
{
    int res = mprotect(addr, sz, PROT_READ | PROT_WRITE);
    return addr;
}

#endif

namespace
{
    //helper functions to become strict aliasing warning conform
    ILINE void** CastCallstack(uint32* pCallstack)
    {
        return alias_cast<void**>(pCallstack);
    }
    ILINE void** CastCallstack(uint64* pCallstack)
    {
        return alias_cast<void**>(pCallstack);
    }
#if !defined(_WIN32) && !defined(LINUX)
    ILINE void** CastCallstack(UINT_PTR* pCallstack)
    {
        return alias_cast<void**>(pCallstack);
    }
#endif
    ILINE volatile long* LongPtr(volatile int* pI)
    {
        return alias_cast<volatile long*>(pI);
    }

    void RetrieveMemReplaySuffix(char* pSuffixBuffer, const char* lwrCommandLine, int bufferLength)
    {
        cry_strcpy(pSuffixBuffer, bufferLength, "");
        const char* pSuffix = strstr(lwrCommandLine, "-memreplaysuffix");
        if (pSuffix != NULL)
        {
            pSuffix += strlen("-memreplaysuffix");
            while (1)
            {
                if (*pSuffix == '\0' || *pSuffix == '+' || *pSuffix == '-')
                {
                    pSuffix = NULL;
                    break;
                }
                if (*pSuffix != ' ')
                {
                    break;
                }
                ++pSuffix;
            }
            if (pSuffix != NULL)
            {
                const char* pEnd = pSuffix;
                while (*pEnd != '\0' && *pEnd != ' ')
                {
                    ++pEnd;
                }
                cry_strcpy(pSuffixBuffer, bufferLength, pSuffix, (size_t)(pEnd - pSuffix));
            }
        }
    }
}

ReplayAllocator::ReplayAllocator()
{
    m_heap = ReserveAddressSpace(MaxSize);
    if (!m_heap)
    {
        __debugbreak();
    }

    m_commitEnd = m_heap;
    m_allocEnd = m_heap;
}

ReplayAllocator::~ReplayAllocator()
{
    Free();
}

void ReplayAllocator::Reserve(size_t sz)
{
    CryAutoLock<CryCriticalSectionNonRecursive> lock(m_lock);

    INT_PTR commitEnd = reinterpret_cast<INT_PTR>(m_commitEnd);
    INT_PTR newCommitEnd = std::min<INT_PTR>(commitEnd + sz, commitEnd + MaxSize);

    if ((newCommitEnd - commitEnd) > 0)
    {
        LPVOID res = MapAddressSpace(m_commitEnd, newCommitEnd - commitEnd);
        if (!res)
        {
            __debugbreak();
        }
    }

    m_commitEnd = reinterpret_cast<void*>(newCommitEnd);
}

void* ReplayAllocator::Allocate(size_t sz)
{
    CryAutoLock<CryCriticalSectionNonRecursive> lock(m_lock);

    sz = (sz + 7) & ~7;

    uint8* alloc = reinterpret_cast<uint8*>(m_allocEnd);
    uint8* newEnd = reinterpret_cast<uint8*>(m_allocEnd) + sz;

    if (newEnd > reinterpret_cast<uint8*>(m_commitEnd))
    {
        uint8* alignedEnd = reinterpret_cast<uint8*>((reinterpret_cast<INT_PTR>(newEnd) + (PageSize - 1)) & ~(PageSize - 1));

        if (alignedEnd > reinterpret_cast<uint8*>(m_allocEnd) + MaxSize)
        {
            __debugbreak();
        }

        LPVOID res = MapAddressSpace(m_commitEnd, alignedEnd - reinterpret_cast<uint8*>(m_commitEnd));
        if (!res)
        {
            __debugbreak();
        }

        m_commitEnd = alignedEnd;
    }

    m_allocEnd = newEnd;

    return alloc;
}

void ReplayAllocator::Free()
{
    CryAutoLock<CryCriticalSectionNonRecursive> lock(m_lock);

    UnreserveAddressSpace(m_heap, m_commitEnd);
}


ReplayCompressor::ReplayCompressor(ReplayAllocator& allocator, IReplayWriter* writer)
    : m_allocator(&allocator)
    , m_writer(writer)
{
    m_zSize = 0;

    memset(&m_compressStream, 0, sizeof(m_compressStream));
    m_compressStream.zalloc = &ReplayCompressor::zAlloc;
    m_compressStream.zfree = &ReplayCompressor::zFree;
    m_compressStream.opaque = this;
    deflateInit(&m_compressStream, 2);//Z_DEFAULT_COMPRESSION);

    uint64 streamLen = 0;
    m_writer->Write(&streamLen, sizeof(streamLen), 1);

    m_compressTarget = (uint8*) m_allocator->Allocate(CompressTargetSize);
}

ReplayCompressor::~ReplayCompressor()
{
    deflateEnd(&m_compressStream);
    m_compressTarget = NULL;
    m_writer = NULL;
}

void ReplayCompressor::Flush()
{
    m_writer->Flush();
}

void ReplayCompressor::Write(const uint8* data, size_t len)
{
    if (CompressTargetSize < len)
    {
        __debugbreak();
    }

    m_compressStream.next_in = const_cast<uint8*>(data);
    m_compressStream.avail_in = len;
    m_compressStream.next_out = m_compressTarget;
    m_compressStream.avail_out = CompressTargetSize;
    m_compressStream.total_out = 0;

    do
    {
        int err = deflate(&m_compressStream, Z_SYNC_FLUSH);

        if ((err == Z_OK && m_compressStream.avail_out == 0) || (err == Z_BUF_ERROR && m_compressStream.avail_out == 0))
        {
            int total_out = m_compressStream.total_out;
            m_writer->Write(m_compressTarget, total_out * sizeof(uint8), 1);

            m_compressStream.next_out = m_compressTarget;
            m_compressStream.avail_out = CompressTargetSize;
            m_compressStream.total_out = 0;

            continue;
        }
        else if (m_compressStream.avail_in == 0)
        {
            int total_out = m_compressStream.total_out;
            m_writer->Write(m_compressTarget, total_out * sizeof(uint8), 1);
        }
        else
        {
            // Shrug?
        }

        break;
    }
    while (true);

    Flush();
}

voidpf ReplayCompressor::zAlloc(voidpf opaque, uInt items, uInt size)
{
    ReplayCompressor* str = reinterpret_cast<ReplayCompressor*>(opaque);
    return str->m_allocator->Allocate(items * size);
}

void ReplayCompressor::zFree(voidpf opaque, voidpf address)
{
    // Doesn't seem to be called, except when shutting down and then we'll just free it all anyway
}

#if REPLAY_RECORD_THREADED

ReplayRecordThread::ReplayRecordThread(ReplayAllocator& allocator, IReplayWriter* writer)
    : m_compressor(allocator, writer)
{
    m_nextCommand = CMD_Idle;
    m_nextData = NULL;
    m_nextLength = 0;
}

void ReplayRecordThread::Flush()
{
    Lock();

    while (m_nextCommand != CMD_Idle)
    {
        Unlock();
        Sleep(1);
        Lock();
    }

    m_nextCommand = CMD_Flush;
    NotifySingle();

    Unlock();
}

void ReplayRecordThread::Write(const uint8* data, size_t len)
{
    Lock();

    while (m_nextCommand != CMD_Idle)
    {
        Unlock();
        Sleep(1);
        Lock();
    }

    m_nextData = data;
    m_nextLength = len;
    m_nextCommand = CMD_Write;
    NotifySingle();

    Unlock();
}

void ReplayRecordThread::Run()
{
    CryMemStatIgnoreThread(CryGetCurrentThreadId());

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MEMREPLAY_CPP_SECTION_9
#include AZ_RESTRICTED_FILE(MemReplay_cpp, AZ_RESTRICTED_PLATFORM)
#endif

    while (true)
    {
        Lock();

        while (m_nextCommand == CMD_Idle)
        {
            Wait();
        }

        switch (m_nextCommand)
        {
        case CMD_Stop:
            Unlock();
            m_nextCommand = CMD_Idle;
            return;

        case CMD_Write:
        {
            size_t length = m_nextLength;
            const uint8* data = m_nextData;

            m_nextLength = 0;
            m_nextData = NULL;

            m_compressor.Write(data, length);
        }
        break;

        case CMD_Flush:
            m_compressor.Flush();
            break;
        }

        m_nextCommand = CMD_Idle;

        Unlock();
    }
}

void ReplayRecordThread::Cancel()
{
    Lock();

    while (m_nextCommand != CMD_Idle)
    {
        Unlock();
        Sleep(1);
        Lock();
    }

    m_nextCommand = CMD_Stop;
    NotifySingle();

    while (m_nextCommand != CMD_Idle)
    {
        Unlock();
        Sleep(1);
        Lock();
    }

    Unlock();
}

#endif

ReplayLogStream::ReplayLogStream()
    : m_isOpen(0)
    , m_sequenceCheck(0)
#if REPLAY_RECORD_THREADED
    , m_recordThread(NULL)
#else
    , m_compressor(NULL)
#endif
{
}

ReplayLogStream::~ReplayLogStream()
{
    if (IsOpen())
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());
        Close();
    }
}

bool ReplayLogStream::Open(const char* openString)
{
    using std::swap;

    m_allocator.Reserve(512 * 1024);

    while (isspace((unsigned char) *openString))
    {
        ++openString;
    }

    const char* sep = strchr(openString, ':');
    if (!sep)
    {
        sep = openString + strlen(openString);
    }

    char destination[32] = {0};
    const char* arg = "";

    if (sep != openString)
    {
        while (isspace((unsigned char) sep[-1]))
        {
            --sep;
        }

        cry_strcpy(destination, openString, (size_t)(sep - openString));

        arg = sep;

        if (*arg)
        {
            ++arg;
            while (isspace((unsigned char) *arg))
            {
                ++arg;
            }
        }
    }
    else
    {
        azstrcpy(destination, AZ_ARRAY_SIZE(destination), "disk");
    }

    IReplayWriter* writer = NULL;

    if (strcmp(destination, "disk") == 0)
    {
        writer = new (m_allocator.Allocate(sizeof(ReplayDiskWriter)))ReplayDiskWriter(arg);
    }
    else if (strcmp(destination, "socket") == 0)
    {
        writer = new (m_allocator.Allocate(sizeof(ReplaySocketWriter)))ReplaySocketWriter(arg);
    }

    if (writer && writer->Open() == false)
    {
        fprintf(stdout, "Failed to open writer\n");
        writer->~IReplayWriter();
        writer = NULL;

        m_allocator.Free();
        return false;
    }

    swap(m_writer, writer);

    m_bufferSize = 64 * 1024;
    m_bufferA = (uint8*) m_allocator.Allocate(m_bufferSize);

#if REPLAY_RECORD_THREADED
    m_bufferB = (uint8*) m_allocator.Allocate(m_bufferSize);
#endif

    m_buffer = m_bufferA;

#if REPLAY_RECORD_THREADED
    m_recordThread = new (m_allocator.Allocate(sizeof(ReplayRecordThread)))ReplayRecordThread(m_allocator, m_writer);
    m_recordThread->Start();
#else
    m_compressor = new (m_allocator.Allocate(sizeof(ReplayCompressor)))ReplayCompressor(m_allocator, m_writer);
#endif

    {
        m_bufferEnd = &m_buffer[0];

        // Write stream header.
        uint8 version = 2;
        uint32 platform = MemReplayPlatformIds::PI_Unknown;

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MEMREPLAY_CPP_SECTION_10
#include AZ_RESTRICTED_FILE(MemReplay_cpp, AZ_RESTRICTED_PLATFORM)
#elif defined(WIN32) || defined(WIN64) || defined(LINUX) || defined(APPLE)
        platform = MemReplayPlatformIds::PI_PC;
#endif

        MemReplayLogHeader hdr(('M' << 0) | ('R' << 8) | ('L' << 16) | (version << 24), platform, sizeof(void*) * 8);
        memcpy(&m_buffer[0], &hdr, sizeof(hdr));
        m_bufferEnd += sizeof(hdr);

        CryInterlockedIncrement(&m_isOpen);
    }

    return true;
}

void ReplayLogStream::Close()
{
    using std::swap;

    // Flag it as closed here, to prevent recursion into the memory logging as it is closing.
    CryInterlockedDecrement(&m_isOpen);

    Flush();
    Flush();
    Flush();

#if REPLAY_RECORD_THREADED
    m_recordThread->Cancel();
    m_recordThread->WaitForThread();
    m_recordThread->~ReplayRecordThread();
    m_recordThread = NULL;
#else
    m_compressor->~ReplayCompressor();
    m_compressor = NULL;
#endif

    m_writer->Close();
    m_writer = NULL;

    m_buffer = NULL;
    m_bufferEnd = 0;
    m_bufferSize = 0;

    m_allocator.Free();
}

void ReplayLogStream::Flush()
{
    if (m_writer != NULL)
    {
        m_uncompressedLen += m_bufferEnd - &m_buffer[0];

#if REPLAY_RECORD_THREADED
        m_recordThread->Write(&m_buffer[0], m_bufferEnd - &m_buffer[0]);
        m_buffer = (m_buffer == m_bufferA) ? m_bufferB : m_bufferA;
#else
        m_compressor->Write(&m_buffer[0], m_bufferEnd - &m_buffer[0]);
#endif
    }

    m_bufferEnd = &m_buffer[0];
}

void ReplayLogStream::FullFlush()
{
#if REPLAY_RECORD_THREADED
    m_recordThread->Flush();
#else
    m_compressor->Flush();
#endif
}

size_t ReplayLogStream::GetSize() const
{
    return m_allocator.GetCommittedSize();
}

uint64 ReplayLogStream::GetWrittenLength() const
{
    return m_writer ? m_writer->GetWrittenLength() : 0ULL;
}

uint64 ReplayLogStream::GetUncompressedLength() const
{
    return m_uncompressedLen;
}

#define SIZEOF_MEMBER(cls, mbr) (sizeof(reinterpret_cast<cls*>(0)->mbr))

void CryMemStatIgnoreThread(DWORD threadId)
{
    s_ignoreThreadId = threadId;
}

static bool g_memReplayPaused = false;

//////////////////////////////////////////////////////////////////////////
// CMemReplay class implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MEMREPLAY_CPP_SECTION_11
#include AZ_RESTRICTED_FILE(MemReplay_cpp, AZ_RESTRICTED_PLATFORM)
#endif

static int GetCurrentSysAlloced()
{
    int trackingUsage = 0;//m_stream.GetSize();
    IMemoryManager::SProcessMemInfo mi;
    CCryMemoryManager::GetInstance()->GetProcessMemInfo(mi);
    return (uint32)(mi.PagefileUsage - trackingUsage);
}

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MEMREPLAY_CPP_SECTION_12
#include AZ_RESTRICTED_FILE(MemReplay_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
static int GetCurrentExecutableSize()
{
    return GetCurrentSysAlloced();
}
#endif

//////////////////////////////////////////////////////////////////////////
static UINT_PTR GetCurrentSysFree()
{
    IMemoryManager::SProcessMemInfo mi;
    CCryMemoryManager::GetInstance()->GetProcessMemInfo(mi);
    return (UINT_PTR)-(INT_PTR)mi.PagefileUsage;
}

void CReplayModules::RefreshModules()
{
#if defined(WIN32)
    for (unsigned int i = 0; i < AZ::Debug::SymbolStorage::GetNumLoadedModules(); ++i)
    {
        const AZ::Debug::SymbolStorage::ModuleInfo* info = AZ::Debug::SymbolStorage::GetModuleInfo(i);

        ModuleLoadDesc mld;

        // This function was updated to use AZ::Debug instead of dbghelp.dll, preventing collision
        // with the usage of this dll in multiple threaded contexts.  The AZ version does not provide
        // Pdb signature or age; this info was informational only, and is stubbed out with this assignment.
        mld.sig[0] = '\0';

        azstrcpy(mld.name, AZ_ARRAY_SIZE(mld.name), info->m_modName);
        azstrcpy(mld.path, AZ_ARRAY_SIZE(mld.path), info->m_fileName);
        mld.address = (UINT_PTR)info->m_baseAddress;
        mld.size = info->m_size;
        CMemReplay::GetInstance()->RecordModuleLoad(mld);
    }
#endif
}


// Ensure IMemReplay is not destroyed during atExit() procedure which has no defined destruction order.
// In some cases it was destroyed while still being used.
_MS_ALIGN(8) char g_memReplay[sizeof(CMemReplay)] _ALIGN(alignof(CMemReplay));
CMemReplay* CMemReplay::GetInstance()
{
    static bool bIsInit = false;
    if (!bIsInit)
    {
        new (&g_memReplay)CMemReplay();
        bIsInit = true;
    }

    return alias_cast<CMemReplay*>(g_memReplay);
}

CMemReplay::CMemReplay()
    : m_allocReference(0)
    , m_scopeDepth(0)
    , m_scopeClass((EMemReplayAllocClass::Class)0)
    , m_scopeSubClass(0)
    , m_scopeModuleId(0)
{
}

CMemReplay::~CMemReplay()
{
}

//////////////////////////////////////////////////////////////////////////
void CMemReplay::DumpStats()
{
    CMemReplay::DumpSymbols();
}

//////////////////////////////////////////////////////////////////////////
void CMemReplay::DumpSymbols()
{
    if (m_stream.IsOpen())
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());
        if (m_stream.IsOpen())
        {
            RecordModules();

            m_stream.FullFlush();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMemReplay::StartOnCommandLine(const char* cmdLine)
{
    const char* mrCmd = 0;

    CryStackStringT<char, 2048> lwrCmdLine = cmdLine;
    lwrCmdLine.MakeLower();

    if ((mrCmd = strstr(lwrCmdLine.c_str(), "-memreplay")) != 0)
    {
        bool bPaused = strstr(lwrCmdLine.c_str(), "-memreplaypaused") != 0;

        //Retrieve file suffix if present
        const int bufferLength = 64;
        char openCmd[bufferLength] = "disk:";

        // Try and detect a new style open string, and fall back to the old suffix format if it fails.
        if (
            (strncmp(mrCmd, "-memreplay disk", 15) == 0) ||
            (strncmp(mrCmd, "-memreplay socket", 17) == 0))
        {
            const char* arg = mrCmd + strlen("-memreplay ");
            const char* argEnd = arg;
            while (*argEnd && !isspace((unsigned char) *argEnd))
            {
                ++argEnd;
            }
            cry_strcpy(openCmd, arg, (size_t)(argEnd - arg));
        }
        else
        {
            RetrieveMemReplaySuffix(openCmd + 5, lwrCmdLine.c_str(), bufferLength - 5);
        }

        Start(bPaused, openCmd);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMemReplay::Start(bool bPaused, const char* openString)
{
    if (!openString)
    {
        openString = "disk";
    }

    CryAutoLock<CryCriticalSection> lock(GetLogMutex());
    g_memReplayPaused = bPaused;

    if (!m_stream.IsOpen())
    {
        if (m_stream.Open(openString))
        {
            s_replayLastGlobal = GetCurrentSysFree();
            int initialGlobal = GetCurrentSysAlloced();
            int exeSize = GetCurrentExecutableSize();

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MEMREPLAY_CPP_SECTION_13
#include AZ_RESTRICTED_FILE(MemReplay_cpp, AZ_RESTRICTED_PLATFORM)
#endif

            m_stream.WriteEvent(MemReplayAddressProfileEvent(0xffffffff, 0));
            m_stream.WriteEvent(MemReplayInfoEvent(exeSize, initialGlobal, s_replayStartingFree));
            m_stream.WriteEvent(MemReplayFrameStartEvent(g_memReplayFrameCount++));

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MEMREPLAY_CPP_SECTION_14
#include AZ_RESTRICTED_FILE(MemReplay_cpp, AZ_RESTRICTED_PLATFORM)
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MEMREPLAY_CPP_SECTION_15
#include AZ_RESTRICTED_FILE(MemReplay_cpp, AZ_RESTRICTED_PLATFORM)
#endif

            uint32 id = 0;
            //#define MEMREPLAY_DEFINE_CONTEXT(name) RecordContextDefinition(id ++, #name);
            //#include "CryMemReplayContexts.h"
            //#undef MEMREPLAY_DEFINE_CONTEXT
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMemReplay::Stop()
{
    CryAutoLock<CryCriticalSection> lock(GetLogMutex());
    if (m_stream.IsOpen())
    {
        RecordModules();

        m_stream.Close();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CMemReplay::EnterScope(EMemReplayAllocClass::Class cls, uint16 subCls, int moduleId)
{
    if (m_stream.IsOpen())
    {
        m_scope.Lock();

        ++m_scopeDepth;

        if (m_scopeDepth == 1)
        {
            m_scopeClass = cls;
            m_scopeSubClass = subCls;
            m_scopeModuleId = moduleId;
        }

        return true;
    }

    return false;
}

void CMemReplay::ExitScope_Alloc(UINT_PTR id, UINT_PTR sz, UINT_PTR alignment)
{
    --m_scopeDepth;

    if (m_scopeDepth == 0)
    {
        if (id && m_stream.IsOpen() && CryGetCurrentThreadId() != s_ignoreThreadId)
        {
            CryAutoLock<CryCriticalSection> lock(GetLogMutex());

            if (m_stream.IsOpen())
            {
                UINT_PTR global = GetCurrentSysFree();
                INT_PTR changeGlobal = (INT_PTR)(s_replayLastGlobal - global);
                s_replayLastGlobal = global;

                RecordAlloc(m_scopeClass, m_scopeSubClass, m_scopeModuleId, id, alignment, sz, sz, changeGlobal);
            }
        }
    }

    m_scope.Unlock();
}

void CMemReplay::ExitScope_Realloc(UINT_PTR originalId, UINT_PTR newId, UINT_PTR sz, UINT_PTR alignment)
{
    if (!originalId)
    {
        ExitScope_Alloc(newId, sz, alignment);
        return;
    }

    if (!newId)
    {
        ExitScope_Free(originalId);
        return;
    }

    --m_scopeDepth;

    if (m_scopeDepth == 0)
    {
        if (m_stream.IsOpen() && CryGetCurrentThreadId() != s_ignoreThreadId)
        {
            CryAutoLock<CryCriticalSection> lock(GetLogMutex());

            if (m_stream.IsOpen())
            {
                UINT_PTR global = GetCurrentSysFree();
                INT_PTR changeGlobal = (INT_PTR)(s_replayLastGlobal - global);
                s_replayLastGlobal = global;

                RecordRealloc(m_scopeClass, m_scopeSubClass, m_scopeModuleId, originalId, newId, alignment, sz, sz, changeGlobal);
            }
        }
    }

    m_scope.Unlock();
}

void CMemReplay::ExitScope_Free(UINT_PTR id)
{
    --m_scopeDepth;

    if (m_scopeDepth == 0)
    {
        if (id && m_stream.IsOpen() && CryGetCurrentThreadId() != s_ignoreThreadId)
        {
            CryAutoLock<CryCriticalSection> lock(GetLogMutex());

            if (m_stream.IsOpen())
            {
                UINT_PTR global = GetCurrentSysFree();
                INT_PTR changeGlobal = (INT_PTR)(s_replayLastGlobal - global);
                s_replayLastGlobal = global;

                PREFAST_SUPPRESS_WARNING(6326)
                RecordFree(m_scopeClass, m_scopeSubClass, m_scopeModuleId, id, changeGlobal, REPLAY_RECORD_FREECS != 0);
            }
        }
    }

    m_scope.Unlock();
}

void CMemReplay::ExitScope()
{
    --m_scopeDepth;
    m_scope.Unlock();
}

bool CMemReplay::EnterLockScope()
{
    if (m_stream.IsOpen())
    {
        m_scope.Lock();
        return true;
    }

    return false;
}

void CMemReplay::LeaveLockScope()
{
    m_scope.Unlock();
}

//////////////////////////////////////////////////////////////////////////
void CMemReplay::AddLabel(const char* label)
{
    if (m_stream.IsOpen())
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            new (m_stream.AllocateRawEvent<MemReplayLabelEvent>(strlen(label)))MemReplayLabelEvent(label);
        }
    }
}

void CMemReplay::AddLabelFmt(const char* labelFmt, ...)
{
    if (m_stream.IsOpen())
    {
        va_list args;
        va_start(args, labelFmt);

        char msg[256];
        vsprintf_s(msg, 256, labelFmt, args);

        va_end(args);

        AddLabel(msg);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMemReplay::AddFrameStart()
{
    if (m_stream.IsOpen())
    {
        static bool bSymbolsDumped = false;
        if (!bSymbolsDumped)
        {
            DumpSymbols();
            bSymbolsDumped = true;
        }

        if (!g_replayCleanedUp)
        {
            g_replayCleanedUp = true;
        }

        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            m_stream.WriteEvent(MemReplayFrameStartEvent(g_memReplayFrameCount++));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMemReplay::GetInfo(CryReplayInfo& infoOut)
{
    memset(&infoOut, 0, sizeof(CryReplayInfo));

    if (m_stream.IsOpen())
    {
        infoOut.uncompressedLength = m_stream.GetUncompressedLength();
        infoOut.writtenLength = m_stream.GetWrittenLength();
        infoOut.trackingSize = m_stream.GetSize();
        infoOut.filename = m_stream.GetFilename();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMemReplay::AllocUsage(EMemReplayAllocClass::Class allocClass, UINT_PTR id, UINT_PTR used)
{
#if REPLAY_RECORD_USAGE_CHANGES
    if (!ptr)
    {
        return;
    }

    if (m_stream.IsOpen())
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());
        if (m_stream.IsOpen())
        {
            m_stream.WriteEvent(MemReplayAllocUsageEvent((uint16)allocClass, id, used));
        }
    }
#endif
}

void CMemReplay::AddAllocReference(void* ptr, void* ref)
{
    if (ptr && m_stream.IsOpen())
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            MemReplayAddAllocReferenceEvent* ev = m_stream.BeginAllocateRawEvent<MemReplayAddAllocReferenceEvent>(
                    k_maxCallStackDepth * SIZEOF_MEMBER(MemReplayAddAllocReferenceEvent, callstack[0])
                    - SIZEOF_MEMBER(MemReplayAddAllocReferenceEvent, callstack));

            new (ev) MemReplayAddAllocReferenceEvent(reinterpret_cast<UINT_PTR>(ptr), reinterpret_cast<UINT_PTR>(ref));

            uint32 callstackLength = k_maxCallStackDepth;
            CSystem::debug_GetCallStackRaw(CastCallstack(ev->callstack), callstackLength);

            ev->callstackLength = callstackLength;

            m_stream.EndAllocateRawEvent<MemReplayAddAllocReferenceEvent>(ev->callstackLength * sizeof(ev->callstack[0]) - sizeof(ev->callstack));
        }
    }
}

void CMemReplay::RemoveAllocReference(void* ref)
{
    if (ref && m_stream.IsOpen())
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            m_stream.WriteEvent(MemReplayRemoveAllocReferenceEvent(reinterpret_cast<UINT_PTR>(ref)));
        }
    }
}

void CMemReplay::AddContext(int type, uint32 flags, const char* str)
{
    DWORD threadId = CryGetCurrentThreadId();
    if (m_stream.IsOpen() && threadId != s_ignoreThreadId && !g_memReplayPaused)
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            new (m_stream.AllocateRawEvent<MemReplayPushContextEvent>(strlen(str)))
            MemReplayPushContextEvent(threadId, str, (EMemStatContextTypes::Type) type, flags);
        }
    }
}

void CMemReplay::AddContextV(int type, uint32 flags, const char* format, va_list args)
{
    DWORD threadId = CryGetCurrentThreadId();
    if (m_stream.IsOpen() && threadId != s_ignoreThreadId && !g_memReplayPaused)
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            MemReplayPushContextEvent* ev = m_stream.BeginAllocateRawEvent<MemReplayPushContextEvent>(511);
            new (ev) MemReplayPushContextEvent(threadId, "", (EMemStatContextTypes::Type) type, flags);

            azvsnprintf(ev->name, 512, format, args);
            ev->name[511] = 0;

            m_stream.EndAllocateRawEvent<MemReplayPushContextEvent>(strlen(ev->name));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMemReplay::RemoveContext()
{
    DWORD threadId = CryGetCurrentThreadId();

    if (m_stream.IsOpen() && threadId != s_ignoreThreadId && !g_memReplayPaused)
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            m_stream.WriteEvent(MemReplayPopContextEvent(CryGetCurrentThreadId()));
        }
    }
}

void CMemReplay::Flush()
{
    CryAutoLock<CryCriticalSection> lock(GetLogMutex());

    if (m_stream.IsOpen())
    {
        m_stream.Flush();
    }
}

void CMemReplay::MapPage(void* base, size_t size)
{
    if (m_stream.IsOpen() && base && size)
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            MemReplayMapPageEvent* ev = m_stream.BeginAllocateRawEvent<MemReplayMapPageEvent>(
                    k_maxCallStackDepth * sizeof(void*) - SIZEOF_MEMBER(MemReplayMapPageEvent, callstack));
            new (ev) MemReplayMapPageEvent(reinterpret_cast<UINT_PTR>(base), size);

            uint32 callstackLength = k_maxCallStackDepth;
            CSystem::debug_GetCallStackRaw(CastCallstack(ev->callstack), callstackLength);
            ev->callstackLength = callstackLength;

            m_stream.EndAllocateRawEvent<MemReplayMapPageEvent>(sizeof(ev->callstack[0]) * ev->callstackLength - sizeof(ev->callstack));
        }
    }
}

void CMemReplay::UnMapPage(void* base, size_t size)
{
    if (m_stream.IsOpen() && base && size)
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            m_stream.WriteEvent(MemReplayUnMapPageEvent(reinterpret_cast<UINT_PTR>(base), size));
        }
    }
}

void CMemReplay::RegisterFixedAddressRange(void* base, size_t size, const char* name)
{
    if (m_stream.IsOpen() && base && size && name)
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            new (m_stream.AllocateRawEvent<MemReplayRegisterFixedAddressRangeEvent>(strlen(name)))
            MemReplayRegisterFixedAddressRangeEvent(reinterpret_cast<UINT_PTR>(base), size, name);
        }
    }
}

void CMemReplay::MarkBucket(int bucket, size_t alignment, void* base, size_t length)
{
    if (m_stream.IsOpen() && base && length)
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            m_stream.WriteEvent(MemReplayBucketMarkEvent(reinterpret_cast<UINT_PTR>(base), length, bucket, alignment));
        }
    }
}

void CMemReplay::UnMarkBucket(int bucket, void* base)
{
    if (m_stream.IsOpen() && base)
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            m_stream.WriteEvent(MemReplayBucketUnMarkEvent(reinterpret_cast<UINT_PTR>(base), bucket));
        }
    }
}

void CMemReplay::BucketEnableCleanups(void* allocatorBase, bool enabled)
{
    if (m_stream.IsOpen() && allocatorBase)
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            m_stream.WriteEvent(MemReplayBucketCleanupEnabledEvent(reinterpret_cast<UINT_PTR>(allocatorBase), enabled));
        }
    }
}

void CMemReplay::MarkPool(int pool, size_t alignment, void* base, size_t length, const char* name)
{
    if (m_stream.IsOpen() && base && length)
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            new (m_stream.AllocateRawEvent<MemReplayPoolMarkEvent>(strlen(name)))
            MemReplayPoolMarkEvent(reinterpret_cast<UINT_PTR>(base), length, pool, alignment, name);
        }
    }
}

void CMemReplay::UnMarkPool(int pool, void* base)
{
    if (m_stream.IsOpen() && base)
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            m_stream.WriteEvent(MemReplayPoolUnMarkEvent(reinterpret_cast<UINT_PTR>(base), pool));
        }
    }
}

void CMemReplay::AddTexturePoolContext(void* ptr, int mip, int width, int height, const char* name, uint32 flags)
{
    if (m_stream.IsOpen() && ptr)
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            new (m_stream.AllocateRawEvent<MemReplayTextureAllocContextEvent>(strlen(name)))
            MemReplayTextureAllocContextEvent(reinterpret_cast<UINT_PTR>(ptr), mip, width, height, flags, name);
        }
    }
}

void CMemReplay::AddSizerTree(const char* name)
{
    MemReplayCrySizer sizer(m_stream, name);
    static_cast<CSystem*>(gEnv->pSystem)->CollectMemStats(&sizer);
}

void CMemReplay::AddScreenshot()
{
#if 0
    if (m_stream.IsOpen())
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            PREFAST_SUPPRESS_WARNING(6255)
            MemReplayScreenshotEvent * ev = new (alloca(sizeof(MemReplayScreenshotEvent) + 65536))MemReplayScreenshotEvent();
            gEnv->pRenderer->ScreenShotBuf(8, ev->bmp);
            m_stream.WriteEvent(ev, ev->bmp[0] * ev->bmp[1] * 3 + 3);
        }
    }
#endif
}

void CMemReplay::RegisterContainer(const void* key, int type)
{
#if REPLAY_RECORD_CONTAINERS
    if (key)
    {
        if (m_stream.IsOpen())
        {
            CryAutoLock<CryCriticalSection> lock(GetLogMutex());

            if (m_stream.IsOpen())
            {
                MemReplayRegisterContainerEvent* ev = m_stream.BeginAllocateRawEvent<MemReplayRegisterContainerEvent>(
                        k_maxCallStackDepth * SIZEOF_MEMBER(MemReplayRegisterContainerEvent, callstack[0]) - SIZEOF_MEMBER(MemReplayRegisterContainerEvent, callstack));

                new (ev) MemReplayRegisterContainerEvent(reinterpret_cast<UINT_PTR>(key), type);

                uint32 length = k_maxCallStackDepth;
                CSystem::debug_GetCallStackRaw(CastCallstack(ev->callstack), length);

                ev->callstackLength = length;

                m_stream.EndAllocateRawEvent<MemReplayRegisterContainerEvent>(length * sizeof(ev->callstack[0]) - sizeof(ev->callstack));
            }
        }
    }
#endif
}

void CMemReplay::UnregisterContainer(const void* key)
{
#if REPLAY_RECORD_CONTAINERS
    if (key)
    {
        if (m_stream.IsOpen())
        {
            CryAutoLock<CryCriticalSection> lock(GetLogMutex());

            if (m_stream.IsOpen())
            {
                m_stream.WriteEvent(MemReplayUnregisterContainerEvent(reinterpret_cast<UINT_PTR>(key)));
            }
        }
    }
#endif
}

void CMemReplay::BindToContainer(const void* key, const void* alloc)
{
#if REPLAY_RECORD_CONTAINERS
    if (key && alloc)
    {
        if (m_stream.IsOpen())
        {
            CryAutoLock<CryCriticalSection> lock(GetLogMutex());

            if (m_stream.IsOpen())
            {
                m_stream.WriteEvent(MemReplayBindToContainerEvent(reinterpret_cast<UINT_PTR>(key), reinterpret_cast<UINT_PTR>(alloc)));
            }
        }
    }
#endif
}

void CMemReplay::UnbindFromContainer(const void* key, const void* alloc)
{
#if REPLAY_RECORD_CONTAINERS
    if (key && alloc)
    {
        if (m_stream.IsOpen())
        {
            CryAutoLock<CryCriticalSection> lock(GetLogMutex());

            if (m_stream.IsOpen())
            {
                m_stream.WriteEvent(MemReplayUnbindFromContainerEvent(reinterpret_cast<UINT_PTR>(key), reinterpret_cast<UINT_PTR>(alloc)));
            }
        }
    }
#endif
}

void CMemReplay::SwapContainers(const void* keyA, const void* keyB)
{
#if REPLAY_RECORD_CONTAINERS
    if (keyA && keyB && (keyA != keyB) && m_stream.IsOpen())
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream.IsOpen())
        {
            m_stream.WriteEvent(MemReplaySwapContainersEvent(reinterpret_cast<UINT_PTR>(keyA), reinterpret_cast<UINT_PTR>(keyB)));
        }
    }
#endif
}

void CMemReplay::RecordModuleLoad(const CReplayModules::ModuleLoadDesc& mld)
{

    const char* baseName = strrchr(mld.name, '\\');
    if (!baseName)
    {
        baseName = mld.name;
    }
    size_t baseNameLen = strlen(baseName);

    m_stream.WriteEvent(MemReplayModuleRefEvent(mld.name, mld.path, mld.sig));

    DWORD threadId = CryGetCurrentThreadId();

    MemReplayPushContextEvent* pEv = new (m_stream.BeginAllocateRawEvent<MemReplayPushContextEvent>(baseNameLen))MemReplayPushContextEvent(threadId, baseName, EMemStatContextTypes::MSC_Other, 0);
    m_stream.EndAllocateRawEvent<MemReplayPushContextEvent>(baseNameLen);

    RecordAlloc(
        EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CryMalloc, 0,
        (UINT_PTR)mld.address,
        4096,
        mld.size,
        mld.size,
        0);

    m_stream.WriteEvent(MemReplayPopContextEvent(threadId));
}

void CMemReplay::RecordAlloc(EMemReplayAllocClass::Class cls, uint16 subCls, int moduleId, UINT_PTR p, UINT_PTR alignment, UINT_PTR sizeRequested, UINT_PTR sizeConsumed, INT_PTR sizeGlobal)
{
    MemReplayAllocEvent* ev = m_stream.BeginAllocateRawEvent<MemReplayAllocEvent>(
            k_maxCallStackDepth * sizeof(void*) - SIZEOF_MEMBER(MemReplayAllocEvent, callstack));

    new (ev) MemReplayAllocEvent(
        CryGetCurrentThreadId(),
        static_cast<uint16>(moduleId),
        static_cast<uint16>(cls),
        static_cast<uint16>(subCls),
        p,
        static_cast<uint32>(alignment),
        static_cast<uint32>(sizeRequested),
        static_cast<uint32>(sizeConsumed),
        static_cast<int32>(sizeGlobal));

    uint32 callstackLength = k_maxCallStackDepth;
    CSystem::debug_GetCallStackRaw(CastCallstack(ev->callstack), callstackLength);

    ev->callstackLength = callstackLength;

    m_stream.EndAllocateRawEvent<MemReplayAllocEvent>(sizeof(ev->callstack[0]) * ev->callstackLength - sizeof(ev->callstack));
}

void CMemReplay::RecordRealloc(EMemReplayAllocClass::Class cls, uint16 subCls, int moduleId, UINT_PTR originalId, UINT_PTR newId, UINT_PTR alignment, UINT_PTR sizeRequested, UINT_PTR sizeConsumed, INT_PTR sizeGlobal)
{
    MemReplayReallocEvent* ev = new (m_stream.BeginAllocateRawEvent<MemReplayReallocEvent>(
                                         k_maxCallStackDepth * SIZEOF_MEMBER(MemReplayReallocEvent, callstack[0]) - SIZEOF_MEMBER(MemReplayReallocEvent, callstack)))
        MemReplayReallocEvent(
            CryGetCurrentThreadId(),
            static_cast<uint16>(moduleId),
            static_cast<uint16>(cls),
            static_cast<uint16>(subCls),
            originalId,
            newId,
            static_cast<uint32>(alignment),
            static_cast<uint32>(sizeRequested),
            static_cast<uint32>(sizeConsumed),
            static_cast<int32>(sizeGlobal));

    uint32 callstackLength = k_maxCallStackDepth;
    CSystem::debug_GetCallStackRaw(CastCallstack(ev->callstack), callstackLength);
    ev->callstackLength = callstackLength;

    m_stream.EndAllocateRawEvent<MemReplayReallocEvent>(ev->callstackLength * sizeof(ev->callstack[0]) - sizeof(ev->callstack));
}

void CMemReplay::RecordFree(EMemReplayAllocClass::Class cls, uint16 subCls, int moduleId, UINT_PTR id, INT_PTR sizeGlobal, bool captureCallstack)
{
    MemReplayFreeEvent* ev =
        new (m_stream.BeginAllocateRawEvent<MemReplayFreeEvent>(SIZEOF_MEMBER(MemReplayFreeEvent, callstack[0]) * k_maxCallStackDepth - SIZEOF_MEMBER(MemReplayFreeEvent, callstack)))
        MemReplayFreeEvent(
            CryGetCurrentThreadId(),
            static_cast<uint16>(moduleId),
            static_cast<uint16>(cls),
            static_cast<uint16>(subCls),
            id,
            static_cast<int32>(sizeGlobal));

    if (captureCallstack)
    {
        uint32 callstackLength = k_maxCallStackDepth;
        CSystem::debug_GetCallStackRaw(CastCallstack(ev->callstack), callstackLength);
        ev->callstackLength = callstackLength;
    }

    m_stream.EndAllocateRawEvent<MemReplayFreeEvent>(ev->callstackLength * sizeof(ev->callstack[0]) - sizeof(ev->callstack));
}

void CMemReplay::RecordModules()
{
    m_modules.RefreshModules();
}

MemReplayCrySizer::MemReplayCrySizer(ReplayLogStream& stream, const char* name)
    : m_stream(&stream)
    , m_totalSize(0)
    , m_totalCount(0)
{
    Push(name);
}

MemReplayCrySizer::~MemReplayCrySizer()
{
    Pop();

    {
        MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, 0);
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream->IsOpen())
        {
            // Clear the added objects set within the g_replayProcessed lock,
            // as memReplay won't have seen the allocations made by it.
            m_addedObjects.clear();
        }
    }
}

size_t MemReplayCrySizer::GetTotalSize()
{
    return m_totalSize;
}

size_t MemReplayCrySizer::GetObjectCount()
{
    return m_totalCount;
}

void MemReplayCrySizer::Reset()
{
    m_totalSize = 0;
    m_totalCount = 0;
}

bool MemReplayCrySizer::AddObject(const void* pIdentifier, size_t nSizeBytes, int nCount)
{
    bool ret = false;

    if (m_stream->IsOpen())
    {
        MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, 0);
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream->IsOpen())
        {
            std::set<const void*>::iterator it = m_addedObjects.find(pIdentifier);
            if (it == m_addedObjects.end())
            {
                m_totalSize += nSizeBytes;
                m_totalCount += nCount;
                m_addedObjects.insert(pIdentifier);
                m_stream->WriteEvent(MemReplaySizerAddRangeEvent((UINT_PTR)pIdentifier, nSizeBytes, nCount));
                ret = true;
            }
        }
    }

    return ret;
}

static NullResCollector s_nullCollectorMemreplay;

IResourceCollector* MemReplayCrySizer::GetResourceCollector()
{
    return &s_nullCollectorMemreplay;
}

void MemReplayCrySizer::SetResourceCollector(IResourceCollector*)
{
}

void MemReplayCrySizer::Push (const char* szComponentName)
{
    if (m_stream->IsOpen())
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream->IsOpen())
        {
            new (m_stream->AllocateRawEvent<MemReplaySizerPushEvent>(strlen(szComponentName)))MemReplaySizerPushEvent(szComponentName);
        }
    }
}

void MemReplayCrySizer::PushSubcomponent (const char* szSubcomponentName)
{
    Push(szSubcomponentName);
}

void MemReplayCrySizer::Pop()
{
    if (m_stream->IsOpen())
    {
        CryAutoLock<CryCriticalSection> lock(GetLogMutex());

        if (m_stream->IsOpen())
        {
            m_stream->WriteEvent(MemReplaySizerPopEvent());
        }
    }
}

#else // CAPTURE_REPLAY_LOG

#endif //CAPTURE_REPLAY_LOG


