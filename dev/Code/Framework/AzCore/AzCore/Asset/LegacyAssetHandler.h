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

#include <AzCore/Asset/AssetManager.h>

namespace AZ
{
    namespace Data
    {
        /**
        * Derive from LegacyAssetHandler if your AssetHandler blocks on a particular thread loading
        * your asset type, and clear your asset loading queue in ProcessQueuedAssetRequests when it's
        * called.
        *
        * Used for older, CryEngine asset types that block on the main thread. Generally, should not
        * be used. This will be deprecated at some point.
        *
        */
        class LegacyAssetHandler : public AssetHandler
        {
        public:
            AZ_RTTI(LegacyAssetHandler, "{9755D9D4-A21E-4EE5-BF2F-7A89FA1043A0}");

            // Called if the thread registered with AssetManager::RegisterLegacyHandler has an asset request that will be blocking,
            // to process / clear any queues on that thread first
            virtual void ProcessQueuedAssetRequests() = 0;
        };
    }  // namespace Data
}   // namespace AZ

