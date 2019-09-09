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

#include "MaterialBuilderComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Debug/Trace.h>

namespace MaterialBuilder
{
    BuilderPluginComponent::BuilderPluginComponent()
    {
    }

    BuilderPluginComponent::~BuilderPluginComponent()
    {
    }

    void BuilderPluginComponent::Init()
    {
    }

    void BuilderPluginComponent::Activate()
    {
        // Register material builder
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "MaterialBuilderWorker";
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.mtl", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_busId = MaterialBuilderWorker::GetUUID();
        builderDescriptor.m_version = 2;
        builderDescriptor.m_createJobFunction = AZStd::bind(&MaterialBuilderWorker::CreateJobs, &m_materialBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&MaterialBuilderWorker::ProcessJob, &m_materialBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);

        // (optimization) this builder does not emit source dependencies:
        builderDescriptor.m_flags |= AssetBuilderSDK::AssetBuilderDesc::BF_EmitsNoDependencies;
        
        m_materialBuilder.BusConnect(builderDescriptor.m_busId);

        EBUS_EVENT(AssetBuilderSDK::AssetBuilderBus, RegisterBuilderInformation, builderDescriptor);
    }

    void BuilderPluginComponent::Deactivate()
    {
        m_materialBuilder.BusDisconnect();
    }

    void BuilderPluginComponent::Reflect(AZ::ReflectContext* context)
    {
    }

    MaterialBuilderWorker::MaterialBuilderWorker()
    {
    }
    MaterialBuilderWorker::~MaterialBuilderWorker()
    {
    }

    void MaterialBuilderWorker::ShutDown()
    {
        // This will be called on a different thread than the process job thread
        m_isShuttingDown = true;
    }

    // This happens early on in the file scanning pass.
    // This function should always create the same jobs and not do any checking whether the job is up to date.
    void MaterialBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor descriptor;
            descriptor.m_jobKey = "Material Builder Job";
            descriptor.SetPlatformIdentifier(info.m_identifier.c_str());
            descriptor.m_priority = 8; // meshes are more important (at 10) but mats are still pretty important.
            response.m_createJobOutputs.push_back(descriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    // The request will contain the CreateJobResponse you constructed earlier, including any keys and
    // values you placed into the hash table
    void MaterialBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Starting Job.\n");
        AZStd::string fileName;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), fileName);
        AZStd::string destPath;

        // Do all work inside the tempDirPath.
        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileName.c_str(), destPath, true);

        AZ::IO::LocalFileIO fileIO;
        if (!m_isShuttingDown && fileIO.Copy(request.m_fullPath.c_str(), destPath.c_str()) == AZ::IO::ResultCode::Success)
        {
            // Push assets back into the response's product list
            // Assets you created in your temp path can be specified using paths relative to the temp path
            // since that is assumed where you're writing stuff.
            AZStd::string relPath = destPath;
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            AssetBuilderSDK::JobProduct jobProduct(fileName);
            response.m_outputProducts.push_back(jobProduct);
        }
        else
        {
            if (m_isShuttingDown)
            {
                AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Cancelled job %s because shutdown was requested.\n", request.m_fullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            }
            else
            {
                AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Error during processing job %s.\n", request.m_fullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            }
        }
    }

    AZ::Uuid MaterialBuilderWorker::GetUUID()
    {
        return AZ::Uuid::CreateString("{258D34AC-12F8-4196-B535-3206D8E7287B}");
    }
}
