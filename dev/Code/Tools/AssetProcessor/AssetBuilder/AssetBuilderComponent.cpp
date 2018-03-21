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

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/algorithm.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderComponent.h>
#include <AssetBuilderInfo.h>

// Command-line parameter options:
static const char* const s_paramHelp = "help"; // Print help information.
static const char* const s_paramTask = "task"; // Task to run.
static const char* const s_paramGameName = "gamename"; // Name of the current project.
static const char* const s_paramGameCache = "gamecache"; // Full path to the project cache folder.
static const char* const s_paramModule = "module"; // For resident mode, the path to the builder dll folder, otherwise the full path to a single builder dll to use.
static const char* const s_paramPort = "port"; // Optional, port number to use to connect to the AP.
static const char* const s_paramIp = "remoteip"; // optional, IP address to use to connect to the AP
static const char* const s_paramId = "id"; // UUID string that identifies the builder.  Only used for resident mode when the AP directly starts up the AssetBuilder.
static const char* const s_paramInput = "input"; // For non-resident mode, full path to the file containing the serialized job request.
static const char* const s_paramOutput = "output"; // For non-resident mode, full path to the file to write the job response to.
static const char* const s_paramDebug = "debug"; // Debug mode for the create and process job of the specified file.
static const char* const s_paramDebugCreate = "debug_create"; // Debug mode for the create job of the specified file.
static const char* const s_paramDebugProcess = "debug_process"; // Debug mode for the process job of the specified file.

// Task modes:
static const char* const s_taskResident = "resident"; // stays up and running indefinitely, accepting jobs via network connection
static const char* const s_taskRegisterBuilder = "register"; // outputs all the builder descriptors
static const char* const s_taskCreateJob = "create"; // runs a builders createJobs function
static const char* const s_taskProcessJob = "process"; // runs processJob function
static const char* const s_taskDebug = "debug"; // runs a one shot job in a fake environment for a specified file.
static const char* const s_taskDebugCreate = "debug_create"; // runs a one shot job in a fake environment for a specified file.
static const char* const s_taskDebugProcess = "debug_process"; // runs a one shot job in a fake environment for a specified file.

//////////////////////////////////////////////////////////////////////////

void AssetBuilderComponent::PrintHelp()
{
    AZ_TracePrintf("Help", "\nAssetBuilder is part of the Asset Processor so tasks are run in an isolated environment.\n");
    AZ_TracePrintf("Help", "The following command line options are available for the AssetBuilder.\n");
    AZ_TracePrintf("Help", "%s - Print help information.\n", s_paramHelp);
    AZ_TracePrintf("Help", "%s - Task to run.\n", s_paramTask);
    AZ_TracePrintf("Help", "%s - Name of the current project.\n", s_paramGameName);
    AZ_TracePrintf("Help", "%s - Full path to the project cache folder.\n", s_paramGameCache);
    AZ_TracePrintf("Help", "%s - For resident mode, the path to the builder dll folder, otherwise the full path to a single builder dll to use.\n", s_paramModule);
    AZ_TracePrintf("Help", "%s - Optional, port number to use to connect to the AP.\n", s_paramPort);
    AZ_TracePrintf("Help", "%s - UUID string that identifies the builder.  Only used for resident mode when the AP directly starts up the AssetBuilder.\n", s_paramId);
    AZ_TracePrintf("Help", "%s - For non-resident mode, full path to the file containing the serialized job request.\n", s_paramInput);
    AZ_TracePrintf("Help", "%s - For non-resident mode, full path to the file to write the job response to.\n", s_paramOutput);
    AZ_TracePrintf("Help", "%s - Debug mode for the create and process job of the specified file.\n", s_paramDebug);
    AZ_TracePrintf("Help", "  Debug mode optionally uses -%s, -%s, -%s, -%s and -gameroot.\n", s_paramInput, s_paramOutput, s_paramModule, s_paramPort);
    AZ_TracePrintf("Help", "  Example: -%s Objects\\Tutorials\\shapes.fbx\n", s_paramDebug);
    AZ_TracePrintf("Help", "%s - Debug mode for the create job of the specified file.\n", s_paramDebugCreate);
    AZ_TracePrintf("Help", "%s - Debug mode for the process job of the specified file.\n", s_paramDebugProcess);
}

bool AssetBuilderComponent::IsInDebugMode(const AzFramework::CommandLine& commandLine)
{
    if (commandLine.HasSwitch(s_paramDebug) || commandLine.HasSwitch(s_paramDebugCreate) || commandLine.HasSwitch(s_paramDebugProcess))
    {
        return true;
    }

    if (commandLine.HasSwitch(s_paramTask))
    {
        const AZStd::string& task = commandLine.GetSwitchValue(s_paramTask, 0);
        if (task == s_taskDebug || task == s_taskDebugCreate || task == s_taskDebugProcess)
        {
            return true;
        }
    }

    return false;
}

void AssetBuilderComponent::Activate()
{
    BuilderBus::Handler::BusConnect();
    AssetBuilderSDK::AssetBuilderBus::Handler::BusConnect();
}

void AssetBuilderComponent::Deactivate()
{
    BuilderBus::Handler::BusDisconnect();
    AssetBuilderSDK::AssetBuilderBus::Handler::BusDisconnect();
    AzFramework::EngineConnectionEvents::Bus::Handler::BusDisconnect();
}

void AssetBuilderComponent::Reflect(AZ::ReflectContext* context)
{
    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<AssetBuilderComponent, AZ::Component>()
            ->Version(1);
    }
}

bool AssetBuilderComponent::Run()
{
    const AzFramework::CommandLine* commandLine = nullptr;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);
    if (commandLine->HasSwitch(s_paramHelp))
    {
        PrintHelp();
        return true;
    }

    AZStd::string task;

    AZStd::string debugFile;
    if (GetParameter(s_paramDebug, debugFile, false))
    {
        task = s_taskDebug;
    }
    else if (GetParameter(s_paramDebugCreate, debugFile, false))
    {
        task = s_taskDebugCreate;
    }
    else if (GetParameter(s_paramDebugProcess, debugFile, false))
    {
        task = s_taskDebugProcess;
    }
    else if (!GetParameter(s_paramTask, task))
    {
        AZ_Error("AssetBuilder", false, "No task specified. Use -help for options.");
        return false;
    }

    bool isDebugTask = (task == s_taskDebug || task == s_taskDebugCreate || task == s_taskDebugProcess);
    if (!GetParameter(s_paramGameName, m_gameName, !isDebugTask) || !GetParameter(s_paramGameCache, m_gameCache, !isDebugTask))
    {
        if (!isDebugTask)
        {
            return false;
        }
    }

    AssetBuilderSDK::InitializeSerializationContext();

    if (!ConnectToAssetProcessor())
    {
        //AP connection is required to access the asset catalog
        AZ_Error("AssetBuilder", false, "Failed to establish a network connection to the AssetProcessor. Use -help for options.");
        return false;
    }

    bool result = false;

    if (task == s_taskResident)
    {
        result = RunInResidentMode();
    }
    else if (task == s_taskDebug)
    {
        static const bool runCreateJobs = true;
        static const bool runProcessJob = true;
        result = RunDebugTask(AZStd::move(debugFile), runCreateJobs, runProcessJob);
    }
    else if (task == s_taskDebugCreate)
    {
        static const bool runCreateJobs = true;
        static const bool runProcessJob = false;
        result = RunDebugTask(AZStd::move(debugFile), runCreateJobs, runProcessJob);
    }
    else if (task == s_taskDebugProcess)
    {
        static const bool runCreateJobs = false;
        static const bool runProcessJob = true;
        result = RunDebugTask(AZStd::move(debugFile), runCreateJobs, runProcessJob);
    }
    else
    {
        result = RunOneShotTask(task);
    }

    AZ_Error("AssetBuilder", result, "Failed to handle `%s` request", task.c_str());

    bool disconnected = false;
    AzFramework::AssetSystemRequestBus::BroadcastResult(disconnected, &AzFramework::AssetSystem::AssetSystemRequests::Disconnect);
    AZ_Error("AssetBuilder", disconnected, "Failed to disconnect from Asset Processor.");
    
    UnloadBuilders();

    return result;
}

bool AssetBuilderComponent::ConnectToAssetProcessor()
{
    AZStd::string port, ip;

    if (GetParameter(s_paramPort, port, false))
    {
        int portNumber = AZStd::stoi(port);
        AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::SetAssetProcessorPort, portNumber);
    }

    if (!GetParameter(s_paramIp, ip, false))
    {
        ip = "127.0.0.1"; // default to localhost
    }

    AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::SetAssetProcessorIP, ip);

    bool connected = false;
    AzFramework::AssetSystemRequestBus::BroadcastResult(connected, &AzFramework::AssetSystem::AssetSystemRequests::Connect, "Asset Builder");

    return connected;
}

//////////////////////////////////////////////////////////////////////////

bool AssetBuilderComponent::RunInResidentMode()
{
    using namespace AssetBuilderSDK;
    using namespace AZStd::placeholders;

    AZStd::string port, id, builderFolder;

    if (!GetParameter(s_paramId, id)
        || !GetParameter(s_paramModule, builderFolder))
    {
        return false;
    }

    if (!LoadBuilders(builderFolder))
    {
        return false;
    }

    AzFramework::SocketConnection::GetInstance()->AddMessageHandler(CreateJobsNetRequest::MessageType(), AZStd::bind(&AssetBuilderComponent::CreateJobsResidentHandler, this, _1, _2, _3, _4));
    AzFramework::SocketConnection::GetInstance()->AddMessageHandler(ProcessJobNetRequest::MessageType(), AZStd::bind(&AssetBuilderComponent::ProcessJobResidentHandler, this, _1, _2, _3, _4));

    BuilderHelloRequest request;
    BuilderHelloResponse response;

    request.m_uuid = AZ::Uuid::CreateString(id.c_str());

    bool result = AzFramework::AssetSystem::SendRequest(request, response);

    AZ_Error("AssetBuilder", result, "Failed to send hello request to Asset Processor");
    // This error is only shown if we successfully got a response AND the response explicitly indicates the AP rejected the builder
    AZ_Error("AssetBuilder", !result || response.m_accepted, "Asset Processor rejected connection request");

    if (result && response.m_accepted)
    {
        m_running = true;

        m_jobThreadDesc.m_name = "Builder Job Thread";
        m_jobThread = AZStd::thread(AZStd::bind(&AssetBuilderComponent::JobThread, this), &m_jobThreadDesc);

        AzFramework::EngineConnectionEvents::Bus::Handler::BusConnect(); // Listen for disconnects

        AZ_TracePrintf("AssetBuilder", "Builder ID: %s\n", response.m_uuid.ToString<AZStd::string>().c_str());
        AZ_TracePrintf("AssetBuilder", "Resident mode ready\n");
        m_mainEvent.acquire();
        AZ_TracePrintf("AssetBuilder", "Shutting down\n");

        m_running = false;
    }

    if (m_jobThread.joinable())
    {
        m_jobEvent.release();
        m_jobThread.join();
    }

    return result;
}

bool AssetBuilderComponent::RunDebugTask(AZStd::string&& debugFile, bool runCreateJobs, bool runProcessJob)
{
    if (debugFile.empty())
    {
        if (!GetParameter(s_paramInput, debugFile))
        {
            AZ_Error("AssetBuilder", false, "No input file was specified. Use -help for options.");
            return false;
        }
    }
    AzFramework::StringFunc::Path::Normalize(debugFile);

    bool result = false;
    AZ::Data::AssetInfo info;
    AZStd::string watchFolder;
    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, debugFile.c_str(), info, watchFolder);
    if (!result)
    {
        AZ_Error("AssetBuilder", false, "Failed to locate asset info for '%s'.", debugFile.c_str());
        return false;
    }

    AZStd::string binDir;
    AZStd::string module;
    if (GetParameter(s_paramModule, module, false))
    {
        AzFramework::StringFunc::Path::GetFullPath(module.c_str(), binDir);
        if (!LoadBuilder(module))
        {
            AZ_Error("AssetBuilder", false, "Failed to load module '%s'.", module.c_str());
            return false;
        }
    }
    else
    {
        const char* executableFolder = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(executableFolder, &AZ::ComponentApplicationBus::Events::GetExecutableFolder);
        if (!executableFolder)
        {
            AZ_Error("AssetBuilder", false, "Unable to determine application root.");
            return false;
        }

        AzFramework::StringFunc::Path::Join(executableFolder, "Builders", binDir);
        
        if (!LoadBuilders(binDir))
        {
            AZ_Error("AssetBuilder", false, "Failed to load one or more builders from '%s'.", binDir);
            return false;
        }
    }
    
    AZStd::string baseTempDirPath;
    if (!GetParameter(s_paramOutput, baseTempDirPath, false))
    {
        AZStd::string fileName;
        AzFramework::StringFunc::Path::GetFullFileName(debugFile.c_str(), fileName);
        AZStd::replace(fileName.begin(), fileName.end(), '.', '_');
        
        AzFramework::StringFunc::Path::Join(binDir.c_str(), "Debug", baseTempDirPath);
        AzFramework::StringFunc::Path::Join(baseTempDirPath.c_str(), fileName.c_str(), baseTempDirPath);
    }
    auto* fileIO = AZ::IO::FileIOBase::GetInstance();

    for (auto& it : m_assetBuilderDescMap)
    {
        AZStd::unique_ptr<AssetBuilderSDK::AssetBuilderDesc>& builder = it.second;
        AZ_Assert(builder, "Invalid description for builder registered.");
        if (!IsBuilderForFile(debugFile, *builder))
        {
            AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Skipping '%s'.\n", builder->m_name.c_str());
            continue;
        }
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Debugging builder '%s'.\n", builder->m_name.c_str());
        
        AZStd::string tempDirPath;
        AzFramework::StringFunc::Path::Join(baseTempDirPath.c_str(), builder->m_name.c_str(), tempDirPath);

        AZStd::vector<AssetBuilderSDK::PlatformInfo> enabledDebugPlatformInfos =
        {
            {"debug platform", {"tools", "debug"}}
        };

        AZStd::vector<AssetBuilderSDK::JobDescriptor> jobDescriptions;
        if (runCreateJobs)
        {
            AZStd::string createJobsTempDirPath;
            AzFramework::StringFunc::Path::Join(tempDirPath.c_str(), "CreateJobs", createJobsTempDirPath);
            AZ::IO::Result fileResult = fileIO->CreatePath(createJobsTempDirPath.c_str());
            if (!fileResult)
            {
                AZ_Error("AssetBuilder", false, "Unable to create or clear debug folder '%s'.", createJobsTempDirPath.c_str());
                return false;
            }

            AssetBuilderSDK::CreateJobsRequest createRequest(builder->m_busId, info.m_relativePath, watchFolder,
                enabledDebugPlatformInfos, info.m_assetId.m_guid);
            
            AZ_TraceContext("Source", debugFile);
            AZ_TraceContext("Platforms", AssetBuilderSDK::PlatformInfo::PlatformVectorAsString(createRequest.m_enabledPlatforms));

            AssetBuilderSDK::CreateJobsResponse createResponse;
            builder->m_createJobFunction(createRequest, createResponse);

            AZStd::string responseFile;
            AzFramework::StringFunc::Path::ConstructFull(createJobsTempDirPath.c_str(), "CreateJobsResponse.xml", responseFile);
            if (!AZ::Utils::SaveObjectToFile(responseFile, AZ::DataStream::ST_XML, &createResponse))
            {
                AZ_Error("AssetBuilder", false, "Failed to serialize response to file: %s", responseFile.c_str());
                return false;
            }

            if (runProcessJob)
            {
                jobDescriptions = AZStd::move(createResponse.m_createJobOutputs);
            }
        }

        if (runProcessJob)
        {
            AZStd::string processJobTempDirPath;
            AzFramework::StringFunc::Path::Join(tempDirPath.c_str(), "ProcessJobs", processJobTempDirPath);
            AZ::IO::Result fileResult = fileIO->CreatePath(processJobTempDirPath.c_str());
            if (!fileResult)
            {
                AZ_Error("AssetBuilder", false, "Unable to create debug or clear folder '%s'.", processJobTempDirPath.c_str());
                return false;
            }

            AssetBuilderSDK::ProcessJobRequest processRequest;
            processRequest.m_watchFolder = AZStd::move(watchFolder);
            processRequest.m_sourceFile = AZStd::move(info.m_relativePath);
            processRequest.m_sourceFileUUID = info.m_assetId.m_guid;
            AzFramework::StringFunc::Path::Join(processRequest.m_watchFolder.c_str(), processRequest.m_sourceFile.c_str(), processRequest.m_fullPath);
            processRequest.m_tempDirPath = processJobTempDirPath;
            processRequest.m_jobId = 0;
            processRequest.m_builderGuid = builder->m_busId;
            AZ_TraceContext("Source", debugFile);

            if (jobDescriptions.empty())
            {
                for (const auto& platformInfo : enabledDebugPlatformInfos)
                {
                    AssetBuilderSDK::JobDescriptor placeholder;
                    placeholder.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
                    placeholder.m_jobKey = AZStd::string::format("%s_DEBUG", builder->m_name.c_str());
                    placeholder.m_jobParameters[AZ_CRC("Debug", 0x6ca547a7)] = "true";
                    jobDescriptions.emplace_back(AZStd::move(placeholder));
                }
            }

            for (size_t i = 0; i < jobDescriptions.size(); ++i)
            {
                AssetBuilderSDK::AssetBuilderTraceBus::Broadcast(&AssetBuilderSDK::AssetBuilderTraceBus::Events::ResetErrorCount);
                AssetBuilderSDK::AssetBuilderTraceBus::Broadcast(&AssetBuilderSDK::AssetBuilderTraceBus::Events::ResetWarningCount);

                processRequest.m_jobDescription = jobDescriptions[i];

                AZ_TraceContext("Platform", processRequest.m_jobDescription.GetPlatformIdentifier());
                AssetBuilderSDK::ProcessJobResponse processResponse;
                builder->m_processJobFunction(processRequest, processResponse);
                UpdateResultCode(processRequest, processResponse);

                AZStd::string responseFile;
                AzFramework::StringFunc::Path::ConstructFull(processJobTempDirPath.c_str(), 
                    AZStd::string::format("%i_%s", i, AssetBuilderSDK::s_processJobResponseFileName).c_str(), responseFile);
                if (!AZ::Utils::SaveObjectToFile(responseFile, AZ::DataStream::ST_XML, &processResponse))
                {
                    AZ_Error("AssetBuilder", false, "Failed to serialize response to file: %s", responseFile.c_str());
                    return false;
                }
            }
        }
    }

    return true;
}

bool AssetBuilderComponent::RunOneShotTask(const AZStd::string& task)
{
    AZStd::string modulePath, inputFilePath, outputFilePath;

    if (!GetParameter(s_paramModule, modulePath)
        || !GetParameter(s_paramInput, inputFilePath)
        || !GetParameter(s_paramOutput, outputFilePath))
    {
        return false;
    }

    AzFramework::StringFunc::Path::Normalize(inputFilePath);
    AzFramework::StringFunc::Path::Normalize(outputFilePath);

    if (!LoadBuilder(modulePath))
    {
        return false;
    }

    if (task == s_taskRegisterBuilder)
    {
        return HandleRegisterBuilder(inputFilePath, outputFilePath);
    }
    else if (task == s_taskCreateJob)
    {
        auto func = [this](const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
        {
            AZ_TraceContext("Source", request.m_sourceFile);
            AZ_TraceContext("Platforms", AssetBuilderSDK::PlatformInfo::PlatformVectorAsString(request.m_enabledPlatforms));
            m_assetBuilderDescMap.at(request.m_builderid)->m_createJobFunction(request, response);
        };

        return HandleTask<AssetBuilderSDK::CreateJobsRequest, AssetBuilderSDK::CreateJobsResponse>(inputFilePath, outputFilePath, func);
    }
    else if (task == s_taskProcessJob)
    {
        auto func = [this](const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
        {
            AZ_TraceContext("Source", request.m_fullPath);
            AZ_TraceContext("Platform", request.m_platformInfo.m_identifier);
            m_assetBuilderDescMap.at(request.m_builderGuid)->m_processJobFunction(request, response);
            UpdateResultCode(request, response);
        };

        return HandleTask<AssetBuilderSDK::ProcessJobRequest, AssetBuilderSDK::ProcessJobResponse>(inputFilePath, outputFilePath, func);
    }
    else
    {
        AZ_Error("AssetBuilder", false, "Unknown task");
        return false;
    }
}

void AssetBuilderComponent::Disconnected(AzFramework::SocketConnection* connection)
{
    // If we lose connection to the AP, print out an error and shut down.
    // This prevents builders from running indefinitely if the AP crashes
    AZ_Error("AssetBuilder", false, "Lost connection to Asset Processor, shutting down");
    m_mainEvent.release();
}

template<typename TNetRequest, typename TNetResponse>
void AssetBuilderComponent::ResidentJobHandler(AZ::u32 serial, const void* data, AZ::u32 dataLength, JobType jobType)
{
    auto job = AZStd::make_unique<Job>();
    job->m_netResponse = AZStd::make_unique<TNetResponse>();
    job->m_requestSerial = serial;
    job->m_jobType = jobType;

    auto* request = AZ::Utils::LoadObjectFromBuffer<TNetRequest>(data, dataLength);

    if (!request)
    {
        AZ_Error("AssetBuilder", false, "Problem deserializing net request");
        AzFramework::AssetSystem::SendResponse(*(job->m_netResponse), serial);

        return;
    }

    job->m_netRequest = AZStd::unique_ptr<TNetRequest>(request);

    // Queue up the job for the worker thread
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_jobMutex);

        if (!m_queuedJob)
        {
            m_queuedJob.swap(job);
        }
        else
        {
            AZ_Error("AssetBuilder", false, "Builder already has a job queued");
            AzFramework::AssetSystem::SendResponse(*(job->m_netResponse), serial);

            return;
        }
    }

    // Wake up the job thread
    m_jobEvent.release();
}

bool AssetBuilderComponent::IsBuilderForFile(const AZStd::string& filePath, const AssetBuilderSDK::AssetBuilderDesc& builderDescription) const
{
    for (const AssetBuilderSDK::AssetBuilderPattern& pattern : builderDescription.m_patterns)
    {
        AssetBuilderSDK::FilePatternMatcher matcher(pattern);
        if (matcher.MatchesPath(filePath))
        {
            return true;
        }
    }

    return false;
}

void AssetBuilderComponent::JobThread()
{
    while (m_running)
    {
        m_jobEvent.acquire();

        AZStd::unique_ptr<Job> job;

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_jobMutex);
            job.swap(m_queuedJob);
        }

        if (!job)
        {
            if (m_running)
            {
                AZ_TracePrintf("AssetBuilder", "JobThread woke up, but there was no queued job\n");
            }

            continue;
        }

        AssetBuilderSDK::AssetBuilderTraceBus::Broadcast(&AssetBuilderSDK::AssetBuilderTraceBus::Events::ResetErrorCount);
        AssetBuilderSDK::AssetBuilderTraceBus::Broadcast(&AssetBuilderSDK::AssetBuilderTraceBus::Events::ResetWarningCount);
        
        switch (job->m_jobType)
        {
        case JobType::Create:
        {
            using namespace AssetBuilderSDK;

            AZ_TracePrintf("AssetBuilder", "Running createJob task\n");

            auto* netRequest = azrtti_cast<CreateJobsNetRequest*>(job->m_netRequest.get());
            auto* netResponse = azrtti_cast<CreateJobsNetResponse*>(job->m_netResponse.get());
            AZ_Assert(netRequest && netResponse, "Request or response is null");

            AZ_TraceContext("Source", netRequest->m_request.m_sourceFile);
            AZ_TraceContext("Platforms", AssetBuilderSDK::PlatformInfo::PlatformVectorAsString(netRequest->m_request.m_enabledPlatforms));
            m_assetBuilderDescMap.at(netRequest->m_request.m_builderid)->m_createJobFunction(netRequest->m_request, netResponse->m_response);
            break;
        }
        case JobType::Process:
        {
            using namespace AssetBuilderSDK;

            AZ_TracePrintf("AssetBuilder", "Running processJob task\n");

            auto* netRequest = azrtti_cast<ProcessJobNetRequest*>(job->m_netRequest.get());
            auto* netResponse = azrtti_cast<ProcessJobNetResponse*>(job->m_netResponse.get());
            AZ_Assert(netRequest && netResponse, "Request or response is null");

            AZ_TraceContext("Source", netRequest->m_request.m_fullPath);
            AZ_TraceContext("Platforms", netRequest->m_request.m_platformInfo.m_identifier);
            m_assetBuilderDescMap.at(netRequest->m_request.m_builderGuid)->m_processJobFunction(netRequest->m_request, netResponse->m_response);
            UpdateResultCode(netRequest->m_request, netResponse->m_response);
            break;
        }
        default:
            AZ_Error("AssetBuilder", false, "Unhandled job request type");
            continue;
        }

        //Flush our output so the AP can properly associate all output with the current job
        std::fflush(stdout);
        std::fflush(stderr);

        AzFramework::AssetSystem::SendResponse(*(job->m_netResponse), job->m_requestSerial);
    }
}

void AssetBuilderComponent::CreateJobsResidentHandler(AZ::u32 typeId, AZ::u32 serial, const void* data, AZ::u32 dataLength)
{
    using namespace AssetBuilderSDK;

    ResidentJobHandler<CreateJobsNetRequest, CreateJobsNetResponse>(serial, data, dataLength, JobType::Create);
}

void AssetBuilderComponent::ProcessJobResidentHandler(AZ::u32 typeId, AZ::u32 serial, const void* data, AZ::u32 dataLength)
{
    using namespace AssetBuilderSDK;

    ResidentJobHandler<ProcessJobNetRequest, ProcessJobNetResponse>(serial, data, dataLength, JobType::Process);
}

//////////////////////////////////////////////////////////////////////////

template<typename TRequest, typename TResponse>
bool AssetBuilderComponent::HandleTask(const AZStd::string& inputFilePath, const AZStd::string& outputFilePath, const AZStd::function<void(const TRequest& request, TResponse& response)>& assetBuilderFunc)
{
    TRequest request;
    TResponse response;

    if (!AZ::Utils::LoadObjectFromFileInPlace(inputFilePath, request))
    {
        AZ_Error("AssetBuilder", false, "Failed to deserialize request from file: %s", inputFilePath.c_str());
        return false;
    }

    assetBuilderFunc(request, response);

    if (!AZ::Utils::SaveObjectToFile(outputFilePath, AZ::DataStream::ST_XML, &response))
    {
        AZ_Error("AssetBuilder", false, "Failed to serialize response to file: %s", outputFilePath.c_str());
        return false;
    }

    return true;
}

bool AssetBuilderComponent::HandleRegisterBuilder(const AZStd::string& inputFilePath, const AZStd::string& outputFilePath) const
{
    AssetBuilderSDK::RegisterBuilderResponse response;

    for (const auto& pair : m_assetBuilderDescMap)
    {
        AssetBuilderSDK::AssetBuilderRegistrationDesc desc;

        desc.m_name = pair.second->m_name;
        desc.m_busId = pair.second->m_busId;
        desc.m_patterns = pair.second->m_patterns;
        desc.m_version = pair.second->m_version;

        response.m_assetBuilderDescList.push_back(desc);
    }

    return AZ::Utils::SaveObjectToFile(outputFilePath, AZ::DataStream::ST_XML, &response);
}

void AssetBuilderComponent::UpdateResultCode(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
{
    if (request.m_jobDescription.m_failOnError)
    {
        AZ::u32 errorCount = 0;
        AssetBuilderSDK::AssetBuilderTraceBus::BroadcastResult(errorCount, &AssetBuilderSDK::AssetBuilderTraceBus::Events::GetErrorCount);
        if (errorCount > 0 && response.m_resultCode == AssetBuilderSDK::ProcessJobResult_Success)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
        }
    }
}

bool AssetBuilderComponent::GetParameter(const char* paramName, AZStd::string& outValue, bool required /*= true*/) const
{
    const AzFramework::CommandLine* commandLine = nullptr;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);

    outValue = commandLine->GetSwitchValue(paramName, 0);

    if (outValue.empty())
    {
        AZ_Error("AssetBuilder", !required, "Missing required parameter `%s`. Use -help for options.", paramName);
        return false;
    }

    return true;
}

const char* AssetBuilderComponent::GetLibraryExtension()
{
#if defined(AZ_PLATFORM_WINDOWS)
    return "*.dll";
#elif defined(AZ_PLATFORM_LINUX)
    return "*.so";
#elif defined(AZ_PLATFORM_APPLE)
    return "*.dylib";
#endif
}

bool AssetBuilderComponent::LoadBuilders(const AZStd::string& builderFolder)
{
    auto* fileIO = AZ::IO::FileIOBase::GetInstance();
    bool result = false;

    fileIO->FindFiles(builderFolder.c_str(), GetLibraryExtension(), [&result, this](const char* file)
        {
            result = LoadBuilder(file);

            return result;
        });

    return result;
}

bool AssetBuilderComponent::LoadBuilder(const AZStd::string& filePath)
{
    auto assetBuilderInfo = AZStd::make_unique<AssetBuilder::ExternalModuleAssetBuilderInfo>(QString::fromUtf8(filePath.c_str()));

    if (!assetBuilderInfo->IsLoaded())
    {
        AZ_Warning("AssetBuilder", false, "AssetBuilder was not able to load the library: %s\n", filePath.c_str());
        return false;
    }

    AZ_TracePrintf("AssetBuilder", "Initializing and registering builder %s\n", assetBuilderInfo->GetName().toStdString().c_str());

    m_currentAssetBuilder = assetBuilderInfo.get();
    m_currentAssetBuilder->Initialize();
    m_currentAssetBuilder = nullptr;

    m_assetBuilderInfoList.push_back(AZStd::move(assetBuilderInfo));

    return true;
}

void AssetBuilderComponent::UnloadBuilders()
{
    m_assetBuilderDescMap.clear();

    for (auto& assetBuilderInfo : m_assetBuilderInfoList)
    {
        assetBuilderInfo->UnInitialize();
    }

    m_assetBuilderInfoList.clear();
}

void AssetBuilderComponent::RegisterBuilderInformation(const AssetBuilderSDK::AssetBuilderDesc& builderDesc)
{
    m_assetBuilderDescMap.insert({ builderDesc.m_busId, AZStd::make_unique<AssetBuilderSDK::AssetBuilderDesc>(builderDesc) });

    if (m_currentAssetBuilder)
    {
        m_currentAssetBuilder->RegisterBuilderDesc(builderDesc.m_busId);
    }
}

void AssetBuilderComponent::RegisterComponentDescriptor(AZ::ComponentDescriptor* descriptor)
{
    if (m_currentAssetBuilder)
    {
        m_currentAssetBuilder->RegisterComponentDesc(descriptor);
    }
}
