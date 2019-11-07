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

#include <AzCore/IO/CompressionBus.h>
#include <AzCore/IO/Streamer.h>
#include <AzCore/IO/Device.h>
#include <AzCore/IO/StreamerUtil.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>
#include <AzCore/IO/Streamer/StreamStackPreferences.h>

#include <AzCore/IO/StreamerDrillerBus.h>

#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ
{
    namespace IO
    {
        /**
         * Streamer internal data.
         */
        struct StreamerData
        {
        public:
            AZ_CLASS_ALLOCATOR(StreamerData, SystemAllocator, 0)

            AZStd::thread_desc  m_threadDesc;                   ///< Device thread descriptor.
            AZStd::shared_ptr<StreamStackEntry> m_streamStack;  ///< The stack used for file reads that go directly to the file system and decompress asynchronously.

            AZStd::mutex                        m_devicesLock;
            AZStd::fixed_vector<Device*, 32>    m_devices;
            StreamerContext m_streamerContext;
        };
    }
}

using namespace AZ::IO;

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Streamer
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

namespace AZ
{
    namespace IO
    {
        static const char* kStreamerInstanceVarName = "StreamerInstance";

        AZ::EnvironmentVariable<Streamer*> g_streamer;

        Streamer* GetStreamer()
        {
            if (!g_streamer)
            {
                g_streamer = Environment::FindVariable<Streamer*>(kStreamerInstanceVarName);
            }

            return g_streamer ? (*g_streamer) : nullptr;
        }

        namespace Platform
        {
            bool GetDeviceSpecsFromPath(DeviceSpecifications& specs, const char* fullpath);
        }
    }
}

//=========================================================================
// Create
// [10/2/2009]
//=========================================================================
bool
Streamer::Create(const Descriptor& desc)
{
    AZ_Assert(!g_streamer || !g_streamer.Get(), "Data streamer already created!");

    if (!g_streamer)
    {
        g_streamer = Environment::CreateVariable<Streamer*>(kStreamerInstanceVarName);
    }
    if (!g_streamer.Get())
    {
        g_streamer.Set(aznew Streamer(desc));
    }

    return true;
}

//=========================================================================
// Destroy
// [10/2/2009]
//=========================================================================
void
Streamer::Destroy()
{
    AZ_Assert(g_streamer, "Data streamer not created!");
    delete g_streamer.Get();
    *g_streamer = nullptr;
}

//=========================================================================
// IsReady
//=========================================================================
bool Streamer::IsReady()
{
    if (!g_streamer)
    {
        g_streamer = Environment::FindVariable<Streamer*>(kStreamerInstanceVarName);
    }

    return g_streamer && *g_streamer;
}

//=========================================================================
// Instance
//=========================================================================
Streamer& Streamer::Instance()
{
    if (!g_streamer)
    {
        g_streamer = Environment::FindVariable<Streamer*>(kStreamerInstanceVarName);
    }

    AZ_Assert(g_streamer, "Data streamer not created!");
    return *g_streamer.Get();
}

//=========================================================================
// Streamer
// [10/2/2009]
//=========================================================================
Streamer::Streamer(const Descriptor& desc)
{
    m_data = aznew StreamerData;
    m_data->m_streamStack = AZStd::make_shared<StreamStackEntry>("Root");
    if (desc.m_streamStack)
    {
        m_data->m_streamStack->SetNext(desc.m_streamStack);
    }
    else
    {
        m_data->m_streamStack->SetNext(StreamStack::CreateDefaultStreamStack(StreamStack::Preferences{}));
    }
    m_data->m_streamStack->SetContext(m_data->m_streamerContext);

    if (desc.m_threadDesc)
    {
        m_data->m_threadDesc = *desc.m_threadDesc;
    }
}

//=========================================================================
// ~Streamer
// [10/2/2009]
//=========================================================================
Streamer::~Streamer()
{
    ReleaseDevices();

    delete m_data;
}

//=========================================================================
// FindDevice
//=========================================================================
Device* Streamer::FindDevice(const char* streamName, bool resolvePath)
{
    AZ_Assert(streamName, "No name provided for finding a streaming device.");
    
    RequestPath path;
    const char* filePath;
    if (resolvePath)
    {
        if (!path.InitFromRelativePath(streamName))
        {
            AZ_Error("IO", false, "Failed to resolve path '%s' for FindDevice.", streamName);
            return nullptr;
        }
        filePath = path.GetAbsolutePath();
    }
    else
    {
        filePath = streamName;
    }

    AZStd::lock_guard<AZStd::mutex> lock(m_data->m_devicesLock);
    for (Device* device : m_data->m_devices)
    {
        if (device->IsDeviceForPath(filePath))
        {
            return device;
        }
    }

    DeviceSpecifications specifications;
    if (!Platform::GetDeviceSpecsFromPath(specifications, filePath) || specifications.m_deviceName.empty())
    {
        AZ_Error("IO", false, "Unable to retrieve device information for '%s'", streamName);
        return nullptr;
    }

    // Try to find the device based on the name in case it's a new path identifier for an existing device.
    for (Device* device : m_data->m_devices)
    {
        if (specifications.m_deviceName == device->GetPhysicalName())
        {
            for (AZStd::string& identifier : specifications.m_pathIdentifiers)
            {
                device->AddPathIdentifier(AZStd::move(identifier));
            }
            return device;
        }
    }

    // This is a new device so add it to the list of known devices.
    AZStd::thread_desc threadDesc;
    threadDesc = m_data->m_threadDesc;

    // Try to create device?
    threadDesc.m_name = "Streaming device";
    Device* device = aznew Device(specifications, m_data->m_streamStack,
        m_data->m_streamerContext, &threadDesc);
    
    EBUS_DBG_EVENT(StreamerDrillerBus, OnDeviceMounted, device);
    m_data->m_devices.push_back(device);
    return device;
}

void
Streamer::ReleaseDevices()
{
    AZStd::lock_guard<AZStd::mutex> l(m_data->m_devicesLock);
    for (size_t i = 0; i < m_data->m_devices.size(); ++i)
    {
        EBUS_DBG_EVENT(StreamerDrillerBus, OnDeviceUnmounted, m_data->m_devices[i]);
        delete m_data->m_devices[i];
    }
    m_data->m_devices.clear();
}

//=========================================================================
// Read
// [10/5/2009]
//=========================================================================
Streamer::SizeType
Streamer::Read(const char* fileName, SizeType byteOffset, SizeType byteSize, void* dataBuffer, AZStd::chrono::microseconds deadline, 
    Request::PriorityType priority, Request::StateType* state, const char* debugName)
{
    AZ_Assert(fileName, "You can't pass a NULL filename!");
    SyncRequestCallback doneCB;
    if (ReadAsync(fileName, byteOffset, byteSize, dataBuffer, doneCB, deadline, priority, debugName, false))
    {
        doneCB.Wait();
        if (state)
        {
            *state = doneCB.m_state;
        }
        return doneCB.m_bytesTrasfered;
    }

    if (state)
    {
        *state = Request::StateType::ST_ERROR_FAILED_TO_OPEN_FILE;
    }

    return 0;
}

bool Streamer::ReadAsync(const char* filename, SizeType byteOffset, SizeType byteSize, void* dataBuffer, const Request::RequestDoneCB& callback, 
    AZStd::chrono::microseconds deadline, Request::PriorityType priority, const char* debugName, bool deferCallback)
{
    AZStd::shared_ptr<Request> request = CreateAsyncRead(filename, byteOffset, byteSize, dataBuffer, callback, deadline, priority, debugName, deferCallback);
    if (request)
    {
        return QueueRequest(AZStd::move(request));
    }
    return false;
}

AZStd::shared_ptr<Request> Streamer::CreateAsyncRead(const char* fileName, SizeType byteOffset, SizeType byteSize, void* dataBuffer, const Request::RequestDoneCB& callback,
    AZStd::chrono::microseconds deadline, Request::PriorityType priority, const char* debugName, bool deferCallback)
{
    AZ_Assert(fileName, "No file name provided for reading through AZ::IO::Streamer.");

    AZStd::shared_ptr<Request> request;
    RequestPath path;

    CompressionInfo info;
    bool isArchived = false;
    if (!ResolveRequestPath(path, isArchived, info, fileName))
    {
        AZ_Error("IO", false, "Failed to resolve path '%s' for CreateAsyncRead.", fileName);
        return request;
    }

    Request::OperationType operation = Request::OperationType::NONE;
    if (isArchived)
    {
        if (byteOffset + byteSize > info.m_uncompressedSize)
        {
            AZ_Error("IO", false, "Requested offset (%i) and size (%i) for file '%s' is not within the compressed file '%s'.",
                byteOffset, byteSize, fileName, info.m_archiveFilename.GetRelativePath());
            return request;
        }

        if (info.m_isCompressed)
        {
            operation = (byteOffset == 0 && byteSize == info.m_uncompressedSize) ?
                Request::OperationType::FULL_DECOMPRESSED_READ : Request::OperationType::PARTIAL_DECOMPRESSED_READ;
        }
        else
        {
            // The file doesn't need to be decompressed so translate the current request to read from the pak instead.
            byteOffset += info.m_offset;
            operation = Request::OperationType::ARCHIVED_READ;
        }
    }
    else
    {
        if (!path.InitFromRelativePath(fileName))
        {
            AZ_Error("IO", false, "Failed to resolve path '%s' for CreateAsyncRead.", fileName);
            return request;
        }
        operation = Request::OperationType::DIRECT_READ;
    }

    Device* device = FindDevice(path.GetAbsolutePath(), false);
    if (device)
    {
        request = AZStd::make_shared<Request>(device, AZStd::move(path), byteOffset, byteSize, dataBuffer,
            callback, deferCallback, priority, operation, debugName);
        request->SetDeadline(deadline);
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        request->m_originalFilename = fileName;
#endif
        if (request->NeedsDecompression())
        {
            request->AddDecompressionInfo(AZStd::move(info));
        }
        request->m_state = Request::StateType::ST_PREPARING;
    }
    return request;
}

bool Streamer::QueueRequest(AZStd::shared_ptr<Request> request)
{
    AZ_Assert(request, "Invalid request provided for streaming.");
    
    if (request->m_state != Request::StateType::ST_PREPARING)
    {
        AZ_Assert(request->m_state == Request::StateType::ST_PREPARING, "Only requests that are in a prepared state can be queued. "
            "Use one of the PrepareAsync* functions to create a request handle in a prepared state.");
        return false;
    }
    
    if (request->m_device)
    {
        request->m_state = Request::StateType::ST_PENDING;
        if (request->IsReadOperation())
        {
            request->m_device->AddCommand(&DeviceRequest::ReadRequest, AZStd::move(request));
            return true;
        }
        else
        {
            AZ_Assert(false, "Unsupported operation (%i) for AZ::IO::Streamer.", request->m_operation);
            return false;
        }
    }
    else
    {
        AZ_Assert(request->m_device, "Streaming request didn't have an device assigned.");
        return false;
    }
}

//=========================================================================
// CancelRequest
// [10/5/2009]
//=========================================================================
void
Streamer::CancelRequest(const AZStd::shared_ptr<Request>& request)
{
    AZ_Assert(request, "Invalid request provided to cancel.");
    AZStd::semaphore wait;
    CancelRequestAsync(request, &wait); 
    wait.acquire();
}

//=========================================================================
// CancelRequestAsync
// [11/6/2012]
//=========================================================================
void
Streamer::CancelRequestAsync(const AZStd::shared_ptr<Request>& request, AZStd::semaphore* sync)
{
    AZ_Assert(request, "Invalid request provided to cancel.");
    // Only queue if the request hasn't been completed.
    if (!request->HasCompleted())
    {
        Device* device = GetStreamer()->FindDevice(request->m_filename.GetAbsolutePath());
        if (device)
        {
            device->AddCommand(&DeviceRequest::CancelRequest, request, sync);
            return;
        }
    }
    
    if (sync)
    {
        sync->release();
    }
}

//=========================================================================
// RescheduleRequest
// [11/8/2011]
//=========================================================================
void
Streamer::RescheduleRequest(const AZStd::shared_ptr<Request>& request, Request::PriorityType priority, AZStd::chrono::microseconds deadline)
{
    AZ_Assert(request, "Invalid request provided to reschedule.");
    EBUS_DBG_EVENT(StreamerDrillerBus, OnRescheduleRequest, request, AZStd::chrono::system_clock::now() + deadline, priority);
    request->m_priority = priority;
    if (request->m_operation == Request::OperationType::UTIL)
    {
        request->SetDeadline(deadline);
    }
}

//=========================================================================
// EstimateCompletion
// [11/8/2011]
//=========================================================================
AZStd::chrono::microseconds
Streamer::EstimateCompletion(const AZStd::shared_ptr<Request>& request)
{
    AZ_Assert(request, "Invalid request provided to estimate completion for.");
    AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
    if (now < request->m_estimatedCompletion)
    {
        return request->m_estimatedCompletion - now;
    }
    else
    {
        return AZStd::chrono::microseconds(0);
    }
}

//=========================================================================
// ReceiveRequests
// [11/8/2011]
//=========================================================================
void
Streamer::ReceiveRequests()
{
    for (Device* device : m_data->m_devices)
    {
        device->InvokeCompletedCallbacks();
    }
}

void Streamer::SuspendAllDeviceProcessing()
{
    AZStd::lock_guard<AZStd::mutex> lock(m_data->m_devicesLock);
    for (Device* device : m_data->m_devices)
    {
        device->SuspendProcessing();
    }
}

void Streamer::ResumeAllDeviceProcessing()
{
    AZStd::lock_guard<AZStd::mutex> lock(m_data->m_devicesLock);
    for (Device* device : m_data->m_devices)
    {
        device->ResumeProcessing();
    }
}

void Streamer::CreateDedicatedCache(const char* filename)
{
    AZStd::semaphore wait;
    CreateDedicatedCacheAsync(filename, &wait);
    wait.acquire();
}

void Streamer::CreateDedicatedCacheAsync(const char* filename, AZStd::semaphore* sync)
{
    AZ_Assert(filename, "CreateDedicatedCacheAsync requires a valid filename.");

    FileRange range;
    RequestPath path;
    CompressionInfo info;
    bool isArchived = false;
    if (!ResolveRequestPath(path, isArchived, info, filename))
    {
        AZ_Error("IO", false, "Failed to resolve path '%s' for CreateDedicatedCacheAsync.", filename);
        if (sync)
        {
            sync->release();
        }
        return;
    }

    if (isArchived)
    {
        range = FileRange::CreateRange(info.m_offset, info.m_isCompressed ? info.m_compressedSize : info.m_uncompressedSize);
    }
    else
    {
        range = FileRange::CreateRangeForEntireFile();
    }

    Device* device = GetStreamer()->FindDevice(path.GetAbsolutePath(), false);
    if (device)
    {
        device->AddCommand(&DeviceRequest::CreateDedicatedCacheRequest, AZStd::move(path), AZStd::move(range), sync);
    }
}

void Streamer::DestroyDedicatedCache(const char* filename)
{
    AZStd::semaphore wait;
    DestroyDedicatedCacheAsync(filename, &wait);
    wait.acquire();
}

void Streamer::DestroyDedicatedCacheAsync(const char* filename, AZStd::semaphore* sync)
{
    AZ_Assert(filename, "DestroyDedicatedCacheAsync requires a valid filename.");

    FileRange range;
    RequestPath path;
    CompressionInfo info;
    bool isArchived = false;
    if (!ResolveRequestPath(path, isArchived, info, filename))
    {
        AZ_Error("IO", false, "Failed to resolve path '%s' for DestroyDedicatedCacheAsync.", filename);
        if (sync)
        {
            sync->release();
        }
        return;
    }

    if (isArchived)
    {
        range = FileRange::CreateRange(info.m_offset, info.m_isCompressed ? info.m_compressedSize : info.m_uncompressedSize);
    }
    else
    {
        range = FileRange::CreateRangeForEntireFile();
    }

    Device* device = GetStreamer()->FindDevice(path.GetAbsolutePath(), false);
    if (device)
    {
        device->AddCommand(&DeviceRequest::DestroyDedicatedCacheRequest, AZStd::move(path), AZStd::move(range), sync);
    }
}

void Streamer::FlushCache(const char* filename)
{
    AZStd::semaphore wait;
    FlushCacheAsync(filename, &wait);
    wait.acquire();
}

void Streamer::FlushCacheAsync(const char* filename, AZStd::semaphore* sync)
{
    RequestPath path;
    CompressionInfo info;
    bool isArchived = false;
    if (!ResolveRequestPath(path, isArchived, info, filename))
    {
        // If the path couldn't be resolved here, it's also not going to be resolved
        // in any of the calls that could cache, so no need to call the flush.
        if (sync)
        {
            sync->release();
        }
        return;
    }

    if (sync)
    {
        AZStd::semaphore* waits = nullptr;
        size_t deviceCount = 0;
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_data->m_devicesLock);
            deviceCount = m_data->m_devices.size();
            waits = new AZStd::semaphore[deviceCount];

            for (size_t i = 0; i < deviceCount; ++i)
            {
                m_data->m_devices[i]->AddCommand(&DeviceRequest::FlushCacheRequest, path, &waits[i]);
            }
        }

        for (size_t i = 0; i<deviceCount; ++i)
        {
            waits[i].acquire();
        }
        sync->release();
        delete[] waits;
    }
    else
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_data->m_devicesLock);
        size_t deviceCount = m_data->m_devices.size();
        for (size_t i = 0; i < deviceCount; ++i)
        {
            m_data->m_devices[i]->AddCommand(&DeviceRequest::FlushCacheRequest, path, static_cast<AZStd::semaphore*>(nullptr));
        }
    }
}

void Streamer::FlushCaches()
{
    AZStd::semaphore* waits = nullptr;
    size_t deviceCount = 0;
    
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_data->m_devicesLock);
        deviceCount = m_data->m_devices.size();
        waits = new AZStd::semaphore[deviceCount];

        for (size_t i = 0; i < deviceCount; ++i)
        {
            m_data->m_devices[i]->AddCommand(&DeviceRequest::FlushCachesRequest, &waits[i]);
        }
    }

    for (size_t i=0; i<deviceCount; ++i)
    {
        waits[i].acquire();
    }
    delete[] waits;
}

void Streamer::FlushCachesAsync()
{
    AZStd::lock_guard<AZStd::mutex> lock(m_data->m_devicesLock);
    for (Device* device : m_data->m_devices)
    {
        device->AddCommand(&DeviceRequest::FlushCachesRequest, static_cast<AZStd::semaphore*>(nullptr));
    }
}

void Streamer::CollectStatistics(AZStd::vector<Statistic>& statistics)
{
    AZStd::lock_guard<AZStd::mutex> lock(m_data->m_devicesLock);
    for (Device* device : m_data->m_devices)
    {
        device->CollectStatistics(statistics);
    }
}

bool Streamer::ReplaceStreamStackEntry(AZStd::string_view entryName, const AZStd::shared_ptr<StreamStackEntry>& replacement)
{
    AZStd::shared_ptr<StreamStackEntry> previousEntry;
    AZStd::shared_ptr<StreamStackEntry> currentEntry = m_data->m_streamStack;
    do
    {
        if (currentEntry->GetName() == entryName)
        {
            replacement->SetContext(m_data->m_streamerContext);
            replacement->SetNext(currentEntry->GetNext());
            AZ_Assert(previousEntry, "AZ::IO::Streamer is missing a root entry in the streaming stack or the root it being replaced.");
            previousEntry->SetNext(replacement);
            return true;
        }
        previousEntry = currentEntry;
        currentEntry = currentEntry->GetNext();
    } while (currentEntry);
    return false;
}

bool Streamer::ResolveRequestPath(RequestPath& resolvedPath, bool& isArchivedFile, CompressionInfo& foundCompressionInfo, const char* fileName) const
{
    if (AZ::IO::CompressionUtils::FindCompressionInfo(foundCompressionInfo, fileName))
    {
        isArchivedFile = true;
        if (foundCompressionInfo.m_conflictResolution != ConflictResolution::UseArchiveOnly)
        {
            if (!resolvedPath.InitFromRelativePath(fileName))
            {
                return false;
            }

            if (AZ::IO::SystemFile::Exists(resolvedPath.GetAbsolutePath()))
            {
                isArchivedFile = (foundCompressionInfo.m_conflictResolution == ConflictResolution::PreferArchive);
            }
        }

        if (isArchivedFile)
        {
            resolvedPath = foundCompressionInfo.m_archiveFilename;
        }
    }
    else
    {
        AZ_UNUSED(foundCompressionInfo);
        isArchivedFile = false;
        if (!resolvedPath.InitFromRelativePath(fileName))
        {
            return false;
        }
    }
    return true;
}
