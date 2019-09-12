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
#include <AzCore/IO/Streamer/BlockCache.h>
#include <AzCore/IO/Streamer/DedicatedCache.h>
#include <AzCore/IO/Streamer/FullFileDecompressor.h>
#include <AzCore/IO/Streamer/ReadSplitter.h>
#include <AzCore/IO/Streamer/StorageDrive.h>
#include <AzCore/IO/Streamer/StreamStackPreferences.h>
#include <AzCore/IO/Streamer/StorageDrive_Windows.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <winioctl.h>

namespace AZ
{
    namespace IO
    {
        struct DriveInformation
        {
            const char* m_path;
            bool m_hasSeekPenalty;
        };

        static AZStd::shared_ptr<StreamStackEntry> MountNativeDrives(u32 fileHandleCacheSize)
        {
            // Always add a default "catch all" device for any device that currently doesn't have an optimized path.
            AZStd::shared_ptr<StreamStackEntry> result = AZStd::make_shared<StorageDrive>(fileHandleCacheSize);

            char drives[512];
            if (GetLogicalDriveStrings(sizeof(drives) - 1, drives))
            {
                AZStd::unordered_multimap<DWORD, DriveInformation> driveMappings;

                char* driveIt = drives;
                do
                {
                    UINT driveType = GetDriveTypeA(driveIt);
                    // Only a selective set of devices that share similar behavior are supported, in particular
                    // drives that have magnetic or solid state storage. The all types of buses (usb, sata, etc)
                    // are supported except network drives. If network support is needed it's better to use the
                    // virtual file system. Optical drives are not supported as they are currently not in use
                    // on any platform for games, as it's expected games to be downloaded or installed to storage.
                    if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE || driveType == DRIVE_RAMDISK)
                    {
                        AZStd::string deviceName = "\\\\.\\";
                        deviceName += driveIt;
                        deviceName.erase(deviceName.length() - 1); // Erase the slash.
                        
                        HANDLE deviceHandle = CreateFile(deviceName.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
                        if (deviceHandle != INVALID_HANDLE_VALUE)
                        {
                            DriveInformation driveInformation;
                            driveInformation.m_path = driveIt;

                            DWORD bytesReturned = 0;
                            
                            // Get seek penalty information.
                            DEVICE_SEEK_PENALTY_DESCRIPTOR seekPenaltyDescriptor;
                            STORAGE_PROPERTY_QUERY query;
                            query.QueryType = PropertyStandardQuery;
                            query.PropertyId = StorageDeviceSeekPenaltyProperty;
                            if (DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
                                &seekPenaltyDescriptor, sizeof(seekPenaltyDescriptor), &bytesReturned, nullptr))
                            {
                                driveInformation.m_hasSeekPenalty = seekPenaltyDescriptor.IncursSeekPenalty ? true : false;
                            }

                            // Get the device number so multiple partitions and mappings are handled by the same drive.
                            STORAGE_DEVICE_NUMBER storageDeviceNumber;
                            if (DeviceIoControl(deviceHandle, IOCTL_STORAGE_GET_DEVICE_NUMBER, nullptr, 0,
                                &storageDeviceNumber, sizeof(storageDeviceNumber), &bytesReturned, nullptr))
                            {
                                driveMappings.insert(AZStd::make_pair(storageDeviceNumber.DeviceNumber, AZStd::move(driveInformation)));
                            }
                            else
                            {
                                AZ_Printf("Streamer", 
                                    "Skipping drive '%s' because device is not registered with OS as a storage device, generic drive support will be used.\n", driveIt);
                            }

                            CloseHandle(deviceHandle);
                        }
                        else
                        {
                            AZ_Warning("Streamer", false, 
                                "Skipping drive '%s' because device information can't be retrieved, generic drive support will be used.\n", driveIt);
                        }
                    }
                    else
                    {
                        AZ_Printf("Streamer", "Skipping drive '%s', generic drive support will be used.\n", driveIt);
                    }

                    // Move to next drive string. GetLogicalDriveStrings fills the target buffer with null-terminated strings, for instance
                    // "E:\\0F:\\0G:\\0\0". The code below reads forward till the next \0 and the following while will continue iterating until
                    // the entire buffer has been used.
                    while (*driveIt++);
                } while (*driveIt);

                auto end = driveMappings.end();
                auto mapIt = driveMappings.begin();
                AZStd::vector<const char*> drivePaths;
                DWORD driveId = mapIt->first;
                bool hasSeekPenalty = hasSeekPenalty = mapIt->second.m_hasSeekPenalty;
                
                while (mapIt != end)
                {
                    if (mapIt->first != driveId)
                    {
                        AZ_Assert(!drivePaths.empty(), "Expected at least one drive path.");
                        AZStd::shared_ptr<StreamStackEntry> drive = 
                            AZStd::make_shared<StorageDriveWin>(drivePaths, hasSeekPenalty, fileHandleCacheSize);
                        drive->SetNext(result);
                        result = drive;
                        
                        driveId = mapIt->first;
                        drivePaths.clear();
                    }

                    hasSeekPenalty = mapIt->second.m_hasSeekPenalty; // This should be the same for all entries for this drive.
                    drivePaths.push_back(mapIt->second.m_path);
                    ++mapIt;
                }
                
                AZ_Assert(!drivePaths.empty(), "Expected at least one drive path.");
                AZStd::shared_ptr<StreamStackEntry> drive = AZStd::make_shared<StorageDriveWin>(drivePaths, hasSeekPenalty, fileHandleCacheSize);
                drive->SetNext(result);
                result = drive;
            }
            return result;
        }

        static u32 GetCacheSize(StreamStack::BlockCacheConfiguration configuration, bool allowDisabling)
        {
            switch (configuration)
            {
            case StreamStack::BlockCacheConfiguration::Disabled:
                // The windows version will be using async IO which requires reads to start at aligned
                // addresses so this cache will also guarantee that the alignment requirements are met
                // and can therefore not be turned off.
                return allowDisabling ? 0 : 4 * 1024;
            case StreamStack::BlockCacheConfiguration::SmallBlocks:
                return 4 * 1024;
            case StreamStack::BlockCacheConfiguration::Balanced:
                return 64 * 1024;
            case StreamStack::BlockCacheConfiguration::LargeBlocks:
                return 1024 * 1024;
            default:
                return allowDisabling ? 0 : 4 * 1024;
            }
        }

        static AZStd::shared_ptr<StreamStackEntry> MountDrives(const StreamStack::Preferences& preferences)
        {
            u32 fileHandleCacheSize;
            switch (preferences.m_fileHandleCache)
            {
            case StreamStack::FileHandleCache::Small:
                fileHandleCacheSize = 1;
                break;
            case StreamStack::FileHandleCache::Balanced:
                fileHandleCacheSize = 32;
                break;
            case StreamStack::FileHandleCache::Large:
                fileHandleCacheSize = 4096;
                break;
            default:
                AZ_Assert(false, "Unsupported FileHandleCache type %i.", preferences.m_fileHandleCache);
                return nullptr;
            }

            AZStd::shared_ptr<StreamStackEntry> stack;
            switch (preferences.m_platformIntegration)
            {
            case StreamStack::PlatformIntegration::Agnostic:
                stack = AZStd::make_shared<StorageDrive>(fileHandleCacheSize);
                break;
            case StreamStack::PlatformIntegration::Specific:
                stack = MountNativeDrives(fileHandleCacheSize);
                break;
            default:
                AZ_Assert(false, "Unsupported PlatformIntegration type %i.", preferences.m_platformIntegration);
                return nullptr;
            }
            return stack;
        }

        static AZStd::shared_ptr<StreamStackEntry> MountReadSplitter(const StreamStack::Preferences& preferences, const AZStd::shared_ptr<StreamStackEntry>& next)
        {
            u64 splitSize = 0;
            switch (preferences.m_readSplitter)
            {
            case StreamStack::ReadSplitterConfiguration::Disabled:
                return next;
            case StreamStack::ReadSplitterConfiguration::BlockCacheSize:
                splitSize = GetCacheSize(preferences.m_blockCache.m_configuration, preferences.m_platformIntegration != StreamStack::PlatformIntegration::Specific);
                break;
            case StreamStack::ReadSplitterConfiguration::BalancedSize:
                // fall through
            default:
                splitSize = 1 * 1024 * 1024;
                break;
            }

            auto readSplitter = AZStd::make_shared<ReadSplitter>(splitSize);
            readSplitter->SetNext(next);
            return readSplitter;
        }

        static AZStd::shared_ptr<StreamStackEntry> MountVirtualFileSystem(const StreamStack::Preferences& /*preferences*/, const AZStd::shared_ptr<StreamStackEntry>& next)
        {
#if !defined(_RELEASE)
            // The Virtual File System can't be loaded from inside AzCore so insert a placeholder
            // that can optionally be replaced later when the engine has established a connection
            // with the Asset Processor.
            auto vfs = AZStd::make_shared<StreamStackEntry>("Virtual File System");
            vfs->SetNext(next);
            return vfs;
#else
            return next;
#endif
        }

        static AZStd::shared_ptr<StreamStackEntry> MountBlockCache(const StreamStack::Preferences& preferences, const AZStd::shared_ptr<StreamStackEntry>& next)
        {
            u32 cacheBlockSize = GetCacheSize(preferences.m_blockCache.m_configuration, preferences.m_platformIntegration != StreamStack::PlatformIntegration::Specific);
            if (cacheBlockSize != 0)
            {
                u64 memorySize = preferences.m_blockCache.m_blockSizeMB;
                memorySize = memorySize > 0 ? memorySize * 1024 * 1024 : 1 * 1024 * 1024;

                auto blockCache = AZStd::make_shared<BlockCache>(memorySize, cacheBlockSize, false);
                blockCache->SetNext(next);
                return blockCache;
            }
            else
            {
                return next;
            }
        }

        static AZStd::shared_ptr<StreamStackEntry> MountDedicatedCache(const StreamStack::Preferences& preferences, const AZStd::shared_ptr<StreamStackEntry>& next)
        {
            u32 cacheBlockSize = GetCacheSize(preferences.m_dedicatedCache.m_configuration, true);
            if (cacheBlockSize != 0 && preferences.m_dedicatedCache.m_blockSizeMB > 0)
            {
                auto dedicatedCache = AZStd::make_shared<DedicatedCache>(
                    preferences.m_dedicatedCache.m_blockSizeMB * 1024 * 1024, cacheBlockSize, preferences.m_dedicatedCache.m_onlyEpilogWrites);
                dedicatedCache->SetNext(next);
                return dedicatedCache;
            }

            return next;
        }

        static AZStd::shared_ptr<StreamStackEntry> MountFullFileDecompressor(const StreamStack::Preferences& preferences, const AZStd::shared_ptr<StreamStackEntry>& next)
        {
            auto decompressor = AZStd::make_shared<FullFileDecompressor>(preferences.m_fullDecompressionReads, preferences.m_fullDecompressionJobThreads);
            decompressor->SetNext(next);
            return decompressor;
        }

        AZStd::shared_ptr<StreamStackEntry> StreamStack::CreateDefaultStreamStack(const Preferences& preferences)
        {
            // Stack is build bottom up.
            AZStd::shared_ptr<StreamStackEntry> stack;
            stack = MountDrives(preferences);
            stack = MountReadSplitter(preferences, stack);
            stack = MountVirtualFileSystem(preferences, stack);
            stack = MountBlockCache(preferences, stack);
            stack = MountDedicatedCache(preferences, stack);
            stack = MountFullFileDecompressor(preferences, stack);
            return stack;
        }
    } // namespace IO
} // namespace AZ