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
#include <AzCore/Asset/AssetManager_private.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/set.h>


namespace AZ
{
    namespace Data
    {
        // AssetContainer loads an asset and all of its dependencies as a collection which is parallellized as much as possible.
        // With the container, the data will all load in parallel.  Dependent asset loads will still obey the expected rules
        // where PreLoad assets will emit OnAssetReady before the parent does, and QueueLoad assets will emit OnAssetReady in 
        // no guaranteed order.  However, the OnAssetContainerReady signals will not emit until all PreLoad and QueueLoad assets 
        // are ready.  NoLoad dependencies are not loaded by default but can be loaded along with their dependencies using the
        // same rules as above by using the LoadAll dependency rule.
        class AssetContainer :
            AZ::Data::AssetBus::MultiHandler,
            AZ::Data::AssetLoadBus::MultiHandler
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetContainer, SystemAllocator, 0);

            enum class AssetContainerDependencyRules
            {
                // Default will store "NoLoad" dependencies in a separate list which can be loaded on demand
                UseLoadBehavior = 0,
                Default = UseLoadBehavior,
                // LoadAll will ignore "NoLoad" dependencies and load everything
                LoadAll = 1
            };
            AssetContainer() = default;
            explicit AssetContainer(const AZ::Data::AssetId& rootAssetId, const AZ::Data::AssetType& rootAssetType, AssetContainerDependencyRules dependencyRules = AssetContainerDependencyRules::Default, AssetFilterCB loadFilter = nullptr);

            ~AssetContainer();

            bool IsReady() const;
            bool IsLoading() const;

            AssetId GetId() const;
            AssetType GetType() const; 
            AZStd::string_view GetHint() const;
            AZ::Data::AssetData::AssetStatus GetStatus() const;
            Asset<AssetData>& GetAsset();

            const AssetData* operator->() const;
            operator bool() const;

            using DependencyList = AZStd::unordered_map< AZ::Data::AssetId, AZ::Data::Asset<AssetData>>;
            const DependencyList& GetDependencies() const;

            int GetNumWaitingDependencies() const;
            int GetInvalidDependencies() const;

            void ListWaitingAssets() const;
            void ListWaitingPreloads(const AZ::Data::AssetId& assetId) const;

            // Default behavior is to store dependencies flagged as "NoLoad" AutoLoadBehavior
            // These can be kicked off with a LoadDependency request

            const AZStd::unordered_set<AZ::Data::AssetId>& GetUnloadedDependencies() const;
            bool LoadDependency(AssetId dependencyId, AssetFilterCB loadFilter = nullptr, AssetContainerDependencyRules dependencyRules = AssetContainerDependencyRules::Default);

            //////////////////////////////////////////////////////////////////////////
            // AssetBus
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetDataLoaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        protected:
            // Waiting assets are those which have not yet signalled ready.  In the case of PreLoad dependencies the data may have completed the load cycle but
            // the Assets aren't considered "Ready" yet if there are PreLoad dependencies still loading and will still be in the list until the point that asset and
            // All of its preload dependencies have been loaded, when it signals OnAssetReady
            void AddWaitingAsset(const AZ::Data::AssetId& waitingAsset);
            void AddWaitingAssets(const AZStd::vector<AZ::Data::AssetId>& waitingAssets);
            void RemoveWaitingAsset(const AZ::Data::AssetId& waitingAsset);
            void ClearWaitingAssets();

            // Internal check to validate ready status at the end of initialization
            void CheckReady();

            // Add an individual asset to our list of known dependencies.  Does not include the root asset which is in m_rootAset
            void AddDependency(const Asset<AssetData>& newDependency);
            void AddDependency(Asset<AssetData>&& addDependency);

            // Add a "graph section" to our list of dependencies.  This checks the catalog for all Pre and Queue load assets which are dependents of the requested asset and kicks off loads
            // NoLoads which are encounted are placed in another list and can be loaded on demand with the LoadDependency call.  
            void AddDependentAssets(const AZ::Data::AssetId& rootAssetId, const AZ::Data::AssetType& rootAssetType, AssetContainerDependencyRules dependencyRules = AssetContainerDependencyRules::Default, AssetFilterCB loadFilter = nullptr);

            // If "PreLoad" assets are found in the graph these are cached and tracked with both OnAssetReady and OnAssetDataLoaded messages.
            // OnAssetDataLoaded is used to suppress what would normally be an OnAssetReady call - we need to use the container to evaluate whether
            // all of an asset's preload dependencies are ready before completing the load cycle where OnAssetReady will be signalled and the asset
            // will be removed from the waiting list in the container
            void SetupPreloadLists(PreloadAssetListType&& preloadList, const AZ::Data::AssetId& rootAssetId);
            bool HasPreloads(const AZ::Data::AssetId& assetId) const;

            // Remove a specific id from the list an asset is waiting for and complete the load if everything is ready
            void RemoveFromWaitingPreloads(const AZ::Data::AssetId& waitingId, const AZ::Data::AssetId& preloadAssetId);
            // Iterate over the list that was waiting for this asset and remove it from each
            void RemoveFromAllWaitingPreloads(const AZ::Data::AssetId& assetId);
            Asset<AssetData> GetAssetData(const AZ::Data::AssetId& assetId) const;

            // Used for final CheckReady after setup as well as internal handling for OnAssetReady
            // duringInit if we're coming from the checkReady method - containers that start ready don't need to signal
            void HandleReadyAsset(AZ::Data::Asset<AZ::Data::AssetData> asset);

            // Optimization to save the lookup in the dependencies map 
            Asset<AssetData> m_rootAsset;

            mutable AZStd::recursive_mutex m_dependencyMutex;
            DependencyList m_dependencies;

            mutable AZStd::recursive_mutex m_readyMutex;
            AZStd::set<AZ::Data::AssetId> m_waitingAssets;
            AZStd::atomic_int m_waitingCount{0};
            AZStd::atomic_int m_invalidDependencies{ 0 };
            AZStd::unordered_set<AZ::Data::AssetId> m_unloadedDependencies;
            AZStd::atomic_bool m_initComplete{ false };

            mutable AZStd::recursive_mutex m_preloadMutex;
            // AssetId -> List of assets it is still waiting on 
            PreloadAssetListType m_preloadList;

            // AssetId -> List of assets waiting on it
            PreloadAssetListType m_preloadWaitList;
        private:
            AssetContainer operator=(const AssetContainer& copyContainer) = delete;
            AssetContainer operator=(const AssetContainer&& copyContainer) = delete;
            AssetContainer(const AssetContainer& copyContainer) = delete;
            AssetContainer(AssetContainer&& copyContainer) = delete;
        };

    }  // namespace Data
}   // namespace AZ



