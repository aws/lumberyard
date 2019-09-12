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
#ifndef AZ_UNITY_BUILD
#include <AzCore/PlatformIncl.h>
#include <AzCore/IPC/SharedMemory.h>

#include <AzCore/std/algorithm.h>

#include <AzCore/std/parallel/spin_mutex.h>

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_APPLE_OSX)

#if defined(AZ_PLATFORM_APPLE_OSX)
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

static int GetLastError()
{
    return errno;
}
#endif

namespace AZ
{
    namespace Internal
    {
        struct RingData
        {
            AZ::u32 m_readOffset;
            AZ::u32 m_writeOffset;
            AZ::u32 m_startOffset;
            AZ::u32 m_endOffset;
            AZ::u32 m_dataToRead;
            AZ::u8 m_pad[32 - sizeof(AZStd::spin_mutex)];
        };
    } // namespace Internal
} // namespace AZ



using namespace AZ;



#include <stdio.h>

//=========================================================================
// SharedMemory
// [4/27/2011]
//=========================================================================
SharedMemory::SharedMemory()
    : m_mappedBase(nullptr)
    , m_data(nullptr)
    , m_dataSize(0)
#if defined(AZ_PLATFORM_WINDOWS)
    , m_mapHandle(NULL)
    , m_globalMutex(NULL)
    , m_lastLockResult(WAIT_FAILED)
#elif defined(AZ_PLATFORM_APPLE_OSX)
    , m_mapHandle(-1)
    , m_globalMutex(nullptr)
#endif
{
    m_name[0] = '\0';
}

//=========================================================================
// ~SharedMemory
// [4/27/2011]
//=========================================================================
SharedMemory::~SharedMemory()
{
    UnMap();
    Close();
}

void fillMutexFullName(char *fullName, const char *name)
{
#if defined(AZ_PLATFORM_WINDOWS)
    azsnprintf(fullName, AZ_ARRAY_SIZE(fullName), "%s_Mutex", name);
#elif defined(AZ_PLATFORM_APPLE_OSX)
    // sem_open doesn't support paths bigger than 31, so call it s_Mtx.
    // While this is enough for Profiler, maybe the code should be smarter
    // and trim the name instead
    azsnprintf(fullName, AZ_ARRAY_SIZE(fullName), "/tmp/%s_Mtx", name);
#endif
}

//=========================================================================
// Create
// [4/27/2011]
//=========================================================================
SharedMemory::CreateResult
SharedMemory::Create(const char* name, unsigned int size, bool openIfCreated)
{
    AZ_Assert(name && strlen(name) > 1, "Invalid name!");
    AZ_Assert(size > 0, "Invalid buffer size!");
    if (IsReady())
    {
        return CreateFailed;
    }

    char fullName[256];
    azstrncpy(m_name, AZ_ARRAY_SIZE(m_name), name, strlen(name));
    fillMutexFullName(fullName, name);

#if defined(AZ_PLATFORM_WINDOWS)
    // Security attributes
    SECURITY_ATTRIBUTES secAttr;
    char secDesc[ SECURITY_DESCRIPTOR_MIN_LENGTH ];
    secAttr.nLength = sizeof(secAttr);
    secAttr.bInheritHandle = TRUE;
    secAttr.lpSecurityDescriptor = &secDesc;
    InitializeSecurityDescriptor(secAttr.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(secAttr.lpSecurityDescriptor, TRUE, 0, FALSE);
#endif

    // Obtain global mutex
#if defined(AZ_PLATFORM_WINDOWS)
    m_globalMutex = CreateMutex(&secAttr, FALSE, fullName);
    DWORD error = GetLastError();
    if (m_globalMutex == NULL || (error == ERROR_ALREADY_EXISTS && openIfCreated == false))
    {
        AZ_TracePrintf("AZSystem", "CreateMutex failed with error %d\n", error);
        return CreateFailed;
    }
#elif defined(AZ_PLATFORM_APPLE_OSX)
    m_globalMutex = sem_open(fullName, O_CREAT | O_EXCL, 0600, 1); 
    int error = errno;
    if (m_globalMutex == SEM_FAILED && (error == EEXIST))
    {
        sem_unlink(fullName);
        m_globalMutex = sem_open(fullName, O_CREAT | O_EXCL, 0600, 1); 
        error = errno;
    }
    if (m_globalMutex == SEM_FAILED)
    {
        AZ_TracePrintf("AZSystem", "CreateMutex failed with error %d\n", error);
        return CreateFailed;
    }
#endif

    // Create the file mapping.
    azsnprintf(fullName, AZ_ARRAY_SIZE(fullName), "%s_Data", name);
#if defined(AZ_PLATFORM_WINDOWS)
    m_mapHandle = CreateFileMapping(INVALID_HANDLE_VALUE, &secAttr, PAGE_READWRITE, 0, size, fullName);
    error = GetLastError();
    if (m_mapHandle == NULL || (error == ERROR_ALREADY_EXISTS && openIfCreated == false))
    {
        AZ_TracePrintf("AZSystem", "CreateFileMapping failed with error %d\n", error);
        return CreateFailed;
    }
#elif defined(AZ_PLATFORM_APPLE_OSX)
    m_mapHandle = shm_open(fullName, O_RDWR | O_CREAT | O_EXCL, 0600);
    error = errno;
    if (error == EEXIST)
    {
        m_mapHandle = shm_open(fullName, O_RDWR);
        error = errno;
    }
    if (m_mapHandle == -1 || (error == EEXIST && openIfCreated == false))
    {
        AZ_TracePrintf("AZSystem", "CreateFileMapping failed with error %d\n", error);
        return CreateFailed;
    }
    ftruncate(m_mapHandle, size);
#endif

#if defined(AZ_PLATFORM_WINDOWS)
    if (error != ERROR_ALREADY_EXISTS)
#elif defined(AZ_PLATFORM_APPLE_OSX)
    if (error != EEXIST)
#endif
    {
        MemoryGuard l(*this);
        if (Map())
        {
            Clear();
            UnMap();
        }
        else
        {
            return CreateFailed;
        }
        return CreatedNew;
    }

    return CreatedExisting;
}

//=========================================================================
// Open
// [4/27/2011]
//=========================================================================
bool
SharedMemory::Open(const char* name)
{
    AZ_Assert(name && strlen(name) > 1, "Invalid name!");

#if defined(AZ_PLATFORM_WINDOWS)
    if (m_mapHandle != NULL || m_globalMutex != NULL)
#elif defined(AZ_PLATFORM_APPLE_OSX)
    if (m_mapHandle != -1 || m_globalMutex != nullptr)
#endif
    {
        return false;
    }

    char fullName[256];
    azstrncpy(m_name, AZ_ARRAY_SIZE(m_name), name, strlen(name));
    fillMutexFullName(fullName, name);

#if defined(AZ_PLATFORM_WINDOWS)
    m_globalMutex = OpenMutex(SYNCHRONIZE, TRUE, fullName);
    AZ_Warning("AZSystem", m_globalMutex != NULL, "Failed to open OS mutex [%s]\n", m_name);
    if (m_globalMutex == NULL)
    {
        AZ_TracePrintf("AZSystem", "OpenMutex %s failed with error %d\n", m_name, GetLastError());
        return false;
    }
#elif defined(AZ_PLATFORM_APPLE_OSX)
    m_globalMutex = sem_open(fullName, 0);
    AZ_Warning("AZSystem", m_globalMutex != nullptr, "Failed to open OS mutex [%s]\n", m_name);
    if (m_globalMutex == nullptr)
    {
        AZ_TracePrintf("AZSystem", "OpenMutex %s failed with error %d\n", m_name, errno);
        return false;
    }
#endif

    azsnprintf(fullName, AZ_ARRAY_SIZE(fullName), "%s_Data", name);
#if defined(AZ_PLATFORM_WINDOWS)
    m_mapHandle = OpenFileMapping(FILE_MAP_WRITE, false, fullName);
    if (m_mapHandle == NULL)
    {
        AZ_TracePrintf("AZSystem", "OpenFileMapping %s failed with error %d\n", m_name, GetLastError());
        return false;
    }
#elif defined(AZ_PLATFORM_APPLE_OSX)
    m_mapHandle = shm_open(fullName, O_RDWR, 0600);
    if (m_mapHandle == -1)
    {
        AZ_TracePrintf("AZSystem", "OpenFileMapping %s failed with error %d\n", m_name, errno);
        return false;
    }
#endif


    return true;
}

//=========================================================================
// Close
// [4/27/2011]
//=========================================================================
void
SharedMemory::Close()
{
    UnMap();
#if defined(AZ_PLATFORM_WINDOWS)
    if (m_mapHandle != NULL && !CloseHandle(m_mapHandle))
#elif defined(AZ_PLATFORM_APPLE_OSX)
    if (m_mapHandle != -1 && close(m_mapHandle) == -1)
#endif
    {
        AZ_TracePrintf("AZSystem", "CloseHandle failed with error %d\n", GetLastError());
    }
#if defined(AZ_PLATFORM_WINDOWS)
    if (m_globalMutex != NULL && !CloseHandle(m_globalMutex))
#elif defined(AZ_PLATFORM_APPLE_OSX)
    if (m_globalMutex != nullptr && sem_close(m_globalMutex) == -1)
#endif
    {
        AZ_TracePrintf("AZSystem", "CloseHandle failed with error %d\n", GetLastError());
    }
#if defined(AZ_PLATFORM_WINDOWS)
    m_mapHandle = NULL;
    m_globalMutex = NULL;
#elif defined(AZ_PLATFORM_APPLE_OSX)
    m_mapHandle = -1;
    m_globalMutex = nullptr;
#endif
}

//=========================================================================
// Map
// [4/27/2011]
//=========================================================================
bool
SharedMemory::Map(AccessMode mode, unsigned int size)
{
    AZ_Assert(m_mappedBase == NULL, "We already have data mapped");
#if defined(AZ_PLATFORM_WINDOWS)
    AZ_Assert(m_mapHandle != NULL, "You must call Map() first!");
    DWORD dwDesiredAccess = (mode == ReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE);
    m_mappedBase = MapViewOfFile(m_mapHandle, dwDesiredAccess, 0, 0, size);
#elif defined(AZ_PLATFORM_APPLE_OSX)
    AZ_Assert(m_mapHandle != -1, "You must call Map() first!");
    struct stat st;
    fstat(m_mapHandle, &st);
    if (size == 0)
    {
        size = st.st_size;
    }
    int dwDesiredAccess = (mode == ReadOnly ? PROT_READ : PROT_READ | PROT_WRITE);
    m_mappedBase = mmap(0, size, dwDesiredAccess, MAP_SHARED, m_mapHandle, 0);
    m_mappedBase = (m_mappedBase == MAP_FAILED) ? nullptr : m_mappedBase;
#endif
    if (m_mappedBase == nullptr)
    {
        AZ_TracePrintf("AZSystem", "MapViewOfFile failed with error %d\n", GetLastError());
        Close();
        return false;
    }
#if defined(AZ_PLATFORM_WINDOWS)
    // Grab the size of the memory we have been given (a multiple of 4K on windows)
    MEMORY_BASIC_INFORMATION info;
    if (!VirtualQuery(m_mappedBase, &info, sizeof(info)))
    {
        AZ_TracePrintf("AZSystem", "VirtualQuery failed\n");
        return false;
    }
    m_dataSize = static_cast<unsigned int>(info.RegionSize);
#elif defined(AZ_PLATFORM_APPLE_OSX)
    m_dataSize = st.st_size;
#endif
    m_data = reinterpret_cast<char*>(m_mappedBase);
    return true;
}

//=========================================================================
// UnMap
// [4/27/2011]
//=========================================================================
bool
SharedMemory::UnMap()
{
    if (m_mappedBase == nullptr)
    {
        return false;
    }
#if defined(AZ_PLATFORM_WINDOWS)
    if (UnmapViewOfFile(m_mappedBase) == FALSE)
#elif defined(AZ_PLATFORM_APPLE_OSX)
    if (munmap(m_mappedBase, m_dataSize) != 0)
#endif
    {
        AZ_TracePrintf("AZSystem", "UnmapViewOfFile failed with error %d\n", GetLastError());
        return false;
    }
    m_mappedBase = nullptr;
    m_data = nullptr;
    m_dataSize = 0;
    return true;
}

//=========================================================================
// UnMap
// [4/27/2011]
//=========================================================================
void SharedMemory::lock()
{
    AZ_Assert(m_globalMutex, "You need to create/open the global mutex first! Call Create or Open!");
#if defined(AZ_PLATFORM_WINDOWS)
    DWORD lockResult = 0;
    do 
    {
        lockResult = WaitForSingleObject(m_globalMutex, 5);
        if (lockResult == WAIT_OBJECT_0 || lockResult == WAIT_ABANDONED)
        {
            break;
        }

        // If the wait failed, we need to re-acquire the mutex, because most likely
        // something bad has happened to it (we have experienced what looks to be a
        // reference counting issue where the mutex is killed for a process, and an
        // INFINITE wait will indeed wait...infinitely on that mutex, while other
        // processes are able to acquire it just fine)
        if (lockResult == WAIT_FAILED)
        {
            DWORD lastError = ::GetLastError();
            (void)lastError;
            AZ_Warning("AZSystem", lockResult != WAIT_FAILED, "WaitForSingleObject failed with code %u", lastError);
            Close();
            Open(m_name);
        }
        else if (lockResult != WAIT_TIMEOUT)
        {
            // According to the docs: https://msdn.microsoft.com/en-us/library/windows/desktop/ms687032(v=vs.85).aspx
            // WaitForSingleObject can only return WAIT_OBJECT_0, WAIT_ABANDONED, WAIT_FAILED, and WAIT_TIMEOUT
            AZ_Error("AZSystem", false, "WaitForSingleObject returned an undocumented error code: %u, GetLastError: %u", lockResult, ::GetLastError());
        }
    } while (lockResult != WAIT_OBJECT_0 && lockResult != WAIT_ABANDONED);

    m_lastLockResult = lockResult;

    AZ_Warning("AZSystem", m_lastLockResult != WAIT_ABANDONED, "We locked an abandoned Mutex, the shared memory data may be in instable state (corrupted)!");
#elif defined(AZ_PLATFORM_APPLE_OSX)
    sem_wait(m_globalMutex);
#endif
}

//=========================================================================
// UnMap
// [4/27/2011]
//=========================================================================
bool SharedMemory::try_lock()
{
    AZ_Assert(m_globalMutex, "You need to create/open the global mutex first! Call Create or Open!");
#if defined(AZ_PLATFORM_WINDOWS)
    m_lastLockResult = WaitForSingleObject(m_globalMutex, 0);
    AZ_Warning("AZSystem", m_lastLockResult != WAIT_ABANDONED, "We locked an abandoned Mutex, the shared memory data may be in instable state (corrupted)!");
    return (m_lastLockResult == WAIT_OBJECT_0) || (m_lastLockResult == WAIT_ABANDONED);
#elif defined(AZ_PLATFORM_APPLE_OSX)
    return sem_trywait(m_globalMutex) == 0;
#endif
}

//=========================================================================
// UnMap
// [4/27/2011]
//=========================================================================
void SharedMemory::unlock()
{
    AZ_Assert(m_globalMutex, "You need to create/open the global mutex first! Call Create or Open!");
#if defined(AZ_PLATFORM_WINDOWS)
    ReleaseMutex(m_globalMutex);
    m_lastLockResult = WAIT_FAILED;
#elif defined(AZ_PLATFORM_APPLE_OSX)
    sem_post(m_globalMutex);
#endif
}

bool SharedMemory::IsLockAbandoned()
{ 
#if defined(AZ_PLATFORM_WINDOWS)
    return (m_lastLockResult == WAIT_ABANDONED);
#elif defined(AZ_PLATFORM_APPLE_OSX)
    return false;
#endif
}

//=========================================================================
// Clear
// [4/19/2013]
//=========================================================================
void  SharedMemory::Clear()
{
    if (m_mappedBase != nullptr)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        AZ_Warning("AZSystem", m_lastLockResult != WAIT_FAILED, "You are clearing the shared memory %s while the Global lock is NOT locked! This can lead to data corruption!", m_name);
#endif
        memset(m_data, 0, m_dataSize);
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Shared Memory ring buffer
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// SharedMemoryRingBuffer
// [4/29/2011]
//=========================================================================
SharedMemoryRingBuffer::SharedMemoryRingBuffer()
    : m_info(NULL)
{}

//=========================================================================
// Create
// [4/29/2011]
//=========================================================================
bool
SharedMemoryRingBuffer::Create(const char* name, unsigned int size, bool openIfCreated)
{
    return SharedMemory::Create(name, size + sizeof(Internal::RingData), openIfCreated) != SharedMemory::CreateFailed;
}

//=========================================================================
// Map
// [4/28/2011]
//=========================================================================
bool
SharedMemoryRingBuffer::Map(AccessMode mode, unsigned int size)
{
    if (SharedMemory::Map(mode, size))
    {
        MemoryGuard l(*this);
        m_info = reinterpret_cast<Internal::RingData*>(m_data);
        m_data = m_info + 1;
        m_dataSize -= sizeof(Internal::RingData);
        if (m_info->m_endOffset == 0)  // if endOffset == 0 we have never set the info structure, do this only once.
        {
            m_info->m_startOffset = 0;
            m_info->m_readOffset = m_info->m_startOffset;
            m_info->m_writeOffset = m_info->m_startOffset;
            m_info->m_endOffset = m_info->m_startOffset + m_dataSize;
            m_info->m_dataToRead = 0;
        }
        return true;
    }

    return false;
}

//=========================================================================
// UnMap
// [4/28/2011]
//=========================================================================
bool
SharedMemoryRingBuffer::UnMap()
{
    m_info = NULL;
    return SharedMemory::UnMap();
}

//=========================================================================
// Write
// [4/28/2011]
//=========================================================================
bool
SharedMemoryRingBuffer::Write(const void* data, unsigned int dataSize)
{
#if defined(AZ_PLATFORM_WINDOWS)
    AZ_Warning("AZSystem", m_lastLockResult != WAIT_FAILED, "You are writing the ring buffer %s while the Global lock is NOT locked! This can lead to data corruption!", m_name);
#endif
    AZ_Assert(m_info != NULL, "You need to Create and Map the buffer first!");
    if (m_info->m_writeOffset >= m_info->m_readOffset)
    {
        unsigned int freeSpace = m_dataSize - (m_info->m_writeOffset - m_info->m_readOffset);
        // if we are full or don't have enough space return false
        if (m_info->m_dataToRead == m_dataSize || freeSpace < dataSize)
        {
            return false;
        }
        unsigned int copy1MaxSize = m_info->m_endOffset - m_info->m_writeOffset;
        unsigned int dataToCopy1 = AZStd::GetMin(copy1MaxSize, dataSize);
        if (dataToCopy1)
        {
            memcpy(reinterpret_cast<char*>(m_data) + m_info->m_writeOffset, data, dataToCopy1);
        }
        unsigned int dataToCopy2 = dataSize - dataToCopy1;
        if (dataToCopy2)
        {
            memcpy(reinterpret_cast<char*>(m_data) + m_info->m_startOffset, reinterpret_cast<const char*>(data) + dataToCopy1, dataToCopy2);
            m_info->m_writeOffset = m_info->m_startOffset + dataToCopy2;
        }
        else
        {
            m_info->m_writeOffset += dataToCopy1;
        }
    }
    else
    {
        unsigned int freeSpace = m_info->m_readOffset - m_info->m_writeOffset;
        if (freeSpace < dataSize)
        {
            return false;
        }
        memcpy(reinterpret_cast<char*>(m_data) + m_info->m_writeOffset, data, dataSize);
        m_info->m_writeOffset += dataSize;
    }
    m_info->m_dataToRead += dataSize;

    return true;
}

//=========================================================================
// Read
// [4/28/2011]
//=========================================================================
unsigned int
SharedMemoryRingBuffer::Read(void* data, unsigned int maxDataSize)
{
#if defined(AZ_PLATFORM_WINDOWS)
    AZ_Warning("AZSystem", m_lastLockResult != WAIT_FAILED, "You are reading the ring buffer %s while the Global lock is NOT locked! This can lead to data corruption!", m_name);
#endif

    if (m_info->m_dataToRead == 0)
    {
        return 0;
    }

    AZ_Assert(m_info != NULL, "You need to Create and Map the buffer first!");
    unsigned int dataRead;
    if (m_info->m_writeOffset > m_info->m_readOffset)
    {
        unsigned int dataToRead = AZStd::GetMin(m_info->m_writeOffset - m_info->m_readOffset, maxDataSize);
        if (dataToRead)
        {
            memcpy(data, reinterpret_cast<char*>(m_data) + m_info->m_readOffset, dataToRead);
        }
        m_info->m_readOffset += dataToRead;
        dataRead = dataToRead;
    }
    else
    {
        unsigned int dataToRead1 = AZStd::GetMin(m_info->m_endOffset - m_info->m_readOffset, maxDataSize);
        if (dataToRead1)
        {
            maxDataSize -= dataToRead1;
            memcpy(data, reinterpret_cast<char*>(m_data) + m_info->m_readOffset, dataToRead1);
        }
        unsigned int dataToRead2 = AZStd::GetMin(m_info->m_writeOffset - m_info->m_startOffset, maxDataSize);
        if (dataToRead2)
        {
            memcpy(reinterpret_cast<char*>(data) + dataToRead1, reinterpret_cast<char*>(m_data) + m_info->m_startOffset, dataToRead2);
            m_info->m_readOffset = m_info->m_startOffset + dataToRead2;
        }
        else
        {
            m_info->m_readOffset += dataToRead1;
        }
        dataRead = dataToRead1 + dataToRead2;
    }
    m_info->m_dataToRead -= dataRead;

    return dataRead;
}

//=========================================================================
// Read
// [4/28/2011]
//=========================================================================
unsigned int
SharedMemoryRingBuffer::DataToRead() const
{
    return m_info ? m_info->m_dataToRead : 0;
}

//=========================================================================
// Read
// [4/28/2011]
//=========================================================================
unsigned int
SharedMemoryRingBuffer::MaxToWrite() const
{
    return m_info ? (m_dataSize - m_info->m_dataToRead) : 0;
}

//=========================================================================
// Clear
// [4/19/2013]
//=========================================================================
void
SharedMemoryRingBuffer::Clear()
{
    SharedMemory::Clear();
    if (m_info)
    {
        m_info->m_readOffset = m_info->m_startOffset;
        m_info->m_writeOffset = m_info->m_startOffset;
        m_info->m_dataToRead = 0;
    }
}

#endif // AZ_PLATFORM_WINDOWS || AZ_PLATFORM_APPLE_OSX

#endif // #ifndef AZ_UNITY_BUILD
