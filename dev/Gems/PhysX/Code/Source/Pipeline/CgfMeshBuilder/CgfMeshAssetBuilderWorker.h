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

#include <AzCore/RTTI/RTTI.h>

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace PhysX
{
    namespace Pipeline
    {
        class CgfMeshAssetBuilderWorker
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:
            AZ_RTTI(ExampleBuilderWorker, "{434AEC9C-6C98-46A5-9E5E-E5362863A9CB}");

            CgfMeshAssetBuilderWorker() = default;
            ~CgfMeshAssetBuilderWorker() = default;

            // Asset Builder Callback Functions
            void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
            void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

            // AssetBuilderSDK::AssetBuilderCommandBus interface
            void ShutDown() override;

        private:
            void CreatePhysXGemMeshAsset(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

            bool m_isShuttingDown = false;
        };
    }
}