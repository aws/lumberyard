/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace WwiseBuilder
{
    //! Wwise builder is responsible for building material files
    class WwiseBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        WwiseBuilderWorker();
        ~WwiseBuilderWorker();

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override;

        //! Returns the UUID for this builder
        static AZ::Uuid GetUUID();

        AZ::Outcome<AZStd::string, AZStd::string> GatherProductDependencies(const AZStd::string& fullPath, const AZStd::string& relativePath, AssetBuilderSDK::ProductPathDependencySet& dependencies);
    
    private:
        bool m_isShuttingDown = false;
    };

    class BuilderPluginComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(BuilderPluginComponent, "{8630414A-0BA6-4759-809A-C6903994AE30}");
        static void Reflect(AZ::ReflectContext* context);

        BuilderPluginComponent();

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override;
        void Activate() override; 
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        virtual ~BuilderPluginComponent();

    private:
        WwiseBuilderWorker m_wwiseBuilder;
    };
}
