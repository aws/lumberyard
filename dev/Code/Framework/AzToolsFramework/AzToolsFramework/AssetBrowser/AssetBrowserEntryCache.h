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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

namespace AZ
{
    namespace Data
    {
        struct AssetId;
    }
    struct Uuid;
    template<class T> class EnvironmentVariable;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class ProductAssetBrowserEntry;
        class SourceAssetBrowserEntry;
        /**
        * This exists to handle memory caches that the AssetBrowser system needs
        * which need to be available across DLLs but still with managed life cycles and not leaking memory
        */
        class EntryCache
        {
            AZ_CLASS_ALLOCATOR(EntryCache, AZ::SystemAllocator, 0);
        public:
            static EntryCache* GetInstance();

            // life cycle management:
            static void CreateInstance();
            static void DestroyInstance();

            AZStd::unordered_map<AZ::Data::AssetId, ProductAssetBrowserEntry*> m_assetIdMap;
            AZStd::unordered_map<AZ::Uuid, SourceAssetBrowserEntry*> m_sourceIdMap;
            AZStd::unordered_set<AssetBrowserEntry*> m_dirtyThumbnailsSet;
            static const char* s_environmentVariableName;
            static AZ::EnvironmentVariable<EntryCache*> g_globalInstance;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
