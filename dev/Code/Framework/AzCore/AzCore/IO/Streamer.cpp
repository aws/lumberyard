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

#include <AzCore/IO/Streamer.h>
#include <AzCore/IO/Device.h>
#include <AzCore/IO/NetworkDevice.h>
#include <AzCore/IO/OpticalDevice.h>
#include <AzCore/IO/MagneticDevice.h>
#include <AzCore/IO/StreamerUtil.h>
#include <AzCore/IO/Compressor.h>

#include <AzCore/IO/StreamerDrillerBus.h>
#include <AzCore/IO/VirtualStream.h>

#if defined(AZ_PLATFORM_WINDOWS)
#   include <direct.h>
#   include <AzCore/PlatformIncl.h>
#endif


#if defined(AZ_PLATFORM_WINDOWS)
#   define AZ_PATH_SEPARATOR_TOKEN  "\\"
#else
#   define AZ_PATH_SEPARATOR_TOKEN  "/"
#endif

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

            SizeType            m_maxNumOpenLimitedStream;      ///< Max number of open streams with limited flag on. \ref FileIOStream::m_isLimited set to true.
            unsigned int        m_cacheBlockSize;               ///< Must be multiple of device sector size.
            unsigned int        m_numCacheBlocks;
            int                 m_deviceThreadSleepTimeMS;      ///< Device thread sleep time between each operation.
            AZStd::thread_desc  m_threadDesc;                   ///< Device thread descriptor.

            // todo stream overlay map

            AZStd::mutex                        m_devicesLock;
            AZStd::fixed_vector<Device*, 32>     m_devices;
            AZStd::string                       m_fileMountPoint;
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
// GetDeviceSpecsFromPath
// [11/10/2011]
//=========================================================================
bool
GetDeviceSpecsFromPath(const char* fullpath, AZStd::string* deviceName, Device::Type* deviceType)
{
#if defined(AZ_PLATFORM_WINDOWS)
    // extract root
    char root[256] = {0};
    if (strncmp(fullpath, "\\\\", 2) == 0 || strncmp(fullpath, "//", 2) == 0)
    {
        int slashes = 4;
        for (const char* c = fullpath; *c; ++c)
        {
            if (*c == '\\' || *c == '/')
            {
                if (--slashes == 0)
                {
                    azstrncpy(root, fullpath, size_t(c - fullpath + 1));
                    break;
                }
            }
        }
    }
    else
    {
        for (const char* c = fullpath; *c; ++c)
        {
            if (*c == ':')
            {
                if (c[1] == '\\' || c[1] == '/')
                {
                    azstrncpy(root, fullpath, size_t(c - fullpath + 2));
                    break;
                }
            }
        }
    }

    // find device specs based on root
    if (root[0])
    {
        unsigned int driveType = GetDriveTypeA(root);
        if (driveType == DRIVE_CDROM)
        {
            if (deviceType)
            {
                *deviceType = Device::DT_OPTICAL_MEDIA;
            }
            if (deviceName)
            {
                char dummy[256];
                DWORD sn;
                GetVolumeInformationA(root, dummy, 256, &sn, 0, 0, dummy, 256);
                *deviceName = AZStd::string::format("%u", sn);
            }
        }
        else if (driveType == DRIVE_FIXED)
        {
            if (deviceType)
            {
                *deviceType = Device::DT_MAGNETIC_DRIVE;
            }
            if (deviceName)
            {
                char dummy[256];
                DWORD sn;
                GetVolumeInformationA(root, dummy, 256, &sn, 0, 0, dummy, 256);
                *deviceName = AZStd::string::format("%u", sn);
            }
        }
        else
        {
            if (deviceType)
            {
                *deviceType = Device::DT_NETWORK_DRIVE;
            }
            if (deviceName)
            {
                *deviceName = "default";
            }
        }
        return true;
    }
    return false;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)

    #pragma message("AZ_PLATFORM_LINUX: Device and path info, not fully implemented - defaults Device::DT_MAGNETIC_DRIVE.")

    if (deviceType)
    {
        *deviceType = Device::DT_MAGNETIC_DRIVE;
    }
    if (deviceName)
    {
        *deviceName = "default";
    }

    return true;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(Streamer_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    AZ_Assert(false, "GetDeviceSpecsFromPath is not implemented on this platform");
    (void)deviceType;
    (void)deviceName;
    (void)fullpath;
    return false;
#endif
}

//=========================================================================
// NormalizePath
// [11/10/2011]
//=========================================================================
void
NormalizePath(const char* path, const char* defaultRoot, AZStd::string& oFullPath)
{
    if (defaultRoot)
    {
        bool isFullPath = false;
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        if (strncmp(path, "\\\\", 2) == 0)
        {
            isFullPath = true;
        }
        else
        {
            for (const char* c = path; *c; ++c)
            {
                if (*c == ':')
                {
                    isFullPath = true;
                    break;
                }
            }
        }
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
        if (strncmp(path, "./", strlen("./")) == 0 ||
            strncmp(path, "/", strlen("/")) == 0)
        {
            isFullPath = true;
        }
#elif defined(AZ_PLATFORM_ANDROID)
        if (strncmp(path, "/boot/", strlen("/boot/")) == 0 ||
            strncmp(path, "/cache/", strlen("/cache/")) == 0 ||
            strncmp(path, "/data/", strlen("/data/")) == 0 ||
            strncmp(path, "/dev/", strlen("/dev/")) == 0 ||
            strncmp(path, "/mnt/", strlen("/mnt/")) == 0 ||
            strncmp(path, "/proc/", strlen("/proc/")) == 0 ||
            strncmp(path, "/sdcard/", strlen("/sdcard/")) == 0 ||
            strncmp(path, "/recovery/", strlen("/recovery/")) == 0 ||
            strncmp(path, "/system/", strlen("/system/")) == 0)
        {
            isFullPath = true;
        }
#endif
        if (!isFullPath)
        {
            oFullPath = AZStd::string::format("%s%s", defaultRoot, path);
            return;
        }
    }

    // already full path or no default root specified
    oFullPath = path;
}
//=========================================================================
// Streamer
// [10/2/2009]
//=========================================================================
Streamer::Streamer(const Descriptor& desc)
{
    m_data = aznew StreamerData;
    m_data->m_maxNumOpenLimitedStream = desc.m_maxNumOpenLimitedStream;
    m_data->m_cacheBlockSize = desc.m_cacheBlockSize;
    m_data->m_numCacheBlocks = desc.m_numCacheBlocks;
    m_data->m_deviceThreadSleepTimeMS = desc.m_deviceThreadSleepTimeMS;

    if (desc.m_threadDesc)
    {
        m_data->m_threadDesc = *desc.m_threadDesc;
    }
    if (desc.m_fileMountPoint)
    {
        m_data->m_fileMountPoint = desc.m_fileMountPoint;

        // make sure that the mount point always ends in a slash
        size_t len = strlen(desc.m_fileMountPoint);
        if (len > 0 && desc.m_fileMountPoint[len - 1] != '/' && desc.m_fileMountPoint[len - 1] != '\\')
        {
            m_data->m_fileMountPoint += AZ_PATH_SEPARATOR_TOKEN;
        }
    }
    else
    {
#if defined(AZ_PLATFORM_WINDOWS)
        // if not specified get the current working path
        char currentPath[1024];
        _getcwd(currentPath, AZ_ARRAY_SIZE(currentPath));
        m_data->m_fileMountPoint = currentPath;
        m_data->m_fileMountPoint += '\\';
#endif
    }
}

//=========================================================================
// ~Streamer
// [10/2/2009]
//=========================================================================
Streamer::~Streamer()
{
    {
        AZStd::lock_guard<AZStd::mutex> l(m_data->m_devicesLock);
        for (size_t i = 0; i < m_data->m_devices.size(); ++i)
        {
            EBUS_DBG_EVENT(StreamerDrillerBus, OnDeviceUnmounted, m_data->m_devices[i]);
            delete m_data->m_devices[i];
        }
    }

    delete m_data;
}
//=========================================================================
// FindDevice
// [11/8/2011]
//=========================================================================
Device*
Streamer::FindDevice(const char* streamName)
{
    Device* device = NULL;

    AZStd::string fullStreamName;
    NormalizePath(streamName, m_data->m_fileMountPoint.c_str(), fullStreamName);

    // Get physical device name
    AZStd::string oPhysicalName;
    Device::Type oType;
    GetDeviceSpecsFromPath(fullStreamName.c_str(), &oPhysicalName, &oType);
    if (!oPhysicalName.empty())
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_data->m_devicesLock);
        for (size_t i = 0; i < m_data->m_devices.size(); ++i)
        {
            if (strncmp(oPhysicalName.c_str(), m_data->m_devices[i]->GetName().c_str(), m_data->m_devices[i]->GetName().length()) == 0)
            {
                device = m_data->m_devices[i];
                break;
            }
        }
        if (!device)
        {
            AZStd::thread_desc threadDesc;
            threadDesc = m_data->m_threadDesc;

            // Try to create device?
            if (oType == Device::DT_OPTICAL_MEDIA)
            {
                threadDesc.m_name = "AZ::DVD/BLU-RAY stream";
                device = aznew OpticalDevice(oPhysicalName, FileIOBase::GetInstance(), m_data->m_cacheBlockSize, m_data->m_numCacheBlocks, &threadDesc, m_data->m_deviceThreadSleepTimeMS);
            }
            else if (oType == Device::DT_MAGNETIC_DRIVE)
            {
                threadDesc.m_name = "AZ::HDD stream";
                device = aznew MagneticDevice(oPhysicalName, FileIOBase::GetInstance(), m_data->m_cacheBlockSize, m_data->m_numCacheBlocks, &threadDesc, m_data->m_deviceThreadSleepTimeMS);
            }
            else if (oType == Device::DT_NETWORK_DRIVE)
            {
                threadDesc.m_name = "AZ::Net stream";
                device = aznew NetworkDevice(oPhysicalName, FileIOBase::GetInstance(), m_data->m_cacheBlockSize, m_data->m_numCacheBlocks, &threadDesc, m_data->m_deviceThreadSleepTimeMS);
            }
            AZ_Assert(device, "Unsupported device type (0x%x)", oType);
            EBUS_DBG_EVENT(StreamerDrillerBus, OnDeviceMounted, device);
            m_data->m_devices.push_back(device);
        }
    }

    AZ_Assert(device != NULL, "Could not identify a valid device from the path suplied!(%s)", streamName);

    return device;
}

//=========================================================================
// GetMaxNumOpenLimitedStream
// [8/29/2012]
//=========================================================================
SizeType
Streamer::GetMaxNumOpenLimitedStream() const
{
    return m_data->m_maxNumOpenLimitedStream;
}

//=========================================================================
// RegisterFileStream
// [10/2/2009]
//=========================================================================
VirtualStream*
Streamer::RegisterFileStream(const char* fileName, OpenMode flags, bool isLimited)
{
    AZStd::string fullFilename;
    NormalizePath(fileName, m_data->m_fileMountPoint.c_str(), fullFilename);

    Device* device = FindDevice(fullFilename.c_str());
    if (device)
    {
        VirtualStream* stream = nullptr;
        AZStd::semaphore wait;
        auto streamResultFunc = [&stream](Request* request, AZ::u64, void*, Request::StateType)
        {
            stream = request->m_stream;
        };
        Request* request = new Request(static_cast<SizeType>(0U), static_cast<SizeType>(0), nullptr, streamResultFunc, false, Request::PriorityType::DR_PRIORITY_NORMAL, Request::OperationType::OT_READ, nullptr);
        device->AddCommand(&DeviceRequest::RegisterFileStreamRequest, request, fileName, flags, isLimited, &wait);
        wait.acquire();
        return stream;
    }
    return nullptr;
}

//=========================================================================
// UnRegisterFileStream
// [10/2/2009]
//=========================================================================
void
Streamer::UnRegisterFileStream(const char* fileName)
{
    AZStd::string fullFilename;
    NormalizePath(fileName, m_data->m_fileMountPoint.c_str(), fullFilename);

    Device* device = FindDevice(fullFilename.c_str());
    if (device)
    {
        AZStd::semaphore wait;
       
        Request* request = new Request(static_cast<SizeType>(0U), static_cast<SizeType>(0), nullptr, nullptr, false, Request::PriorityType::DR_PRIORITY_NORMAL, Request::OperationType::OT_READ, nullptr);
        device->AddCommand(&DeviceRequest::UnRegisterFileStreamRequest, request, fileName, &wait);
        wait.acquire();
    }
}


//=========================================================================
// RegisterStream
// [8/29/2012]
//=========================================================================
bool
Streamer::RegisterStream(VirtualStream* stream, OpenMode flags, bool isLimited)
{
    AZ_Assert(stream, "Invalid stream");

    Device* device = FindDevice(stream->GetFilename());
    if (device)
    {
        bool registered = false;
        AZStd::semaphore wait;
        auto streamResultFunc = [&registered](Request*, AZ::u64, void*, Request::StateType)
        {
            registered = true;
        };
        Request* request = new Request(static_cast<SizeType>(0U), static_cast<SizeType>(0), nullptr, streamResultFunc, false, Request::PriorityType::DR_PRIORITY_NORMAL, Request::OperationType::OT_READ, nullptr);
        device->AddCommand(&DeviceRequest::RegisterStreamRequest, request, stream, flags, isLimited, &wait);
        wait.acquire();
        return registered;
    }
    return false;
}

//=========================================================================
// UnRegisterStream
// [8/29/2012]
//=========================================================================
void
Streamer::UnRegisterStream(VirtualStream* stream)
{
    if (stream)
    {
        Device* device = FindDevice(stream->GetFilename());
        if (device)
        {
            AZStd::semaphore wait;

            Request* request = new Request(static_cast<SizeType>(0U), static_cast<SizeType>(0), nullptr, nullptr, false, Request::PriorityType::DR_PRIORITY_NORMAL, Request::OperationType::OT_READ, nullptr);
            device->AddCommand(&DeviceRequest::UnRegisterStreamRequest, request, stream, &wait);
            wait.acquire();
        }
    }
}

//=========================================================================
// Read
// [10/5/2009]
//=========================================================================
Streamer::SizeType
Streamer::Read(VirtualStream* stream, SizeType byteOffset, SizeType byteSize, void* dataBuffer, AZStd::chrono::microseconds deadline, Request::PriorityType priority, Request::StateType* state, const char* debugName)
{
    AZ_Assert(stream != NULL, "You can't pass a NULL stream!");

    SizeType byteOffsetStartAfterCache  = byteOffset;
    SizeType byteOffsetEndAfterCache    = byteOffset + byteSize;

    Device* device = GetStreamer()->FindDevice(stream->GetFilename());
    if (device->GetReadCache() && device->GetReadCache()->IsInCacheUserThread(stream, byteOffsetStartAfterCache, byteOffsetEndAfterCache, dataBuffer))
    {
        if (state)
        {
            *state = Request::StateType::ST_COMPLETED;
        }
        return byteSize;
    }
    else
    {
        SyncRequestCallback doneCB; // we will need to wait on a load

        Request* request = aznew Request(byteOffset, byteSize, dataBuffer, doneCB, false, priority, Request::OperationType::OT_READ, debugName);
        request->m_bytesProcessedStart += byteOffsetStartAfterCache - byteOffset;
        request->m_bytesProcessedEnd += byteOffset + byteSize - byteOffsetEndAfterCache;
        request->SetDeadline(deadline);

        device->AddCommand(&DeviceRequest::ReadRequest, request, stream, stream->GetFilename(), nullptr);

        doneCB.Wait();
        if (state)
        {
            *state = doneCB.m_state;
        }
        return doneCB.m_bytesTrasfered;
    }
}

//=========================================================================
// ReadAsync
// [10/5/2009]
//=========================================================================
RequestHandle
Streamer::ReadAsync(VirtualStream* stream, SizeType byteOffset, SizeType byteSize, void* dataBuffer, const Request::RequestDoneCB& callback, AZStd::chrono::microseconds deadline, Request::PriorityType priority, const char* debugName, bool deferCallback)
{
    AZ_Assert(stream != NULL, "You can't pass a NULL stream!");
 
    Request* request = InvalidRequestHandle;
    Device* device = GetStreamer()->FindDevice(stream->GetFilename());
    if (device)
    {
        SizeType byteOffsetStartAfterCache = byteOffset;
        SizeType byteOffsetEndAfterCache = byteOffset + byteSize;
        
        request = aznew Request(byteOffset, byteSize, dataBuffer, callback, deferCallback, priority, Request::OperationType::OT_READ, debugName);
        request->m_bytesProcessedStart += byteOffsetStartAfterCache - byteOffset;
        request->m_bytesProcessedEnd += byteOffset + byteSize - byteOffsetEndAfterCache;
        request->SetDeadline(deadline);

        device->AddCommand(&DeviceRequest::ReadRequest, request, stream, stream->GetFilename(), nullptr);
    }
    return request;
}

//=========================================================================
// ReadAsyncDeferred
// [10/5/2009]
//=========================================================================
RequestHandle
Streamer::ReadAsyncDeferred(VirtualStream* stream, SizeType byteOffset, SizeType byteSize, void* dataBuffer, const Request::RequestDoneCB& callback, AZStd::chrono::microseconds deadline, Request::PriorityType priority, const char* debugName)
{
    return ReadAsync(stream, byteOffset, byteSize, dataBuffer, callback, deadline, priority, debugName, true);
}


//=========================================================================
// Read
// [10/5/2009]
//=========================================================================
Streamer::SizeType
Streamer::Read(const char* fileName, SizeType byteOffset, SizeType& byteSize, void* dataBuffer, AZStd::chrono::microseconds deadline, Request::PriorityType priority, Request::StateType* state, const char* debugName)
{
    AZ_Assert(fileName != NULL, "You can't pass a NULL filename!");
    SyncRequestCallback doneCB;
    if (ReadAsync(fileName, byteOffset, byteSize, dataBuffer, doneCB, deadline, priority, debugName) != InvalidRequestHandle)
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
        *state = Request::StateType::ST_ERROR_FAILED_TO_OPEN_STREAM;
    }

    return 0;
}

//=========================================================================
// ReadAsync
// [10/5/2009]
//=========================================================================
RequestHandle
Streamer::ReadAsync(const char* fileName, SizeType byteOffset, SizeType byteSize, void* dataBuffer, const Request::RequestDoneCB& callback, AZStd::chrono::microseconds deadline, Request::PriorityType priority, const char* debugName, bool deferCallback)
{
    AZ_Assert(fileName != NULL, "You can't pass a NULL filename!");
    AZStd::string fullpath;
    NormalizePath(fileName, m_data->m_fileMountPoint.c_str(), fullpath);
    Device* device = FindDevice(fullpath.c_str());
    if (device)
    {
        Request* request = aznew Request(byteOffset, byteSize, dataBuffer, callback, deferCallback, priority, Request::OperationType::OT_READ, debugName);
        request->SetDeadline(deadline);

        device->AddCommand(&DeviceRequest::ReadRequest, request, nullptr, fileName, nullptr);
        return request;
    }
    return InvalidRequestHandle;
}

//=========================================================================
// ReadAsyncDeferred
// [10/5/2009]
//=========================================================================
RequestHandle
Streamer::ReadAsyncDeferred(const char* fileName, SizeType byteOffset, SizeType byteSize, void* dataBuffer, const Request::RequestDoneCB& callback,  AZStd::chrono::microseconds deadline, Request::PriorityType priority, const char* debugName)
{
    return ReadAsync(fileName, byteOffset, byteSize, dataBuffer, callback, deadline, priority, debugName, true);
}


//=========================================================================
// Write
// [10/5/2009]
//=========================================================================
Streamer::SizeType
Streamer::Write(VirtualStream* stream, const void* dataBuffer, SizeType byteSize, Request::PriorityType priority, Request::StateType* state, SizeType byteOffset, const char* debugName)
{
    AZ_Assert(stream != NULL, "You can't pass a NULL stream!");
    SyncRequestCallback doneCB;
    if (WriteAsync(stream, dataBuffer, byteSize, doneCB, priority, byteOffset, debugName) != InvalidRequestHandle)
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
        *state = Request::StateType::ST_ERROR_FAILED_IN_OPERATION;
    }
    return 0;
}

//=========================================================================
// WriteAsync
// [10/5/2009]
//=========================================================================
RequestHandle
Streamer::WriteAsync(VirtualStream* stream, const void* dataBuffer, SizeType byteSize, const Request::RequestDoneCB& callback, Request::PriorityType priority, SizeType byteOffset, const char* debugName, bool deferCallback)
{
    AZ_Assert(stream != NULL, "You can't pass a NULL stream!");
    // const-casting the buffer pointer because we know we will not modify the buffer during write operations.
    // buffer still needs to persist throughout the async operation though and the caller needs to know whether he can
    // or cannot modify the buffer when the buffer is passed back to him in the completion callback.
    Request* request = InvalidRequestHandle;
    Device* device = GetStreamer()->FindDevice(stream->GetFilename());
    if (device)
    {
        request = aznew Request(byteOffset, byteSize, const_cast<void*>(dataBuffer), callback, deferCallback, priority, Request::OperationType::OT_WRITE, debugName);
        device->AddCommand(&DeviceRequest::WriteRequest, request, stream, stream->GetFilename(), nullptr);
    }

    return request;
}

//=========================================================================
// WriteAsyncDeferred
// [10/5/2009]
//=========================================================================
RequestHandle
Streamer::WriteAsyncDeferred(VirtualStream* stream, const void* dataBuffer, SizeType byteSize, const Request::RequestDoneCB& callback, Request::PriorityType priority, SizeType byteOffset, const char* debugName)
{
    return WriteAsync(stream, dataBuffer, byteSize, callback, priority, byteOffset, debugName, true);
}

//=========================================================================
// WriteAsync
// [10/5/2009]
//=========================================================================
Streamer::SizeType
Streamer::Write(const char* fileName, const void* dataBuffer, SizeType byteSize, Request::PriorityType priority, Request::StateType* state, SizeType byteOffset, const char* debugName)
{
    AZ_Assert(fileName != NULL, "You can't pass a NULL file name!");
    SyncRequestCallback doneCB;
    if (WriteAsync(fileName, dataBuffer, byteSize, doneCB, priority, byteOffset, debugName) != InvalidRequestHandle)
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
        *state = Request::StateType::ST_ERROR_FAILED_IN_OPERATION;
    }

    return 0;
}

//=========================================================================
// WriteAsync
// [10/5/2009]
//=========================================================================
RequestHandle
Streamer::WriteAsync(const char* fileName, const void* dataBuffer, SizeType byteSize, const Request::RequestDoneCB& callback, Request::PriorityType priority, SizeType byteOffset, const char* debugName, bool deferCallback)
{
    AZ_Assert(fileName != NULL, "You can't pass a NULL file name!");
    AZStd::string fullpath;
    NormalizePath(fileName, m_data->m_fileMountPoint.c_str(), fullpath);
    Device* device = FindDevice(fullpath.c_str());
    if (device)
    {
        // const-casting the buffer pointer because we know we will not modify the buffer during write operations.
        // buffer still needs to persist throughout the async operation though and the caller needs to know whether he can
        // or cannot modify the buffer when the buffer is passed back to him in the completion callback.
        Request* request = aznew Request(byteOffset, byteSize, const_cast<void*>(dataBuffer), callback, deferCallback, priority, Request::OperationType::OT_WRITE, debugName);

        device->AddCommand(&DeviceRequest::WriteRequest, request, nullptr, fileName, nullptr);

        return request;
    }
    return InvalidRequestHandle;
}

//=========================================================================
// WriteAsyncDeferred
// [10/5/2009]
//=========================================================================
RequestHandle
Streamer::WriteAsyncDeferred(const char* fileName, const void* dataBuffer, SizeType byteSize, const Request::RequestDoneCB& callback, Request::PriorityType priority, SizeType byteOffset, const char* debugName)
{
    return WriteAsync(fileName, dataBuffer, byteSize, callback, priority, byteOffset, debugName, true);
}

//=========================================================================
// CancelRequest
// [10/5/2009]
//=========================================================================
void
Streamer::CancelRequest(RequestHandle request)
{
    AZ_Assert(request != InvalidRequestHandle, "You can't pass a InvalidRequestHandle!");
    AZStd::semaphore wait;
    CancelRequestAsync(request, &wait); 
    wait.acquire();
}

//=========================================================================
// CancelRequestAsync
// [11/6/2012]
//=========================================================================
void
Streamer::CancelRequestAsync(RequestHandle request, AZStd::semaphore* sync)
{
    AZ_Assert(request != InvalidRequestHandle, "You can't pass a InvalidRequestHandle!");
    Device* device = GetStreamer()->FindDevice(request->m_stream->GetFilename());
    if (device)
    {
        device->AddCommand(&DeviceRequest::CancelRequest, request, sync);
    }
}

//=========================================================================
// RescheduleRequest
// [11/8/2011]
//=========================================================================
void
Streamer::RescheduleRequest(RequestHandle request, Request::PriorityType priority, AZStd::chrono::microseconds deadline)
{
    AZ_Assert(request != InvalidRequestHandle, "You can't pass a InvalidRequestHandle!");
    EBUS_DBG_EVENT(StreamerDrillerBus, OnRescheduleRequest, request, AZStd::chrono::system_clock::now() + deadline, priority);
    request->m_priority = priority;
    if (request->m_operation == Request::OperationType::OT_READ)
    {
        request->SetDeadline(deadline);
    }
}

//=========================================================================
// EstimateCompletion
// [11/8/2011]
//=========================================================================
AZStd::chrono::microseconds
Streamer::EstimateCompletion(RequestHandle request)
{
    AZ_Assert(request != InvalidRequestHandle, "You can't pass a InvalidRequestHandle!");
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

#endif // #ifndef AZ_UNITY_BUILD
