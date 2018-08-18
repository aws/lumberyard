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

#include <AzCore/Asset/AssetInternal/LegacyBlockingAssetTypeManager.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/LegacyAssetHandler.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Debug/Trace.h>

namespace AZ
{
    namespace Data
    {
        namespace AssetInternal
        {
            //=========================================================================
            // RegisterAssetHandler - registers an asset type and handler that
            // block on a specific (usually the main) thread
            //=========================================================================
            void LegacyBlockingAssetTypeManager::RegisterAssetHandler(const AssetType& assetType, LegacyAssetHandler* handler, AZStd::thread::id threadToBlockOn)
            {
                bool threadRegisteredAlready = false;
                unsigned int blockingJobIndex = 0;

                // m_perThreadInfo doesn't shrink and items indexed in it will always be
                // available, but new items are not pushed onto the vector atomically,
                // so we get the size using a lock in GetNumberOfBlockingThreads() and then
                // are safe to iterate over the vector to search.
                unsigned int blockingThreadCount = GetNumberOfBlockingThreads();
                for (unsigned int i = 0; i < blockingThreadCount; i++)
                {
                    BlockingThreadInfo& threadInfo = m_perThreadInfo[i];
                    if (threadInfo.threadId == threadToBlockOn)
                    {
                        blockingJobIndex = i;
                        threadRegisteredAlready = true;
                    }
                }

                if (!threadRegisteredAlready)
                {
                    // Use the lock to ensure that we're pushing into the per thread info vector atomically.
                    // Locking here is the magic that allows us to only block getting the size of the perThread
                    // collection everywhere else in this code.
                    BlockingThreadInfo newThreadInfo(threadToBlockOn);
                    blockingJobIndex = m_perThreadInfo.push_back(newThreadInfo);
                }

                // find this asset type in the per asset type info
                {
                    // Ensure we're locked before modifying the per asset type info
                    AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_perAssetTypeInfoMutex);

                    bool infoRegisteredAlready = false;
                    for (unsigned int i = 0; i < m_perAssetTypeInfo.size(); i++)
                    {
                        BlockingAssetTypeInfo& assetTypeInfo = m_perAssetTypeInfo[i];
                        if (assetTypeInfo.assetType == assetType)
                        {
                            if (assetTypeInfo.handler)
                            {
                                AZStd::string assetTypeString = assetType.ToString<AZStd::string>();
                                AZ_Assert(!assetTypeInfo.handler, "Only a single AssetHandler can be registered for an asset type at a time! Ensure that the previous AssetHandler registered for %s has already been Unregistered!", assetTypeString.c_str());
                            }

                            assetTypeInfo.handler = handler;
                            assetTypeInfo.blockingJobCountIndex = blockingJobIndex;
                            assetTypeInfo.threadId = threadToBlockOn;
                            infoRegisteredAlready = true;
                            break;
                        }
                    }

                    if (!infoRegisteredAlready)
                    {
                        m_perAssetTypeInfo.push_back({ threadToBlockOn, assetType, handler, blockingJobIndex });
                    }
                }
            }

            //=========================================================================
            // UnregisterAssetHandler
            //=========================================================================
            void LegacyBlockingAssetTypeManager::UnregisterAssetHandler(AssetHandler* handler)
            {
                // This assumes that the AssetManager has already asserted that this AssetHandler doesn't
                // currently have any outstanding jobs.

                AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_perAssetTypeInfoMutex);
                for (unsigned int i = 0; i < m_perAssetTypeInfo.size(); i++)
                {
                    BlockingAssetTypeInfo& assetTypeInfo = m_perAssetTypeInfo[i];
                    AssetHandler* storedHandler = assetTypeInfo.handler; // explicitly cast, so that we don't have to trust the compiler to do the right thing with the comparison of different types.
                    if (storedHandler == handler)
                    {
                        // just clear the handler; we never shrink the list or remove items, so that
                        // indexes are always valid
                        assetTypeInfo.handler = nullptr;
                    }
                }
            }

            //=========================================================================
            // AssetLoadStarted - signals that an asset is starting to load. Wakes up
            // any threads currently blocking for a particular asset to finish (so that
            // any handlers that require ProcessQueuedAssetRequests can be called, if
            // needed)
            //=========================================================================
            void LegacyBlockingAssetTypeManager::AssetLoadStarted(const AssetType& assetType, const AssetId& assetId)
            {
                AZ_UNUSED(assetId);

                bool signalCondition = false;
                unsigned int blockingJobCountIndex = 0;

                {
                    // block to figure out if we need to signal, but make sure the mutex isn't locked when we signal the condition variable
                    AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_perAssetTypeInfoMutex);

                    for (unsigned int i = 0; i < m_perAssetTypeInfo.size(); i++)
                    {
                        BlockingAssetTypeInfo& assetTypeInfo = m_perAssetTypeInfo[i];
                        if (assetTypeInfo.assetType == assetType)
                        {
                            blockingJobCountIndex = assetTypeInfo.blockingJobCountIndex;
                            signalCondition = true;
                            break;
                        }
                    }
                }

                if (signalCondition)
                {
                    // atomically increment the blocking loading asset counter; safe to do without a lock because we aren't inserting anything new into the job count array
                    BlockingThreadInfo& threadInfo = m_perThreadInfo[blockingJobCountIndex];
                    ++threadInfo.assetCount;

                    // wake up anything listening
                    m_condition.notify_all();
                }
            }

            //=========================================================================
            // AssetLoadFinished - signals that an asset is finished loading. Wakes up
            // any threads currently blocking for a particular asset to finish (so that
            // any handlers that require ProcessQueuedAssetRequests can be called, if
            // needed)
            //=========================================================================
            void LegacyBlockingAssetTypeManager::AssetLoadFinished(const AssetType& assetType, const AssetId& assetId)
            {
                AZ_UNUSED(assetId);

                bool signalCondition = false;
                unsigned int blockingJobCountIndex = 0;

                {
                    // block to figure out if we need to signal, but make sure the mutex isn't locked when we signal the condition variable
                    AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_perAssetTypeInfoMutex);

                    for (unsigned int i = 0; i < m_perAssetTypeInfo.size(); i++)
                    {
                        BlockingAssetTypeInfo& assetTypeInfo = m_perAssetTypeInfo[i];
                        if (assetTypeInfo.assetType == assetType)
                        {
                            blockingJobCountIndex = assetTypeInfo.blockingJobCountIndex;
                            signalCondition = true;
                            break;
                        }
                    }
                }

                if (signalCondition)
                {
                    // atomically decrement the blocking asset load counter; safe to do without a lock because we aren't inserting anything new into the job count array
                    BlockingThreadInfo& threadInfo = m_perThreadInfo[blockingJobCountIndex];
                    --threadInfo.assetCount;

                    // wake up anything listening
                    m_condition.notify_all();
                }
            }

            //=========================================================================
            // HasBlockingHandlersForCurrentThread
            //=========================================================================
            bool LegacyBlockingAssetTypeManager::HasBlockingHandlersForCurrentThread() const
            {
                AZStd::thread::id threadId = AZStd::this_thread::get_id();

                // don't have to block here because we get the current job size atomically in GetNumberOfBlockingThreads()
                // and are guaranteed that the vector will have at least that many elements
                unsigned int blockingThreadCount = GetNumberOfBlockingThreads();
                for (unsigned int i = 0; i < blockingThreadCount; i++)
                {
                    const BlockingThreadInfo& threadInfo = m_perThreadInfo[i];
                    if (threadInfo.threadId == threadId)
                    {
                        return true;
                    }
                }

                return false;
            }

            //=========================================================================
            // BlockingAssetFinished - signals that an asset currently blocking in
            // BlockOnAsset is now finished loading
            //=========================================================================
            void LegacyBlockingAssetTypeManager::BlockingAssetFinished(Asset<AssetData>& asset)
            {
                AZ_UNUSED(asset);

                m_condition.notify_all();
            }

            //=========================================================================
            // BlockOnAsset - blocks until the input asset is finished loading.
            // Processes queued, blocking asset loads on this thread (via
            // ProcessQueuedAssetRequests) if there are any outstanding loads on this
            // thread requiring it - which stops any weird deadlocks caused by this
            // thread waiting for itself to process previously queued asset loads.
            //=========================================================================
            void LegacyBlockingAssetTypeManager::BlockOnAsset(Asset<AssetData>& asset)
            {
                AZStd::thread::id threadId = AZStd::this_thread::get_id();

                bool threadFound = false;
                unsigned int threadIndex = 0;

                // don't have to block here because we get the current job size atomically in GetNumberOfBlockingThreads()
                // and are guaranteed that the vector will have at least that many elements
                unsigned int blockingThreadCount = GetNumberOfBlockingThreads();
                for (unsigned int i = 0; i < blockingThreadCount; i++)
                {
                    BlockingThreadInfo& threadInfo = m_perThreadInfo[i];
                    if (threadInfo.threadId == threadId)
                    {
                        threadFound = true;
                        threadIndex = i;
                        break;
                    }
                }

                AZ_Assert(threadFound, "Blocking on an asset but didn't find a matching job counter for this thread!");
                if (!threadFound)
                {
                    AZStd::unique_lock<AZStd::mutex> lock(m_conditionMutex);

                    // THIS CASE SHOULD NEVER HAPPEN. If it does, just block until the condition_variable signals
                    m_condition.wait(lock, [asset] {
                        bool finishedWaiting = !asset.IsLoading();
                        return finishedWaiting;
                    });

                    return;
                }

                AZStd::atomic_uint& loadingAssetCounter = m_perThreadInfo[threadIndex].assetCount;
                while (asset.IsLoading())
                {
                    // process any outstanding, queued assets on this thread already
                    while (loadingAssetCounter.load() > 0)
                    {
                        // need to block when dealing with handlers because they could get
                        // registered/unregistered while we're processing their queues
                        AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_perAssetTypeInfoMutex);
                        for (unsigned int i = 0; i < m_perAssetTypeInfo.size(); i++)
                        {
                            BlockingAssetTypeInfo& assetTypeInfo = m_perAssetTypeInfo[i];
                            LegacyAssetHandler* handler = assetTypeInfo.handler;
                            if ((assetTypeInfo.threadId == threadId) && handler)
                            {
                                handler->ProcessQueuedAssetRequests();
                            }
                        }
                    }


                    // wait until the asset is loaded or until we have more work on this thread specifically
                    AZStd::unique_lock<AZStd::mutex> lock(m_conditionMutex);
                    m_condition.wait(lock, [asset, &loadingAssetCounter] {
                        bool finishedWaiting = (loadingAssetCounter > 0) || !asset.IsLoading();
                        return finishedWaiting;
                    });
                }
            }

            //=========================================================================
            // GetNumberOfBlockingThreads - thread safe way to retrieve the number of
            // blocking threads. After this call, any i < GetNumberOfBlockingThreads()
            // in m_perThreadInfo is guaranteed to be valid and readable, with the
            // assetCount variable able to be incremented and decremented atomically
            //=========================================================================
            unsigned int LegacyBlockingAssetTypeManager::GetNumberOfBlockingThreads() const
            {
                AZStd::unique_lock<AZStd::mutex> lock(m_perThreadSizeMutex);

                return m_perThreadInfo.size();
            }
        }
    }   // namespace Data
}   // namespace AZ