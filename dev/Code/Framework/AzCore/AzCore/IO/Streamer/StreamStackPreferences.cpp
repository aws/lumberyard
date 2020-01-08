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

#include <AzCore/IO/Streamer/BlockCache.h>
#include <AzCore/IO/Streamer/DedicatedCache.h>
#include <AzCore/IO/Streamer/FullFileDecompressor.h>
#include <AzCore/IO/Streamer/ReadSplitter.h>
#include <AzCore/IO/Streamer/StorageDrive.h>
#include <AzCore/IO/Streamer/StreamStackPreferences.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ
{
    namespace IO
    {
        static u32 GetCacheSize(StreamStack::BlockCacheConfiguration configuration)
        {
            switch (configuration)
            {
            case StreamStack::BlockCacheConfiguration::Disabled:
                return 0;
            case StreamStack::BlockCacheConfiguration::SmallBlocks:
                return 4 * 1024;
            case StreamStack::BlockCacheConfiguration::Balanced:
                return 64 * 1024;
            case StreamStack::BlockCacheConfiguration::LargeBlocks:
                return 1024 * 1024;
            default:
                return 0;
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
                fileHandleCacheSize = 1024;
                break;
            default:
                AZ_Assert(false, "Unsupported FileHandleCache type %i.", preferences.m_fileHandleCache);
            }
            return AZStd::make_shared<StorageDrive>(fileHandleCacheSize);
        }

        static AZStd::shared_ptr<StreamStackEntry> MountReadSplitter(const StreamStack::Preferences& preferences, const AZStd::shared_ptr<StreamStackEntry>& next)
        {
            u64 splitSize = 0;
            switch (preferences.m_readSplitter)
            {
            case StreamStack::ReadSplitterConfiguration::Disabled:
                return next;
            case StreamStack::ReadSplitterConfiguration::BlockCacheSize:
                splitSize = GetCacheSize(preferences.m_blockCache.m_configuration);
                if (splitSize == 0)
                {
                    return next;
                }
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
            u32 cacheBlockSize = GetCacheSize(preferences.m_blockCache.m_configuration);
            if (cacheBlockSize != 0)
            {
                auto blockCache = AZStd::make_shared<BlockCache>(
                    preferences.m_blockCache.m_blockSizeMB * 1024 * 1024, cacheBlockSize, preferences.m_blockCache.m_onlyEpilogWrites);
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
            u32 cacheBlockSize = GetCacheSize(preferences.m_dedicatedCache.m_configuration);
            if (cacheBlockSize != 0)
            {
                auto dedicatedCache = AZStd::make_shared<DedicatedCache>(
                    preferences.m_dedicatedCache.m_blockSizeMB * 1024 * 1024, cacheBlockSize, preferences.m_dedicatedCache.m_onlyEpilogWrites);
                dedicatedCache->SetNext(next);
                return dedicatedCache;
            }
            else
            {
                return next;
            }
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
            AZStd::shared_ptr<StreamStackEntry> stack = MountDrives(preferences);
            stack = MountReadSplitter(preferences, stack);
            stack = MountVirtualFileSystem(preferences, stack);
            stack = MountBlockCache(preferences, stack);
            stack = MountDedicatedCache(preferences, stack);
            stack = MountFullFileDecompressor(preferences, stack);
            return stack;
        }
    } // namespace IO
} // namespace AZ