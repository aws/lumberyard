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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include <AzCore/base.h>
#include <AzCore/PlatformDef.h>
#include <AzCore/std/parallel/thread.h>

#include "CrySimpleServer.hpp"
#include "CrySimpleSock.hpp"
#include "CrySimpleJob.hpp"
#include "CrySimpleJobCompile1.hpp"
#include "CrySimpleJobCompile2.hpp"
#include "CrySimpleJobRequest.hpp"
#include "CrySimpleCache.hpp"
#include "CrySimpleErrorLog.hpp"
#include "ShaderList.hpp"

#include <Core/StdTypes.hpp>
#include <Core/Error.hpp>
#include <Core/STLHelper.hpp>
#include <Core/tinyxml/tinyxml.h>
#include <Core/WindowsAPIImplementation.h>
#include <Core/Mailer.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Jobs/Job.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/time.h>
#include <AzCore/std/bind/bind.h>
#include <AzCore/std/string/string.h>

#if defined(AZ_PLATFORM_APPLE_OSX)
#include <sys/stat.h>
#endif

#include <assert.h>
#include <algorithm>
#include <memory>
#include <unordered_map>


#ifdef WIN32
    #define EXTENSION ".exe"
#else
    #define EXTENSION ""
#endif

volatile AtomicCountType CCrySimpleServer::ms_ExceptionCount    = 0;

const static std::string SHADER_PROFILER            =   "NVShaderPerf" EXTENSION;

const static std::string SHADER_PATH_SOURCE         =   "Source";
const static std::string SHADER_PATH_BINARY         =   "Binary";
const static std::string SHADER_PATH_HALFSTRIPPED   =   "HalfStripped";
const static std::string SHADER_PATH_DISASSEMBLED   =   "DisAsm";
const static std::string SHADER_PATH_STRIPPPED      =   "Stripped";
const static std::string SHADER_PATH_CACHE          =   "Cache";

static const bool autoDeleteJobWhenDone = true;
static const int sleepTimeWhenWaiting = 10;

static AtomicCountType g_ConnectionCount = 0;

static SEnviropment gEnv;

SEnviropment&   SEnviropment::Instance()
{
    return gEnv;
}

class CThreadData
{
    uint32_t        m_Counter;
    CCrySimpleSock* m_pSock;
public:
    CThreadData(uint32_t        Counter, CCrySimpleSock* pSock)
        : m_Counter(Counter)
        , m_pSock(pSock){}

    ~CThreadData(){delete m_pSock; }

    CCrySimpleSock* Socket(){return m_pSock; }
    uint32_t        ID() const{return m_Counter; }
};

//////////////////////////////////////////////////////////////////////////

bool CopyFileOnPlatform(const char* nameOfFileToCopy, const char* copiedFileName, bool failIfFileExists)
{
    if (AZ::IO::SystemFile::Exists(copiedFileName) && failIfFileExists)
    {
        AZ_Warning(0, ("File to copy to, %s, already exists."), copiedFileName);
        return false;
    }

    AZ::IO::SystemFile fileToCopy;
    if (fileToCopy.Open(nameOfFileToCopy, AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
    {
        AZ_Warning(0, ("Unable to open file: %s for copying."), nameOfFileToCopy);
        return false;
    }

    AZ::IO::SystemFile::SizeType fileLength = fileToCopy.Length();

    AZ::IO::SystemFile newFile;
    if (newFile.Open(copiedFileName, AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE))
    {
        AZ_Warning(0, ("Unable to open new file: %s for copying."), copiedFileName);
        return false;
    }

    char* fileContents = new char[fileLength];
    fileToCopy.Read(fileLength, fileContents);
    newFile.Write(fileContents, fileLength);
    delete[] fileContents;

    return true;
}

void MakeErrorVec(const std::string& errorText, tdDataVector& Vec)
{
    Vec.resize(errorText.size() + 1);
    for (size_t i = 0; i < errorText.size(); i++)
    {
        Vec[i] = errorText[i];
    }
    Vec[errorText.size()] = 0;

    // Compress output data
    tdDataVector rDataRaw;
    rDataRaw.swap(Vec);
    if (!CSTLHelper::Compress(rDataRaw, Vec))
    {
        Vec.resize(0);
    }
}

//////////////////////////////////////////////////////////////////////////
class CompileJob
    : public AZ::Job
{
public:
    CompileJob()
        : Job(autoDeleteJobWhenDone, nullptr) { }
    void SetThreadData(CThreadData* threadData) { m_pThreadData.reset(threadData); }
protected:
    void Process() override;
private:
    std::unique_ptr<CThreadData> m_pThreadData;
};

void CompileJob::Process()
{
    std::vector<uint8_t> Vec;
    std::unique_ptr<CCrySimpleJob> Job;
    EProtocolVersion Version    =   EPV_V001;
    ECrySimpleJobState State    =   ECSJS_JOBNOTFOUND;
    try
    {
        if (m_pThreadData->Socket()->Recv(Vec))
        {
            std::string Request(reinterpret_cast<const char*>(&Vec[0]), Vec.size());
            TiXmlDocument ReqParsed("Request.xml");
            ReqParsed.Parse(Request.c_str());

            if (ReqParsed.Error())
            {
                CrySimple_ERROR("failed to parse request XML");
                return;
            }
            const TiXmlElement* pElement = ReqParsed.FirstChildElement();
            if (!pElement)
            {
                CrySimple_ERROR("failed to extract First Element of the request");
                return;
            }
            const char* pVersion   = pElement->Attribute("Version");
            const char* pPlatform  = pElement->Attribute("Platform");

            if (!pPlatform)
            {
                CrySimple_ERROR("failed to extract required platform attribute from request.");
                return;
            }

            std::string platform(pPlatform);

            SEnviropment::Instance().m_Platform = "";

            static std::unordered_map<std::string, std::string> dumpShadersFolders {
                {
                    "GL4", "Shaders\\GL4\\"
                }, {
                    "GLES3_0", "Shaders\\GLES3_0\\"
                }, {
                    "GLES3_1", "Shaders\\GLES3_1\\"
                }, {
                    "DX11", "Shaders\\DX11\\"
                }, {
                    "METAL", "Shaders\\METAL\\"
                }
            };
            auto foundShaderFolder = dumpShadersFolders.find(platform);
            if (foundShaderFolder != end(dumpShadersFolders))
            {
                SEnviropment::Instance().m_Platform = foundShaderFolder->first;
                SEnviropment::Instance().m_Shader = SEnviropment::Instance().m_Root + foundShaderFolder->second;
            }

            AZ::IO::SystemFile::CreateDir(SEnviropment::Instance().m_Shader.c_str());

            //new request type?
            if (pVersion)
            {
                if (std::string(pVersion) == "2.2")
                {
                    Version =   EPV_V0022;
                }
                else if (std::string(pVersion) == "2.1")
                {
                    Version =   EPV_V0021;
                }
                else if (std::string(pVersion) == "2.0")
                {
                    Version =   EPV_V002;
                }
            }

            if (Version >= EPV_V002)
            {
                if (Version >= EPV_V0021)
                {
                    m_pThreadData->Socket()->WaitForShutDownEvent(true);
                }

                const char* pJobType    =   pElement->Attribute("JobType");
                if (pJobType)
                {
                    const std::string JobType(pJobType);
                    if (JobType == "RequestLine")
                    {
                        Job = std::make_unique<CCrySimpleJobRequest>(m_pThreadData->Socket()->PeerIP());
                        Job->Execute(pElement);
                        State   =   Job->State();
                        Vec.resize(0);
                    }
                    else
                    if (JobType == "Compile")
                    {
                        Job = std::make_unique<CCrySimpleJobCompile2>(Version, m_pThreadData->Socket()->PeerIP(), &Vec);
                        Job->Execute(pElement);
                        State   =   Job->State();
                    }
                    else
                    {
                        printf("\nRequested unkown job %s\n", pJobType);
                    }
                }
                else
                {
                    printf("\nVersion 2.0 or higher but has no JobType tag\n");
                }
            }
            else
            {
                //legacy request
                Version =   EPV_V001;
                Job = std::make_unique<CCrySimpleJobCompile1>(m_pThreadData->Socket()->PeerIP(), &Vec);
                Job->Execute(pElement);
            }
            m_pThreadData->Socket()->Send(Vec, State, Version);

            if (Version >= EPV_V0021)
            {
                /*
                // wait until message has been succesfully delived before shutting down the connection
                if(!m_pThreadData->Socket()->RecvResult())
                {
                    printf("\nInvalid result from client\n");
                }
                */
            }
        }
    }
    catch (const ICryError* err)
    {
        CCrySimpleServer::IncrementExceptionCount();

        CRYSIMPLE_LOG("<Error> " + err->GetErrorName());

        std::string returnStr = err->GetErrorDetails(ICryError::OUTPUT_TTY);

        // Send error back
        MakeErrorVec(returnStr, Vec);

        if (Job.get())
        {
            State   =   Job->State();

            if (State == ECSJS_ERROR_COMPILE && SEnviropment::Instance().m_PrintErrors)
            {
                printf("\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
                printf("%s\n", err->GetErrorName().c_str());
                printf("%s\n", returnStr.c_str());
                printf("\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n");
            }
        }

        bool added = CCrySimpleErrorLog::Instance().Add((ICryError*)err);

        // error log hasn't taken ownership, delete this error.
        if (!added)
        {
            delete err;
        }

        m_pThreadData->Socket()->Send(Vec, State, Version);
    }
    InterlockedDecrement(&g_ConnectionCount);
}

//////////////////////////////////////////////////////////////////////////
void TickThread()
{
    AZ::u64 t0 = AZStd::GetTimeUTCMilliSecond();

    while (true)
    {
        CrySimple_SECURE_START

        AZ::u64 t1 = AZStd::GetTimeUTCMilliSecond();
        if ((t1 < t0) || (t1 - t0 > 100))
        {
            t0 = t1;
            const int maxStringSize = 512;
            char str[maxStringSize] = { 0 };
            azsnprintf(str, maxStringSize, "Amazon Remote Shader Compiler (%d compile tasks | %d open sockets | %d exceptions)",
                CCrySimpleJobCompile::GlobalCompileTasks(), CCrySimpleSock::GetOpenSockets() + CSMTPMailer::GetOpenSockets(),
                CCrySimpleServer::GetExceptionCount());
#if defined(AZ_PLATFORM_WINDOWS)
            SetConsoleTitle(str);
#endif
        }

        const AZ::u64 T1    =   AZStd::GetTimeUTCMilliSecond();
        CCrySimpleErrorLog::Instance().Tick();
        CShaderList::Instance().Tick();
        CCrySimpleCache::Instance().ThreadFunc_SavePendingCacheEntries();
        const AZ::u64 T2    =   AZStd::GetTimeUTCMilliSecond();
        if (T2 - T1 < 100)
#if defined(AZ_PLATFORM_WINDOWS)
        {
            Sleep(static_cast<DWORD>(100 - T2 + T1));
        }
#else
        {
            sleep(100 - T2 + T1);
        }
#endif

        CrySimple_SECURE_END
    }
}

//////////////////////////////////////////////////////////////////////////
void LoadCache()
{
    if (CCrySimpleCache::Instance().LoadCacheFile(SEnviropment::Instance().m_Cache + "Cache.dat"))
    {
        printf("Creating cache backup...\n");
        AZ::IO::SystemFile::Delete((SEnviropment::Instance().m_Cache + "Cache.bak2").c_str());
        printf("Move %s to %s\n", (SEnviropment::Instance().m_Cache + "Cache.bak").c_str(), (SEnviropment::Instance().m_Cache + "Cache.bak2").c_str());
        AZ::IO::SystemFile::Rename((SEnviropment::Instance().m_Cache + "Cache.bak").c_str(), (SEnviropment::Instance().m_Cache + "Cache.bak2").c_str());
        printf("Copy %s to %s\n", (SEnviropment::Instance().m_Cache + "Cache.dat").c_str(), (SEnviropment::Instance().m_Cache + "Cache.bak").c_str());
        CopyFileOnPlatform((SEnviropment::Instance().m_Cache + "Cache.dat").c_str(), (SEnviropment::Instance().m_Cache + "Cache.bak").c_str(), FALSE);
        printf("Cache backup done.\n");
    }
    else
    {
        // Restoring backup cache!
        printf("Cache file corrupted!!!\n");
        printf("Restoring backup cache...\n");
        AZ::IO::SystemFile::Delete((SEnviropment::Instance().m_Cache + "Cache.dat").c_str());
        printf("Copy %s to %s\n", (SEnviropment::Instance().m_Cache + "Cache.bak").c_str(), (SEnviropment::Instance().m_Cache + "Cache.dat").c_str());
        CopyFileOnPlatform((SEnviropment::Instance().m_Cache + "Cache.bak").c_str(), (SEnviropment::Instance().m_Cache + "Cache.dat").c_str(), FALSE);
        if (!CCrySimpleCache::Instance().LoadCacheFile(SEnviropment::Instance().m_Cache + "Cache.dat"))
        {
            // Backup file corrupted too!
            printf("Backup file corrupted too!!!\n");
            printf("Deleting cache completely\n");
            AZ::IO::SystemFile::Delete((SEnviropment::Instance().m_Cache + "Cache.dat").c_str());
        }
    }

    CCrySimpleCache::Instance().Finalize();
    printf("Ready\n");
}


//////////////////////////////////////////////////////////////////////////
CCrySimpleServer::CCrySimpleServer(const char* pShaderModel, const char* pDst, const char* pSrc, const char* pEntryFunction)
    : m_pServerSocket(nullptr)
{
    Init();
}

CCrySimpleServer::CCrySimpleServer()
    : m_pServerSocket(nullptr)
{
    CrySimple_SECURE_START

    uint32_t Port = SEnviropment::Instance().m_port;

    m_pServerSocket =   new CCrySimpleSock(Port, SEnviropment::Instance().m_WhitelistAddresses);
    Init();
    m_pServerSocket->Listen();

    AZ::Job* tickThreadJob = AZ::CreateJobFunction(&TickThread, autoDeleteJobWhenDone);
    tickThreadJob->Start();

    uint32_t JobCounter = 0;
    while (1)
    {
        CThreadData* pData = new CThreadData(JobCounter++, m_pServerSocket->Accept());
        if (pData->Socket() != nullptr)
        {
            InterlockedIncrement(&g_ConnectionCount);
            CompileJob* compileJob = new CompileJob();
            compileJob->SetThreadData(pData);
            compileJob->Start();
        }

        bool printedMessage = false;
        while (g_ConnectionCount > SEnviropment::Instance().m_MaxConnections)
        {
            if (!printedMessage)
            {
                logmessage("Waiting for currenet requests to finish before accepting another connection...\n");
                printedMessage = true;
            }

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleepTimeWhenWaiting));
        };
    }
    CrySimple_SECURE_END
}

void GetCurrentDirectoryOnPlatform(size_t bufferSize, char* buffer)
{
#if defined(AZ_PLATFORM_WINDOWS)
    GetCurrentDirectory(DWORD(bufferSize), buffer);
#else
    getcwd(buffer, bufferSize);
#endif
}

void NormalizePath(std::string& pathToNormalize)
{
    AZStd::string tempString = pathToNormalize.c_str();
    AzFramework::StringFunc::Root::Normalize(tempString);
    pathToNormalize = tempString.c_str();
}

void CCrySimpleServer::Init()
{
    //creating cache file structure
    AZ::IO::SystemFile::CreateDir("Error");
    AZ::IO::SystemFile::CreateDir("Cache");

    char CurrentDir[1024] = { 0 };
    GetCurrentDirectoryOnPlatform(sizeof(CurrentDir), CurrentDir);
    SEnviropment::Instance().m_Root         =   CurrentDir;
    SEnviropment::Instance().m_Root         +=  "\\";

    SEnviropment::Instance().m_Compiler =   SEnviropment::Instance().m_Root + "../../Compiler/";

    SEnviropment::Instance().m_Cache        =   SEnviropment::Instance().m_Root + "Cache/";
    if (SEnviropment::Instance().m_Temp.empty())
    {
        SEnviropment::Instance().m_Temp         =   SEnviropment::Instance().m_Root + "Temp/";
    }
    if (SEnviropment::Instance().m_Error.empty())
    {
        SEnviropment::Instance().m_Error            =   SEnviropment::Instance().m_Root + "Error/";
    }
    if (SEnviropment::Instance().m_Shader.empty())
    {
        SEnviropment::Instance().m_Shader = SEnviropment::Instance().m_Root + "Shaders/";
    }

    NormalizePath(SEnviropment::Instance().m_Root);
    NormalizePath(SEnviropment::Instance().m_Compiler);
    NormalizePath(SEnviropment::Instance().m_Cache);
    NormalizePath(SEnviropment::Instance().m_Error);
    NormalizePath(SEnviropment::Instance().m_Temp);
    NormalizePath(SEnviropment::Instance().m_Shader);

    AZ::IO::SystemFile::CreateDir(SEnviropment::Instance().m_Error.c_str());
    AZ::IO::SystemFile::CreateDir(SEnviropment::Instance().m_Temp.c_str());
    AZ::IO::SystemFile::CreateDir(SEnviropment::Instance().m_Cache.c_str());
    AZ::IO::SystemFile::CreateDir(SEnviropment::Instance().m_Shader.c_str());


    if (SEnviropment::Instance().m_Caching)
    {
        AZ::Job* loadCacheJob = AZ::CreateJobFunction(&LoadCache, autoDeleteJobWhenDone);
        loadCacheJob->Start();
    }
    else
    {
        printf("\nNO CACHING, disabled by config\n");
    }
}

void CCrySimpleServer::IncrementExceptionCount()
{
    InterlockedIncrement(&ms_ExceptionCount);
}
