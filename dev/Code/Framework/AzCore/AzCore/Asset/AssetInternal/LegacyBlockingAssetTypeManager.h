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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/std/parallel/containers/concurrent_vector.h>

namespace AZ
{
    namespace Data
    {
        class LegacyAssetHandler;
        class AssetHandler;
        class AssetManager;

        namespace AssetInternal
        {
            /**
             * Class to handle two different cases which sound the same:
             *
             * Case 1 - threads that block waiting on a particular asset to finish
             *   (via AssetManager::Instance().GetAsset(..., loadBlocking==true);
             * Case 2 - assets that only get processed on a specific (usually the main) thread
             *   and therefore block their loading jobs until the specific thread
             *   runs and clears them
             *
             * Case 2 doesn't generally cause any problems... unless one of those jobs
             * is running and someone triggers a Case 1. In that case, GetAsset()
             * will sit and spin waiting for it's load to complete, but it never will
             * because it's blocking the very thread that would clear up any jobs
             * blocking on that thread.
             *
             * LegacyBlockingAssetTypeManager solves that problem by allowing AssetHandlers
             * to run ProcessQueuedAssetRequests when it detects both cases in play.
             * 
             * It uses concurrent_vector so that memory is never freed accidentally
             * when mucking with our lists.
             **/
            class LegacyBlockingAssetTypeManager
            {
            public:
                AZ_CLASS_ALLOCATOR(LegacyBlockingAssetTypeManager, SystemAllocator, 0);

                /// Registers a blocking asset handler for a particular asset type, with the thread that does the actual work
                void RegisterAssetHandler(const AssetType& assetType, LegacyAssetHandler* handler, AZStd::thread::id threadToBlockOn);

                /// Unregisters all references to the handler parameter. Note that the asset type will still be marked as blocking, even though the handler's been removed
                void UnregisterAssetHandler(AssetHandler* handler);

                /// Marks that a blocking job has started
                void AssetLoadStarted(const AssetType& assetType, const AssetId& assetId);

                /// Marks that a blocking job has finished
                void AssetLoadFinished(const AssetType& assetType, const AssetId& assetId);

                /// Returns whether or not the currently running thread has any blocking asset handlers
                bool HasBlockingHandlersForCurrentThread() const;

                /**
                 * Block until the asset is loading. Efficiently waits without blocking other threads.
                 * Assets sent to this method must also be sent to BlockingAssetFinished, once the
                 * asset is done loading.
                 */
                void BlockOnAsset(Asset<AssetData>& asset);

                /**
                 * Marks a blocking asset as finished loading. Must be called only on assets that were previously
                 * (in any thread) sent to BlockOnAsset
                 */
                void BlockingAssetFinished(Asset<AssetData>& asset);

            private:
                friend class AZ::Data::AssetManager;

                // only the AssetManager can create this class
                LegacyBlockingAssetTypeManager() = default;

                /**
                 * Locks the mutex for the per thread info, but once this method is done,
                 * per thread info can be read in a thread-safe manner.
                 */
                unsigned int GetNumberOfBlockingThreads() const;

                struct BlockingAssetTypeInfo
                {
                    AZStd::thread::id threadId; // thread that does the loading work
                    AssetType assetType;
                    LegacyAssetHandler* handler; // can be nullptr;
                    unsigned int blockingJobCountIndex; // index for this thread within m_perThreadInfo
                };

                struct BlockingThreadInfo
                {
                    AZStd::thread::id threadId;
                    AZStd::atomic_uint assetCount;

                    BlockingThreadInfo(AZStd::thread::id tId) : threadId(tId), assetCount(0) {}

                    // the copy constructor must be defined so that we can push_back into m_perThreadInfo.
                    // It should only ever get called from AZStd::concurrent_vector<BlockingThreadInfo>::push_back.
                    // NOTE that it's intentional that the asset count is zero when this happens.
                    BlockingThreadInfo(const BlockingThreadInfo& other) : threadId(other.threadId), assetCount(0) {}


                    // the = operator must be defined so that we can push_back into m_perThreadInfo.
                    // It should only ever get called from AZStd::concurrent_vector<BlockingThreadInfo>::push_back.
                    // NOTE that it's intentional that the asset count is zero when this happens.
                    BlockingThreadInfo& operator=(const BlockingThreadInfo& v)
                    {
                        threadId = v.threadId;
                        assetCount.store(0);
                        return *this;
                    }
                };

                AZStd::mutex                                    m_conditionMutex; // Needed for use with the condition; doesn't actually lock anything.
                AZStd::condition_variable                       m_condition; // Condition to signal when an asset is finished loading or there's a blocking job being processed

                AZStd::recursive_mutex                          m_perAssetTypeInfoMutex; // Mutex to protect access to the per asset type info
                AZStd::concurrent_vector<BlockingAssetTypeInfo> m_perAssetTypeInfo; // never shrinking list of blocking asset type info

                mutable AZStd::mutex                            m_perThreadSizeMutex; // Mutex to ensure items are added to the per thread info atomically
                AZStd::concurrent_vector<BlockingThreadInfo>    m_perThreadInfo; // info per blocking thread; can be read from without being thread-safe if the size is retrieved when locked with the mutex above this; never shrinks
            };
        }
    }   // namespace Data
}   // namespace AZ