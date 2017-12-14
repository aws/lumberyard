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

#include <AssetBuilderComponent.h>
#include <AssetBuilderInfo.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzCore/RTTI/RTTI.h>

// Command-line parameter options:
static const char* const s_paramTask = "task"; // task to run
static const char* const s_paramGameName = "gamename"; // name of the current project
static const char* const s_paramGameCache = "gamecache"; // full path to the project cache folder
static const char* const s_paramModule = "module"; // for resident mode, the path to the builder dll folder, otherwise the full path to a single builder dll to use
static const char* const s_paramPort = "port"; // optional, port number to use to connect to the AP
static const char* const s_paramIp = "remoteip"; // optional, IP address to use to connect to the AP
static const char* const s_paramId = "id"; // UUID string that identifies the builder.  Only used for resident mode when the AP directly starts up the AssetBuilder
static const char* const s_paramInput = "input"; // for non-resident mode, full path to the file containing the serialized job request
static const char* const s_paramOutput = "output"; // for non-resident mode, full path to the file to write the job response to

// Task modes:
static const char* const taskResident = "resident"; // stays up and running indefinitely, accepting jobs via network connection
static const char* const taskRegisterBuilder = "register"; // outputs all the builder descriptors
static const char* const taskCreateJob = "create"; // runs a builders createJobs function
static const char* const taskProcessJob = "process"; // runs processJob function

//////////////////////////////////////////////////////////////////////////

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
    AZStd::string task;

    if (!GetParameter(s_paramTask, task)
        || !GetParameter(s_paramGameName, m_gameName)
        || !GetParameter(s_paramGameCache, m_gameCache))
    {
        return false;
    }

    AssetBuilderSDK::InitializeSerializationContext();

    if (!ConnectToAssetProcessor())
    {
        //AP connection is required to access the asset catalog
        AZ_Error("AssetBuilder", false, "Failed to establish a network connection to the AssetProcessor");
        return false;
    }

    bool result = false;

    if (task == taskResident)
    {
        result = RunInResidentMode();
    }
    else
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

        if (task == taskRegisterBuilder)
        {
            result = HandleRegisterBuilder(inputFilePath, outputFilePath);
        }
        else if (task == taskCreateJob)
        {
            auto func = [this](const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
                {
                    m_assetBuilderDescMap.at(request.m_builderid)->m_createJobFunction(request, response);
                };

            result = HandleTask<AssetBuilderSDK::CreateJobsRequest, AssetBuilderSDK::CreateJobsResponse>(inputFilePath, outputFilePath, func);
        }
        else if (task == taskProcessJob)
        {
            auto func = [this](const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
                {
                    m_assetBuilderDescMap.at(request.m_builderGuid)->m_processJobFunction(request, response);
                };

            result = HandleTask<AssetBuilderSDK::ProcessJobRequest, AssetBuilderSDK::ProcessJobResponse>(inputFilePath, outputFilePath, func);
        }
    }

    AZ_Error("AssetBuilder", result, "Failed to handle `%s` request", task.c_str());

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

        switch (job->m_jobType)
        {
        case JobType::Create:
        {
            using namespace AssetBuilderSDK;

            AZ_TracePrintf("AssetBuilder", "Running createJob task\n");

            auto* netRequest = azrtti_cast<CreateJobsNetRequest*>(job->m_netRequest.get());
            auto* netResponse = azrtti_cast<CreateJobsNetResponse*>(job->m_netResponse.get());
            AZ_Assert(netRequest && netResponse, "Request or response is null");

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

            m_assetBuilderDescMap.at(netRequest->m_request.m_builderGuid)->m_processJobFunction(netRequest->m_request, netResponse->m_response);
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

bool AssetBuilderComponent::GetParameter(const char* paramName, AZStd::string& outValue, bool required /*= true*/) const
{
    const AzFramework::CommandLine* commandLine = nullptr;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);

    outValue = commandLine->GetSwitchValue(paramName, 0);

    if (outValue.empty())
    {
        AZ_Error("AssetBuilder", !required, "Missing required parameter `%s`", paramName);
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

    if (!assetBuilderInfo->Load())
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
