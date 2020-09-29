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

#include <AzCore/Asset/AssetContainer.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetManager.h>

namespace AZ
{
    namespace Data
    {
   
        AssetContainer::AssetContainer(const AZ::Data::AssetId& rootAssetId, const AZ::Data::AssetType& assetType, AssetContainerDependencyRules dependencyRules, AssetFilterCB loadFilter)
        {
            AZ::Data::AssetInfo assetInfo;
            AssetCatalogRequestBus::BroadcastResult(assetInfo, &AssetCatalogRequestBus::Events::GetAssetInfoById, rootAssetId);
            if (!assetInfo.m_assetId.IsValid())
            {
                AZ_Warning("AssetContainer", false, "Invalid asset for container : %s\n", assetInfo.m_assetId.ToString<AZStd::string>().c_str());
                return;
            }
            if (!AssetManager::Instance().GetHandler(assetInfo.m_assetType))
            {
                AZ_Warning("AssetContainer", false, "No handler for asset : %s\n", assetInfo.m_assetId.ToString<AZStd::string>().c_str());
                return;
            }
            AddDependentAssets(rootAssetId, assetType, dependencyRules, loadFilter);
        }

        AssetContainer::~AssetContainer()
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            AZ::Data::AssetLoadBus::MultiHandler::BusDisconnect();
        }

        void AssetContainer::AddDependentAssets(const AZ::Data::AssetId& rootAssetId, const AZ::Data::AssetType& assetType, AssetContainerDependencyRules dependencyRules, AssetFilterCB loadFilter)
        {
            // Every asset we're going to be waiting on a load for - the root and all valid dependencies
            AZStd::vector<AZ::Data::AssetId> waitingList;
            waitingList.push_back(rootAssetId);

            // Cached AssetInfo to save another lookup inside Assetmanager
            AZStd::vector<Data::AssetInfo> dependencyInfoList;
            AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> getDependenciesResult = AZ::Failure(AZStd::string());

            // Track preloads in an additional list - they're in our waiting/dependenciyInfo lists as well, but preloads require us to
            // suppress emitting "AssetReady" until everything we care about in this context is ready
            AZ::Data::PreloadAssetListType preloadDependencies;
            if (dependencyRules == AssetContainerDependencyRules::UseLoadBehavior)
            {
                AZStd::unordered_set<AZ::Data::AssetId> noloadDependencies;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(getDependenciesResult, &AZ::Data::AssetCatalogRequestBus::Events::GetLoadBehaviorProductDependencies,
                    rootAssetId, noloadDependencies, preloadDependencies);
                if (!noloadDependencies.empty())
                {
                    AZStd::lock_guard<AZStd::recursive_mutex> dependencyLock(m_dependencyMutex);
                   m_unloadedDependencies.insert(noloadDependencies.begin(), noloadDependencies.end());
                }
            }
            else if (dependencyRules == AssetContainerDependencyRules::LoadAll)
            {
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(getDependenciesResult, &AZ::Data::AssetCatalogRequestBus::Events::GetAllProductDependencies, rootAssetId);
            }
            // Do as much validation of dependencies as we can before the AddWaitingAssets and GetAsset calls for dependencies below
            if (getDependenciesResult.IsSuccess())
            {
                for (const auto& thisAsset : getDependenciesResult.GetValue())
                {
                    AZ::Data::AssetInfo assetInfo;
                    AssetCatalogRequestBus::BroadcastResult(assetInfo, &AssetCatalogRequestBus::Events::GetAssetInfoById, thisAsset.m_assetId);

                    if (!assetInfo.m_assetId.IsValid())
                    {
                        // Handlers may just not currently be around for a given asset type so we only warn here
                        AZ_Warning("AssetContainer", false, "Couldn't get asset info for dependency %s under asset %s", assetInfo.m_assetId.ToString<AZStd::string>().c_str(),
                            rootAssetId.ToString<AZStd::string>().c_str());
                        m_invalidDependencies++;
                        continue;
                    }
                    if (assetInfo.m_assetId == rootAssetId)
                    {
                        // Circular dependencies in our graph need to be raised as errors as they could cause problems elsewhere
                        AZ_Error("AssetContainer", false, "Circular dependency found under asset %s", rootAssetId.ToString<AZStd::string>().c_str());
                        m_invalidDependencies++;
                        continue;
                    }
                    if (!AssetManager::Instance().GetHandler(assetInfo.m_assetType))
                    {
                        // Handlers may just not currently be around for a given asset type so we only warn here
                        m_invalidDependencies++;
                        continue;
                    }
                    if (loadFilter)
                    {
                        AZ::Data::Asset<AssetData> testAsset(thisAsset.m_assetId, assetInfo.m_assetType, assetInfo.m_relativePath);
                        if (!loadFilter(testAsset))
                        {
                            continue;
                        }
                    }
                    dependencyInfoList.push_back(assetInfo);
                }
            }
            for (auto& thisInfo : dependencyInfoList)
            {
                waitingList.push_back(thisInfo.m_assetId);
            }
            // Add waiting assets ahead of time to hear signals for any which may already be loading
            AddWaitingAssets(waitingList);
            SetupPreloadLists(AZStd::move(preloadDependencies), rootAssetId);
            auto thisAsset = AssetManager::Instance().GetAsset(rootAssetId, assetType, true, &AZ::Data::AssetFilterNoAssetLoading, false, false, AZ::Data::AssetInfo(), HasPreloads(rootAssetId));
            if (!thisAsset)
            {
                ClearWaitingAssets();
                return;
            }

            AZStd::vector< Asset<AssetData>> newDependencies;
            if (!m_rootAsset)
            {
                m_rootAsset = AZStd::move(thisAsset);
            }
            else
            {
                newDependencies.push_back(AZStd::move(thisAsset));
            }
            for (auto& thisInfo : dependencyInfoList)
            {
                auto depInfo = AssetManager::Instance().GetAsset(thisInfo.m_assetId, thisInfo.m_assetType, true, &AZ::Data::AssetFilterNoAssetLoading, false, true, thisInfo, HasPreloads(thisInfo.m_assetId));

                if (!depInfo || !depInfo.GetId().IsValid())
                {
                    AZ_Warning("AssetContainer", false, "Dependency Asset %s (%s) was not found\n", thisInfo.m_assetId.ToString<AZStd::string>().c_str(), thisInfo.m_relativePath.c_str());
                    RemoveWaitingAsset(thisInfo.m_assetId);
                    continue;
                }
                newDependencies.push_back(AZStd::move(depInfo));

            }
            {
                AZStd::lock_guard<AZStd::recursive_mutex> dependencyLock(m_dependencyMutex);
                for (auto& thisDependency : newDependencies)
                {
                    AddDependency(AZStd::move(thisDependency));
                }
            }
            CheckReady();
            m_initComplete = true;
        }

        bool AssetContainer::LoadDependency(AZ::Data::AssetId unloadedDependency, AssetFilterCB loadFilter, AssetContainerDependencyRules dependencyRules)
        {
            {
                AZStd::lock_guard<AZStd::recursive_mutex> dependencyLock(m_dependencyMutex);

                if (!m_unloadedDependencies.erase(unloadedDependency))
                {
                    AZ_Error("AssetContainer", false, "Unloaded Dependency Asset %s was not found in container for %s\n", unloadedDependency.ToString<AZStd::string>().c_str(), m_rootAsset ? m_rootAsset->GetId().ToString<AZStd::string>().c_str() : "Null");
                    return false;
                }
            }
            AZ::Data::AssetInfo assetInfo;
            AssetCatalogRequestBus::BroadcastResult(assetInfo, &AssetCatalogRequestBus::Events::GetAssetInfoById, unloadedDependency);
            if (!assetInfo.m_assetId.IsValid())
            {
                AZ_Error("AssetContainer", false, "Couldn't get info for unloaded dependency Asset %s in container for %s\n", unloadedDependency.ToString<AZStd::string>().c_str(), m_rootAsset ? m_rootAsset->GetId().ToString<AZStd::string>().c_str() : "Null");
                return false;
            }
            AddDependentAssets(unloadedDependency, assetInfo.m_assetType, dependencyRules, loadFilter);
            return true;
        }

        bool AssetContainer::IsReady() const
        {
            return (m_rootAsset && m_waitingCount == 0);
        }

        bool AssetContainer::IsLoading() const
        {
            return (m_rootAsset && m_waitingCount);
        }

        void AssetContainer::CheckReady()
        {
            if (!m_dependencies.empty())
            {
                for (auto& [assetId, dependentAsset] : m_dependencies)
                {
                    if (dependentAsset->IsReady())
                    {
                        HandleReadyAsset(dependentAsset);
                    }
                }
            }
            if (m_rootAsset && m_rootAsset->IsReady())
            {
                HandleReadyAsset(m_rootAsset);
            }
        }

        Asset<AssetData>& AssetContainer::GetAsset()
        {
            return m_rootAsset;
        }

        void AssetContainer::AddDependency(const Asset<AssetData>& newDependency)
        {
            m_dependencies[newDependency->GetId()] = newDependency;
        }
        void AssetContainer::AddDependency(Asset<AssetData>&& newDependency)
        {
            m_dependencies[newDependency->GetId()] = AZStd::move(newDependency);
        }

        void AssetContainer::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            HandleReadyAsset(asset);
        }

        void AssetContainer::HandleReadyAsset(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            RemoveWaitingAsset(asset->GetId());
            RemoveFromAllWaitingPreloads(asset->GetId());
        }

        void AssetContainer::OnAssetDataLoaded(Asset<AssetData> asset)
        {
            // Remove only from this asset's waiting list.  Anything else should
            // listen for OnAssetReady as the true signal.  This is essentially removing the
            // "marker" we placed in SetupPreloads that we need to wait for our own data
            RemoveFromWaitingPreloads(asset->GetId(), asset->GetId());
        }

        void AssetContainer::RemoveFromWaitingPreloads(const AZ::Data::AssetId& waiterId, const AZ::Data::AssetId& preloadID)
        {
            {
                AZStd::lock_guard<AZStd::recursive_mutex> preloadGuard(m_preloadMutex);

                auto remainingPreloadIter = m_preloadList.find(waiterId);
                if (remainingPreloadIter == m_preloadList.end())
                {
                    AZ_Warning("AssetContainer", !m_initComplete, "Couldn't find waiting list for %s", waiterId.ToString<AZStd::string>().c_str());
                    return;
                }
                if (!remainingPreloadIter->second.erase(preloadID))
                {
                    AZ_Warning("AssetContainer", !m_initComplete, "Couldn't remove %s from waiting list of %s", preloadID.ToString<AZStd::string>().c_str(), waiterId.ToString<AZStd::string>().c_str());
                    return;
                }
                if (!remainingPreloadIter->second.empty())
                {
                    return;
                }
            }
            auto thisAsset = GetAssetData(waiterId);
            AssetManager::Instance().ValidateAndPostLoad(thisAsset, true, false, nullptr);
        }

        void AssetContainer::RemoveFromAllWaitingPreloads(const AZ::Data::AssetId& thisId) 
        {
            AZStd::unordered_set<AZ::Data::AssetId> checkList;
            {
                AZStd::lock_guard<AZStd::recursive_mutex> preloadGuard(m_preloadMutex);

                auto waitingList = m_preloadWaitList.find(thisId);
                if (waitingList != m_preloadWaitList.end())
                {
                    checkList = AZStd::move(waitingList->second);
                    m_preloadWaitList.erase(waitingList);
                }
            }
            for (auto& thisDepId : checkList)
            {
                if (thisDepId != thisId)
                {
                    RemoveFromWaitingPreloads(thisDepId, thisId);
                }
            }
        }

        void AssetContainer::ClearWaitingAssets()
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_readyMutex);
            m_waitingCount = 0;
            for (auto& thisAsset : m_waitingAssets)
            {
                AZ::Data::AssetBus::MultiHandler::BusDisconnect(thisAsset);
            }
            m_waitingAssets.clear();
        }

        void AssetContainer::ListWaitingAssets() const
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_readyMutex);
            AZ_TracePrintf("AssetContainer", "Waiting on assets:\n");
#if defined(AZ_ENABLE_TRACING)
            for (auto& thisAsset : m_waitingAssets)
            {
                AZ_TracePrintf("AssetContainer", "  %s\n",thisAsset.ToString<AZStd::string>().c_str());
            }
#endif
        }

        void AssetContainer::ListWaitingPreloads(const AZ::Data::AssetId& assetId) const
        {
            AZStd::lock_guard<AZStd::recursive_mutex> preloadGuard(m_preloadMutex);
            auto preloadEntry = m_preloadList.find(assetId);
            if (preloadEntry != m_preloadList.end())
            {
                AZ_TracePrintf("AssetContainer", "%s waiting on preloads : \n",assetId.ToString<AZStd::string>().c_str());
#if defined(AZ_ENABLE_TRACING)
                for (auto& thisId: preloadEntry->second)
                {
                    AZ_TracePrintf("AssetContainer", "  %s\n",thisId.ToString<AZStd::string>().c_str());
                }
#endif
            }
            else
            {
                AZ_TracePrintf("AssetContainer", "%s isn't waiting on any preloads:\n", assetId.ToString<AZStd::string>().c_str());
            }
        }

        void AssetContainer::AddWaitingAssets(const AZStd::vector<AZ::Data::AssetId>& assetList)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_readyMutex);
            for (auto& thisAsset : assetList)
            {
                if (m_waitingAssets.insert(thisAsset).second)
                {
                    ++m_waitingCount;
                    AZ::Data::AssetBus::MultiHandler::BusConnect(thisAsset);
                    AZ::Data::AssetLoadBus::MultiHandler::BusConnect(thisAsset);
                }
            }
        }

        void AssetContainer::AddWaitingAsset(const AZ::Data::AssetId& thisAsset)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_readyMutex);
            if (m_waitingAssets.insert(thisAsset).second)
            {
                ++m_waitingCount;
                AZ::Data::AssetBus::MultiHandler::BusConnect(thisAsset);
                AZ::Data::AssetLoadBus::MultiHandler::BusConnect(thisAsset);
            }
        }

        void AssetContainer::RemoveWaitingAsset(const AZ::Data::AssetId& thisAsset)
        {
            bool allReady{ false };
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_readyMutex);
                // If we're trying to remove something already removed, just ignore it
                if (m_waitingAssets.erase(thisAsset))
                {
                    m_waitingCount -= 1;

                    AZ::Data::AssetBus::MultiHandler::BusDisconnect(thisAsset);
                    AZ::Data::AssetLoadBus::MultiHandler::BusDisconnect(thisAsset);

                    if (m_rootAsset && m_waitingAssets.empty())
                    {
                        allReady = true;
                    }
                }
            }
            if (allReady && m_initComplete)
            {
                AZ::Data::AssetBus::Event(m_rootAsset->GetId(), &AZ::Data::AssetEvents::OnAssetContainerReady, m_rootAsset);
            }
        }
        AssetId  AssetContainer::GetId() const
        {
            return m_rootAsset ? m_rootAsset->GetId() : AssetId();
        }

        AssetType  AssetContainer::GetType() const
        {
            return m_rootAsset ? m_rootAsset->GetType() : s_invalidAssetType;
        }

        const AssetData* AssetContainer::operator->() const
        {
            return m_rootAsset.Get();
        }

        AssetContainer::operator bool() const
        {
            return m_rootAsset ? true : false;
        }

        AZStd::string_view AssetContainer::GetHint() const
        {
            static constexpr const char s_empty[] = "";
            return m_rootAsset ? m_rootAsset.GetHint() : s_empty;
        }

        AZ::Data::AssetData::AssetStatus AssetContainer::GetStatus() const
        {
            return m_rootAsset ? m_rootAsset.GetStatus() : AZ::Data::AssetData::AssetStatus::Error;
        }

        const AssetContainer::DependencyList& AssetContainer::GetDependencies() const
        {
            return m_dependencies;
        }

        const AZStd::unordered_set<AZ::Data::AssetId>& AssetContainer::GetUnloadedDependencies() const
        {
            return m_unloadedDependencies;
        }

        void AssetContainer::SetupPreloadLists(PreloadAssetListType&& preloadList, const AZ::Data::AssetId& rootAssetId)
        {
            if (!preloadList.empty())
            {
                // This method can be entered as additional NoLoad dependency groups are loaded - the container could
                // be in the middle of loading so we need to grab both mutexes.  
                AZStd::scoped_lock<AZStd::recursive_mutex, AZStd::recursive_mutex> lock(m_readyMutex, m_preloadMutex);

                for (auto thisListPair = preloadList.begin(); thisListPair != preloadList.end();)
                {
                    // We only should add ourselves if we have another valid preload we're waiting on
                    bool foundAsset{ false };
                    // It's possible this set of preload dependencies was culled out by lack of asset handler
                    // Or filtering rules.  This is not an error, we should just remove it from the list of
                    // Preloads we're waiting on
                    if (!m_waitingAssets.count(thisListPair->first))
                    {
                        thisListPair = preloadList.erase(thisListPair);
                        continue;
                    }
                    for (auto thisAsset = thisListPair->second.begin(); thisAsset != thisListPair->second.end();)
                    {
                        // These are data errors.  We'll emit the error but carry on.  The container
                        // will load the assets but won't/can't create a circular preload dependency chain
                        if (*thisAsset == rootAssetId)
                        {
                            AZ_Error("AssetContainer", false, "Circular preload dependency found - %s has a preload" 
                                "dependency back to root %s\n",
                                thisListPair->first.ToString<AZStd::string>().c_str(),
                                rootAssetId.ToString<AZStd::string>().c_str());
                            thisAsset = thisListPair->second.erase(thisAsset);
                            continue;
                        }
                        else if (*thisAsset == thisListPair->first)
                        {
                            AZ_Error("AssetContainer", false, "Circular preload dependency found - Root asset %s has a preload"
                                "dependency on %s which depends back back to itself\n",
                                rootAssetId.ToString<AZStd::string>().c_str(),
                                thisListPair->first.ToString<AZStd::string>().c_str());
                            thisAsset = thisListPair->second.erase(thisAsset);
                            continue;
                        }
                        else if (m_preloadWaitList.count(thisListPair->first) && m_preloadWaitList[thisListPair->first].count(*thisAsset))
                        {
                            AZ_Error("AssetContainer", false, "Circular dependency found - Root asset %s has a preload"
                                "dependency on %s which has a circular dependency with %s\n",
                                rootAssetId.ToString<AZStd::string>().c_str(),
                                thisListPair->first.ToString<AZStd::string>().c_str(),
                                thisAsset->ToString<AZStd::string>().c_str());
                            thisAsset = thisListPair->second.erase(thisAsset);
                            continue;
                        }
                        else if (m_waitingAssets.count(*thisAsset))
                        {
                            foundAsset = true;
                            m_preloadWaitList[*thisAsset].insert(thisListPair->first);
                            ++thisAsset;
                        }
                        else
                        {
                            // This particular preload dependency of this asset was culled
                            // similar to the case above this can be due to no established asset handler
                            // or filtering rules.  We'll just erase the entry because we're not loading this
                            thisAsset = thisListPair->second.erase(thisAsset);
                            continue;
                        }
                    }
                    if (foundAsset)
                    {
                        // We've established that this asset has at least one preload dependency it needs to wait on
                        // so we additionally add the waiting asset as its own preload so all of our "waiting assets"
                        // are managed in the same list.  We can't consider this asset to be "ready" until all
                        // of its preloads are ready and it has been loaded.  It will request an OnAssetDataLoaded
                        // notification from AssetManager rather than an OnAssetReady because of these additional dependencies.
                        thisListPair->second.insert(thisListPair->first);
                        m_preloadWaitList[thisListPair->first].insert(thisListPair->first);
                    }
                    ++thisListPair;
                }
                for(auto& thisList : preloadList)
                {
                    m_preloadList[thisList.first].insert(thisList.second.begin(), thisList.second.end());
                }
            }
        }

        bool AssetContainer::HasPreloads(const AZ::Data::AssetId& assetId) const
        {
            AZStd::lock_guard<AZStd::recursive_mutex> preloadGuard(m_preloadMutex);
            auto preloadEntry = m_preloadList.find(assetId);
            if (preloadEntry != m_preloadList.end())
            {
                return !preloadEntry->second.empty();
            }
            return false;
        }

        Asset<AssetData> AssetContainer::GetAssetData(const AZ::Data::AssetId& assetId) const
        {
            AZStd::lock_guard<AZStd::recursive_mutex> dependenciesGuard(m_dependencyMutex);
            if (m_rootAsset.GetId() == assetId)
            {
                return m_rootAsset;
            }
            auto dependencyIter = m_dependencies.find(assetId);
            if (dependencyIter != m_dependencies.end())
            {
                return dependencyIter->second;
            }
            AZ_Warning("AssetContainer", false, "Asset %s not found in container", assetId.ToString<AZStd::string>().c_str());
            return {};
        }

        int AssetContainer::GetNumWaitingDependencies() const
        {
            return m_waitingCount.load();
        }

        int AssetContainer::GetInvalidDependencies() const
        {
            return m_invalidDependencies.load();
        }
    }   // namespace Data
}   // namespace AZ
