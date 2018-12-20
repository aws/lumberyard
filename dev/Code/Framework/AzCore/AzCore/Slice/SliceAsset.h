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
#ifndef AZCORE_SLICE_ASSET_H
#define AZCORE_SLICE_ASSET_H

#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    class SliceComponent;
    class Entity;

    /**
     * Represents a Slice asset.
     */
    class SliceAsset
        : public Data::AssetData
    {
        friend class SliceAssetHandler;

    public:
        AZ_CLASS_ALLOCATOR(SliceAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(SliceAsset, "{C62C7A87-9C09-4148-A985-12F2C99C0A45}", AssetData);

        SliceAsset(const Data::AssetId& assetId = Data::AssetId());
        ~SliceAsset();

        /**
         * Overwrites the current asset data (if any) and set the asset ready (if not).
         * \param deleteExisting deletes the existing entity on set (if one exist)
         * \returns true of data was set, or false if the asset is currently loading.
         */
        bool SetData(Entity* entity, SliceComponent* component, bool deleteExisting = true);

        Entity* GetEntity()
        {
            return m_entity;
        }

        SliceComponent* GetComponent()
        {
            return m_component;
        }

        virtual SliceAsset* Clone();

        static const char* GetFileFilter()
        {
            return "*.slice";
        }

    protected:
        Entity* m_entity; ///< Root entity that should contain only the slice component
        SliceComponent* m_component; ///< Slice component for this asset
    };

    /**
     * Represents an exported Dynamic Slice.
     */
    class DynamicSliceAsset
        : public SliceAsset
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicSliceAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(DynamicSliceAsset, "{78802ABF-9595-463A-8D2B-D022F906F9B1}", SliceAsset);

        DynamicSliceAsset(const Data::AssetId& assetId = Data::AssetId())
            : SliceAsset(assetId)
        {
        }
        ~DynamicSliceAsset() = default;

        static const char* GetFileFilter()
        {
            return "*.dynamicslice";
        }
    };

    /// @deprecated Use SliceComponent.
    using PrefabComponent = SliceComponent;

    /// @deprecated Use SliceAsset.
    using PrefabAsset = SliceAsset;

    /// @deprecated Use DynamicSliceAsset.
    using DynamicPrefabAsset = DynamicSliceAsset;

    namespace Data
    {
        /// Asset filter helper for stripping all assets except slices.
        bool AssetFilterSourceSlicesOnly(const AZ::Data::Asset<AZ::Data::AssetData>& asset);
    }

} // namespace AZ

#endif // AZCORE_SLICE_ASSET_H
#pragma once
