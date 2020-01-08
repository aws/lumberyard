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

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    namespace IO
    {
        class StreamStackEntry;

        namespace StreamStack
        {
            //! Determines whether to prefer general implementations that work on all platforms or to
            //! use platform specific features if available. Platform specific implementations will use
            //! file operation native to the host OS, but may require additional restrictions such as
            //! specific cache sizes. Platform agnostic implementations use implementations that work
            //! across all platforms, but might not provide peak performance.
            enum class PlatformIntegration
            {
                Agnostic, //!< Prefer implementations that work on all platforms.
                Specific //!< Prefer implementations that are optimized for a specific platform.
            };

            //! Controls the number file handles that will be cached. A small cache will cause
            //! files to be frequently re-opened reduces the memory and OS resources that need
            //! to allocated to managing file handles. A large cache will generally perform
            //! better, but at the cost of more resources.
            //! Note that when running exclusively from archives far few file handles are needed
            //! then when loose files are included.
            enum class FileHandleCache
            {
                Small, //!< Only a few handles will be cached. This generally performs slower but uses less resources.
                Balanced, //!< A small amount of file handles are cached, enough to cover the archive needs of an average game.
                Large, //!< A large number of file handles are cached. Particularly useful if there are a lot of loose files that need to be loaded.
            };

            //! Controls how the block cache is configured. Using many small blocks can help increase
            //! hit rate, but also makes the cache provide less data. Using large blocks will have less
            //! cache hits, but hits are more effective. If data is accessed in mostly random order,
            //! small blocks can provide better performance, but if data is mostly linearly addressed
            //! large blocks will be more effective.
            //! Note that even if the cache is disabled, a small cache may still be added in order to 
            //! satisfy alignment requirements.
            enum class BlockCacheConfiguration
            {
                Disabled, //!< No cache or a very small cache is created.
                SmallBlocks, //!< A large number of tiny blocks are used.
                Balanced, //!< A number and size for blocks is chosen that works reasonably well in most scenarios.
                LargeBlocks, //!< A small number of blocks of a large size are used.
            };

            struct CacheConfiguration
            {
                AZ_TYPE_INFO(CacheConfiguration, "{D85E236D-C782-4C60-9DA5-B2AB2EE28BC5}");

                CacheConfiguration() = default;
                CacheConfiguration(BlockCacheConfiguration configuration, u64 blockSizeMB, bool onlyEpilogWrites)
                    : m_configuration(configuration)
                    , m_blockSizeMB(blockSizeMB)
                    , m_onlyEpilogWrites(onlyEpilogWrites)
                {}

                BlockCacheConfiguration m_configuration = BlockCacheConfiguration::Balanced;
                u64 m_blockSizeMB = 1; //!< The size in megabytes used by the block cache. Some additional overhead for bookkeeping will be added.
                bool m_onlyEpilogWrites = true; //!< If true both prolog and epilog can read from the cache but only the epilog can write, otherwise both can write.
            };

            //! Reads can be split up in smaller bits before being send to the OS. Depending on the platform and available hardware this results in a better
            //! balance between throughput and responsiveness to canceling or priority changes.
            enum class ReadSplitterConfiguration
            {
                Disabled, //!< No splitter is added and all requests are issued as one.
                BlockCacheSize, //!< Use the same read size the block cache will be using.
                BalancedSize //!< Picks a platform/hardware specific size that has a good balance between responsiveness and read size.
            };

            struct Preferences
            {
                AZ_TYPE_INFO(Preferences, "{8F265F08-EA03-4F3F-8533-CF03ADC7884D}");

                PlatformIntegration m_platformIntegration{ PlatformIntegration::Specific };
                FileHandleCache m_fileHandleCache{ FileHandleCache::Balanced };
                CacheConfiguration m_blockCache{ BlockCacheConfiguration::Balanced, 5, false };
                CacheConfiguration m_dedicatedCache{ BlockCacheConfiguration::LargeBlocks, 1, true };
                ReadSplitterConfiguration m_readSplitter{ ReadSplitterConfiguration::BalancedSize };
                u32 m_fullDecompressionJobThreads{ 2 };
                u32 m_fullDecompressionReads{ 2 };
            };

            extern AZStd::shared_ptr<StreamStackEntry> CreateDefaultStreamStack(const Preferences& preferences);
        }
    } // namespace IO
} // namespace AZ