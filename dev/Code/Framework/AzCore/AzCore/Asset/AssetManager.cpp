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

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetInternal/LegacyBlockingAssetTypeManager.h>
#include <AzCore/Asset/LegacyAssetHandler.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/IO/Streamer.h>
#include <AzCore/IO/GenericStreams.h>

namespace AZ
{
    namespace Data
    {
        static const char* kAssetDBInstanceVarName = "AssetDatabaseInstace";

        /*
         * This is the base class for Async AssetDatabase jobs
         */
        class AssetDatabaseAsyncJob
            : public AssetDatabaseJob
            , public Job
        {
        public:
            AssetDatabaseAsyncJob(JobContext* jobContext, bool deleteWhenDone, AssetManager* owner, const Asset<AssetData>& asset, AssetHandler* assetHandler)
                : AssetDatabaseJob(owner, asset, assetHandler)
                , Job(deleteWhenDone, jobContext)
            {
            }

            ~AssetDatabaseAsyncJob()
            {
            }
        };

        /**
         * Internally allows asset jobs to be notified by their dependent jobs.
         */
        class AssetJobEvents
            : public EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            typedef AssetId                 BusIdType;
            typedef AZStd::recursive_mutex  MutexType;

            template <class Bus>
            struct AssetJobConnectionPolicy
                : public EBusConnectionPolicy<Bus>
            {
                static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, const typename Bus::BusIdType& id = 0)
                {
                    typename Bus::BusIdType actualId = AssetInternal::ResolveAssetId(id);
                    EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, actualId);

                    // If the asset is loaded or failed already, deliver the status update immediately
                    // Note that we check IsReady here, ReadyPreNotify must be tested because there is
                    // a small gap between ReadyPreNotify and Ready where the callback could be missed
                    Asset<AssetData> assetData(AssetInternal::GetAssetData(actualId));
                    if (assetData)
                    {
                        if (assetData.Get()->IsReady())
                        {
                            handler->OnAssetReady(assetData);
                        }
                        else if (assetData.Get()->IsError())
                        {
                            handler->OnAssetError(assetData);
                        }
                    }
                }
            };

            template <typename Bus>
            using ConnectionPolicy = AssetJobConnectionPolicy<Bus>;

            virtual void OnAssetReady(const Asset<AssetData>& /*asset*/) {}
            virtual void OnAssetError(const Asset<AssetData>& /*asset*/) {}
        };

        typedef EBus<AssetJobEvents> AssetJobBus;

        /*
         * This class processes async AssetDatabase load jobs
         */
        class LoadAssetJob
            : public AssetDatabaseAsyncJob
        {
        public:
            AZ_CLASS_ALLOCATOR(LoadAssetJob, ThreadPoolAllocator, 0);

            LoadAssetJob(JobContext* jobContext, AssetManager* owner, const Asset<AssetData>& asset, AssetHandler* assetHandler, const AssetFilterCB& assetLoadFilterCB)
                : AssetDatabaseAsyncJob(jobContext, false, owner, asset, assetHandler)
                , m_assetLoadFilterCB(assetLoadFilterCB)
            {
            }

            ~LoadAssetJob() override
            {
            }

            void Process() override
            {
                m_owner->RegisterAssetLoading(m_asset);
                
                const bool loadSucceeded = LoadData();

                // Queue the result for dispatch to main thread.
                m_assetHandler->InitAsset(m_asset, loadSucceeded, false);

                // Notify any dependent jobs.
                if (loadSucceeded)
                {
                    EBUS_EVENT_ID(m_asset.GetId(), AssetJobBus, OnAssetReady, m_asset);
                }
                else
                {
                    EBUS_EVENT_ID(m_asset.GetId(), AssetJobBus, OnAssetError, m_asset);
                }

                m_owner->UnregisterAssetLoading(m_asset);

                delete this;
            }

            bool LoadData()
            {
                AssetStreamInfo loadInfo = m_owner->GetLoadStreamInfoForAsset(m_asset.GetId(), m_asset.GetType());
                if (loadInfo.IsValid())
                {
                    if (loadInfo.m_isCustomStreamType)
                    {
                        return m_assetHandler->LoadAssetData(m_asset, loadInfo.m_streamName.c_str(), m_assetLoadFilterCB);
                    }
                    else
                    {
                        AZ_Assert(IO::Streamer::IsReady(), "We only support streamer IO at the moment. Make sure the streamer has been started!");
                        IO::StreamerStream stream(loadInfo.m_streamName.c_str(), loadInfo.m_streamFlags, loadInfo.m_dataOffset, loadInfo.m_dataLen);
                        return  m_assetHandler->LoadAssetData(m_asset, &stream, m_assetLoadFilterCB);
                    }
                }

                return false;
            }

            AssetFilterCB                   m_assetLoadFilterCB;            
        };

        /**
         * Base class to handle blocking on an asset load. Takes care connecting to the AssetJobBus
         * and clean up of the object.
         */
        class WaitForAsset
            : public AssetJobBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(WaitForAsset, ThreadPoolAllocator, 0);

            WaitForAsset(const Asset<AssetData>& assetToWaitFor)
                : m_assetData(assetToWaitFor)
            {
            }

            ~WaitForAsset() override
            {
            }

            void OnAssetReady(const Asset<AssetData>& /*asset*/) override
            {
                Finish();
            }

            void OnAssetError(const Asset<AssetData>& /*asset*/) override
            {
                Finish();
            }

            void WaitAndDestroy()
            {
                BusConnect(m_assetData.GetId());

                Wait();

                delete this;
            }

        protected:
            virtual void Finish() = 0;
            virtual void Wait() = 0;

            Asset<AssetData> m_assetData;
        };

        /**
         * Utility class to wait when a blocking load is requested for an asset that's already
         * loading asynchronously. Uses a semaphore to signal completion.
         */
        class WaitForAssetOnThreadWithNoBlockingJobs
            : public WaitForAsset
        {
        public:
            AZ_CLASS_ALLOCATOR(WaitForAssetOnThreadWithNoBlockingJobs, ThreadPoolAllocator, 0);

            WaitForAssetOnThreadWithNoBlockingJobs(const Asset<AssetData>& assetToWaitFor)
                : WaitForAsset(assetToWaitFor)
            {
            }

        protected:
            void Wait() override
            {
                m_waitEvent.acquire();
            }

            void Finish()
            {
                m_waitEvent.release();
            }

        private:
            AZStd::binary_semaphore m_waitEvent;
        };

        /**
        * Utility class to wait when a blocking load is requested for an asset that's already
        * loading asynchronously, but on a thread that processes blocking asset loads and requires
        * pumping occasionally to drain queued loads.
        */
        class WaitForAssetOnThreadWithBlockingJobs
            : public WaitForAsset
        {
        public:
            AZ_CLASS_ALLOCATOR(WaitForAssetOnThreadWithBlockingJobs, ThreadPoolAllocator, 0);

            WaitForAssetOnThreadWithBlockingJobs(const Asset<AssetData>& assetToWaitFor, AssetInternal::LegacyBlockingAssetTypeManager* blockingAssetTypeManager)
                : WaitForAsset(assetToWaitFor)
                , m_blockingAssetTypeManager(blockingAssetTypeManager)
            {
                AZ_Assert(m_blockingAssetTypeManager, "LegacyBlockingAssetTypeManager can not be null!");
            }

        protected:
            void Wait()
            {
                m_blockingAssetTypeManager->BlockOnAsset(m_assetData);
            }

            void Finish()
            {
                m_blockingAssetTypeManager->BlockingAssetFinished(m_assetData);
            }

        private:
            AssetInternal::LegacyBlockingAssetTypeManager* m_blockingAssetTypeManager = nullptr;
        };

        class ReloadAssetJob
            : public LoadAssetJob
        {
        public:

            ReloadAssetJob(JobContext* jobContext, AssetManager* owner, const Asset<AssetData>& asset, AssetHandler* assetHandler)
                : LoadAssetJob(jobContext, owner, asset, assetHandler, nullptr)
            {
            }

            void Process() override
            {
                m_owner->RegisterAssetLoading(m_asset);

                const bool loadSucceeded = LoadData();

                m_assetHandler->InitAsset(m_asset, loadSucceeded, true);

                m_owner->UnregisterAssetLoading(m_asset);

                delete this;
            }
        };

        /*
         * This class processes async AssetDatabase save jobs
         */
        class SaveAssetJob
            : public AssetDatabaseAsyncJob
        {
        public:
            AZ_CLASS_ALLOCATOR(SaveAssetJob, ThreadPoolAllocator, 0);

            SaveAssetJob(JobContext* jobContext, AssetManager* owner, const Asset<AssetData>& asset, AssetHandler* assetHandler)
                : AssetDatabaseAsyncJob(jobContext, true, owner, asset, assetHandler)
            {
            }

            ~SaveAssetJob() override
            {
            }

            void Process() override
            {
                SaveAsset();
            }

            void SaveAsset()
            {
                bool isSaved = false;
                AssetStreamInfo saveInfo = m_owner->GetSaveStreamInfoForAsset(m_asset.GetId(), m_asset.GetType());
                if (saveInfo.IsValid())
                {
                    AZ_Assert(IO::Streamer::IsReady(), "We only support streamer IO at the moment. Make sure the streamer has been started!");
                    {
                        IO::StreamerStream stream(saveInfo.m_streamName.c_str(), saveInfo.m_streamFlags, saveInfo.m_dataOffset);
                        isSaved = m_assetHandler->SaveAssetData(m_asset, &stream);
                    }
                }
                // queue broadcast message for delivery on game thread
                EBUS_QUEUE_EVENT_ID(m_asset.GetId(), AssetBus, OnAssetSaved, m_asset, isSaved);
            }
        };

        //////////////////////////////////////////////////////////////////////////
        // Globals
        EnvironmentVariable<AssetManager*> AssetManager::s_assetDB = nullptr;
        //////////////////////////////////////////////////////////////////////////

        //=========================================================================
        // AssetDatabaseJob
        // [4/3/2014]
        //=========================================================================
        AssetDatabaseJob::AssetDatabaseJob(AssetManager* owner, const Asset<AssetData>& asset, AssetHandler* assetHandler)
        {
            m_owner = owner;
            m_asset = asset;
            m_assetHandler = assetHandler;
            owner->AddJob(this);
        }

        //=========================================================================
        // ~AssetDatabaseJob
        // [4/3/2014]
        //=========================================================================
        AssetDatabaseJob::~AssetDatabaseJob()
        {
            m_owner->RemoveJob(this);
        }

        //=========================================================================
        // Create
        // [6/12/2012]
        //=========================================================================
        bool AssetManager::Create(const Descriptor& desc)
        {
            AZ_Assert(!s_assetDB || !s_assetDB.Get(), "AssetManager already created!");

            if (!s_assetDB)
            {
                s_assetDB = Environment::CreateVariable<AssetManager*>(kAssetDBInstanceVarName);
            }
            if (!s_assetDB.Get())
            {
                s_assetDB.Set(aznew AssetManager(desc));
            }

            return true;
        }

        //=========================================================================
        // Destroy
        // [6/12/2012]
        //=========================================================================
        void AssetManager::Destroy()
        {
            AZ_Assert(s_assetDB, "AssetManager not created!");
            delete (*s_assetDB);
            *s_assetDB = nullptr;
        }

        //=========================================================================
        // IsReady
        //=========================================================================
        bool AssetManager::IsReady()
        {
            if (!s_assetDB)
            {
                s_assetDB = Environment::FindVariable<AssetManager*>(kAssetDBInstanceVarName);
            }

            return s_assetDB && *s_assetDB;
        }

        //=========================================================================
        // Instance
        //=========================================================================
        AssetManager& AssetManager::Instance()
        {
            if (!s_assetDB)
            {
                s_assetDB = Environment::FindVariable<AssetManager*>(kAssetDBInstanceVarName);
            }

            AZ_Assert(s_assetDB, "AssetManager not created!");
            return *(*s_assetDB);
        }

        //=========================================================================
        // AssetDatabase
        // [6/12/2012]
        //=========================================================================
        AssetManager::AssetManager(const AssetManager::Descriptor& desc)
            : m_blockingAssetTypeManager(aznew AssetInternal::LegacyBlockingAssetTypeManager())
        {
            (void)desc;

            m_firstThreadCPU = 0;

            JobManagerDesc jobDesc;
#if !(defined AZCORE_JOBS_IMPL_SYNCHRONOUS)
            m_numberOfWorkerThreads = AZ::GetMin(desc.m_maxWorkerThreads, AZStd::thread::hardware_concurrency());

            JobManagerThreadDesc threadDesc(AFFINITY_MASK_USERTHREADS);
            for (unsigned int i = 0; i < m_numberOfWorkerThreads; ++i)
            {
                jobDesc.m_workerThreads.push_back(threadDesc);
            }
#else
            m_numberOfWorkerThreads = 0;
#endif

            m_jobManager = aznew JobManager(jobDesc);
            m_jobContext = aznew JobContext(*m_jobManager);

            AssetManagerBus::Handler::BusConnect();
        }

        //=========================================================================
        // ~AssetManager
        // [6/12/2012]
        //=========================================================================
        AssetManager::~AssetManager()
        {
            delete m_jobContext;
            delete m_jobManager;

            DispatchEvents();

            // Acquire the asset lock to make sure nobody else is trying to do anything fancy with assets
            AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_assetMutex);

            while (!m_activeJobs.empty())
            {
                delete &*m_activeJobs.begin();
            }
            while (!m_handlers.empty())
            {
                AssetHandlerMap::iterator it = m_handlers.begin();
                AssetHandler* handler = it->second;
                UnregisterHandler(handler);
                delete handler;
            }
            AssetManagerBus::Handler::BusDisconnect();

            delete m_blockingAssetTypeManager;
        }

        //=========================================================================
        // DispatchEvents
        // [04/02/2014]
        //=========================================================================
        void AssetManager::DispatchEvents()
        {
            AssetBus::ExecuteQueuedEvents();

            EBUS_EVENT(AssetManagerNotificationBus, OnAssetEventsDispatched);
        }

        //=========================================================================
        void AssetManager::SetAssetInfoUpgradingEnabled(bool enable)
        {
            m_assetInfoUpgradingEnabled = enable;
        }

        bool AssetManager::GetAssetInfoUpgradingEnabled() const
        {
#if defined(_RELEASE)
            // in release ("FINAL") builds, we never do this.
            return false;
#else
            return m_assetInfoUpgradingEnabled;
#endif
        }

        //=========================================================================
        // RegisterHandler
        // [7/9/2014]
        //=========================================================================
        void AssetManager::RegisterHandler(AssetHandler* handler, const AssetType& assetType)
        {
            AZ_Error("AssetDatabase", handler != nullptr, "Attempting to register a null asset handler!");
            if (handler)
            {
                if (m_handlers.insert(AZStd::make_pair(assetType, handler)).second)
                {
                    handler->m_nHandledTypes++;
                }
                else
                {
                    AZ_Error("AssetDatabase", false, "Asset type %s already has a handler registered! New registration ignored!", assetType.ToString<AZStd::string>().c_str());
                }
            }
        }

        void AssetManager::RegisterLegacyHandler(LegacyAssetHandler* handler, const AssetType& assetType, AZStd::thread::id threadThatLoadsAssets)
        {
            RegisterHandler(handler, assetType);

            m_blockingAssetTypeManager->RegisterAssetHandler(assetType, handler, threadThatLoadsAssets);
        }

        //=========================================================================
        // UnregisterHandler
        // [7/9/2014]
        //=========================================================================
        void AssetManager::UnregisterHandler(AssetHandler* handler)
        {
            AZ_Error("AssetDatabase", handler != nullptr, "Attempting to unregister a null asset handler!");
            if (handler)
            {
                AZ_Error("AssetDatabase", handler->m_nActiveAssets == 0, "Asset handler '%s' is being removed but there are still %d active assets being handled by it!", handler->RTTI_GetTypeName(), (int)handler->m_nActiveAssets);
                for (AssetHandlerMap::iterator it = m_handlers.begin(); it != m_handlers.end(); /*++it*/)
                {
                    if (it->second == handler)
                    {
                        it = m_handlers.erase(it);
                        handler->m_nHandledTypes--;
                    }
                    else
                    {
                        ++it;
                    }
                }
                AZ_Error("AssetDatabase", handler->m_nHandledTypes == 0, "%s->m_nHandledTypes has incorrect count %d!", handler->RTTI_GetTypeName(), (int)handler->m_nHandledTypes);
            }

            // clear the single threaded map too
            m_blockingAssetTypeManager->UnregisterAssetHandler(handler);
        }

        //=========================================================================
        // RegisterCatalog
        // [8/27/2012]
        //=========================================================================
        void AssetManager::RegisterCatalog(AssetCatalog* catalog, const AssetType& assetType)
        {
            AZ_Error("AssetDatabase", catalog != nullptr, "Attempting to register a null catalog!");
            if (catalog)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> l(m_catalogMutex);
                if (m_catalogs.insert(AZStd::make_pair(assetType, catalog)).second == false)
                {
                    AZ_Error("AssetDatabase", false, "Asset type %s already has a catalog registered! New registration ignored!", assetType.ToString<AZStd::string>().c_str());
                }
            }
        }

        //=========================================================================
        // UnregisterCatalog
        // [8/27/2012]
        //=========================================================================
        void AssetManager::UnregisterCatalog(AssetCatalog* catalog)
        {
            AZ_Error("AssetDatabase", catalog != nullptr, "Attempting to unregister a null catalog!");
            if (catalog)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> l(m_catalogMutex);
                for (AssetCatalogMap::iterator iter = m_catalogs.begin(); iter != m_catalogs.end(); )
                {
                    if (iter->second == catalog)
                    {
                        iter = m_catalogs.erase(iter);
                    }
                    else
                    {
                        ++iter;
                    }
                }
            }
        }

        //=========================================================================
        // GetHandledAssetTypes
        // [6/27/2016]
        //=========================================================================
        void AssetManager::GetHandledAssetTypes(AssetCatalog* catalog, AZStd::vector<AssetType>& assetTypes)
        {
            for (AssetCatalogMap::iterator iter = m_catalogs.begin(); iter != m_catalogs.end(); iter++)
            {
                if (iter->second == catalog)
                {
                    assetTypes.push_back(iter->first);
                }
            }
        }

        //=========================================================================
        // FindAsset
        //=========================================================================
        Asset<AssetData> AssetManager::FindAsset(const AssetId& assetId)
        {
            // Look up the asset id in the catalog, and use the result of that instead.
            // If assetId is a legacy id, assetInfo.m_assetId will be the canonical id. Otherwise, assetInfo.m_assetID == assetId.
            // This is because only canonical ids are stored in m_assets (see below).
            // Only do the look up if upgrading is enabled
            AZ::Data::AssetInfo assetInfo;
            if (GetAssetInfoUpgradingEnabled())
            {
                AssetCatalogRequestBus::BroadcastResult(assetInfo, &AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
            }

            // If the catalog is not available, use the original assetId
            const AssetId& assetToFind(assetInfo.m_assetId.IsValid() ? assetInfo.m_assetId : assetId);

            AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_assetMutex);
            AssetMap::iterator it = m_assets.find(assetToFind);
            if (it != m_assets.end())
            {
                return it->second;
            }
            return Asset<AssetData>();
        }

        //=========================================================================
        // GetAsset
        // [6/19/2012]
        //=========================================================================
        Asset<AssetData> AssetManager::GetAsset(const AssetId& assetId, const AssetType& assetType, bool queueLoadData, const AssetFilterCB& assetLoadFilterCB, bool loadBlocking, bool isCreate /* = false */)
        {
            AZ_Error("AssetDatabase", assetId.IsValid(), "GetAsset called with invalid asset Id.");
            AZ_Error("AssetDatabase", !assetType.IsNull(), "GetAsset called with invalid asset type.");

            LoadAssetJob* loadJob = nullptr;
            WaitForAsset* blockingWait = nullptr;
            AssetData* assetData = nullptr;
            Asset<AssetData> asset; // Used to hold a reference while job is dispatched and while outside of the assetMutex lock.
            AZ::Data::AssetInfo assetInfo;
            {
                // Attempt to look up asset info from catalog
                // This is so that when assetId is a legacy id, we're operating on the canonical id anyway
                // Only do the lookup if we know this isn't a new asset
                if (!isCreate)
                {
                    EBUS_EVENT_RESULT(assetInfo, AssetCatalogRequestBus, GetAssetInfoById, assetId);
                }

                // If the asset was found in the catalog, ensure the type infos match
                if (assetInfo.m_assetId.IsValid())
                {
                    AZ_Warning("AssetDatabase", assetInfo.m_assetType == assetType, "Requested asset id %s with type %s, but type is actually %s.",
                        assetId.ToString<AZStd::string>().c_str(), assetType.ToString<AZStd::string>().c_str(), assetInfo.m_assetType.ToString<AZStd::string>().c_str());
                }
                else
                {
                    // If asset not found, use the id and type given. This will likely only happen when calling Asset::Create().
                    assetInfo.m_assetId = assetId;
                    assetInfo.m_assetType = assetType;
                }

                bool isNewEntry = false;
                AssetHandler* handler = nullptr;

                AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_assetMutex);
                {
                    // check if asset already exists
                    {
                        
                        AssetMap::iterator it = m_assets.find(assetInfo.m_assetId);
                        if (it != m_assets.end())
                        {
                            assetData = it->second;
                            asset = assetData;
                        }
                        else
                        {
                            isNewEntry = true;
                        }
                    }

                    {

                    // find the asset type handler
                    AssetHandlerMap::iterator handlerIt = m_handlers.find(assetInfo.m_assetType);
                    AZ_Error("AssetDatabase", handlerIt != m_handlers.end(), "No handler was registered for this asset [type:%s id:%s]!",
                        assetInfo.m_assetType.ToString<AZ::OSString>().c_str(), assetInfo.m_assetId.ToString<AZ::OSString>().c_str());
                    if (handlerIt != m_handlers.end())
                    {
                        // Create the asset ptr and insert it into our asset map.
                        handler = handlerIt->second;
                        if (isNewEntry)
                        {
                            assetData = handler->CreateAsset(assetInfo.m_assetId, assetInfo.m_assetType);
                            if (assetData)
                            {
                                assetData->m_assetId = assetInfo.m_assetId;
                                ++handler->m_nActiveAssets;
                                assetData->m_creationToken = ++m_creationTokenGenerator;
                                asset = assetData;
                            }
                            else
                            {
                                AZ_Error("AssetDatabase", false, "Failed to create asset with (id=%s, type=%s)",
                                    assetInfo.m_assetId.ToString<AZ::OSString>().c_str(), assetInfo.m_assetType.ToString<AZ::OSString>().c_str());
                            }
                        }
                    }
                    }

                    if (assetData)
                    {
                        if (isNewEntry && assetData->IsRegisterReadonlyAndShareable())
                        {
                            m_assets.insert(AZStd::make_pair(assetInfo.m_assetId, assetData));
                        }
                        if (queueLoadData && assetData->GetStatus() == AssetData::AssetStatus::NotLoaded)
                        {
                            // start the data loading
                            assetData->m_status = static_cast<int>(AssetData::AssetStatus::Loading);
                            loadJob = aznew LoadAssetJob(m_jobContext, this, assetData, handler, assetLoadFilterCB);
                        }
                        else if (loadBlocking && assetData->GetStatus() == AssetData::AssetStatus::Loading)
                        {
                            // The danger here is that we are in a blocking load for a cyclic asset reference.
                            // interestingly enough, its actually "okay" to just return without blocking, because the blocking guarantee
                            // will still ultimately hold as the original blocking load will complete higher up the chain.
                            auto foundthread = m_assetsLoadingByThread.find(assetId);
                            AZStd::thread::id threadId = AZStd::this_thread::get_id();

                            if ((foundthread != m_assetsLoadingByThread.end()) && (foundthread->second == threadId))
                            {
                                // this thread is already loading this asset.  Its very likely that 
                                // the asset we are loading refers to its own asset.
                                // cyclic dependencies are generally data ERRORS because they mean something will certianly not work.
                                AZ_Error("AssetManager", false,
                                    "Trying to load %s blocking but is already loading by the same thread also blocking.\n"
                                    "This means an asset has a cyclic dependency in it!",
                                    asset.ToString<AZStd::string>().c_str());
                                loadBlocking = false;
                            }
                            else
                            {
                                if (m_blockingAssetTypeManager->HasBlockingHandlersForCurrentThread())
                                {
                                    blockingWait = aznew WaitForAssetOnThreadWithBlockingJobs(assetData, m_blockingAssetTypeManager);
                        }
                                else
                                {
                                    blockingWait = aznew WaitForAssetOnThreadWithNoBlockingJobs(assetData);
                                }
                            }
                    }
                }
            }
            }

            if (!assetInfo.m_relativePath.empty())
            {
                asset.m_assetHint = assetInfo.m_relativePath;
            }
            // We delay the start of the job until we release m_assetMutex to avoid a deadlock
            // when AZCORE_JOBS_IMPL_SYNCHRONOUS is defined
            if (loadBlocking)
            {
                if(loadJob)
                {
                    // Process directly within the calling thread if loadBlocking flag is set.
                    loadJob->Process();
                }

                if (blockingWait)
                {
                    blockingWait->WaitAndDestroy();
                    blockingWait = nullptr;
                }
            }
            else
            {
                if (loadJob)
                {
                    // Otherwise, queue job through the job system.
                    loadJob->Start();
                }
            }

            return asset;
        }

        //=========================================================================
        // CreateAsset
        // [8/31/2012]
        //=========================================================================
        Asset<AssetData> AssetManager::CreateAsset(const AssetId& assetId, const AssetType& assetType)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> asset_lock(m_assetMutex);

            // check if asset already exist
            AssetMap::iterator it = m_assets.find(assetId);
            if (it == m_assets.end())
            {
                // find the asset type handler
                AssetHandlerMap::iterator handlerIt = m_handlers.find(assetType);
                AZ_Error("AssetDatabase", handlerIt != m_handlers.end(), "No handler was registered for this asset [type:0x%x id:%s]!", assetType, assetId.ToString<AZ::OSString>().c_str());
                if (handlerIt != m_handlers.end())
                {
                    // Create the asset ptr
                    AssetHandler* handler = handlerIt->second;
                    Asset<AssetData> asset(handler->CreateAsset(assetId, assetType));
                    auto assetData = asset.Get();
                    AZ_Error("AssetDatabase", assetData, "Failed to create asset with (id=%s, type=%s)", assetId.ToString<AZ::OSString>().c_str(), assetType.ToString<AZ::OSString>().c_str());
                    if (assetData)
                    {
                        assetData->m_assetId = assetId;
                        assetData->m_creationToken = ++m_creationTokenGenerator;
                        ++handler->m_nActiveAssets;
                        if (assetData->IsRegisterReadonlyAndShareable())
                        {
                            m_assets.insert(AZStd::make_pair(assetId, assetData));
                        }
                        return asset;
                    }
                }
            }
            else
            {
                AZ_Error("AssetDatabase", false, "Asset [type:0x%x id:%s] already exists in the database! Asset not created!", assetType, assetId.ToString<AZ::OSString>().c_str());
            }
            return nullptr;
        }

        //=========================================================================
        // ReleaseAsset
        // [6/19/2012]
        //=========================================================================
        void AssetManager::ReleaseAsset(AssetData* asset, AssetId assetId, AssetType assetType, bool removeAssetFromHash, int creationToken)
        {
            AZ_Assert(asset, "Cannot release NULL AssetPtr!");
            bool wasInAssetsHash = false; // We do support assets that are not registered in the asset manager (with the same ID too).
            bool destroyAsset = false;
            if (removeAssetFromHash)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> asset_lock(m_assetMutex);
                AssetMap::iterator it = m_assets.find(assetId);
                // need to check the count again in here in case
               // someone was trying to get the asset on another thread
               // Set it to -1 so only this thread will attempt to clean up the cache and delete the asset
                int expectedRefCount = 0;
                // if the assetId is not in the map or if the identifierId
                // do not match it implies that the asset has been already destroyed.
                // if the usecount is non zero it implies that we cannot destroy this asset.
                if (it != m_assets.end() && it->second->m_creationToken == creationToken && it->second->m_useCount.compare_exchange_strong(expectedRefCount, -1))
                {
                    wasInAssetsHash = true;
                    m_assets.erase(it);
                    destroyAsset = true;
                }
            }
            else
            {
                // if an asset is not shareable, it implies that that asset is not in the map 
                // and therefore once its ref count goes to zero it cannot go back up again and therefore we can safely destroy it
                destroyAsset = true;
            }

            // We have to separate the code which was removing the asset from the m_asset map while being locked, but then actually destroy the asset
            // while the lock is not held since destroying the asset while holding the lock can cause a deadlock.
            if(destroyAsset)
            {
                // find the asset type handler
                AssetHandlerMap::iterator handlerIt = m_handlers.find(assetType);
                AZ_Assert(handlerIt != m_handlers.end(), "No handler was registered for this asset [type:0x%x id:%s]!", assetType.ToString<AZ::OSString>().c_str(), assetId.ToString<AZ::OSString>().c_str());
                AssetHandler* handler = handlerIt->second;
                if (asset)
                {
                    handler->DestroyAsset(asset);
                    if (wasInAssetsHash)
                    {
                        --handler->m_nActiveAssets;
                        EBUS_QUEUE_EVENT_ID(assetId, AssetBus, OnAssetUnloaded, assetId, assetType);
                    }
                }
            }
        }

        //=========================================================================
        // SaveAsset
        // [9/13/2012]
        //=========================================================================
        void AssetManager::SaveAsset(const Asset<AssetData>& asset)
        {
            AssetHandler* handler;
            {
                // find the asset type handler
                AssetHandlerMap::iterator handlerIt = m_handlers.find(asset.GetType());
                AZ_Assert(handlerIt != m_handlers.end(), "No handler was registered for this asset [type:%s id:%s]!", asset.GetType().ToString<AZ::OSString>().c_str(), asset.GetId().ToString<AZ::OSString>().c_str());
                handler = handlerIt->second;
            }

            // start the data saving
            SaveAssetJob* saveJob = aznew SaveAssetJob(m_jobContext, this, asset, handler);
            saveJob->Start();
        }

        //=========================================================================
        // ReloadAsset
        //=========================================================================
        void AssetManager::ReloadAsset(const AssetId& assetId)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_assetMutex);
            auto assetIter = m_assets.find(assetId);

            if (assetIter == m_assets.end() || assetIter->second->IsLoading())
            {
                // Only existing assets can be reloaded.
                return;
            }

            auto reloadIter = m_reloads.find(assetId);
            if (reloadIter != m_reloads.end() && reloadIter->second.GetData()->IsLoading())
            {
                // This asset may already be loading stale data. Queue this requested reload until it's finished.
                AssetBus::QueueFunction([this, assetId]() {this->ReloadAsset(assetId); });
                return;
            }

            AssetData* currentAssetData = assetIter->second;
            AssetData* newAssetData = nullptr;
            AssetHandler* handler = nullptr;

            if (!currentAssetData->IsRegisterReadonlyAndShareable())
            {
                // Reloading an "instance asset" is basically a no-op.
                // We'll simply notify users to reload the asset.
                EBUS_QUEUE_FUNCTION(AssetBus, &AssetManager::NotifyAssetReloaded, this, Asset<AssetData>(currentAssetData));
                return;
            }
            else
            {
                EBUS_QUEUE_FUNCTION(AssetBus, &AssetManager::NotifyAssetPreReload, this, Asset<AssetData>(currentAssetData));
            }

            // Resolve the asset handler and allocate new data for the reload.
            {
                AssetHandlerMap::iterator handlerIt = m_handlers.find(currentAssetData->GetType());
                AZ_Assert(handlerIt != m_handlers.end(), "No handler was registered for this asset [type:0x%x id:%s]!",
                    currentAssetData->GetType().ToString<AZ::OSString>().c_str(), currentAssetData->GetId().ToString<AZ::OSString>().c_str());
                handler = handlerIt->second;

                newAssetData = handler->CreateAsset(currentAssetData->GetId(), currentAssetData->GetType());
                if (newAssetData)
                {
                    newAssetData->m_assetId = currentAssetData->GetId();
                }
            }

            if (newAssetData)
            {
                // For reloaded assets, we need to hold an internal reference to ensure the data
                // isn't immediately destroyed. Since reloads are not a shipping feature, we'll
                // hold this reference indefinitely, but we'll only hold the most recent one for
                // a given asset Id.
                m_reloads[assetId] = newAssetData;

                // Kick off the reload job.
                newAssetData->m_status = static_cast<int>(AssetData::AssetStatus::Loading);
                ReloadAssetJob* reloadJob = aznew ReloadAssetJob(m_jobContext, this, newAssetData, handler);
                reloadJob->Start();
            }
        }

        //=========================================================================
        // ReloadAssetFromData
        //=========================================================================
        void AssetManager::ReloadAssetFromData(const Asset<AssetData>& asset)
        {
            AZ_Assert(asset.Get(), "Asset data for reload is missing.");
            AZ_Assert(m_assets.find(asset.GetId()) != m_assets.end(),"Unable to reload asset %s because its not in the AssetManager's asset list.", asset.ToString<AZStd::string>().c_str());
            AZ_Assert(m_assets.find(asset.GetId()) == m_assets.end() || asset.Get()->RTTI_GetType() == m_assets.find(asset.GetId())->second->RTTI_GetType(),
                "New and old data types are mismatched!");

            auto found = m_assets.find(asset.GetId());
            if ((found == m_assets.end()) || (asset.Get()->RTTI_GetType() != found->second->RTTI_GetType()))
            {
                return; // this will just lead to crashes down the line and the above asserts cover this.
            }

            AssetData* newData = asset.Get();

            if (found->second != newData)
            {
                // Notify users that we are about to change asset
                EBUS_EVENT_ID(asset.GetId(), AssetBus, OnAssetPreReload, asset);

                // Resolve the asset handler and account for the new asset instance.
                {
                    AssetHandlerMap::iterator handlerIt = m_handlers.find(newData->GetType());
                    AZ_Assert(handlerIt != m_handlers.end(), "No handler was registered for this asset [type:0x%x id:%s]!",
                        newData->GetType().ToString<AZ::OSString>().c_str(), newData->GetId().ToString<AZ::OSString>().c_str());
                }

                AssignAssetData(asset);
            }
        }

        //=========================================================================
        // GetHandler
        //=========================================================================
        const AssetHandler* AssetManager::GetHandler(const AssetType& assetType)
        {
            auto handlerEntry = m_handlers.find(assetType);
            if (handlerEntry != m_handlers.end())
            {
                return handlerEntry->second;
            }
            return nullptr;
        }

        //=========================================================================
        // AssignAssetData
        //=========================================================================
        void AssetManager::AssignAssetData(const Asset<AssetData>& asset)
        {
            AZ_Assert(asset.Get(), "Reloaded data is missing!");

            const AssetId& assetId = asset.GetId();

            asset.Get()->m_status = static_cast<int>(AssetData::AssetStatus::Ready);

            if (asset.Get()->IsRegisterReadonlyAndShareable())
            {
                AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_assetMutex);
                auto found = m_assets.find(assetId);
                AZ_Assert(found == m_assets.end() || asset.Get()->RTTI_GetType() == found->second->RTTI_GetType(),
                    "New and old data types are mismatched!");

                // if we are here it implies that we have two assets with the same asset id, and we are 
                // trying to replace the old asset with the new asset which was not created using the asset manager system.
                // In this scenario if any other system have cached the old asset then the asset wont be destroyed 
                // because of creation token mismatch when it's ref count finally goes to zero. Since the old asset is not shareable anymore 
                // manually setting the creationToken to default creation token will ensure that the asset is destroyed correctly.  
                asset.m_assetData->m_creationToken = ++m_creationTokenGenerator;
                found->second->m_creationToken = AZ::Data::s_defaultCreationToken;

                // Held references to old data are retained, but replace the entry in the DB for future requests.
                // Fire an OnAssetReloaded message so listeners can react to the new data.
                m_assets[assetId] = asset.Get();

                // Release the reload reference.
                m_reloads.erase(assetId);
            }

            // Notify users the data has changed.
            EBUS_EVENT_ID(assetId, AssetBus, OnAssetReloaded, asset);
        }

        //=========================================================================
        // NotifyAssetReady
        // [4/3/2014]
        //=========================================================================
        void AssetManager::NotifyAssetReady(Asset<AssetData> asset)
        {
            AssetData* data = asset.Get();
            AZ_Assert(data, "NotifyAssetReady: asset is missing info!");
            data->m_status = static_cast<int>(AssetData::AssetStatus::Ready);
            EBUS_EVENT_ID(asset.GetId(), AssetBus, OnAssetReady, asset);
        }

        //=========================================================================
        // NotifyAssetPreReload
        //=========================================================================
        void AssetManager::NotifyAssetPreReload(Asset<AssetData> asset)
        {
            EBUS_EVENT_ID(asset.GetId(), AssetBus, OnAssetPreReload, asset);
        }

        //=========================================================================
        // NotifyAssetReloaded
        //=========================================================================
        void AssetManager::NotifyAssetReloaded(Asset<AssetData> asset)
        {
            AssignAssetData(asset);
        }

        //=========================================================================
        // NotifyAssetReloaded
        //=========================================================================
        void AssetManager::NotifyAssetReloadError(Asset<AssetData> asset)
        {
            // Failed reloads have no side effects. Just notify observers (error reporting, etc).
            m_reloads.erase(asset.GetId());
            EBUS_EVENT_ID(asset.GetId(), AssetBus, OnAssetReloadError, asset);
        }

        //=========================================================================
        // NotifyAssetError
        // [4/3/2014]
        //=========================================================================
        void AssetManager::NotifyAssetError(Asset<AssetData> asset)
        {
            asset.Get()->m_status = static_cast<int>(AssetData::AssetStatus::Error);
            EBUS_EVENT_ID(asset.GetId(), AssetBus, OnAssetError, asset);
        }

        //=========================================================================
        // AddJob
        // [04/02/2014]
        //=========================================================================
        void AssetManager::AddJob(AssetDatabaseJob* job)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_assetMutex);

            m_activeJobs.push_back(*job);
        }

        //=========================================================================
        // RegisterAssetLoading
        //=========================================================================
        void AssetManager::RegisterAssetLoading(const Asset<AssetData>& asset)
        {
            {
                const AZ::Data::AssetId& assetId = asset.GetId();

                AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_assetMutex);
                m_assetsLoadingByThread[assetId] = AZStd::this_thread::get_id();
            }

            m_blockingAssetTypeManager->AssetLoadStarted(asset.GetType(), asset.GetId());
        }

        //=========================================================================
        // UnregisterAssetLoadingByThread
        //=========================================================================
        void AssetManager::UnregisterAssetLoading(const Asset<AssetData>& asset)
        {
            m_blockingAssetTypeManager->AssetLoadFinished(asset.GetType(), asset.GetId());

            {
                const AZ::Data::AssetId& assetId = asset.GetId();

                AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_assetMutex);
                m_assetsLoadingByThread.erase(assetId);
            }
        }

        //=========================================================================
        // RemoveJob
        // [04/02/2014]
        //=========================================================================
        void AssetManager::RemoveJob(AssetDatabaseJob* job)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_assetMutex);

            m_activeJobs.erase(*job);
        }

        //=========================================================================
        // GetLoadStreamInfoForAsset()
        // [04/04/2014]
        //=========================================================================
        AssetStreamInfo AssetManager::GetLoadStreamInfoForAsset(const AssetId& assetId, const AssetType& assetType)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> catalogLock(m_catalogMutex);
            AssetCatalogMap::iterator catIt = m_catalogs.find(assetType);
            if (catIt == m_catalogs.end())
            {
                AZ_Error("Asset", false, "Asset [type:0x%x id:0x%x] with this type doesn't have a catalog!", assetType, assetId);
                return AssetStreamInfo();
            }
            return catIt->second->GetStreamInfoForLoad(assetId, assetType);
        }

        //=========================================================================
        // GetSaveStreamInfoForAsset()
        // [04/04/2014]
        //=========================================================================
        AssetStreamInfo AssetManager::GetSaveStreamInfoForAsset(const AssetId& assetId, const AssetType& assetType)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> catalogLock(m_catalogMutex);
            AssetCatalogMap::iterator catIt = m_catalogs.find(assetType);
            if (catIt == m_catalogs.end())
            {
                AZ_Error("Asset", false, "Asset [type:0x%x id:0x%x] with this type doesn't have a catalog!", assetType, assetId);
                return AssetStreamInfo();
            }
            return catIt->second->GetStreamInfoForSave(assetId, assetType);
        }

        //=========================================================================
        // OnAssetReady
        // [04/02/2014]
        //=========================================================================
        void AssetManager::OnAssetReady(const Asset<AssetData>& asset)
        {
            AZ_Assert(asset.Get(), "OnAssetReady fired for an asset with no data.");

            // Set status immediately from within the AssetManagerBus dispatch, so it's committed before anyone is notified (e.g. job to job, via AssetJobBus).
            asset.Get()->m_status = static_cast<int>(AssetData::AssetStatus::ReadyPreNotify);

            // Queue broadcast message for delivery on game thread.
            AssetBus::QueueFunction(&AssetManager::NotifyAssetReady, this, Asset<AssetData>(asset));
        }

        //=========================================================================
        // OnAssetError
        //=========================================================================
        void AssetManager::OnAssetError(const Asset<AssetData>& asset)
        {
            // Set status immediately from within the AssetManagerBus dispatch, so it's committed before anyone is notified (e.g. job to job, via AssetJobBus).
            asset.Get()->m_status = static_cast<int>(AssetData::AssetStatus::Error);

            // Queue broadcast message for delivery on game thread.
            AssetBus::QueueFunction(&AssetManager::NotifyAssetError, this, Asset<AssetData>(asset));
        }

        //=========================================================================
        // OnAssetReloaded
        //=========================================================================
        void AssetManager::OnAssetReloaded(const Asset<AssetData>& asset)
        {
            // Queue broadcast message for delivery on game thread.
            AssetBus::QueueFunction(&AssetManager::NotifyAssetReloaded, this, Asset<AssetData>(asset));
        }

        //=========================================================================
        // OnAssetReloadError
        //=========================================================================
        void AssetManager::OnAssetReloadError(const Asset<AssetData>& asset)
        {
            // Queue broadcast message for delivery on game thread.
            AssetBus::QueueFunction(&AssetManager::NotifyAssetReloadError, this, Asset<AssetData>(asset));
        }

        //=========================================================================
        // AssetHandler
        // [04/03/2014]
        //=========================================================================
        AssetHandler::AssetHandler()
            : m_nActiveAssets(0)
            , m_nHandledTypes(0)
        {
        }

        //=========================================================================
        // ~AssetHandler
        // [04/03/2014]
        //=========================================================================
        AssetHandler::~AssetHandler()
        {
            if (m_nHandledTypes > 0)
            {
                AssetManager::Instance().UnregisterHandler(this);
            }

            AZ_Error("AssetDatabase", m_nHandledTypes == 0, "Asset handler is being destroyed but there are still %d asset types being handled by it!", (int)m_nHandledTypes);
            AZ_Error("AssetDatabase", m_nActiveAssets == 0, "Asset handler is being destroyed but there are still %d active assets being handled by it!", (int)m_nActiveAssets);
        }

        //=========================================================================
        // LoadAssetData
        // [04/03/2014]
        //=========================================================================
        bool AssetHandler::LoadAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream, const AssetFilterCB& assetLoadFilterCB)
        {
            (void)asset;
            (void)stream;
            (void)assetLoadFilterCB;
            AZ_Assert(false, "User asset handler did not implement this function!");
            return false;
        }

        //=========================================================================
        // LoadAssetData
        // [04/03/2014]
        //=========================================================================
        bool AssetHandler::LoadAssetData(const Asset<AssetData>& asset, const char* assetPath, const AssetFilterCB& assetLoadFilterCB)
        {
            (void)asset;
            (void)assetPath;
            (void)assetLoadFilterCB;
            AZ_Assert(false, "User asset handler did not implement this function!");
            return false;
        }

        //=========================================================================
        // InitAsset
        // [04/03/2014]
        //=========================================================================
        void AssetHandler::InitAsset(const Asset<AssetData>& asset, bool loadStageSucceeded, bool isReload)
        {
            if (loadStageSucceeded)
            {
                if (isReload)
                {
                    EBUS_EVENT(AssetManagerBus, OnAssetReloaded, asset);
                }
                else
                {
                    EBUS_EVENT(AssetManagerBus, OnAssetReady, asset);
                }
            }
            else
            {
                if (!isReload)
                {
                    EBUS_EVENT(AssetManagerBus, OnAssetError, asset);
                }
                else
                {
                    EBUS_EVENT(AssetManagerBus, OnAssetReloadError, asset);
                }
            }
        }
    } // namespace Data
}   // namespace AZ

#endif // #ifndef AZ_UNITY_BUILD
