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

#include "StdAfx.h"

#include <AzCore/Rtti/BehaviorContext.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <CloudCanvas/ICloudCanvasEditor.h>

#include "CloudGemWebCommunicatorComponent.h"
#include <AWS/ServiceApi/CloudGemWebCommunicatorClientComponent.h>

#include <CloudCanvas/CloudCanvasIdentityBus.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/Module/ModuleManager.h>
#include <AzCore/Module/DynamicModuleHandle.h>

#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <network/WebSocket/WebSocketConnection.hpp>
#include <network/OpenSSL/OpenSSLConnection.hpp>
#include <ResponseCode.hpp>
#include <util/Utf8String.hpp>
#include <Action.hpp>

#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <rapidjson/reader.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/document.h>

#include <functional>

namespace CloudGemWebCommunicator
{
    static const std::chrono::milliseconds cMqttTimeout{ 20000 };
    AZ::EntityId CloudGemWebCommunicatorComponent::m_moduleEntity;
    AZStd::string CloudGemWebCommunicatorComponent::m_region{ "" };
    static const std::chrono::milliseconds subscriptionTimeout{ 10000 };

    // Update Bus
    class BehaviorCloudGemWebCommunicatorUpdateBusHandler : public CloudGemWebCommunicatorUpdateBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorCloudGemWebCommunicatorUpdateBusHandler, "{C6ADF0D4-0301-417A-9824-045AF0747C96}", AZ::SystemAllocator,
            ConnectionStatusChanged, MessageReceived, RegistrationStatusChanged, SubscriptionStatusChanged);

        void ConnectionStatusChanged(const AZStd::string& connectionStatus) override
        {
            Call(FN_ConnectionStatusChanged, connectionStatus);
        }

        void MessageReceived(const AZStd::string& channelName, const AZStd::string& channelMessage) override
        {
            Call(FN_MessageReceived, channelName, channelMessage);
        }

        void RegistrationStatusChanged(const AZStd::string& registrationStatus) override
        {
            Call(FN_RegistrationStatusChanged, registrationStatus);
        }

        void SubscriptionStatusChanged(const AZStd::string& channelName, const AZStd::string& subscriptionStatus) override
        {
            Call(FN_SubscriptionStatusChanged, channelName, subscriptionStatus);
        }

    };

    CloudGemWebCommunicatorComponent::CloudGemWebCommunicatorComponent()
    {
        SetupJobManager();
    }

    CloudGemWebCommunicatorComponent::~CloudGemWebCommunicatorComponent()
    {
        EndJobManager();
    }

    void CloudGemWebCommunicatorComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudGemWebCommunicatorComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudGemWebCommunicatorComponent>("CloudGemWebCommunicator", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<CloudGemWebCommunicatorRequestBus>("CloudGemWebCommunicatorRequestBus")
                ->Event("RequestRegistration", &CloudGemWebCommunicatorRequestBus::Events::RequestRegistration)
                ->Event("RequestRefreshDeviceInfo", &CloudGemWebCommunicatorRequestBus::Events::RequestRefreshDeviceInfo)
                ->Event("RequestConnection", &CloudGemWebCommunicatorRequestBus::Events::RequestConnection)
                ->Event("RequestDisconnect", &CloudGemWebCommunicatorRequestBus::Events::RequestDisconnect)
                ->Event("RequestChannelList", &CloudGemWebCommunicatorRequestBus::Events::RequestChannelList)
                ->Event("RequestSubscribeChannel", &CloudGemWebCommunicatorRequestBus::Events::RequestSubscribeChannel)
                ->Event("RequestSubscribeChannelList", &CloudGemWebCommunicatorRequestBus::Events::RequestSubscribeChannelList)
                ->Event("PostClientMessage", &CloudGemWebCommunicatorRequestBus::Events::PostClientMessage)
                ->Event("RequestSendMessageDirect", &CloudGemWebCommunicatorRequestBus::Events::RequestSendMessageDirect)
                ->Event("RequestUnsubscribeChannel", &CloudGemWebCommunicatorRequestBus::Events::RequestUnsubscribeChannel)
                ->Event("GetRegistrationStatus", &CloudGemWebCommunicatorRequestBus::Events::GetRegistrationStatus)
                ->Event("GetConnectionStatus", &CloudGemWebCommunicatorRequestBus::Events::GetConnectionStatus)
                ->Event("GetSubscriptionStatus", &CloudGemWebCommunicatorRequestBus::Events::GetSubscriptionStatus)
                ->Event("GetEndpointPortString", &CloudGemWebCommunicatorRequestBus::Events::GetEndpointPortString)
                ->Event("GetSubscriptionList", &CloudGemWebCommunicatorRequestBus::Events::GetSubscriptionList)
                ;

            behaviorContext->EBus<CloudGemWebCommunicatorUpdateBus>("CloudGemWebCommunicatorUpdateBus")
                ->Handler<BehaviorCloudGemWebCommunicatorUpdateBusHandler>()
                ;

            behaviorContext->Class<CloudGemWebCommunicatorComponent>("WebCommunicator")
                ->Property("ModuleEntity", BehaviorValueGetter(&CloudGemWebCommunicatorComponent::m_moduleEntity), nullptr);

        }
    }

    void CloudGemWebCommunicatorComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemWebCommunicatorService"));
    }

    void CloudGemWebCommunicatorComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CloudGemWebCommunicatorService"));
    }

    void CloudGemWebCommunicatorComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void CloudGemWebCommunicatorComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void CloudGemWebCommunicatorComponent::SetupJobManager()
    {
        static const unsigned int desiredMaxWorkers = 2;
        // Default job manager
        unsigned int numWorkerThreads = AZ::GetMin(desiredMaxWorkers, AZStd::thread::hardware_concurrency());

        AZ::JobManagerDesc jobDesc;
        AZ::JobManagerThreadDesc threadDesc;
        for (unsigned int i = 0; i < numWorkerThreads; ++i)
        {
            jobDesc.m_workerThreads.push_back(threadDesc);
        }
        m_jobManager = aznew AZ::JobManager{ jobDesc };
        m_jobContext = aznew AZ::JobContext{ *m_jobManager };
    }

    void CloudGemWebCommunicatorComponent::EndJobManager()
    {
        delete m_jobContext;
        delete m_jobManager;
    }

    void CloudGemWebCommunicatorComponent::Init()
    {
    }

    void CloudGemWebCommunicatorComponent::Activate()
    {
        if (azrtti_istypeof<AZ::ModuleEntity>(GetEntity()))
        {
            m_moduleEntity = GetEntityId();
        }

        AZ::SystemTickBus::Handler::BusConnect();
        CloudGemWebCommunicatorRequestBus::Handler::BusConnect();
        CloudGemWebCommunicatorLibraryResponseBus::Handler::BusConnect(GetEntityId());
        CloudCanvasCommon::CloudCanvasCommonNotificationsBus::Handler::BusConnect();
    }

    void CloudGemWebCommunicatorComponent::OnPostInitialization()
    {
        ReadDeviceInfo();
    }

    void CloudGemWebCommunicatorComponent::Deactivate()
    {
        CloudCanvasCommon::CloudCanvasCommonNotificationsBus::Handler::BusDisconnect();
        CloudGemWebCommunicatorLibraryResponseBus::Handler::BusDisconnect(GetEntityId());
        CloudGemWebCommunicatorRequestBus::Handler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();

        if (azrtti_istypeof<AZ::ModuleEntity>(GetEntity()))
        {
            m_moduleEntity.SetInvalid();
        }
    }

    void CloudGemWebCommunicatorComponent::OnSystemTick()
    {
        CloudGemWebCommunicatorLibraryResponseBus::ExecuteQueuedEvents();
    }

    bool CloudGemWebCommunicatorComponent::RequestRegistration(const AZStd::string& registrationType)
    {

        auto requestJob = ServiceAPI::get_client_registrationRequestJob::Create([this](ServiceAPI::get_client_registrationRequestJob* job)
        {
            auto thisResult = job->result;

            AZStd::string resultStr = thisResult.Result;
            AZStd::string privateKey = thisResult.PrivateKey;
            AZStd::string deviceCert = thisResult.DeviceCert;

            if (privateKey.length())
            {
                WriteDeviceFile(GetKeyPath(), privateKey);
            }

            if (deviceCert.length())
            {
                WriteDeviceFile(GetDeviceCertPath(), deviceCert);
            }

            m_endpoint = thisResult.Endpoint;
            m_endpointPort = thisResult.EndpointPort;
            m_connectionType = thisResult.ConnectionType;

            AZ_TracePrintf("CloudCanvas","Device successfully registered - %s", resultStr.c_str());
            WriteDeviceInfo();
            ReadCertificate(GetDeviceCertPath());
            EBUS_EVENT(CloudGemWebCommunicatorUpdateBus, RegistrationStatusChanged, GetRegistrationStatus());
        },
            [this](ServiceAPI::get_client_registrationRequestJob* job)
        {
            auto thisResult = job->result;
            AZStd::string resultStr = thisResult.Result;
            AZ_TracePrintf("CloudCanvas", "Device registration failed %s", resultStr.c_str());
        }
        );

        requestJob->parameters.registration_type = registrationType;
        requestJob->Start();
        return true;
    }

    AZStd::string CloudGemWebCommunicatorComponent::GetResolvedPath(const char* filePath) 
    {
        char resolvedGameFolder[AZ_MAX_PATH_LEN] = { 0 };
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(filePath, resolvedGameFolder, AZ_MAX_PATH_LEN);
        return resolvedGameFolder;
    }       

    AZStd::string CloudGemWebCommunicatorComponent::GetDeviceInfoFilePath() const
    {
        return GetUserOrStorage("certs/aws/deviceinfo.json");
    }

    AZStd::string CloudGemWebCommunicatorComponent::GetEndpoint() const
    {
        return m_endpoint;
    }

    int CloudGemWebCommunicatorComponent::GetEndpointPort() const
    {
        return m_endpointPort;
    }

    bool CloudGemWebCommunicatorComponent::WriteDeviceInfo() const
    {
        rapidjson::StringBuffer writeString;
        rapidjson::Writer<rapidjson::StringBuffer> writer(writeString);

        writer.StartObject();
        writer.Key("endpoint");
        writer.String(GetEndpoint().c_str());
        writer.Key("endpointPort");
        writer.Int(GetEndpointPort());
        writer.Key("connectionType");
        writer.String(m_connectionType.c_str());
        writer.EndObject();

        WriteDeviceFile(GetDeviceInfoFilePath(), writeString.GetString());
        return true;
    }

    bool CloudGemWebCommunicatorComponent::RequestRefreshDeviceInfo()
    {
        return ReadDeviceInfo();
    }

    bool CloudGemWebCommunicatorComponent::ReadDeviceInfo() 
    {
        AZ::IO::HandleType inputFile;
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetDirectInstance();
        if (!fileIO)
        {
            AZ_Warning("CloudCanvas", false, "Could not get FileIO direct instance to retrieve device info");
            return false;
        }
        fileIO->Open(GetDeviceInfoFilePath().c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, inputFile);
        if (!inputFile)
        {
            AZ_TracePrintf("CloudCanvas", "Could not open %s for reading", GetDeviceInfoFilePath().c_str());
            return false;
        }

        AZ::u64 fileSize{ 0 };
        fileIO->Size(inputFile, fileSize);
        if (fileSize > 0)
        {
            AZStd::string fileBuf;
            fileBuf.resize(fileSize);
            size_t read = fileIO->Read(inputFile, fileBuf.data(), fileSize);

            rapidjson::Document parseDoc;
            parseDoc.Parse<rapidjson::kParseStopWhenDoneFlag>(fileBuf.data());

            if (parseDoc.HasParseError())
            {
                return false;
            }

            m_endpoint.clear();
            m_endpointPort = 0;
            m_connectionType.clear();

            auto itr = parseDoc.FindMember("endpoint");
            if(itr != parseDoc.MemberEnd() && itr->value.IsString())
            {  
                m_endpoint = itr->value.GetString();
            }

            itr = parseDoc.FindMember("endpointPort");
            if (itr != parseDoc.MemberEnd() && itr->value.IsInt())
            {
                m_endpointPort = itr->value.GetInt();
            }

            itr = parseDoc.FindMember("connectionType"); 
            if (itr != parseDoc.MemberEnd() && itr->value.IsString())
            {
                m_connectionType = itr->value.GetString();
            }
        }
        fileIO->Close(inputFile);
        ReadCertificate(GetDeviceCertPath());
        EBUS_EVENT(CloudGemWebCommunicatorUpdateBus, RegistrationStatusChanged, GetRegistrationStatus());
        return true;
    }

    AZStd::string CloudGemWebCommunicatorComponent::GetCAPath() 
    {
        CloudCanvas::RequestRootCAFileResult requestResult;
        AZStd::string filePath;
        EBUS_EVENT_RESULT(requestResult, CloudCanvasCommon::CloudCanvasCommonRequestBus, GetUserRootCAFile, filePath);
        return filePath;
    }

    AZStd::string CloudGemWebCommunicatorComponent::GetUserOrStorage(const AZStd::string& filePath)
    {
        AZStd::string userPath{ "@user@/" + filePath };
        userPath = GetResolvedPath(userPath.c_str());
        if (!AZ::IO::FileIOBase::GetInstance()->Exists(userPath.c_str()))
        {
            if (AZ::IO::FileIOBase::GetInstance()->Exists(filePath.c_str()))
            {
                CopyToUserStorage(filePath, userPath);
            }
        }
        return userPath;
    }

    // These files need to be in a place accessible to the 3rdParty library by file name on all platforms.  For consistency we maintain this behavior with
    // all platforms.
    bool CloudGemWebCommunicatorComponent::CopyToUserStorage(const AZStd::string& filePath, const AZStd::string& userPath)
    {

        AZ::IO::HandleType inputFile;
        AZ::IO::FileIOBase::GetInstance()->Open(filePath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, inputFile);
        if (!inputFile)
        {
            AZ_Warning("CloudCanvas", false, "Could not open %s for reading", filePath);
            return false;
        }

        AZ::IO::SystemFile outputFile;
        if (!outputFile.Open(userPath.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_READ_WRITE))
        {
            AZ::IO::FileIOBase::GetInstance()->Close(inputFile);
            AZ_Warning("CloudCanvas", false, "Could not open %s for writing", userPath.c_str());
            return false;
        }

        AZ::u64 fileSize{ 0 };
        AZ::IO::FileIOBase::GetInstance()->Size(inputFile, fileSize);
        if (fileSize > 0)
        {
            AZStd::string fileBuf;
            fileBuf.resize(fileSize);
            size_t read = AZ::IO::FileIOBase::GetInstance()->Read(inputFile, fileBuf.data(), fileSize);
            outputFile.Write(fileBuf.data(), fileSize);
            AZ_TracePrintf("CloudCanvas", "Successfully wrote %s to %s", filePath.c_str(), userPath.c_str());
        }
        AZ::IO::FileIOBase::GetInstance()->Close(inputFile);
        outputFile.Close();
        return true;
    }


    AZStd::string CloudGemWebCommunicatorComponent::GetKeyPath() 
    {
        return GetUserOrStorage("certs/aws/webcommunicatorkey.pem");
    }

    AZStd::string CloudGemWebCommunicatorComponent::GetDeviceCertPath() 
    {
        return GetUserOrStorage("certs/aws/webcommunicatordevice.pem");
    }

    bool CloudGemWebCommunicatorComponent::WriteDeviceFile(const AZStd::string& filePath, const AZStd::string& fileData) const
    {
        AZ::IO::SystemFile outputFile;
        if (!outputFile.Open(filePath.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_READ_WRITE))
        {
            AZ_Warning("CloudCanvas", false, "Could not open %s for writing", filePath.c_str());
            return false;
        }
        outputFile.Write(fileData.data(), fileData.length());
        outputFile.Close();
        return true;
    }
    
    bool CloudGemWebCommunicatorComponent::ReadCertificate(const AZStd::string& filePath) 
    {
        AZ::IO::HandleType inputFile;
        AZ::IO::FileIOBase::GetDirectInstance()->Open(filePath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, inputFile);
        if (!inputFile)
        {
            AZ_TracePrintf("CloudCanvas", "CloudGemWebCommunicatorComponent could not open device certificate %s for reading", filePath.c_str());
            return false;
        }

        AZ::u64 fileSize{ 0 };
        AZ::IO::FileIOBase::GetDirectInstance()->Size(inputFile, fileSize);
        if (fileSize > 0)
        {
            AZStd::string fileBuf;
            fileBuf.resize(fileSize);
            size_t read = AZ::IO::FileIOBase::GetDirectInstance()->Read(inputFile, fileBuf.data(), fileSize);

            X509* certificate;
            BIO* bio;

            bio = BIO_new(BIO_s_mem());
            BIO_puts(bio, fileBuf.c_str());
            certificate = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);

            ASN1_INTEGER* serialNum = X509_get_serialNumber(certificate);
            BIGNUM* bigNum = ASN1_INTEGER_to_BN(serialNum, NULL);
            if (!bigNum)
            {
                AZ_Warning("CloudCanvas", false, "Could not convert certificate ASN1_INTEGER to BIGNUM");
            }
            else
            {
                char* sslStr = BN_bn2dec(bigNum);
                m_certificateSN = sslStr;
                OPENSSL_free(sslStr);
                AZ_TracePrintf("CloudCanvas", "Got CertificateSN of %s", m_certificateSN.c_str());
            }
            X509_free(certificate);
            BIO_free(bio);
        }
        AZ::IO::FileIOBase::GetDirectInstance()->Close(inputFile);
        return true;
    }
    bool CloudGemWebCommunicatorComponent::IsConnected() const
    {
        return m_iotClient && m_iotClient->IsConnected();
    }

    bool CloudGemWebCommunicatorComponent::Connect(const AZStd::string& connectionType, const AZStd::string& endpoint, int endpointPort)
    {
        if (IsConnected())
        {
            AZ_Warning("CloudCanvas", false, "Attempted to reconnect existing connection.");
            return false;
        }
        else if (m_connectionStatus == ConnectionStatus::CONNECTING)
        {
            AZ_Warning("CloudCanvas", false, "Already connecting");
            return false;
        }

        AZ::EntityId entityId = GetEntityId();
        // VS 2013 requires capture of 'this' for referencing CloudgemWebCommunicatorComponent::CloudGemWebCOmmunicatorLibraryResponse::GotConnectionResponse
        auto connectLambda = [entityId, this](AZStd::string connectionType, const AZStd::string endpoint, int endpointPort)
        {
            std::shared_ptr<awsiotsdk::NetworkConnection> connectionPtr;
            if (connectionType == "WEBSOCKET")
            {
                connectionPtr = ConnectWebsocket(endpoint, endpointPort);
            }
            else if (connectionType == "OPENSSL")
            {
                connectionPtr = ConnectSSL(endpoint, endpointPort);
            }
            awsiotsdk::ResponseCode responseCode;
            std::shared_ptr<awsiotsdk::MqttClient> mqttClient = CreateClient(connectionPtr, responseCode, GetClientId());
            EBUS_QUEUE_EVENT_ID(entityId, CloudGemWebCommunicatorLibraryResponseBus, GotConnectionResponse, responseCode, mqttClient);
        };

        AZ::Job* connectJob = AZ::CreateJobFunction(AZStd::bind(connectLambda, connectionType, endpoint, endpointPort), true, m_jobContext);
        SetConnectionStatus(ConnectionStatus::CONNECTING);
        EBUS_EVENT(CloudGemWebCommunicatorUpdateBus, ConnectionStatusChanged, GetConnectionStatus());
        connectJob->Start();
        return true;
    }

    void CloudGemWebCommunicatorComponent::GotConnectionResponse(ResponseCodeType responseCode, std::shared_ptr<awsiotsdk::MqttClient> connectionClient)
    {
        m_iotClient = connectionClient;
        SetConnectionStatus(m_iotClient && m_iotClient->IsConnected() ? ConnectionStatus::CONNECTED : ConnectionStatus::NOT_CONNECTED);
        EBUS_EVENT(CloudGemWebCommunicatorUpdateBus, ConnectionStatusChanged, GetConnectionStatus());
    }

    awsiotsdk::ResponseCode CloudGemWebCommunicatorComponent::Disconnect()
    {
        awsiotsdk::ResponseCode returnCode{ awsiotsdk::ResponseCode::SUCCESS };

        if (m_iotClient)
        {
            returnCode = m_iotClient->Disconnect(cMqttTimeout);
        }
        SetConnectionStatus(ConnectionStatus::NOT_CONNECTED);

        {
            AZStd::lock_guard<AZStd::mutex> channelLock{ m_channelMutex };
            m_channelList.clear();
        }
        EBUS_EVENT(CloudGemWebCommunicatorUpdateBus, ConnectionStatusChanged, GetConnectionStatus());
        return returnCode;
    }

    std::shared_ptr<awsiotsdk::NetworkConnection> CloudGemWebCommunicatorComponent::ConnectWebsocket(const AZStd::string& endpoint, int endpointPort)
    {
        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> playerCredentials;

        EBUS_EVENT_RESULT(playerCredentials, CloudGemFramework::CloudCanvasPlayerIdentityBus, GetPlayerCredentialsProvider);
        if (!playerCredentials)
        {
            AZ_Warning("CloudCanvas", false, "Could not get player credentials");
            return {};
        }
        Aws::Auth::AWSCredentials accountCredentials = playerCredentials->GetAWSCredentials();

        std::chrono::milliseconds cTimeoutMS{ 20000 };

        AZStd::string caPath = GetCAPath();

        std::shared_ptr<awsiotsdk::NetworkConnection> connectionPtr;

        connectionPtr = std::make_shared<awsiotsdk::network::WebSocketConnection>(endpoint.c_str(), endpointPort,
                caPath.c_str(), GetEndpointRegion(endpoint).c_str(),
                accountCredentials.GetAWSAccessKeyId().c_str(),
                accountCredentials.GetAWSSecretKey().c_str(),
                accountCredentials.GetSessionToken().c_str(),
                cTimeoutMS,
                cTimeoutMS,
                cTimeoutMS, true);

        if (!connectionPtr)
        {
            AZ_TracePrintf("CloudCanvas","WebSocket Connection failed!");
        }
        else
        {
            AZ_TracePrintf("CloudCanvas", "WebSocket Connection successful!");
        }
        return connectionPtr;
    }

    const AZStd::string CloudGemWebCommunicatorComponent::GetEndpointRegion(const AZStd::string& endpoint)
    {
        if (m_region != "")
        {
            return m_region;
        }

        if (endpoint.find(".iot.") == AZStd::string::npos || endpoint.find(".amazonaws.com") == AZStd::string::npos)
        {
            AZ_Warning("CloudCanvas", false, "Failed to parse the region from the endpoint: %s", endpoint.c_str());
            return "us-east-1";
        }

        auto startPosition = endpoint.find(".iot.") + 5;
        auto endPosition = endpoint.find(".amazonaws.com") - 1;
        m_region = endpoint.substr(startPosition, endPosition - startPosition + 1);
        return m_region;
    }

    std::shared_ptr<awsiotsdk::NetworkConnection> CloudGemWebCommunicatorComponent::ConnectSSL(const AZStd::string& endpoint, int endpointPort)
    {
        std::chrono::milliseconds cTimeoutMS{ 10000 };
        std::shared_ptr<awsiotsdk::network::OpenSSLConnection> connectionPtr =
            std::make_shared<awsiotsdk::network::OpenSSLConnection>(endpoint.c_str(), endpointPort,
                GetCAPath().c_str(),
                GetDeviceCertPath().c_str(),
                GetKeyPath().c_str(),
                cTimeoutMS,
                cTimeoutMS,
                cTimeoutMS, true);
        auto rc = connectionPtr->Initialize();

        if (rc != awsiotsdk::ResponseCode::SUCCESS) 
        {
            AZ_Warning("CloudCanvas", false,"Failed to initialize SSL Connection with rc : %d", static_cast<int>(rc));
        }
        else
        {
            AZ_TracePrintf("CloudCanvas", "OpenSSL Connection initialized with response code : %d", static_cast<int>(rc));
        }
        return connectionPtr;
    }

    AZStd::string CloudGemWebCommunicatorComponent::GetCertificateClientId()
    {
        return m_certificateSN;
    }

    AZStd::string CloudGemWebCommunicatorComponent::GetCognitoClientId() 
    {
        AZStd::string clientId;
        EBUS_EVENT_RESULT(clientId, CloudGemFramework::CloudCanvasPlayerIdentityBus, GetIdentityId);

        return clientId;
    }

    AZStd::string CloudGemWebCommunicatorComponent::GetClientId()
    {
        if (m_connectionType == "WEBSOCKET")
        {
            return GetCognitoClientId();
        }
        else if (m_connectionType == "OPENSSL")
        {
            return GetCertificateClientId();
        }
        AZ_Warning("CloudCanvas", false, "Could not get ClientID for connection");
        return{};
    }

    std::shared_ptr<awsiotsdk::MqttClient> CloudGemWebCommunicatorComponent::CreateClient(std::shared_ptr<awsiotsdk::NetworkConnection>& connectionPtr, awsiotsdk::ResponseCode& responseCode, const AZStd::string& clientId)
    {
        std::shared_ptr<awsiotsdk::MqttClient> mqttClient;

        const bool cCleanSession = true;
        const std::chrono::seconds keepAliveSeconds{ 30 };
        mqttClient = std::shared_ptr<awsiotsdk::MqttClient>(awsiotsdk::MqttClient::Create(connectionPtr, cMqttTimeout));
        if (nullptr == mqttClient) 
        {
            AZ_Warning("CloudCanvas", false, "MQTT client creation failed!");
            responseCode = awsiotsdk::ResponseCode::FAILURE;
            return mqttClient;
        }

        std::unique_ptr<awsiotsdk::Utf8String> client_id = awsiotsdk::Utf8String::Create(clientId.c_str());

        responseCode = mqttClient->Connect(cMqttTimeout, cCleanSession,
            awsiotsdk::mqtt::Version::MQTT_3_1_1, keepAliveSeconds,
            std::move(client_id), nullptr, nullptr, nullptr);

        if(responseCode == awsiotsdk::ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED || responseCode == awsiotsdk::ResponseCode::SUCCESS)
        {
            AZ_TracePrintf("CloudCanvas", "MQTT client created successfully");
            responseCode = awsiotsdk::ResponseCode::SUCCESS;
        }
        else if (responseCode == awsiotsdk::ResponseCode::NETWORK_ALREADY_CONNECTED_ERROR)
        {
            AZ_TracePrintf("CloudCanvas", "MQTT client already created.");
            responseCode = awsiotsdk::ResponseCode::SUCCESS;
        }
        else
        {
            AZ_Warning("CloudCanvas", false, "MQTT client creation failed with response code %d!", responseCode);
        }
        return mqttClient;
    }
    bool CloudGemWebCommunicatorComponent::RequestConnection(const AZStd::string& connectionType)
    {
        return Connect(connectionType,m_endpoint, m_endpointPort);
    }
    bool CloudGemWebCommunicatorComponent::RequestDisconnect()
    {
        return (Disconnect() == awsiotsdk::ResponseCode::SUCCESS);
    }

    bool CloudGemWebCommunicatorComponent::RequestChannelList()
    {
        AZStd::string identityId = GetClientId();

        auto requestJob = ServiceAPI::get_client_channelsRequestJob::Create([this, identityId](ServiceAPI::get_client_channelsRequestJob* job)
        {
            auto thisResult = job->result;
            AZ_TracePrintf("CloudCanvas", "Get_client_channelRequest succeeded");
            const char* identityTokenStr = "${cognito-identity.amazonaws.com:sub}";
            const size_t identityTokenLen = strlen(identityTokenStr);
            AZStd::vector<AZStd::string> subscriptionList;
            for (auto thisChannel : thisResult.Channels)
            {
                AZStd::string subscriptionChannel = thisChannel.Subscription;
                auto identityToken = subscriptionChannel.find(identityTokenStr);
                if (identityToken != AZStd::string::npos)
                {
                    subscriptionChannel.replace(identityToken, identityTokenLen, identityId);
                }
                subscriptionList.push_back(subscriptionChannel);
            }
            RequestSubscribeChannelList(subscriptionList);
        },
            [this](ServiceAPI::get_client_channelsRequestJob* job)
        {
            auto thisResult = job->result;

            AZ_TracePrintf("CloudCanvas", "Get_client_channelRequest failed %s", job->error.message.c_str());
        }
        );
        requestJob->Start();
        return true;
    }

    bool CloudGemWebCommunicatorComponent::RequestSubscribeChannel(const AZStd::string& channelName)
    {
        return RequestSubscribeChannelList(AZStd::vector<AZStd::string> { channelName} );
    }

    bool CloudGemWebCommunicatorComponent::RequestSubscribeChannelList(const AZStd::vector<AZStd::string>& channelList)
    {
        return (Subscribe(channelList) == awsiotsdk::ResponseCode::SUCCESS);
    }

    bool CloudGemWebCommunicatorComponent::RequestUnsubscribeChannel(const AZStd::string& channelName)
    {
        return (Unsubscribe(channelName) == awsiotsdk::ResponseCode::SUCCESS);
    }

    bool CloudGemWebCommunicatorComponent::RequestSendMessageDirect(const AZStd::string& channelName, const AZStd::string& message)
    {
        if (!IsConnected())
        {
            AZ_TracePrintf("CloudCanvas", "Send failed, not connectd.");
            return false;
        }

        awsiotsdk::ActionData::AsyncAckNotificationHandlerPtr publishResultHandler = [](uint16_t action_id, awsiotsdk::ResponseCode rc)
        {
            AZ_TracePrintf("CloudCanvas", "Publish returned response %d", rc);
        };
        uint16_t requestId = 0;
        std::unique_ptr<awsiotsdk::Utf8String> channelPtr = awsiotsdk::Utf8String::Create(channelName.c_str(), channelName.length());
        m_iotClient->PublishAsync(std::move(channelPtr), false, false, awsiotsdk::mqtt::QoS::QOS1,
            message.c_str(), publishResultHandler, requestId);
        return true;
    }

    bool CloudGemWebCommunicatorComponent::PostClientMessage(const AZStd::string& channelName, const AZStd::string& message)
    {
        auto requestJob = ServiceAPI::client_channel_broadcastRequestJob::Create([this](ServiceAPI::client_channel_broadcastRequestJob* job)
        {
            auto thisResult = job->result;
            AZ_TracePrintf("CloudCanvas", "PostClientMessage succeeded");
        },
        [this](ServiceAPI::client_channel_broadcastRequestJob* job)
        {
            auto thisResult = job->result;

            AZStd::string resultStr = thisResult.Result;
            AZ_TracePrintf("CloudCanvas", "PostClientMessage failed %s", job->error.message.c_str());
        }
        );

        requestJob->parameters.message_info.ChannelName = channelName;
        requestJob->parameters.message_info.Message = message;
        requestJob->Start();
        return true;
    }

    void CloudGemWebCommunicatorComponent::GotMessage(const AZStd::string channelName, const AZStd::string payload)
    {
        AZ_TracePrintf("CloudCanvas","Received Message on topic %s: %s", channelName.c_str(), payload.c_str());
        EBUS_EVENT(CloudGemWebCommunicatorUpdateBus, MessageReceived, channelName, payload);
    }

    awsiotsdk::ResponseCode CloudGemWebCommunicatorComponent::Subscribe(const AZStd::vector<AZStd::string>& topicList) 
    {
        if (!m_iotClient)
        {
            AZ_Warning("CloudCanvas", false, "Not connected");
            return awsiotsdk::ResponseCode::FAILURE;
        }

        if (!topicList.size())
        {
            return awsiotsdk::ResponseCode::FAILURE;
        }

        AZ::EntityId entityId = GetEntityId();
        // VS 2013 requires capture of 'this' for referencing CloudgemWebCommunicatorComponent::CloudGemWebCOmmunicatorLibraryResponse::GotMessage
        awsiotsdk::mqtt::Subscription::ApplicationCallbackHandlerPtr messageHandler = [entityId, this](awsiotsdk::util::String topic_name, awsiotsdk::util::String payload, std::shared_ptr<awsiotsdk::mqtt::SubscriptionHandlerContextData> p_app_handler_data) -> awsiotsdk::ResponseCode
        {
            rapidjson::Document parseDoc;
            parseDoc.Parse<rapidjson::kParseStopWhenDoneFlag>(payload.c_str());

            if (parseDoc.HasParseError())
            {
                return awsiotsdk::ResponseCode::FAILURE;
            }
            AZStd::string channelName;
            AZStd::string channelMessage;
            auto itr = parseDoc.FindMember("Channel");
            if (itr != parseDoc.MemberEnd() && itr->value.IsString())
            {
                channelName = itr->value.GetString();
            }

            itr = parseDoc.FindMember("Message");
            if (itr != parseDoc.MemberEnd() && itr->value.IsString())
            {
                channelMessage = itr->value.GetString();
            }
            EBUS_QUEUE_EVENT_ID(entityId, CloudGemWebCommunicatorLibraryResponseBus, GotMessage, channelName, channelMessage);
            return awsiotsdk::ResponseCode::SUCCESS;
        };

        // VS 2013 requires capture of 'this' for referencing CloudgemWebCommunicatorComponent::CloudGemWebCommunicatorLibraryResponse::GotSubscrbeResponse
        auto subscribeLambda = [entityId, this, messageHandler](AZStd::vector<AZStd::string> channelList)
        {
            awsiotsdk::util::Vector<std::shared_ptr<awsiotsdk::mqtt::Subscription>> topic_vector;

            awsiotsdk::ResponseCode rc;
            {
                AZStd::lock_guard<AZStd::mutex> subscriptionLock{ m_subscriptionMutex };
                auto channelIter = channelList.begin();
                while(channelIter != channelList.end())
                {
                    if (GetSubscriptionStatus(*channelIter) == "Subscribed")
                    {
                        AZ_Warning("CloudCanvas", false, "Requesting to subscribe to a channel we're subscribed to");
                        channelIter = channelList.erase(channelIter);
                        continue;
                    }
                    if (!channelIter->length())
                    {
                        AZ_Warning("CloudCanvas", false, "Can't subscribe to null channel");
                        channelIter = channelList.erase(channelIter);
                        continue;
                    }
                    std::shared_ptr<awsiotsdk::mqtt::Subscription> p_subscription = awsiotsdk::mqtt::Subscription::Create(awsiotsdk::Utf8String::Create(channelIter->c_str(), channelIter->length()), awsiotsdk::mqtt::QoS::QOS0, messageHandler, nullptr);
                    topic_vector.push_back(p_subscription);
                    ++channelIter;
                }
                if (!topic_vector.size())
                {
                    AZ_Warning("CloudCanvas", false, "Nothing to subscribe to.");
                    return;
                }
                if (!m_iotClient || !m_iotClient->IsConnected())
                {
                    AZ_Warning("CloudCanvas", false, "Not connected");
                    return;
                }
                rc = m_iotClient->Subscribe(topic_vector, subscriptionTimeout);

                // Update our data immediately so additional requests don't subscribe to the same channel
                AZStd::lock_guard<AZStd::mutex> channelLock{ m_channelMutex };
                for (auto channelName : channelList)
                {
                    if (rc == awsiotsdk::ResponseCode::SUCCESS)
                    {
                        m_channelList.push_back(channelName);
                    }
                }
            }
            EBUS_QUEUE_EVENT_ID(entityId, CloudGemWebCommunicatorLibraryResponseBus, GotSubscribeResponse, rc, channelList, m_iotClient);
        };

        AZ::Job* subscribeJob = AZ::CreateJobFunction(AZStd::bind(subscribeLambda, topicList), true, m_jobContext);
        subscribeJob->Start();
        return awsiotsdk::ResponseCode::SUCCESS;
    }

    void CloudGemWebCommunicatorComponent::GotSubscribeResponse(ResponseCodeType responseCode,  AZStd::vector<AZStd::string> channelList, std::shared_ptr<awsiotsdk::MqttClient> connectionClient)
    {
        for (auto channelName : channelList)
        {
            AZ_TracePrintf("CloudCanvas", "Got Subscription Response for channel %s: %d", channelName.c_str(), responseCode);
            EBUS_EVENT(CloudGemWebCommunicatorUpdateBus, SubscriptionStatusChanged, channelName, GetSubscriptionStatus(channelName));
        }
    }

    awsiotsdk::ResponseCode  CloudGemWebCommunicatorComponent::Unsubscribe(const AZStd::string& topicName)
    {
        AZ::EntityId entityId = GetEntityId();
        // VS 2013 requires capture of 'this' for referencing CloudgemWebCommunicatorComponent::CloudGemWebCommunicatorLibraryResponse::GotSubscrbeResponse
        auto unsubscribeLambda = [entityId, this](AZStd::string channelName)
        {
            std::unique_ptr<awsiotsdk::Utf8String> p_topic_name = awsiotsdk::Utf8String::Create(channelName.c_str(), channelName.length());
            awsiotsdk::util::Vector<std::unique_ptr<awsiotsdk::Utf8String>> topic_vector;
            topic_vector.push_back(std::move(p_topic_name));
            if (!channelName.length())
            {
                AZ_Warning("CloudCanvas", false, "Invalid Channel");
                return;
            }
            if (!m_iotClient || !m_iotClient->IsConnected())
            {
                AZ_Warning("CloudCanvas", false, "Not connected");
                return;
            }
            awsiotsdk::ResponseCode rc = m_iotClient->Unsubscribe(std::move(topic_vector), subscriptionTimeout);
            EBUS_QUEUE_EVENT_ID(entityId, CloudGemWebCommunicatorLibraryResponseBus, GotUnsubscribeResponse, rc, channelName, m_iotClient);
        };
        AZ::Job* unsubscribeJob = AZ::CreateJobFunction(AZStd::bind(unsubscribeLambda, topicName), true, m_jobContext);
        unsubscribeJob->Start();
        return awsiotsdk::ResponseCode::SUCCESS;
    }
    void CloudGemWebCommunicatorComponent::GotUnsubscribeResponse(ResponseCodeType responseCode, AZStd::string channelName, std::shared_ptr<awsiotsdk::MqttClient> connectionClient)
    {
        AZ_TracePrintf("CloudCanvas", "Got Unsubscribe Response for channel %s: %d", channelName.c_str(), responseCode);
        if (responseCode == awsiotsdk::ResponseCode::SUCCESS)
        {
            AZStd::lock_guard<AZStd::mutex> channelLock{ m_channelMutex };
            m_channelList.erase(AZStd::remove_if(m_channelList.begin(), m_channelList.end(), [channelName](const AZStd::string& thisChannel) { return thisChannel == channelName; }), m_channelList.end());
        }
        EBUS_EVENT(CloudGemWebCommunicatorUpdateBus, SubscriptionStatusChanged, channelName, GetSubscriptionStatus(channelName));
    }


    AZStd::string CloudGemWebCommunicatorComponent::GetRegistrationStatus()
    {
        return GetEndpoint().length() ? "Registered" : "Not Registered";
    }

    AZStd::string CloudGemWebCommunicatorComponent::GetConnectionStatus()
    {
        // We rely on our data for the connecting state, otherwise better to trust the lower level iotClient
        if (m_connectionStatus == ConnectionStatus::CONNECTING)
        {
            return "Connecting";
        }

        return m_iotClient && m_iotClient->IsConnected() ? "Connected" : "Not Connected";
    }

    void CloudGemWebCommunicatorComponent::SetConnectionStatus(ConnectionStatus newStatus)
    {
        m_connectionStatus = newStatus;
    }

    AZStd::string CloudGemWebCommunicatorComponent::GetSubscriptionStatus(const AZStd::string& channelName)
    {
        AZStd::lock_guard<AZStd::mutex> channelLock{ m_channelMutex };
        for (auto currentChannel : m_channelList)
        {
            if (currentChannel == channelName)
            {
                return "Subscribed";
            }
        }
        return "Not Subscribed";
    }

    AZStd::vector<AZStd::string> CloudGemWebCommunicatorComponent::GetSubscriptionList()
    {
        AZStd::lock_guard<AZStd::mutex> channelLock{ m_channelMutex };
        return m_channelList;
    }

    AZStd::string CloudGemWebCommunicatorComponent::GetEndpointPortString()
    {
        AZStd::string returnString{ GetEndpoint() };
        returnString += ":";
        returnString += AZStd::to_string(GetEndpointPort());
        return returnString;
    }
}
