/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>

namespace AssetBuilderSDK
{
    struct CreateJobsRequest;
    struct CreateJobsResponse;
    struct ProcessJobRequest;
    struct ProcessJobResponse;
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace Events
        {
            struct ExportProduct;
        }
    }
}

namespace SceneBuilder
{
    class SceneBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        ~SceneBuilderWorker() override = default;

        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        void ShutDown() override;
        const char* GetFingerprint() const;
        static AZ::Uuid GetUUID();

    protected:
        bool LoadScene(AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& result, 
            const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);
        bool ExportScene(const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene, 
            const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        AZ::u32 BuildSubId(const AZ::SceneAPI::Events::ExportProduct& product) const;

        bool m_isShuttingDown = false;
        mutable AZStd::string m_cachedFingerprint;
    };
} // namespace SceneBuilder
