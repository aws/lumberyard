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

#include <CrtDebugStats.h>

// Included only once per DLL module.
#include <platform_impl.h>

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/SystemFile.h>

#include <AzCore/std/containers/deque.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/mutex.h>

#ifdef SendMessage
#undef SendMessage
#endif

#ifdef GetObject
#undef GetObject
#endif

#include <Configuration/ClientManagerImpl.h>
#include <Configuration/MaglevConfigImpl.h>
#include <Memory/LmbrAWSMemoryManager.h>
#include <LmbrAWS.h>
#include <LmbrAWS/ILmbrAWS.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <AzCore/std/containers/queue.h>
#include <AzFramework/StringFunc/StringFunc.h>

#pragma warning(push)
#pragma warning(disable: 4819) // Invalid character not in default code page
// Core_EXPORTS.h disables 4251, but #pragma once is preventing it from being included in this block
#pragma warning(disable: 4251) // C4251: needs to have dll-interface to be used by clients of class
#include <aws/core/Aws.h>
#include <aws/core/utils/logging/DefaultLogSystem.h>
#include <aws/core/utils/logging/AWSLogging.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/UnreferencedParam.h>
#pragma warning(pop)

#if defined(WIN64) || defined(MAC) || defined(IOS)
#include <cstdlib> // for std::getenv
#endif

using namespace AzFramework::StringFunc;
using namespace Aws::Auth;
using namespace Aws::Client;

namespace LmbrAWS
{
    static const int QUEUE_INITIAL_SIZE = 1024;
    static const char* AWS_LOG_PATH = "AWSLogs";
    static const char* ALLOC_TAG = "LmbrAWS";

    static const char* OVERRIDE_RESOURCE_MAP_FILE_ARGUMENT = "cc_override_resource_map";

    //////////////////////////////////////////////////////////////////////////////////////////
    /// Wraps a MainThreadTask in a dynamically-allocated object that implements the
    /// IMainThreadRunnable interface.
    //////////////////////////////////////////////////////////////////////////////////////////
    class AWSTaskWrapper
        : public IAWSMainThreadRunnable
    {
    public:
        AWSTaskWrapper(const ILmbrAWS::AWSMainThreadTask& task);

    public:
        virtual void RunOnMainThread() override;

    private:
        ILmbrAWS::AWSMainThreadTask m_task;
    };


    //////////////////////////////////////////////////////////////////////////////////////////
    /// TaskWrapper methods
    //////////////////////////////////////////////////////////////////////////////////////////
    AWSTaskWrapper::AWSTaskWrapper(const ILmbrAWS::AWSMainThreadTask& task)
        : m_task(task)
    {
    }

    void AWSTaskWrapper::RunOnMainThread()
    {
        if (m_task)
        {
            m_task();
        }

        delete this;
    }

    //////////////////////////////////////////////////////////////////////////////////////////

    class LmbrAWSImpl
        : public ILmbrAWS
    {
    public:
        virtual void PostCompletedRequests() final override;
        virtual IClientManager* GetClientManager() final override;
        virtual void PostToMainThread(IAWSMainThreadRunnable* runnable) final override;
        virtual void PostToMainThread(const AWSMainThreadTask& task) final override;
        virtual void Release() override;
        MaglevConfig* GetMaglevConfig() final override;

        virtual void LoadCloudSettings(const char* applicationName, ILmbrAWS::AwsCloudSettings& settings) override;
        virtual void SaveCloudSettings(const char* applicationName, const ILmbrAWS::AwsCloudSettings& settings) override;

        virtual void Initialize() override;
    private:
        friend class EngineModule_LmbrAWS;

        LmbrAWSImpl();
        virtual ~LmbrAWSImpl();

        string GetLogicalMappingsPath();

        void InitializeGameMappings();

        typedef AZStd::queue<IAWSMainThreadRunnable*> RunnableQueue;
        typedef std::vector<IAWSMainThreadRunnable*> RunnableStage;

        AZStd::mutex m_runnableQueueLock;
        RunnableQueue m_queue;
        RunnableStage m_stage;

        MaglevConfigImpl m_maglevConfiguration;
        LmbrAWSMemoryManager m_memoryManager;

        //this needs to be hit after the constructor is entered. We need to control the lifetime of this memory.
        ClientManagerImpl* m_clientManager;

        Aws::SDKOptions m_awsSDKOptions;
        bool m_shouldShutdownAwsApi{false};

        const string MAPPING_FILE_GLOB = "*.awsLogicalMappings.json";
        const string MAPPING_FILE_FOLDER = "Config/";
    };

    LmbrAWSImpl::LmbrAWSImpl()
        : m_stage()
        , m_maglevConfiguration(AZStd::make_shared<MaglevConfigPersistenceLayerCryPakImpl>())
        , m_clientManager(nullptr)
    {

        // Determine if the Aws API has already been initialized. If present,
        // the CloudCanvasCommon gem will initialize the API, otherwise LmbrAWS
        // does it.

        bool isAwsApiInitialized = false;
        EBUS_EVENT_RESULT(isAwsApiInitialized, AwsApiInitRequestBus, IsAwsApiInitialized);

        if(!isAwsApiInitialized)
        {
            Aws::Utils::Logging::LogLevel logLevel = Aws::Utils::Logging::LogLevel::Info;
#ifdef _DEBUG
            logLevel = Aws::Utils::Logging::LogLevel::Trace;
#endif
            const char* logRoot = gEnv->pFileIO->GetAlias("@log@");

            if (logRoot)
            {
                AZStd::string loggingPath;
                if (Path::Join(logRoot, AWS_LOG_PATH, loggingPath))
                {
                    AZ::IO::SystemFile::CreateDir(loggingPath.c_str());

                    AZStd::string logPrefix;
                    if (Path::Join(loggingPath.c_str(), "LmbrAWS", logPrefix))
                    {
                        m_awsSDKOptions.loggingOptions.defaultLogPrefix = logPrefix.c_str();
                    }
                    else
                    {
                        AZ_Warning("CloudCanvas", false, "Failed to join paths %s and LmbrAWS", loggingPath.c_str(), AWS_LOG_PATH);
                    }
                }
                else
                {
                    AZ_Warning("CloudCanvas", false, "Failed to join paths %s and %s", logRoot, AWS_LOG_PATH);
                }
            }
            else
            {
                AZ_Warning("CloudCanvas", false, "Failed to get Log Path to initialize AWS SDK");
            }

            m_awsSDKOptions.loggingOptions.logLevel = logLevel;
            m_awsSDKOptions.memoryManagementOptions.memoryManager = &m_memoryManager;

            Aws::InitAPI(m_awsSDKOptions);

            m_shouldShutdownAwsApi = true;


        }


        m_clientManager = Aws::New<ClientManagerImpl>(ALLOC_TAG);

        m_maglevConfiguration.LoadConfig();

    }

    void LmbrAWSImpl::Initialize()
    {
        InitializeGameMappings();
    }

    void LmbrAWSImpl::InitializeGameMappings()
    {
        if (!gEnv->IsEditor())
        {
            // note that the LmbrAWS module is initialized before the game is loaded so a console variable can't be
            // used to drive this mapping. Instead a command line parameter is used.
            string mappingPath;

            ICmdLine* cmdLine = gEnv->pSystem->GetICmdLine();
            if (cmdLine)
            {
                const ICmdLineArg* command = cmdLine->FindArg(eCLAT_Pre, OVERRIDE_RESOURCE_MAP_FILE_ARGUMENT);
                if (command)
                {
                    mappingPath = command->GetValue();
                }
            }

#if defined(WIN64) || defined(MAC) || defined(IOS)
            if (mappingPath.empty())
            {
                const char* value = std::getenv(OVERRIDE_RESOURCE_MAP_FILE_ARGUMENT);
                if (value)
                {
                    mappingPath = value;
                }
            }
#endif

            if (mappingPath.empty())
            {
                mappingPath = GetLogicalMappingsPath();
            }

            if (m_clientManager->LoadLogicalMappingsFromFile(mappingPath))
            {
                bool shouldApplyMapping = true;

#if defined (WIN32) && defined (_DEBUG)
                // Dialog boxes seem to be only available on windows for now :(
                static const char* PROTECTED_MAPPING_MSG_TITLE = "AWS Mapping Is Protected";
                static const char* PROTECTED_MAPPING_MSG_TEXT = "Warning: The AWS resource mapping file is marked as protected and shouldn't be used for normal development work. Are you sure you want to continue?";
                if (m_clientManager->IsProtectedMapping())
                {
                    shouldApplyMapping = CryMessageBox(PROTECTED_MAPPING_MSG_TEXT, PROTECTED_MAPPING_MSG_TITLE, MB_ICONEXCLAMATION | MB_YESNO) == IDYES ? true : false;
                    m_clientManager->SetIgnoreProtection(shouldApplyMapping);
                }
#endif
                if (shouldApplyMapping)
                {
                    m_clientManager->ApplyConfiguration();
                }
            }
        }
    }

    string LmbrAWSImpl::GetLogicalMappingsPath()
    {
        // enumerate files
        auto cryPak = gEnv->pCryPak;
        _finddata_t findData;
        intptr_t findHandle = cryPak->FindFirst(MAPPING_FILE_FOLDER + MAPPING_FILE_GLOB, &findData);
        std::vector<string> mappingFiles;
        if (findHandle != -1)
        {
            do
            {
                mappingFiles.push_back(findData.name);
            } while (cryPak->FindNext(findHandle, &findData) != -1);
            cryPak->FindClose(findHandle);
        }

        // if only one mapping file, provide that name
        if (mappingFiles.size() == 1)
        {
            return MAPPING_FILE_FOLDER + mappingFiles[0];
        }
        else if (mappingFiles.size() > 1)
        {
            gEnv->pLog->LogWarning("Multiple Cloud Canvas mapping files found. Please use the %s commands line parameter to select a mapping file.", OVERRIDE_RESOURCE_MAP_FILE_ARGUMENT);
            return string();
        }
        else
        {
            gEnv->pLog->LogWarning("No Cloud Canvas mapping file found");
            return string();
        }
    }

    LmbrAWSImpl::~LmbrAWSImpl()
    {
        Aws::Delete(m_clientManager);
        if(m_shouldShutdownAwsApi)
        {
            Aws::ShutdownAPI(m_awsSDKOptions);
        }
    }

    void LmbrAWSImpl::Release()
    {
        delete this;
    }

    void LmbrAWSImpl::PostCompletedRequests()
    {
        // Unload tasks of the threadsafe queue into a staging array. This ensures that any new tasks that may be
        // generated as side-effects will be run on the next frame instead of this one, and we can't accidentally
        // get stuck endlessly processing tasks.
        {
            AZStd::lock_guard<AZStd::mutex> runableLockGuard(m_runnableQueueLock);
            IAWSMainThreadRunnable* runnable;
            while (!m_queue.empty())
            {
                runnable = m_queue.back();
                m_stage.push_back(runnable);
                m_queue.pop();
            }
        }

        // Run the tasks we've staged for running this frame.
        for (auto itr : m_stage)
        {
            itr->RunOnMainThread();
        }

        // Reuse the same staging vector from frame-to-frame, purely as a performance optimization.
        m_stage.clear();
    }

    void LmbrAWSImpl::PostToMainThread(IAWSMainThreadRunnable* runnable)
    {
        if (runnable)
        {
            AZStd::lock_guard<AZStd::mutex> runableLockGuard(m_runnableQueueLock);
            // push() should never fail, but we put in a loop anyway to conform to the standard usage pattern
            m_queue.push(runnable);
        }
    }

    void LmbrAWSImpl::PostToMainThread(const AWSMainThreadTask& task)
    {
        // Wrap the task in dynamically-allocated memory before putting it on the queue
        auto wrapper = new AWSTaskWrapper(task);
        PostToMainThread(wrapper);
    }

    IClientManager* LmbrAWSImpl::GetClientManager()
    {
        return m_clientManager;
    }

    MaglevConfig* LmbrAWSImpl::GetMaglevConfig()
    {
        return &m_maglevConfiguration;
    }

    void LmbrAWSImpl::LoadCloudSettings(const char* applicationName, ILmbrAWS::AwsCloudSettings& settings)
    {
        AWS_UNREFERENCED_PARAM(applicationName);
        AWS_UNREFERENCED_PARAM(settings);
    }

    void LmbrAWSImpl::SaveCloudSettings(const char* applicationName, const ILmbrAWS::AwsCloudSettings& settings)
    {
        AWS_UNREFERENCED_PARAM(applicationName);
        AWS_UNREFERENCED_PARAM(settings);
    }

    class EngineModule_LmbrAWS
        : public IEngineModule
    {
        CRYINTERFACE_SIMPLE(IEngineModule)

            CRYGENERATE_SINGLETONCLASS(EngineModule_LmbrAWS, "EngineModule_LmbrAWS", 0xE3EEEBC474C6435C, 0x9AF000711C613950)

            virtual const char* GetName() const {
            return "LmbrAWS";
        };
        virtual const char* GetCategory() const { return "CryEngine"; };

#ifdef AUTH_TOKEN_CVAR_ENABLED // See security warning in ClientManagerImpl.h before enabling the auth_token cvar.
        static void AuthTokenSet(ICVar* pCVar)
        {
            auto authTokenVar = pCVar->GetString();
            ClientManagerImpl::ParsedAuthToken parsedAuthToken;
            if (ClientManagerImpl::ParseAuthTokensFromConsoleVariable(authTokenVar, parsedAuthToken))
            {
                GetLmbrAWS()->GetClientManager()->Login(parsedAuthToken.provider.c_str(), parsedAuthToken.code.c_str());
            }
        }
#endif // AUTH_TOKEN_CVAR_ENABLED

        virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
        {
            ISystem* system = env.pSystem;
            ModuleInitISystem(system, "LmbrAWS");
            env.pLmbrAWS = new LmbrAWSImpl();
            env.pLmbrAWS->Initialize();

#ifdef AUTH_TOKEN_CVAR_ENABLED // See security warning in ClientManagerImpl.h before enabling the auth_token cvar.
            REGISTER_STRING_CB("auth_token", "", 0, "Sets the auth provider token for federated login into other systems. Should be <provider>:<code>", AuthTokenSet);
#endif // AUTH_TOKEN_CVAR_ENABLED
            return env.pLmbrAWS != 0;
        }
    };

    CRYREGISTER_SINGLETON_CLASS(EngineModule_LmbrAWS)

    EngineModule_LmbrAWS::EngineModule_LmbrAWS()
    {
    };

    EngineModule_LmbrAWS::~EngineModule_LmbrAWS()
    {
    };

    IClientManager* GetClientManager()
    {
        return GetLmbrAWS()->GetClientManager();
    }

    MaglevConfig* GetMaglevConfig()
    {
        return GetLmbrAWS()->GetMaglevConfig();
    }
} // Namespace LmbrAWS

ILmbrAWS* GetLmbrAWS()
{
    return gEnv->pLmbrAWS;
}

