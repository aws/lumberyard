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

#include <ExampleBuilder/Source/ExampleBuilderComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Debug/Trace.h>

namespace ExampleBuilder
{
    BuilderPluginComponent::BuilderPluginComponent()
    {
        // AZ Components should only initialize their members to null and empty in constructor
        // after construction, they may be deserialized from file.
    }

    BuilderPluginComponent::~BuilderPluginComponent()
    {
    }

    void BuilderPluginComponent::Init()
    {
        // init is where you'd actually allocate memory or create objects since it happens after deserialization.
    }

    void BuilderPluginComponent::Activate()
    {
        // activate is where you'd perform registration with other objects and systems.

        // since we want to register our builder, we do that here:
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "Example Worker Builder";
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.example", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.exampleinclude", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.examplesource", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_busId = ExampleBuilderWorker::GetUUID();
        builderDescriptor.m_createJobFunction = AZStd::bind(&ExampleBuilderWorker::CreateJobs, &m_exampleBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&ExampleBuilderWorker::ProcessJob, &m_exampleBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        m_exampleBuilder.BusConnect(builderDescriptor.m_busId);

        EBUS_EVENT(AssetBuilderSDK::AssetBuilderBus, RegisterBuilderInformation, builderDescriptor);
    }

    void BuilderPluginComponent::Deactivate()
    {
        m_exampleBuilder.BusDisconnect();
    }

    void BuilderPluginComponent::Reflect(AZ::ReflectContext* context)
    {
        // components also get Reflect called automatically
        // this is your opportunity to perform static reflection or type registration of any types you want the serializer to know about
    }

    
    ExampleBuilderWorker::ExampleBuilderWorker()
    {
    }
    ExampleBuilderWorker::~ExampleBuilderWorker()
    {
    }

    void ExampleBuilderWorker::ShutDown()
    {
        // it is important to note that this will be called on a different thread than your process job thread
        m_isShuttingDown = true;
    }

    // this happens early on in the file scanning pass
    // this function should consistently always create the same jobs, and should do no checking whether the job is up to date or not - just be consistent.
    void ExampleBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        AZStd::string ext;
        AzFramework::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), ext, false);
        if (AzFramework::StringFunc::Equal(ext.c_str(), "example"))
        {
            // since we're a source file, we also add a job to do the actual compilation:
            for (size_t idx = 0; idx < request.GetEnabledPlatformsCount(); idx++)
            {
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = "Compile Example";
                descriptor.m_platform = descriptor.m_platform = request.GetEnabledPlatformAt(idx);

                // you can also place whatever parameters you want to save for later into this map:
                descriptor.m_jobParameters[AZ_CRC("hello", 0x3610a686)] = "World";
                response.m_createJobOutputs.push_back(descriptor);
                // One builder can make multiple jobs for the same source file, for the same platform, as long as it emits a different job key per job. 
                // This allows you to break large compilations up into smaller jobs. Jobs emitted in this manner may be run in paralell.
                descriptor.m_jobKey = "Second Compile Example";
                response.m_createJobOutputs.push_back(descriptor);
            }

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return;
        }

        // This example shows how you would be able to declare source file dependencies on source files inside a builder and forward the info to the asset processor
        // Basically here we are creating source file dependencies as follows 
        // the source file .../test.examplesource depends on the source file  .../test.exampleinclude and 
        // the source file .../test.exampleinclude depends on the source file .../common.exampleinclude
        // the source file .../common.exampleinclude depends on the non-source file .../common.examplefile
        // Important to note that both file extensions "exampleinclude" and "examplesource" are being handled by this builder.
        // Also important to note is that files with extension "exampleinclude" are not creating any jobdescriptor here, which imply they do not create any jobs.
        AssetBuilderSDK::SourceFileDependency sourceFileDependencyInfo;
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullPath, false);
        AzFramework::StringFunc::Path::Normalize(fullPath);
        AZStd::string relPath = request.m_sourceFile;

        // source files in this example generate dependencies and also generate a job to compile it for each platform:
        if (AzFramework::StringFunc::Equal(ext.c_str(), "examplesource"))
        {
            AzFramework::StringFunc::Path::ReplaceExtension(relPath, "exampleinclude");
            // declare and add the dependency on the .exampleinclude file:
            sourceFileDependencyInfo.m_sourceFileDependencyPath = relPath;
            response.m_sourceFileDependencyList.push_back(sourceFileDependencyInfo);

            // since we're a source file, we also add a job to do the actual compilation:
            for (size_t idx = 0; idx < request.GetEnabledPlatformsCount(); idx++)
            {
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = "Compile Example";
                descriptor.m_platform = request.GetEnabledPlatformAt(idx);

                // you can also place whatever parameters you want to save for later into this map:
                descriptor.m_jobParameters[AZ_CRC("hello", 0x3610a686)] = "World";
                response.m_createJobOutputs.push_back(descriptor);
            }
        }
        else if (AzFramework::StringFunc::Equal(ext.c_str(), "exampleinclude"))
        {
            if (AzFramework::StringFunc::Find(request.m_sourceFile.c_str(), "common.exampleinclude") != AZStd::string::npos)
            {
                // Add any dependencies that common.exampleinclude would like to depend on here, we can also add a non source file as a dependency like we are doing here
                sourceFileDependencyInfo.m_sourceFileDependencyPath = "common.examplefile";
            }
            else
            {
                AzFramework::StringFunc::Path::ReplaceFullName(fullPath, "common.exampleinclude");
                // Assigning full path to sourceFileDependency path
                sourceFileDependencyInfo.m_sourceFileDependencyPath = fullPath;
            }

            response.m_sourceFileDependencyList.push_back(sourceFileDependencyInfo);
        }
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    // later on, this function will be called for jobs that actually need doing.
    // the request will contain the CreateJobResponse you constructed earlier, including any keys and values you placed into the hash table
    void ExampleBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        // Here's an example of listening for cancellation requests.  You should listen for cancellation requests and then cancel work if possible.
        //You can always derive from the Job Cancel Listener and reimplement Cancel() if you need to do more things such as signal a semaphore or other threading work.

        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Starting Job.");
        AZStd::string fileName;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), fileName);
        AZStd::string ext;
        AzFramework::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), ext, false);

        if (AzFramework::StringFunc::Equal(ext.c_str(), "example"))
        {
            if (AzFramework::StringFunc::Equal(request.m_jobDescription.m_jobKey.c_str(), "Compile Example"))
            {
                AzFramework::StringFunc::Path::ReplaceExtension(fileName, "example1");
            }
            else if (AzFramework::StringFunc::Equal(request.m_jobDescription.m_jobKey.c_str(), "Second Compile Example"))
            {
                AzFramework::StringFunc::Path::ReplaceExtension(fileName, "example2");
            }
        }
        else if (AzFramework::StringFunc::Equal(ext.c_str(), "examplesource"))
        {
            AzFramework::StringFunc::Path::ReplaceExtension(fileName, "examplesourceprocessed");
        }
        AZStd::string destPath;

        // you should do all your work inside the tempDirPath.
        // don't write outside this path
        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileName.c_str(), destPath, true);

        // use AZ_TracePrintF to communicate job details.  The logging system will automatically file the text under the appropriate log file and category.

        AZ::IO::LocalFileIO fileIO;
        if (!m_isShuttingDown && !jobCancelListener.IsCancelled() && fileIO.Copy(request.m_fullPath.c_str(), destPath.c_str()) == AZ::IO::ResultCode::Success)
        {
            // if you succeed in building assets into your temp dir, you should push them back into the response's product list
            // Assets you created in your temp path can be specified using paths relative to the temp path, since that is assumed where you're writing stuff.
            AZStd::string relPath = destPath;
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

            AssetBuilderSDK::JobProduct jobProduct(fileName);
            response.m_outputProducts.push_back(jobProduct);
        }
        else
        {
            if (m_isShuttingDown)
            {
                AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Cancelled job %s because shutdown was requested", request.m_fullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            }
            else if (jobCancelListener.IsCancelled())
            {
                AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Cancelled was requested for job %s", request.m_fullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            }
            else
            {
                AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Error during processing job %s.", request.m_fullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            }
        }
    }

    AZ::Uuid ExampleBuilderWorker::GetUUID()
    {
        return AZ::Uuid::CreateString("C163F950-BF25-4D60-90D7-8E181E25A9EA");
    }
}
