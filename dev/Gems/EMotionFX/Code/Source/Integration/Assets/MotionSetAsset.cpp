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


#include "EMotionFX_precompiled.h"

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/TickBus.h>

#include <Integration/Assets/MotionSetAsset.h>

namespace EMotionFX
{
    namespace Integration
    {
        AZ_CLASS_ALLOCATOR_IMPL(MotionSetAsset, EMotionFXAllocator, 0)
        AZ_CLASS_ALLOCATOR_IMPL(MotionSetAssetHandler, EMotionFXAllocator, 0)

        /**
         * Custom callback registered with EMotion FX for the purpose of intercepting
         * motion load requests. We want to pipe all requested loads through our
         * asset system.
         */
        class CustomMotionSetCallback
            : public EMotionFX::MotionSetCallback
        {
        public:
            AZ_CLASS_ALLOCATOR(CustomMotionSetCallback, EMotionFXAllocator, 0);

            CustomMotionSetCallback(const AZ::Data::Asset<MotionSetAsset>& asset)
                : MotionSetCallback(asset.Get()->m_emfxMotionSet.get())
                , m_assetData(asset.Get())
            {
            }

            EMotionFX::Motion* LoadMotion(EMotionFX::MotionSet::MotionEntry* entry) override
            {
                // When EMotionFX requests a motion to be loaded, retrieve it from the asset database.
                // It should already be loaded through a motion set.
                const char* motionFile = entry->GetFilename();
                AZ::Data::AssetId motionAssetId;
                EBUS_EVENT_RESULT(motionAssetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, motionFile, azrtti_typeid<MotionAsset>(), false);
                
                // if it failed to find it, it might be still compiling - try forcing an immediate compile:
                if (!motionAssetId.IsValid())
                {
                    AZ_TracePrintf("EMotionFX", "Motion \"%s\" is missing, requesting the asset system to compile it now.\n", motionFile);
                    AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::CompileAssetSync, motionFile);
                    // and then try again:
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(motionAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, motionFile, azrtti_typeid<MotionAsset>(), false);
                    if (motionAssetId.IsValid())
                    {
                        AZ_TracePrintf("EMotionFX", "Motion \"%s\" successfully compiled.\n", motionFile);
                    }
                }

                if (motionAssetId.IsValid())
                {
                    for (const auto& motionAsset : m_assetData->m_motionAssets)
                    {
                        if (motionAsset.GetId() == motionAssetId)
                        {
                            AZ_Assert(motionAsset, "Motion \"%s\" was found in the asset database, but is not initialized.", entry->GetFilename());
                            AZ_Error("EMotionFX", motionAsset.Get()->m_emfxMotion.get(), "Motion \"%s\" was found in the asset database, but is not valid.", entry->GetFilename());
                            return motionAsset.Get()->m_emfxMotion.get();
                        }
                    }
                }

                AZ_Error("EMotionFX", false, "Failed to locate motion \"%s\" in the asset database.", entry->GetFilename());
                return nullptr;
            }

            MotionSetAsset* m_assetData;
        };

        MotionSetAsset::MotionSetAsset(AZ::Data::AssetId id)
            : EMotionFXAsset(id)
        {}

        MotionSetAsset::~MotionSetAsset()
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        }

        void MotionSetAsset::SetData(EMotionFX::MotionSet* motionSet)
        {
            m_emfxMotionSet.reset(motionSet);
            m_status = static_cast<int>(AZ::Data::AssetData::AssetStatus::Ready);
        }

        //////////////////////////////////////////////////////////////////////////
        void MotionSetAsset::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            for (AZ::Data::Asset<MotionAsset>& motionAsset : m_motionAssets)
            {
                if (motionAsset.GetId() == asset.GetId())
                {
                    motionAsset = asset;

                    NotifyMotionSetModified(AZ::Data::Asset<MotionSetAsset>(this));

                    break;
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void MotionSetAsset::NotifyMotionSetModified(const AZ::Data::Asset<MotionSetAsset>& asset)
        {
            // When a dependent motion reloads, consider the motion set reloaded as well.
            // This allows characters using this motion set to refresh state and reference the new motions.
            if (!asset.Get()->m_isReloadPending)
            {
                AZStd::function<void()> notifyReload = [asset]()
                    {
                        using namespace AZ::Data;
                        AssetBus::Event(asset.GetId(), &AssetBus::Events::OnAssetReloaded, asset);
                        asset.Get()->m_isReloadPending = false;
                    };
                AZ::TickBus::QueueFunction(notifyReload);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        bool MotionSetAssetHandler::OnInitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            MotionSetAsset* assetData = asset.GetAs<MotionSetAsset>();
            EMotionFX::Importer::MotionSetSettings motionSettings;
            motionSettings.m_isOwnedByRuntime = true;
            assetData->m_emfxMotionSet.reset(EMotionFX::GetImporter().LoadMotionSet(
                assetData->m_emfxNativeData.data(),
                assetData->m_emfxNativeData.size(),
                &motionSettings));

            if (!assetData->m_emfxMotionSet)
            {
                AZ_Error("EMotionFX", false, "Failed to initialize motion set asset %s", asset.GetHint().c_str());
                return false;
            }

            // The following code is required to be set so the FileManager detects changes to the files loaded
            // through this method. Once EMotionFX is integrated to the asset system this can go away.
            AZStd::string assetFilename;
            EBUS_EVENT_RESULT(assetFilename, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, asset.GetId());
            const char* devAssetsPath = AZ::IO::FileIOBase::GetInstance()->GetAlias("@devassets@");
            if (devAssetsPath)
            {
                AZStd::string assetSourcePath = devAssetsPath;

                AzFramework::StringFunc::AssetDatabasePath::Normalize(assetSourcePath);
                AZStd::string filename;
                AzFramework::StringFunc::AssetDatabasePath::Join(assetSourcePath.c_str(), assetFilename.c_str(), filename, true);

                assetData->m_emfxMotionSet->SetFilename(filename.c_str());
            }
            else
            {
                if (GetEMotionFX().GetIsInEditorMode())
                {
                    AZ_Warning("EMotionFX", false, "Failed to retrieve asset source path with alias '@devassets@'. Cannot set absolute filename for '%s'", assetFilename.c_str());
                }
                assetData->m_emfxMotionSet->SetFilename(assetFilename.c_str());
            }
            
            // now load them in:
            const EMotionFX::MotionSet::MotionEntries& motionEntries = assetData->m_emfxMotionSet->GetMotionEntries();
            // Get the motions in the motion set.  Escalate them to the top of the build queue first so that they can be done in parallel.
            // This call is fire-and-forget and is very lightweight.
            for (const auto& item : motionEntries)
            {
                const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;
                const char* motionFilename = motionEntry->GetFilename();
                AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetBySearchTerm, motionFilename);
            }
            
            // now that they're all escalated, the asset processor will be processing them across all threads, and we can request them one by one:
            for (const auto& item : motionEntries)
            {
                const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;
                const char* motionFilename = motionEntry->GetFilename();

                // Find motion file in catalog and grab the asset.
                // Jump on the AssetBus for the asset, and queue load.
                AZ::Data::AssetId motionAssetId;
                EBUS_EVENT_RESULT(motionAssetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, motionFilename, AZ::Data::s_invalidAssetType, false);

                // if it failed to find it, it might be still compiling - try forcing an immediate compile.  CompileAssetSync
                // will block until the compilation completes AND the catalog is up to date.
                if (!motionAssetId.IsValid())
                {
                    AZ_TracePrintf("EMotionFX", "Motion \"%s\" is missing, requesting the asset system to compile it now.\n", motionFilename);
                    AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::CompileAssetSync, motionFilename);
                    // and then try again:
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(motionAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, motionFilename, azrtti_typeid<MotionAsset>(), false);
                    if (motionAssetId.IsValid())
                    {
                        AZ_TracePrintf("EMotionFX", "Motion \"%s\" successfully compiled.\n", motionFilename);
                    }
                }

                if (motionAssetId.IsValid())
                {
                    AZ::Data::Asset<MotionAsset> motionAsset = AZ::Data::AssetManager::Instance().
                        GetAsset<MotionAsset>(motionAssetId, true, nullptr, true);

                    if (motionAsset)
                    {
                        assetData->BusConnect(motionAssetId);
                        assetData->m_motionAssets.push_back(motionAsset);
                    }
                    else
                    {
                        AZ_Warning("EMotionFX", false, "Motion \"%s\" in motion set \"%s\" could not be loaded.", motionFilename, assetFilename.c_str());
                    }
                }
                else
                {
                    AZ_Warning("EMotionFX", false, "Motion \"%s\" in motion set \"%s\" could not be found in the asset catalog.", motionFilename, assetFilename.c_str());
                }
            }

            // Set motion set's motion load callback, so if EMotion FX queries back for a motion,
            // we can pull the one managed through an AZ::Asset.
            assetData->m_emfxMotionSet->SetCallback(aznew CustomMotionSetCallback(asset));

            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Data::AssetType MotionSetAssetHandler::GetAssetType() const
        {
            return azrtti_typeid<MotionSetAsset>();
        }

        //////////////////////////////////////////////////////////////////////////
        void MotionSetAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
        {
            extensions.push_back("motionset");
        }

        //////////////////////////////////////////////////////////////////////////
        const char* MotionSetAssetHandler::GetAssetTypeDisplayName() const
        {
            return "EMotion FX Motion Set";
        }
        //////////////////////////////////////////////////////////////////////////
        const char* MotionSetAssetHandler::GetBrowserIcon() const
        {
            return "Editor/Images/AssetBrowser/MotionSet_16.svg";
        }
        //////////////////////////////////////////////////////////////////////////
        void MotionSetAssetBuilderHandler::InitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset, bool loadStageSucceeded, bool isReload)
        {
            // Don't need to load the referenced motionset and motion assets since we only care about the product ID ot relative path of the product dependency
            AZ_UNUSED(asset);
            AZ_UNUSED(loadStageSucceeded);
            AZ_UNUSED(isReload);
        }

        bool MotionSetAssetBuilderHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            AZ_UNUSED(asset);
            AZ_UNUSED(stream);
            AZ_UNUSED(assetLoadFilterCB);

            return true;
        }

        bool MotionSetAssetBuilderHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            AZ_UNUSED(asset);
            AZ_UNUSED(assetPath);
            AZ_UNUSED(assetLoadFilterCB);

            return true;
        }

    } // namespace Integration
} // namespace EMotionFX
