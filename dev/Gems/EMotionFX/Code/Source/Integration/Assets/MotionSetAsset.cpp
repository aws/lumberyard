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
                EBUS_EVENT_RESULT(motionAssetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, motionFile, AZ::AzTypeInfo<MotionAsset>::Uuid(), false);
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

        //////////////////////////////////////////////////////////////////////////
        MotionSetAsset::MotionSetAsset()
            : m_isReloadPending(false)
        {
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
            assetData->m_emfxMotionSet = EMotionFXPtr<EMotionFX::MotionSet>::MakeFromNew(EMotionFX::GetImporter().LoadMotionSet(
                assetData->m_emfxNativeData.data(),
                assetData->m_emfxNativeData.size()));

            if (!assetData->m_emfxMotionSet)
            {
                AZ_Error("EMotionFX", false, "Failed to initialize motion set asset %s", asset.GetId().ToString<AZStd::string>().c_str());
                return false;
            }

            assetData->m_emfxMotionSet->SetIsOwnedByRuntime(true);

            // Get the motions in the motion set.
            const EMotionFX::MotionSet::EntryMap& motionEntries = assetData->m_emfxMotionSet->GetMotionEntries();
            for (const auto& item : motionEntries)
            {
                const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;
                const char* motionFilename = motionEntry->GetFilename();

                // Find motion file in catalog and grab the asset.
                // Jump on the AssetBus for the asset, and queue load.
                AZ::Data::AssetId motionAssetId;
                EBUS_EVENT_RESULT(motionAssetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, motionFilename, AZ::Data::s_invalidAssetType, false);
                if (motionAssetId.IsValid())
                {
                    AZ::Data::Asset<MotionAsset> motionAsset = AZ::Data::AssetManager::Instance().
                        GetAsset<MotionAsset>(motionAssetId, true, nullptr, true);

                    if (motionAsset)
                    {
                        assetData->BusConnect(motionAssetId);
                        // since the motion asset is added to the vector we need to increment the use count
                        motionAsset.Get()->Acquire();
                        assetData->m_motionAssets.push_back(motionAsset);
                    }
                    else
                    {
                        AZ_Warning("EMotionFX", false, "Motion \"%s\" in motion set could not be loaded.", motionFilename);
                    }
                }
                else
                {
                    AZ_Warning("EMotionFX", false, "Motion \"%s\" in motion set could not be found in the asset catalog.", motionFilename);
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
            return AZ::AzTypeInfo<MotionSetAsset>::Uuid();
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

    } // namespace Integration
} // namespace EMotionFX