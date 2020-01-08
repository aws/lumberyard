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

#include <AzCore/PlatformIncl.h>
#include <AzCore/IO/Device.h>

#include <direct.h>
#include <winioctl.h>

namespace 
{
    bool ExtractRootPathAndDeviceIdentifier(char* rootPath, size_t rootPathSize, char* deviceIdentifier, size_t deviceIdentifierSize, const char* fullPath)
    {
        rootPath[0] = 0;
        deviceIdentifier[0] = 0;

        if (strncmp(fullPath, "\\\\", 2) == 0 || strncmp(fullPath, "//", 2) == 0)
        {
            int slashes = 4;
            for (const char* c = fullPath; *c; ++c)
            {
                if (*c == '\\' || *c == '/')
                {
                    if (--slashes == 0)
                    {
                        azstrncpy(rootPath, rootPathSize, fullPath, size_t(c - fullPath + 1));
                        break;
                    }
                }
            }
        }
        else
        {
            for (const char* c = fullPath; *c; ++c)
            {
                if (*c == ':')
                {
                    if (c[1] == '\\' || c[1] == '/')
                    {
                        azstrncpy(rootPath, rootPathSize, fullPath, size_t(c - fullPath + 2));
                        break;
                    }
                }
            }
        }

        if (rootPath[0])
        {
            deviceIdentifier[0] = '\\';
            deviceIdentifier[1] = '\\';
            deviceIdentifier[2] = '.';
            deviceIdentifier[3] = '\\';
            const char* c = rootPath;
            size_t index = 4;
            while (*c && index < deviceIdentifierSize - 1)
            {
                if (*c == ':' && c[1] == '\\' || c[1] == '/')
                {
                    deviceIdentifier[index++] = ':';
                    break;
                }
                deviceIdentifier[index++] = *c++;
            }
            deviceIdentifier[index] = 0;
        };

        return rootPath[0] && deviceIdentifier[0];
    }
}

namespace AZ
{
    namespace IO
    {
        namespace Platform
        {
            bool GetDeviceSpecsFromPath(DeviceSpecifications& specs, const char* fullpath)
            {
                // extract root
                char root[256] = { 0 };
                char device[256] = { 0 };

                // find device specs based on root
                if (ExtractRootPathAndDeviceIdentifier(root, AZ_ARRAY_SIZE(root), device, AZ_ARRAY_SIZE(device), fullpath))
                {
                    // This assumes that the path identifiers have already been checked for the given path
                    // and it was not yet found.
                    specs.m_pathIdentifiers.emplace_back(root);

                    unsigned int driveType = GetDriveTypeA(root);
                    if (driveType == DRIVE_CDROM)
                    {
                        AZ_Error("IO", false, "Optical devices are not supported. The device will be treated as if it's a fixed device."
                            "This will negatively impact performance. Consider first installing on a fixed device.");
                        specs.m_deviceType = DeviceSpecifications::DeviceType::Fixed;

                        char dummy[256];
                        DWORD sn;
                        GetVolumeInformationA(root, dummy, 256, &sn, 0, 0, dummy, 256);
                        specs.m_deviceName = AZStd::string::format("%u", sn);
                    }
                    else if (driveType == DRIVE_FIXED)
                    {
                        specs.m_deviceType = DeviceSpecifications::DeviceType::Fixed;

                        char dummy[256];
                        DWORD sn;
                        GetVolumeInformationA(root, dummy, 256, &sn, 0, 0, dummy, 256);
                        specs.m_deviceName = AZStd::string::format("%u", sn);
                    }
                    else
                    {
                        specs.m_deviceType = DeviceSpecifications::DeviceType::Network;
                        specs.m_deviceName = "default";
                    }

                    size_t minCacheSize = 0;
                    size_t maxCacheSize = 0;
                    DWORD flags = 0;
                    if (GetSystemFileCacheSize(&minCacheSize, &maxCacheSize, &flags))
                    {
                        specs.m_recommendedCacheSize = minCacheSize;
                    }

                    // Note that the DeviceIoControl calls are send to the vendor specific driver for the drive and might fail if not implemented.
                    // In this case, just fall back to the default values. Mapped network drives are also not supported, so in this case, also 
                    // fall back to default values.
                    HANDLE deviceHandle = CreateFile(device, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
                    if (deviceHandle != INVALID_HANDLE_VALUE)
                    {
                        // Get sector size.
                        DISK_GEOMETRY diskGeometry;
                        DWORD bytesReturned = 0;
                        if (DeviceIoControl(deviceHandle, IOCTL_DISK_GET_DRIVE_GEOMETRY, nullptr, 0,
                            &diskGeometry, sizeof(diskGeometry), &bytesReturned, nullptr))
                        {
                            specs.m_sectorSize = diskGeometry.BytesPerSector;
                        }

                        // Get seek penalty information.
                        DEVICE_SEEK_PENALTY_DESCRIPTOR seekPenaltyDescriptor;
                        STORAGE_PROPERTY_QUERY query;
                        query.QueryType = PropertyStandardQuery;
                        query.PropertyId = StorageDeviceSeekPenaltyProperty;
                        if (DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
                            &seekPenaltyDescriptor, sizeof(seekPenaltyDescriptor), &bytesReturned, nullptr))
                        {
                            specs.m_hasSeekPenalty = seekPenaltyDescriptor.IncursSeekPenalty ? true : false;
                        }

                        CloseHandle(deviceHandle);
                    }

                    return true;
                }
                return false;
            }
        }
    }
}
