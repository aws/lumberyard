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

#include <Builder/TestAssetBuilderComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace TestAssetBuilder
{
    TestAssetBuilderComponent::TestAssetBuilderComponent()
    {
    }

    TestAssetBuilderComponent::~TestAssetBuilderComponent()
    {
    }

    void TestAssetBuilderComponent::Init()
    {
    }

    void TestAssetBuilderComponent::Activate()
    {
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = " Test Asset Builder";
        builderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.source", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.dependent", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_busId = azrtti_typeid<TestAssetBuilderComponent>();
        builderDescriptor.m_createJobFunction = AZStd::bind(&TestAssetBuilderComponent::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&TestAssetBuilderComponent::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

        BusConnect(builderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDescriptor);
    }

    void TestAssetBuilderComponent::Deactivate()
    {
        BusDisconnect();
    }

    void TestAssetBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TestAssetBuilderComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
            ;
        }
    }

    void TestAssetBuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("TestAssetBuilderPluginService", 0xa380f578));
    }

    void TestAssetBuilderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("TestAssetBuilderPluginService", 0xa380f578));
    }

    void TestAssetBuilderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void TestAssetBuilderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void TestAssetBuilderComponent::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        AZStd::string ext;
        AzFramework::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), ext, false);

        if (AzFramework::StringFunc::Equal(ext.c_str(), "dependent"))
        {
            // Since we're a source file, we also add a job to do the actual compilation (for each enabled platform)
            for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
            {
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = "Compile Example";
                descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
                response.m_createJobOutputs.push_back(descriptor);
            }

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return;
        }
        else if (AzFramework::StringFunc::Equal(ext.c_str(), "source"))
        {
            for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
            {
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = "Compile Example";
                descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());

                // add a dependency on the other job:
                AssetBuilderSDK::SourceFileDependency sourceFile;
                sourceFile.m_sourceFileDependencyPath = request.m_sourceFile.c_str();
                AzFramework::StringFunc::Path::ReplaceExtension(sourceFile.m_sourceFileDependencyPath, "dependent");
                descriptor.m_jobDependencyList.push_back({ "Compile Example", platformInfo.m_identifier.c_str(), AssetBuilderSDK::JobDependencyType::Order, sourceFile });

                response.m_createJobOutputs.push_back(descriptor);
            }
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return;
        }

        AZ_Assert(false, "Unhandled extension type in TestAssetBuilderWorker.");
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
    }

    void TestAssetBuilderComponent::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Starting Job.\n");

        AZ::IO::FileIOBase* fileIO = AZ::IO::LocalFileIO::GetInstance();
        AZStd::string outputData;

        AZ::IO::HandleType sourcefileHandle;
        if (!fileIO->Open(request.m_fullPath.c_str(), AZ::IO::OpenMode::ModeRead, sourcefileHandle))
        {
            AZ_Error("AssetBuilder", false, " Unable to open file ( %s ).", request.m_fullPath.c_str());
            return;
        }

        AZ::u64 sourceSizeBytes = 0;
        if (fileIO->Size(sourcefileHandle, sourceSizeBytes))
        {
            outputData.resize(sourceSizeBytes);
            if (!fileIO->Read(sourcefileHandle, outputData.data(), sourceSizeBytes))
            {
                AZ_Error("AssetBuilder", false, " Unable to read file ( %s ).", request.m_fullPath.c_str());
                fileIO->Close(sourcefileHandle);
                return;
            }
        }

        fileIO->Close(sourcefileHandle);

        AZStd::string fileName = request.m_sourceFile.c_str();
        AZStd::string ext;
        AzFramework::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), ext, false);
        
        if (AzFramework::StringFunc::Equal(ext.c_str(), "source"))
        {
            AzFramework::StringFunc::Path::ReplaceExtension(fileName, "dependentprocessed");
            // By default fileIO uses @asset@ alias therefore if we give fileIO a filename 
            // it will try to check in the cache instead of the source folder.
            AZ::IO::HandleType dependentfileHandle;
            if (!fileIO->Open(fileName.c_str(), AZ::IO::OpenMode::ModeRead, dependentfileHandle))
            {
                AZ_Error("AssetBuilder", false, " Unable to open file in cache ( %s ) while processing source ( %s ) ", fileName.c_str(), request.m_sourceFile.c_str());
                return;
            }

            AZ::u64 dependentsizeBytes = 0;
            if (fileIO->Size(dependentfileHandle, dependentsizeBytes))
            {
                outputData.resize(outputData.size() + dependentsizeBytes);
                if (!fileIO->Read(dependentfileHandle, outputData.data() + sourceSizeBytes, dependentsizeBytes))
                {
                    AZ_Error("AssetBuilder", false, " Unable to read file data from cache ( %s ).", fileName.c_str());
                    fileIO->Close(dependentfileHandle);
                    return;
                }
            }

            fileIO->Close(dependentfileHandle);

            // Validating AssetCatalogRequest API's here which operates on assetpath and assetid 
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, fileName.c_str(), AZ::Data::s_invalidAssetType, false);
            if (!assetId.IsValid())
            {
                AZ_Error("AssetBuilder", assetId.IsValid(), "GetAssetIdByPath - Asset id should be valid for this asset ( %s ).", fileName.c_str());
                return;
            }

            AZStd::string assetpath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetpath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, assetId);
            if (assetpath.empty())
            {
                AZ_Error("AssetBuilder", !assetpath.empty(), "Asset path should not be empty for this assetid ( %s ) ( %s )", assetId.ToString<AZStd::string>().c_str(), fileName.c_str());
                return;
            }

            AzFramework::StringFunc::Path::ReplaceExtension(fileName, "sourceprocessed");
        }
        else if (AzFramework::StringFunc::Equal(ext.c_str(), "dependent"))
        {
            AzFramework::StringFunc::Path::ReplaceExtension(fileName, "dependentprocessed");
        }

        // write the file to the cache at (temppath)/filenameOnly
        AZStd::string destPath;
        AZStd::string fileNameOnly;
        AzFramework::StringFunc::Path::GetFullFileName(fileName.c_str(), fileNameOnly); // this removes the path from fileName.
        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileNameOnly.c_str(), destPath, true);

        // Check if we are cancelled or shutting down before doing intensive processing on this source file
        if (jobCancelListener.IsCancelled())
        {
            AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Cancel was requested for job %s.\n", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }
        if (m_isShuttingDown)
        {
            AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Cancelled job %s because shutdown was requested.\n", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        AZ::IO::HandleType assetfileHandle;
        if (!fileIO->Open(destPath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, assetfileHandle))
        {
            AZ_Error("AssetBuilder", false, " Unable to open file for writing ( %s ).", destPath.c_str());
            return;
        }
        if (!fileIO->Write(assetfileHandle, outputData.data(), outputData.size()))
        {
            AZ_Error("AssetBuilder", false, " Unable to write to file data ( %s ).", destPath.c_str());
            fileIO->Close(assetfileHandle);
            return;
        }
        fileIO->Close(assetfileHandle);

        AssetBuilderSDK::JobProduct jobProduct(fileNameOnly);
        jobProduct.m_productSubID = 0;

        // once you've filled up the details of the product in jobProduct, add it to the result list:
        response.m_outputProducts.push_back(jobProduct);

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

    }

    void TestAssetBuilderComponent::ShutDown()
    {
        m_isShuttingDown = true;
    }

} // namespace TestAssetBuilder

