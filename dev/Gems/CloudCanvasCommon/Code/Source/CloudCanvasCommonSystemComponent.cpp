/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#include "StdAfx.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/base.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <ISystem.h>
#include <ICryPak.h>

#include <LmbrAWS/ILmbrAWS.h>
#include <LmbrAWS/IAWSClientManager.h>

#include "CloudCanvasCommonSystemComponent.h"

#include <aws/core/Aws.h>
#include <aws/core/utils/logging/DefaultLogSystem.h>
#include <aws/core/utils/logging/AWSLogging.h>
#include <aws/core/utils/logging/LogSystemInterface.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/UnreferencedParam.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>

#include <AzCore/IO/SystemFile.h>


namespace CloudCanvasCommon
{

    const char* CloudCanvasCommonSystemComponent::COMPONENT_DISPLAY_NAME = "CloudCanvasCommon";
    const char* CloudCanvasCommonSystemComponent::COMPONENT_DESCRIPTION = "Provides functionality used by other Cloud Canvas gems.";
    const char* CloudCanvasCommonSystemComponent::COMPONENT_CATEGORY = "CloudCanvas";
    const char* CloudCanvasCommonSystemComponent::SERVICE_NAME = "CloudCanvasCommonService";

    const char* CloudCanvasCommonSystemComponent::AWS_API_ALLOC_TAG = "AwsApi";
    const char* CloudCanvasCommonSystemComponent::AWS_API_LOG_PREFIX = "AwsApi-";

    static const char* pakCertPath = "certs/aws/cacert.pem";

    void CloudCanvasCommonSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudCanvasCommonSystemComponent, AZ::Component>()
                ->Version(0)
                ->SerializerForEmptyClass();

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudCanvasCommonSystemComponent>(COMPONENT_DISPLAY_NAME, COMPONENT_DESCRIPTION)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, COMPONENT_CATEGORY)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC(COMPONENT_CATEGORY))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<LmbrAWS::ClientManagerNotificationBus>("ClientManagerNotificationBus")
                ->Handler<LmbrAWS::ClientManagerNotificationBusHandler>();
        }
    }

    void CloudCanvasCommonSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC(SERVICE_NAME));
    }

    void CloudCanvasCommonSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC(SERVICE_NAME));
    }

    void CloudCanvasCommonSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void CloudCanvasCommonSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    bool CloudCanvasCommonSystemComponent::IsAwsApiInitialized()
    {
        return m_awsApiInitialized;
    }

    void CloudCanvasCommonSystemComponent::Init()
    {
    }

    void CloudCanvasCommonSystemComponent::Activate()
    {
        
        InitAwsApi();

        CloudCanvasCommonRequestBus::Handler::BusConnect();
        LmbrAWS::AwsApiInitRequestBus::Handler::BusConnect();

    }

    void CloudCanvasCommonSystemComponent::Deactivate()
    {
        
        LmbrAWS::AwsApiInitRequestBus::Handler::BusDisconnect();
        CloudCanvasCommonRequestBus::Handler::BusDisconnect();

        // We require that anything that owns memory allocated by the AWS 
        // API be destructed before this component is deactivated.
        ShutdownAwsApi();

    }

    class LumberyardLogSystemInterface : 
        public Aws::Utils::Logging::LogSystemInterface
    {

    public:

        LumberyardLogSystemInterface(Aws::Utils::Logging::LogLevel logLevel)
            : m_logLevel(logLevel)
        {
        }

        /**
        * Gets the currently configured log level for this logger.
        */
        Aws::Utils::Logging::LogLevel GetLogLevel(void) const override
        {
            Aws::Utils::Logging::LogLevel newLevel = m_logLevel;
            if (gEnv && gEnv->pConsole)
            {
                ICVar* pCvar = gEnv->pConsole->GetCVar("sys_SetLogLevel");

                if (pCvar)
                {
                    newLevel = (Aws::Utils::Logging::LogLevel) pCvar->GetIVal();
                }
            }    

            return newLevel != m_logLevel ? newLevel : m_logLevel;            
        }

        /**
        * Does a printf style output to the output stream. Don't use this, it's unsafe. See LogStream
        */
        void Log(Aws::Utils::Logging::LogLevel logLevel, const char* tag, const char* formatStr, ...)
        {

            if (!ShouldLog(logLevel))
            {
                return;
            }

            char message[MAX_MESSAGE_LENGTH];

            va_list mark;
            va_start(mark, formatStr);
            azvsnprintf(message, MAX_MESSAGE_LENGTH, formatStr, mark);
            va_end(mark);

            ForwardAwsApiLogMessage(logLevel, tag, message);

        }

        /**
        * Writes the stream to the output stream.
        */
        void LogStream(Aws::Utils::Logging::LogLevel logLevel, const char* tag, const Aws::OStringStream &messageStream)
        {

            if(!ShouldLog(logLevel)) 
            {
                return;
            }

            ForwardAwsApiLogMessage(logLevel, tag, messageStream.str().c_str());

        }

    private:

        bool ShouldLog(Aws::Utils::Logging::LogLevel logLevel)
        {
            Aws::Utils::Logging::LogLevel newLevel = GetLogLevel();
            if (newLevel > Aws::Utils::Logging::LogLevel::Info && newLevel <= Aws::Utils::Logging::LogLevel::Trace && newLevel != m_logLevel)
            {
                SetLogLevel(newLevel);
            }

            return (logLevel <= m_logLevel);
        }

        void SetLogLevel(Aws::Utils::Logging::LogLevel newLevel)
        {
            Aws::Utils::Logging::ShutdownAWSLogging();
            Aws::Utils::Logging::InitializeAWSLogging(Aws::MakeShared<LumberyardLogSystemInterface>("AWS", newLevel));
            m_logLevel = newLevel;
        }

        static const int MAX_MESSAGE_LENGTH = 4096;
        Aws::Utils::Logging::LogLevel m_logLevel;
        const char* MESSAGE_FORMAT = "[AWS] %s - %s";

        void ForwardAwsApiLogMessage(Aws::Utils::Logging::LogLevel logLevel, const char* tag, const char* message)
        {

            switch (logLevel)
            {

            case Aws::Utils::Logging::LogLevel::Off:
                break;

            case Aws::Utils::Logging::LogLevel::Fatal:
                AZ::Debug::Trace::Instance().Error(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, CloudCanvasCommonSystemComponent::COMPONENT_DISPLAY_NAME, MESSAGE_FORMAT, tag, message);
                break;

            case Aws::Utils::Logging::LogLevel::Error:
                AZ::Debug::Trace::Instance().Warning(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, CloudCanvasCommonSystemComponent::COMPONENT_DISPLAY_NAME, MESSAGE_FORMAT, tag, message);
                break;

            case Aws::Utils::Logging::LogLevel::Warn:
                AZ::Debug::Trace::Instance().Warning(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, CloudCanvasCommonSystemComponent::COMPONENT_DISPLAY_NAME, MESSAGE_FORMAT, tag, message);
                break;

            case Aws::Utils::Logging::LogLevel::Info:
                AZ::Debug::Trace::Instance().Printf(CloudCanvasCommonSystemComponent::COMPONENT_DISPLAY_NAME, MESSAGE_FORMAT, tag, message);
                break;

            case Aws::Utils::Logging::LogLevel::Debug:
                AZ::Debug::Trace::Instance().Printf(CloudCanvasCommonSystemComponent::COMPONENT_DISPLAY_NAME, MESSAGE_FORMAT, tag, message);
                break;

            case Aws::Utils::Logging::LogLevel::Trace:
                AZ::Debug::Trace::Instance().Printf(CloudCanvasCommonSystemComponent::COMPONENT_DISPLAY_NAME, MESSAGE_FORMAT, tag, message);
                break;

            default:
                break;

            }

        }
    };

    void CloudCanvasCommonSystemComponent::InitAwsApi()
    {
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        Aws::Utils::Logging::LogLevel logLevel;
#ifdef _DEBUG
        logLevel = Aws::Utils::Logging::LogLevel::Warn;
#else
        logLevel = Aws::Utils::Logging::LogLevel::Warn;
#endif
        m_awsSDKOptions.loggingOptions.logLevel = logLevel;
        m_awsSDKOptions.loggingOptions.logger_create_fn = [logLevel]() 
        {
            return Aws::MakeShared<LumberyardLogSystemInterface>("AWS", logLevel);
        };
        m_awsSDKOptions.memoryManagementOptions.memoryManager = &m_memoryManager;

        LmbrAWS::RequestRootCAFileResult requestResult = GetRootCAFile(m_resolvedCertPath);

        AZ_TracePrintf("CloudCanvas", "Initialized CloudCanvasCommon with cert path %s result %d", m_resolvedCertPath.c_str(), requestResult);


        Aws::InitAPI(m_awsSDKOptions);

        m_awsApiInitialized = true;
#endif // #if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
    }

    void CloudCanvasCommonSystemComponent::ShutdownAwsApi()
    {
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        m_awsApiInitialized = false;
        Aws::ShutdownAPI(m_awsSDKOptions);

#endif // #if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
    }

    AZStd::string CloudCanvasCommonSystemComponent::GetLogicalToPhysicalResourceMapping(const AZStd::string& logicalResourceName)
    {
        AZStd::string physicalResourceName = gEnv->pLmbrAWS->GetClientManager()->GetConfigurationParameters().GetParameter(logicalResourceName.c_str()).c_str();
        AZ_Error(COMPONENT_DISPLAY_NAME, !physicalResourceName.empty(), "No mapping provided for the %s resource.", logicalResourceName.c_str());
        return physicalResourceName;
    }

    LmbrAWS::S3::BucketClient CloudCanvasCommonSystemComponent::GetBucketClient(const AZStd::string& bucketName)
    {
        LmbrAWS::S3::BucketClient bucketClient = gEnv->pLmbrAWS->GetClientManager()->GetS3Manager().CreateBucketClient(bucketName.c_str());
        return bucketClient;
    }

    AZStd::vector<AZStd::string> CloudCanvasCommonSystemComponent::GetLogicalResourceNames(const AZStd::string& resourceType)
    {
        if (resourceType == "AWS::Lambda::Function")
        {
            // settings use old CryStrings so no easy conversion
            if (gEnv)
            {
                std::vector<string> names = gEnv->pLmbrAWS->GetClientManager()->GetLambdaManager().GetFunctionClientSettingsCollection().GetNames();
                AZStd::vector<AZStd::string> retContainter;
                retContainter.reserve(names.size());
                for (const string& cryString : names)
                {
                    retContainter.push_back(AZStd::string(cryString.c_str(), cryString.length()));
                }
                return retContainter;
            }
        }
        else
        {
            AZ_Error(COMPONENT_DISPLAY_NAME, false, "GetLogicalResourceNames currently only supports Lambda");
        }
        return AZStd::vector<AZStd::string>();
    }



    LmbrAWS::RequestRootCAFileResult CloudCanvasCommonSystemComponent::LmbrRequestRootCAFile(AZStd::string& resultPath)
    {
        return GetRootCAFile(resultPath);
    }

    LmbrAWS::RequestRootCAFileResult CloudCanvasCommonSystemComponent::RequestRootCAFile(AZStd::string& resultPath)
    {
        return GetRootCAFile(resultPath);
    }

    AZStd::string CloudCanvasCommonSystemComponent::GetRootCAUserPath() const
    {
        AZStd::string userPath{ "@user@/" };
        userPath += pakCertPath;
        return userPath;
    }

    bool CloudCanvasCommonSystemComponent::DoesPlatformUseRootCAFile()
    {
#if defined(AZ_PLATFORM_ANDROID)
        return true;
#else
        return false;
#endif
    }

    LmbrAWS::RequestRootCAFileResult CloudCanvasCommonSystemComponent::GetRootCAFile(AZStd::string& resultPath)
    {
        if (!DoesPlatformUseRootCAFile() )
        {
            return LmbrAWS::ROOTCA_PLATFORM_NOT_SUPPORTED;
        }

        if (m_resolvedCert)
        {
            AZStd::lock_guard<AZStd::mutex> pathLock(m_certPathMutex);
            resultPath = m_resolvedCertPath;
            return resultPath.length() ? LmbrAWS::ROOTCA_FOUND_FILE_SUCCESS : LmbrAWS::ROOTCA_USER_FILE_NOT_FOUND;
        }

        CheckCopyRootCAFile();

        AZStd::string resolvedPath;
        if (!GetResolvedUserPath(resolvedPath))
        {
            AZ_TracePrintf("CloudCanvas","Failed to resolve RootCA User path, returning");
            return LmbrAWS::ROOTCA_USER_FILE_RESOLVE_FAILED;
        }

        AZStd::lock_guard<AZStd::mutex> pathLock(m_certPathMutex);
        if (AZ::IO::SystemFile::Exists(resolvedPath.c_str()))
        {
             m_resolvedCertPath = resolvedPath;
        }
        m_resolvedCert = true;
        resultPath = m_resolvedCertPath;
        return resultPath.length() ? LmbrAWS::ROOTCA_FOUND_FILE_SUCCESS : LmbrAWS::ROOTCA_USER_FILE_NOT_FOUND;
    }

    bool CloudCanvasCommonSystemComponent::GetResolvedUserPath(AZStd::string& resolvedPath) const
    {
        static char resolvedGameFolder[AZ_MAX_PATH_LEN] = { 0 };

        AZStd::lock_guard<AZStd::mutex> resolvePathGuard(m_resolveUserPathMutex);
        // No need to re resolve this if we already have
        if (resolvedGameFolder[0] != 0)
        {
            resolvedPath = resolvedGameFolder;
            return true;
        }

        AZStd::string userPath = GetRootCAUserPath();
        if (!AZ::IO::FileIOBase::GetInstance()->ResolvePath(userPath.c_str(), resolvedGameFolder, AZ_MAX_PATH_LEN))
        {
            AZ_TracePrintf("CloudCanvas","Could not resolve path for %s", userPath.c_str());
            resolvedGameFolder[0] = 0;
            return false;
        }

        AZ_TracePrintf("CloudCanvas","Resolved cert path as %s", resolvedGameFolder);
        resolvedPath = resolvedGameFolder;
        return true;
    }

    void CloudCanvasCommonSystemComponent::CheckCopyRootCAFile()
    {
        if (m_resolvedCert )
        {
            return;
        }

        AZStd::lock_guard<AZStd::mutex> fileLock(m_certCopyMutex);
        AZStd::string userPath;
        if (!GetResolvedUserPath(userPath) || !userPath.length())
        {
            AZ_TracePrintf("CloudCanvas","Could not get resolved path");
            return;
        }
 
        AZ_TracePrintf("CloudCanvas","Ensuring RootCA in path %s", userPath.c_str());

        AZStd::string inputPath{ pakCertPath };

        AZ::IO::HandleType inputFile;
        AZ::IO::FileIOBase::GetInstance()->Open(inputPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, inputFile);
        if (!inputFile)
        {
            AZ_TracePrintf("CloudCanvas","Could not open %s for reading", pakCertPath);
            return;
        }

        AZ::IO::SystemFile outputFile;
        if (!outputFile.Open(userPath.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_READ_WRITE))
        {
            AZ::IO::FileIOBase::GetInstance()->Close(inputFile);
            AZ_Warning("CloudCanvas",false,"Could not open %s for writing", userPath.c_str());
            return;
        }

        AZ::u64 fileSize{ 0 };
        AZ::IO::FileIOBase::GetInstance()->Size(inputFile, fileSize);
        if (fileSize > 0)
        {
            AZStd::string fileBuf;
            fileBuf.resize(fileSize);
            size_t read = AZ::IO::FileIOBase::GetInstance()->Read(inputFile, fileBuf.data(), fileSize);
            outputFile.Write(fileBuf.data(), fileSize);
            AZ_TracePrintf("CloudCanvas","Successfully wrote RootCA to %s", userPath.c_str());
        }
        AZ::IO::FileIOBase::GetInstance()->Close(inputFile);
        outputFile.Close();
    }
}
