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

// Description : Defines the entry point for the console application.


#include "stdafx.h"

// Must be included only once in DLL module.
#include "CryAssert_impl.h"

#include <platform_implRC.h>

#include <time.h>
#include <stdio.h>

#if defined(AZ_PLATFORM_WINDOWS)
#include "CpuInfo.h"
#include "MathHelpers.h"
#include <psapi.h>       // GetProcessMemoryInfo()
#endif //AZ_PLATFORM_WINDOWS

#include "ResourceCompiler.h"
#include "CmdLine.h"
#include "Config.h"
#include "CfgFile.h"
#include "FileUtil.h"
#include "IConvertor.h"
#include "CrashHandler.h"

#include "CryPath.h"
#include "Mailer.h"
#include "StringHelpers.h"
#include "ListFile.h"
#include "Util.h"
#include "ICryXML.h"
#include "IXMLSerializer.h"

#include "NameConvertor.h"
#include "CryCrc32.h"
#include "PropertyVars.h"
#include "StlUtils.h"
#include "TextFileReader.h"
#include "ZipEncryptor.h"
#include "IResourceCompilerHelper.h"
#include "SettingsManagerHelpers.h"
#include "UnitTestHelper.h"
#include "CryLibrary.h"
#include "FunctionThread.h"

#include <ParseEngineConfig.h>
#include <AzCore/Module/Environment.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/CommandLine/CommandLine.h>

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Debug/TraceMessagesDrillerBus.h>
#include <AzCore/base.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QSettings>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#if defined(AZ_PLATFORM_APPLE)
#include <mach-o/dyld.h>  // Needed for _NSGetExecutablePath
#endif

#pragma comment( lib, "Version.lib" )

const char* const ResourceCompiler::m_filenameRcExe            = "rc.exe";
const char* const ResourceCompiler::m_filenameRcIni            = "rc.ini";
const char* const ResourceCompiler::m_filenameOptions          = "rc_options.txt";
const char* const ResourceCompiler::m_filenameLog              = "rc_log.log";
const char* const ResourceCompiler::m_filenameLogWarnings      = "rc_log_warnings.log";
const char* const ResourceCompiler::m_filenameLogErrors        = "rc_log_errors.log";
const char* const ResourceCompiler::m_filenameCrashDump        = "rc_crash.dmp";
const char* const ResourceCompiler::m_filenameOutputFileList   = "rc_outputfiles.txt";
const char* const ResourceCompiler::m_filenameDeletedFileList  = "rc_deletedfiles.txt";
const char* const ResourceCompiler::m_filenameCreatedFileList  = "rc_createdfiles.txt";

const size_t ResourceCompiler::s_internalBufferSize;
const size_t ResourceCompiler::s_environmentBufferSize;

#if defined(AZ_PLATFORM_WINDOWS)
static CrashHandler s_crashHandler;
#endif

namespace
{
    volatile bool g_gotCTRLBreakSignalFromOS = false;
#if defined(AZ_PLATFORM_WINDOWS)
    BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
    {
        // we got this.
        RCLogError("CTRL-BREAK was pressed!");
        g_gotCTRLBreakSignalFromOS = true;
        return TRUE;
    }
#else
    void CtrlHandlerRoutine(int signo)
    {
        if (signo == SIGINT)
        {
            RCLogError("CTRL-BREAK was pressed!");
            g_gotCTRLBreakSignalFromOS = true;
        }
    }
#endif

}

// used to intercept asserts during shutdown.
class AssertInterceptor
    : public AZ::Debug::TraceMessageBus::Handler
{
public:
    AssertInterceptor()
    {
        BusConnect();
    }

    virtual ~AssertInterceptor()
    {
    }

    bool OnAssert(const char* message) override
    {
        RCLogWarning("An assert occurred during shutdown!  %s", message);
        return true;
    }
};

//////////////////////////////////////////////////////////////////////////
// ResourceCompiler implementation.
//////////////////////////////////////////////////////////////////////////
ResourceCompiler::ResourceCompiler()
{
    m_platformCount = 0;
    m_bQuiet = false;
    m_verbosityLevel = 0;
    m_maxThreads = 0;
    m_numWarnings = 0;
    m_numErrors = 0;
    m_memorySizePeakMb = 0;
    m_pAssetWriter = 0;
    m_pPakManager = 0;
    m_systemDll = nullptr;
    InitializeThreadIds();

    // install our ctrl handler by default, before we load system modules.  Unfortunately
    // some modules may install their own, so we will install ours again after loading perforce and crysystem.
#if defined(AZ_PLATFORM_WINDOWS)
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine, TRUE);
#elif defined(AZ_PLATFORM_APPLE) || defined(AZ_PLATFORM_LINUX)
    signal(SIGINT, CtrlHandlerRoutine);
#endif
}

ResourceCompiler::~ResourceCompiler()
{
    delete m_pPakManager;

    {
        if (m_pSystem)
        {
            AssertInterceptor interceptor;
            m_pSystem.reset();
        }


        if (m_systemDll)
        {
            typedef void*(* PtrFunc_ModuleShutdownISystem)(ISystem* pSystem);
            PtrFunc_ModuleShutdownISystem pfnModuleShutdownISystem =
                reinterpret_cast<PtrFunc_ModuleShutdownISystem>(CryGetProcAddress(m_systemDll, "ModuleShutdownISystem"));

            if (pfnModuleShutdownISystem)
            {
                AssertInterceptor interceptor;
                pfnModuleShutdownISystem(nullptr);
            }

            while (CryFreeLibrary(m_systemDll))
            {
                // keep freeing until free.
                // this is unfortunate, but CryMemoryManager currently in its internal impl grabs an extra handle to this and does not free it nor
                // does it have an interface to do so.
                // until we can rewrite the memory manager init and shutdown code, we need to do this here because we need crysystem GONE
                // before we attempt to destroy the memory managers which it uses.
                ;
            }
            m_systemDll = nullptr;
        }
    }

    TLSFREE(m_tlsIndex_pThreadData);
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::StartProgress()
{
    m_progressLastPercent = -1;
}

void ResourceCompiler::ShowProgress(const char* pMessage, size_t progressValue, size_t maxProgressValue)
{
    float percent = (progressValue * 100.0f) / maxProgressValue;
    if ((percent <= 100.0f) && (percent != m_progressLastPercent))
    {
        m_progressLastPercent = percent;
        char str[1024];
        azsnprintf(str, sizeof(str), "Progress: %d.%d%% %s", int(percent), int(percent * 10.0f) % 10, pMessage);
#if defined(AZ_PLATFORM_WINDOWS)
        SetConsoleTitle(str);
#else
        //TODO:: Needs cross platform support.
#endif
    }
}

void ResourceCompiler::FinishProgress()
{
#if defined(AZ_PLATFORM_WINDOWS)
    SetConsoleTitle("Progress: 100%");
#else
    //TODO:: Needs cross platform support.
#endif
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::RegisterConvertor(const char* name, IConvertor* conv)
{
    m_extensionManager.RegisterConvertor(name, conv, this);
}

//////////////////////////////////////////////////////////////////////////
IPakSystem* ResourceCompiler::GetPakSystem()
{
    return (m_pPakManager ? m_pPakManager->GetPakSystem() : 0);
}

//////////////////////////////////////////////////////////////////////////
const ICfgFile* ResourceCompiler::GetIniFile() const
{
    return &m_iniFile;
}

//////////////////////////////////////////////////////////////////////////
int ResourceCompiler::GetPlatformCount() const
{
    return m_platformCount;
}

//////////////////////////////////////////////////////////////////////////
const PlatformInfo* ResourceCompiler::GetPlatformInfo(int index) const
{
    if (index < 0 || index > m_platformCount)
    {
        assert(0);
        return 0;
    }
    return &m_platforms[index];
}

//////////////////////////////////////////////////////////////////////////
int ResourceCompiler::FindPlatform(const char* name) const
{
    for (int i = 0; i < m_platformCount; ++i)
    {
        if (m_platforms[i].HasName(name))
        {
            return i;
        }
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////
bool ResourceCompiler::AddPlatform(const string& names, bool bBigEndian, int pointerSize)
{
    if (m_platformCount >= kMaxPlatformCount)
    {
        return false;
    }

    if (pointerSize != 4 && pointerSize != 8)
    {
        return false;
    }

    std::vector<string> arrNames;
    StringHelpers::SplitByAnyOf(names, ",; ", false, arrNames);

    if (arrNames.empty() || arrNames.size() > PlatformInfo::kMaxPlatformNames)
    {
        return false;
    }

    PlatformInfo& p = m_platforms[m_platformCount];

    p.Clear();

    for (size_t i = 0; i < arrNames.size(); ++i)
    {
        if (!p.SetName(i, arrNames[i].c_str()))
        {
            return false;
        }
    }

    p.index = m_platformCount++;
    p.bBigEndian = bBigEndian;
    p.pointerSize = pointerSize;

    return true;
}

//////////////////////////////////////////////////////////////////////////
const char* ResourceCompiler::GetLogPrefix() const
{
    return m_logPrefix.c_str();
}

void ResourceCompiler::RemoveOutputFiles()
{
    AZ::IO::SystemFile::Delete(FormLogFileName(m_filenameDeletedFileList));
    AZ::IO::SystemFile::Delete(FormLogFileName(m_filenameCreatedFileList));
}



class FilesToConvert
{
public:
    std::vector<RcFile> m_allFiles;
    std::vector<RcFile> m_inputFiles;
    std::vector<RcFile> m_outOfMemoryFiles;
    std::vector<RcFile> m_failedFiles;
    std::vector<RcFile> m_convertedFiles;

private:
    AZStd::mutex m_lock;

public:
    void lock()
    {
        m_lock.lock();
    }

    void unlock()
    {
        m_lock.unlock();
    }
};


struct RcThreadData
{
    ResourceCompiler* rc;
    FilesToConvert* pFilesToConvert;
    unsigned long tlsIndex_pThreadData;   // copy of private rc->m_tlsIndex_pThreadData
    int threadId;
    IConvertor* convertor;
    ICompiler* compiler;

    bool bLogMemory;
    bool bWarningHeaderLine;
    bool bErrorHeaderLine;
    string logHeaderLine;
};


unsigned int WINAPI ThreadFunc(void* threadDataMemory);


static void CompileFilesMultiThreaded(
    ResourceCompiler* pRC,
    unsigned long a_tlsIndex_pThreadData,
    FilesToConvert& a_files,
    int threadCount,
    IConvertor* convertor)
{
    assert(pRC);

    if (threadCount <= 0)
    {
        return;
    }

    bool bLogMemory = false;

    while (!a_files.m_inputFiles.empty())
    {
        // Never create more threads than needed
        if (threadCount > a_files.m_inputFiles.size())
        {
            threadCount = a_files.m_inputFiles.size();
        }

        RCLog("Spawning %d thread%s", threadCount, ((threadCount > 1) ? "s" : ""));
        RCLog("");

        // Initialize the convertor
        {
            ConvertorInitContext initContext;
            initContext.config = &pRC->GetMultiplatformConfig().getConfig();
            initContext.inputFiles = a_files.m_inputFiles.empty() ? 0 : &a_files.m_inputFiles[0];
            initContext.inputFileCount = a_files.m_inputFiles.size();
            initContext.appRootPath = pRC->GetAppRoot();
            convertor->Init(initContext);
        }

        // Initialize the thread data for each thread.
        std::vector<RcThreadData> threadData(threadCount);
        for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex)
        {
            threadData[threadIndex].rc = pRC;
            threadData[threadIndex].convertor = convertor;
            threadData[threadIndex].compiler = convertor->CreateCompiler();
            threadData[threadIndex].tlsIndex_pThreadData = a_tlsIndex_pThreadData;
            threadData[threadIndex].threadId = threadIndex + 1;  // our "thread ids" are 1-based indices
            threadData[threadIndex].pFilesToConvert = &a_files;
            threadData[threadIndex].bLogMemory = bLogMemory;
            threadData[threadIndex].bWarningHeaderLine = false;
            threadData[threadIndex].bErrorHeaderLine = false;
            threadData[threadIndex].logHeaderLine = "";
        }
        AZStd::binary_semaphore waiter;
#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_APPLE)
        // Spawn the threads.
        QVector<QThread*> threads(threadCount);
        for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex)
        {
            threads[threadIndex] = FunctionThread::CreateThread(
                    0,                           //void *security,
                    0,                           //unsigned stack_size,
                    ThreadFunc,                  //unsigned ( *start_address )( void * ),
                    &threadData[threadIndex],    //void *arglist,
                    0,                           //unsigned initflag,
                    0);                          //unsigned *thrdaddr

            QObject::connect(threads[threadIndex], &QThread::finished, threads[threadIndex], [&waiter, threadIndex, &threads]()
            {
                threads.remove(threadIndex);
                if (threads.isEmpty())
                {
                    // this makes the app break out of its event loop (below) ASAP.
                    waiter.release();
                }
            });
        }

        // Wait until all the threads have exited
        while (!threads.isEmpty())
        {
            // Show progress

            // unfortunately the below call to processEvents(...),
            // even with a 'wait for more events' flag applied, causes it to still use up an entire CPU core
            // on windows platforms due to implementation details.
            // To avoid this, we will periodically pump events, but we will also wait on a semaphore to be raised to let us quit instantly
            // this allows us to spend most of our time with a sleeping main thread, but still instantly exit once the job(s) are done.
            waiter.try_acquire_for(AZStd::chrono::milliseconds(50));
            qApp->processEvents(QEventLoop::AllEvents);

            a_files.lock();

            if (!a_files.m_inputFiles.empty())
            {
                const int processedFileCount = a_files.m_outOfMemoryFiles.size() + a_files.m_failedFiles.size() + a_files.m_convertedFiles.size();

                pRC->ShowProgress(a_files.m_inputFiles.back().m_sourceInnerPathAndName.c_str(), processedFileCount, a_files.m_allFiles.size());
            }

            a_files.unlock();
        }
#elif defined(AZ_PLATFORM_LINUX)
        std::vector<AZStd::thread> threads(threadCount);
        for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex)
        {
            threads[threadIndex] = AZStd::thread(AZStd::bind(ThreadFunc, &threadData[threadIndex]));
        }

        //TODO:: Implement an atomic counter to track progress for these threads
        //and switch the windows version to use this implementation.
        for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex)
        {
            threads[threadIndex].join();
        }
#else
        #error Needs implementation!
#endif

        pRC->FinishProgress();

        assert(a_files.m_inputFiles.empty());

        // Release all the compiler objects.
        for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex)
        {
            threadData[threadIndex].compiler->Release();
        }

        // Clean up the converter.
        convertor->DeInit();

        if (!a_files.m_outOfMemoryFiles.empty())
        {
            pRC->LogMemoryUsage(false);

            if (threadCount > 1)
            {
                // If we ran out of memory while processing files, we should try
                // to process the files again in single thread (since we may
                // have run out of memory just because we had multiple threads).
                RCLogWarning("Run out of memory while processing %i file(s):", (int)a_files.m_outOfMemoryFiles.size());
                for (int i = 0; i < (int)a_files.m_outOfMemoryFiles.size(); ++i)
                {
                    const RcFile& rf = a_files.m_outOfMemoryFiles[i];
                    RCLogWarning(" \"%s\" \"%s\"", rf.m_sourceLeftPath.c_str(), rf.m_sourceInnerPathAndName.c_str());
                }
                RCLogWarning("Switching to single-thread mode and trying to convert the files again");
                a_files.m_inputFiles.insert(a_files.m_inputFiles.end(), a_files.m_outOfMemoryFiles.begin(), a_files.m_outOfMemoryFiles.end());
                threadCount = 1;
                bLogMemory = true;
            }
            else
            {
                RCLogError("Run out of memory while processing %i file(s):", (int)a_files.m_outOfMemoryFiles.size());
                for (int i = 0; i < (int)a_files.m_outOfMemoryFiles.size(); ++i)
                {
                    const RcFile& rf = a_files.m_outOfMemoryFiles[i];
                    RCLogError(" \"%s\" \"%s\"", rf.m_sourceLeftPath.c_str(), rf.m_sourceInnerPathAndName.c_str());
                }

                a_files.m_failedFiles.insert(a_files.m_failedFiles.end(), a_files.m_outOfMemoryFiles.begin(), a_files.m_outOfMemoryFiles.end());
            }

            a_files.m_outOfMemoryFiles.resize(0);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
typedef std::set<string, stl::less_stricmp<string> > TStringSet;
// Adds file into vector<RcFile> ensuring that only one sourceInnerPathAndName exists.
static void AddRcFile(
    std::vector<RcFile>& files,
    TStringSet& addedFilenames,
    const std::vector<string>& sourceRootsReversed,
    const string& sourceLeftPath,
    const string& sourceInnerPathAndName,
    const string& targetLeftPath)
{
    if (sourceRootsReversed.size() == 1)
    {
        files.push_back(RcFile(sourceLeftPath, sourceInnerPathAndName, targetLeftPath));
    }
    else
    {
        if (addedFilenames.find(sourceInnerPathAndName) == addedFilenames.end())
        {
            files.push_back(RcFile(sourceLeftPath, sourceInnerPathAndName, targetLeftPath));
            addedFilenames.insert(sourceInnerPathAndName);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::GetSourceRootsReversed(const IConfig* config, std::vector<string>& sourceRootsReversed)
{
    const int verbosityLevel = config->GetAsInt("verbose", 0, 1);
    const string sourceRootsStr = config->GetAsString("sourceroot", "", "");

    std::vector<string> sourceRoots;
    StringHelpers::Split(sourceRootsStr, ";", true, sourceRoots);

    const size_t sourceRootCount = sourceRoots.size();
    sourceRootsReversed.resize(sourceRootCount);

    if (verbosityLevel >= 2)
    {
        RCLog("Source Roots (%i):", (int)sourceRootCount);
    }

    for (size_t i = 0; i < sourceRootCount; ++i)
    {
        sourceRootsReversed[i] = PathHelpers::GetAbsoluteAsciiPath(sourceRoots[sourceRootCount - 1 - i]);
        sourceRootsReversed[i] = PathHelpers::ToPlatformPath(sourceRootsReversed[i]);

        if (verbosityLevel >= 3)
        {
            RCLog("  [%i]: '%s' (%s)", (int)i, sourceRootsReversed[i].c_str(), sourceRoots[sourceRootCount - 1 - i].c_str());
        }
        else if (verbosityLevel >= 2)
        {
            RCLog("  [%i]: '%s'", (int)i, sourceRootsReversed[i].c_str());
        }
    }

    if (sourceRootsReversed.empty())
    {
        sourceRootsReversed.push_back(string());
    }
}

//////////////////////////////////////////////////////////////////////////
bool ResourceCompiler::CollectFilesToCompile(const string& filespec, std::vector<RcFile>& files)
{
    const IConfig* const config = &m_multiConfig.getConfig();

    files.clear();

    const bool bVerbose = GetVerbosityLevel() > 0;
    const bool bRecursive = config->GetAsBool("recursive", true, true);
    const bool bSkipMissing = config->GetAsBool("skipmissing", false, true);

    // RC expects source filenames in command line in form
    // "<source left path><mask for recursion>" or "<source left path><non-masked name>".
    // After recursive subdirectory scan we will have a list with source filenames in
    // form "<source left path><source inner path><name>".
    // The target filename can be written as "<target left path><source inner path><name>".

    // Determine the target output path (may be a different directory structure).
    // If none is specified, the target path is the same as the <source left path>.
    const string targetLeftPath = PathHelpers::ToPlatformPath(PathHelpers::CanonicalizePath(config->GetAsString("targetroot", "", "")));

    std::vector<string> sourceRootsReversed;
    GetSourceRootsReversed(config, sourceRootsReversed);

    const string listFile = PathHelpers::ToPlatformPath(config->GetAsString("listfile", "", ""));

    TStringSet addedFiles;
    if (!listFile.empty())
    {
        const string listFormat = config->GetAsString("listformat", "", "");

        for (size_t i = 0; i < sourceRootsReversed.size(); ++i)
        {
            const string& sourceRoot = sourceRootsReversed[i];
            {
                std::vector< std::pair<string, string> > filenames;

                CListFile(this).Process(listFile, listFormat, filespec, sourceRoot, filenames);

                for (size_t i = 0; i < filenames.size(); ++i)
                {
                    AddRcFile(files, addedFiles, sourceRootsReversed, filenames[i].first, filenames[i].second, targetLeftPath);
                }
            }
        }

        if (files.empty())
        {
            if (!bSkipMissing)
            {
                RCLogError("No files to convert found in list file \"%s\" (filter is \"%s\")", listFile.c_str(), filespec.c_str());
            }
            return bSkipMissing;
        }

        if (bVerbose)
        {
            RCLog("Contents of the list file \"%s\" (filter is \"%s\"):", listFile.c_str(), filespec.c_str());
            for (size_t i = 0; i < files.size(); ++i)
            {
                RCLog(" %3d: \"%s\" \"%s\"", i, files[i].m_sourceLeftPath.c_str(), files[i].m_sourceInnerPathAndName.c_str());
            }
        }
    }
    else
    {
        if (filespec.find_first_of("*?") != string::npos)
        {
            // It's a mask (path\*.mask). Scan directory and accumulate matching filenames in the list.
            // Multiple masks allowed, for example path\*.xml;*.dlg;path2\*.mtl

            std::vector<string> tokens;
            StringHelpers::Split(filespec, ";", false, tokens);

            for (size_t i = 0; i < sourceRootsReversed.size(); ++i)
            {
                const string& sourceRoot = sourceRootsReversed[i];
                for (size_t t = 0; t < tokens.size(); ++t)
                {
                    // Scan directory and accumulate matching filenames in the list.
                    const string path = PathHelpers::ToPlatformPath(PathHelpers::Join(sourceRoot, PathHelpers::GetDirectory(tokens[t])));
                    const string pattern = PathHelpers::GetFilename(tokens[t]);
                    RCLog("Scanning directory '%s' for '%s'...", path.c_str(), pattern.c_str());
                    std::vector<string> filenames;
                    FileUtil::ScanDirectory(path, pattern, filenames, bRecursive, targetLeftPath);
                    for (size_t i = 0; i < filenames.size(); ++i)
                    {
                        string sourceLeftPath;
                        string sourceInnerPathAndName;
                        if (sourceRoot.empty())
                        {
                            sourceLeftPath = PathHelpers::GetDirectory(tokens[t]);
                            sourceInnerPathAndName = filenames[i];
                        }
                        else
                        {
                            sourceLeftPath = sourceRoot;
                            sourceInnerPathAndName = PathHelpers::Join(PathHelpers::GetDirectory(tokens[t]), filenames[i]);
                        }

                        const DWORD dwFileSpecAttr = GetFileAttributes(PathHelpers::Join(sourceLeftPath, sourceInnerPathAndName));
                        if (dwFileSpecAttr & FILE_ATTRIBUTE_DIRECTORY)
                        {
                            RCLog("Skipping adding file '%s' matched by wildcard '%s' because it is a directory", sourceInnerPathAndName.c_str(), filespec.c_str());
                        }
                        else
                        {
                            AddRcFile(files, addedFiles, sourceRootsReversed, sourceLeftPath, sourceInnerPathAndName, targetLeftPath);
                        }
                    }
                }
            }

            if (files.empty())
            {
                // We failed to find any file matching the mask specified by user.
                // Using mask (say, *.cgf) usually means that user doesn't know if
                // the file exists or not, so it's better to return "success" code.
                RCLog("RC can't find files matching '%s', 0 files converted", filespec.c_str());
                return true;
            }
        }
        else
        {
            for (size_t i = 0; i < sourceRootsReversed.size(); ++i)
            {
                const string& sourceRoot = sourceRootsReversed[i];
                const DWORD dwFileSpecAttr = GetFileAttributes(PathHelpers::Join(sourceRoot, filespec));

                if (dwFileSpecAttr == INVALID_FILE_ATTRIBUTES)
                {
                    // There's no such file
                    RCLog("RC can't find file '%s'. Will try to find the file in .pak files.", filespec.c_str());
                    if (sourceRoot.empty())
                    {
                        AddRcFile(files, addedFiles, sourceRootsReversed, PathHelpers::GetDirectory(filespec), PathHelpers::GetFilename(filespec), targetLeftPath);
                    }
                    else
                    {
                        AddRcFile(files, addedFiles, sourceRootsReversed, sourceRoot, filespec, targetLeftPath);
                    }
                }
                else
                {
                    // The file exists

                    if (dwFileSpecAttr & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        // We found a file, but it's a directory, not a regular file.
                        // Let's assume that the user wants to export every file in the
                        // directory (with subdirectories if bRecursive == true) or
                        // that he wants to export a file specified in /file option.
                        const string path = PathHelpers::Join(sourceRoot, filespec);
                        const string pattern = config->GetAsString("file", "*.*", "*.*");
                        RCLog("Scanning directory '%s' for '%s'...", path.c_str(), pattern.c_str());
                        std::vector<string> filenames;
                        FileUtil::ScanDirectory(path, pattern, filenames, bRecursive, targetLeftPath);
                        for (size_t i = 0; i < filenames.size(); ++i)
                        {
                            string sourceLeftPath;
                            string sourceInnerPathAndName;
                            if (sourceRoot.empty())
                            {
                                sourceLeftPath = filespec;
                                sourceInnerPathAndName = filenames[i];
                            }
                            else
                            {
                                sourceLeftPath = sourceRoot;
                                sourceInnerPathAndName = PathHelpers::Join(filespec, filenames[i]);
                            }
                            AddRcFile(files, addedFiles, sourceRootsReversed, sourceLeftPath, sourceInnerPathAndName, targetLeftPath);
                        }
                    }
                    else
                    {
                        // we found a regular file
                        if (sourceRoot.empty())
                        {
                            AddRcFile(files, addedFiles, sourceRootsReversed, PathHelpers::GetDirectory(filespec), PathHelpers::GetFilename(filespec), targetLeftPath);
                        }
                        else
                        {
                            AddRcFile(files, addedFiles, sourceRootsReversed, sourceRoot, filespec, targetLeftPath);
                        }
                    }
                }
            }
        }

        if (files.empty())
        {
            if (!bSkipMissing)
            {
                RCLogError("No files found to convert.");
            }
            return bSkipMissing;
        }
    }

    // Remove excluded files from the list of files to process.
    FilterExcludedFiles(files);

    if (files.empty())
    {
        if (!bSkipMissing)
        {
            RCLogError("No files to convert (all files were excluded by /exclude command).");
        }
        return bSkipMissing;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
// Returns true if successfully converted at least one file
bool ResourceCompiler::CompileFilesBySingleProcess(const std::vector<RcFile>& files)
{
    const IConfig* const config = &m_multiConfig.getConfig();
    const clock_t compilationStartTime = clock();

    FilesToConvert filesToConvert;
    filesToConvert.m_allFiles = files;

    if (g_gotCTRLBreakSignalFromOS)
    {
        // abort!
        return false;
    }

    if (config->GetAsBool("copyonly", false, true) || config->GetAsBool("copyonlynooverwrite", false, true))
    {
        const string targetroot = PathHelpers::ToPlatformPath(PathHelpers::CanonicalizePath(config->GetAsString("targetroot", "", "")));
        if (targetroot.empty() && config->GetAsBool("copyonly", false, true))
        {
            RCLogError("/copyonly: you must specify /targetroot.");
            return false;
        }
        else if (targetroot.empty() && config->GetAsBool("copyonlynooverwrite", false, true))
        {
            RCLogError("/copyonlynooverwrite: you must specify /targetroot.");
            return false;
        }

        CopyFiles(filesToConvert.m_allFiles, config->GetAsBool("copyonlynooverwrite", false, true));

        return true;
    }

    const PakManager::ECallResult eResult = m_pPakManager->CompileFilesIntoPaks(config, filesToConvert.m_allFiles);

    if (eResult != PakManager::eCallResult_Skipped)
    {
        return eResult == PakManager::eCallResult_Succeeded;
    }

    //////////////////////////////////////////////////////////////////////////

    // Split up the files based on the convertor they are to use.
    typedef std::map<IConvertor*, std::vector<RcFile> > FileConvertorMap;
    FileConvertorMap fileConvertorMap;
    for (size_t i = 0; i < filesToConvert.m_allFiles.size(); ++i)
    {
        if (g_gotCTRLBreakSignalFromOS)
        {
            // abort!
            return false;
        }

        string filenameForConvertorSearch;
        {
            const string sOverWriteExtension = config->GetAsString("overwriteextension", "", "");
            if (!sOverWriteExtension.empty())
            {
                filenameForConvertorSearch = string("filename.") + sOverWriteExtension;
            }
            else
            {
                filenameForConvertorSearch = filesToConvert.m_allFiles[i].m_sourceInnerPathAndName;
            }
        }

        IConvertor* const convertor = m_extensionManager.FindConvertor(filenameForConvertorSearch.c_str());

        if (!convertor)
        {
            // it is an error to fail to find a convertor.
            RCLogError("Cannot find convertor for %s", filenameForConvertorSearch.c_str());
            filesToConvert.m_failedFiles.push_back(filesToConvert.m_allFiles[i]);
            filesToConvert.m_allFiles.erase(filesToConvert.m_allFiles.begin() + i);

            --i;
            continue;
        }

        FileConvertorMap::iterator convertorIt = fileConvertorMap.find(convertor);
        if (convertorIt == fileConvertorMap.end())
        {
            convertorIt = fileConvertorMap.insert(std::make_pair(convertor, std::vector<RcFile>())).first;
        }
        (*convertorIt).second.push_back(filesToConvert.m_allFiles[i]);
    }

    if (GetVerbosityLevel() > 0)
    {
        RCLog("%d file%s to convert:", filesToConvert.m_allFiles.size(), ((filesToConvert.m_allFiles.size() != 1) ? "s" : ""));
        for (size_t i = 0; i < filesToConvert.m_allFiles.size(); ++i)
        {
            RCLog("  \"%s\"  \"%s\" -> \"%s\"",
                filesToConvert.m_allFiles[i].m_sourceLeftPath.c_str(),
                filesToConvert.m_allFiles[i].m_sourceInnerPathAndName.c_str(),
                filesToConvert.m_allFiles[i].m_targetLeftPath.c_str());
        }
        RCLog("");
    }

    // Loop through all the convertors that we need to invoke.
    for (FileConvertorMap::iterator convertorIt = fileConvertorMap.begin(); convertorIt != fileConvertorMap.end(); ++convertorIt)
    {
        assert(filesToConvert.m_inputFiles.empty());
        assert(filesToConvert.m_outOfMemoryFiles.empty());

        IConvertor* const convertor = (*convertorIt).first;
        assert(convertor);

        // Check whether this convertor is thread-safe.
        assert(m_maxThreads >= 1);
        int threadCount = m_maxThreads;
        if ((threadCount > 1) && (!convertor->SupportsMultithreading()))
        {
            RCLog("/threads specified, but convertor does not support multi-threading. Falling back to single-threading.");
            threadCount = 1;
        }

        const std::vector<RcFile>& convertorFiles = (*convertorIt).second;
        assert(convertorFiles.size() > 0);

        // implementation note: we insert filenames starting from last, because converting function will take filenames one by one from the end(!) of the array
        for (int i = convertorFiles.size() - 1; i >= 0; --i)
        {
            filesToConvert.m_inputFiles.push_back(convertorFiles[i]);
        }

        LogMemoryUsage(false);

        CompileFilesMultiThreaded(this, m_tlsIndex_pThreadData, filesToConvert, threadCount, convertor);

        assert(filesToConvert.m_inputFiles.empty());
        assert(filesToConvert.m_outOfMemoryFiles.empty());
    }

    const int numFilesConverted = filesToConvert.m_convertedFiles.size();
    const int numFilesFailed = filesToConvert.m_failedFiles.size();
    assert(numFilesConverted + numFilesFailed == filesToConvert.m_allFiles.size());

    const bool savedTimeLogging = GetTimeLogging();
    SetTimeLogging(false);

    char szTimeMsg[128];
    const float secondsElapsed = float(clock() - compilationStartTime) / CLOCKS_PER_SEC;
    azsnprintf(szTimeMsg, sizeof(szTimeMsg), " in %.1f sec", secondsElapsed);

    RCLog("");

    if (numFilesFailed <= 0)
    {
        RCLog("%d file%s processed%s.", numFilesConverted, (numFilesConverted > 1 ? "s" : ""), szTimeMsg);
        RCLog("");
    }
    else
    {
        const bool bLogSourceControlInfo = config->HasKey("p4_workspace");

        RCLog("");
        RCLog(
            "%d of %d file%s were converted%s. Couldn't convert the following file%s:",
            numFilesConverted,
            numFilesConverted + numFilesFailed,
            (numFilesConverted + numFilesFailed > 1 ? "s" : ""),
            szTimeMsg,
            (numFilesFailed > 1 ? "s" : ""));
        RCLog("");
        for (size_t i = 0; i < numFilesFailed; ++i)
        {
            const string failedFilename = PathHelpers::Join(filesToConvert.m_failedFiles[i].m_sourceLeftPath, filesToConvert.m_failedFiles[i].m_sourceInnerPathAndName);
            RCLog("  %s", failedFilename.c_str());
        }
        RCLog("");
    }

    SetTimeLogging(savedTimeLogging);

    return numFilesConverted > 0;
}

bool ResourceCompiler::CompileSingleFileBySingleProcess(const char* filename)
{
    std::vector<RcFile> list;
    list.push_back(RcFile("", filename, ""));
    return CompileFilesBySingleProcess(list);
}

void EnableFloatingPointExceptions()
{
#if defined(AZ_PLATFORM_WINDOWS)
    MathHelpers::EnableFloatingPointExceptions(~(_CW_DEFAULT));
#endif
}

unsigned int WINAPI ThreadFunc(void* threadDataMemory)
{
    EnableFloatingPointExceptions();

    RcThreadData* const data = static_cast<RcThreadData*>(threadDataMemory);

    // Initialize the thread local storage, so the log can prepend the thread id to each line.
    TLSSET(data->tlsIndex_pThreadData, static_cast<void *>(data));

    data->compiler->BeginProcessing(&data->rc->GetMultiplatformConfig().getConfig());

    for (;; )
    {
        data->pFilesToConvert->lock();

        if (g_gotCTRLBreakSignalFromOS)
        {
            RCLogError("Abort was requested during compilation.");
            data->pFilesToConvert->m_failedFiles.insert(data->pFilesToConvert->m_failedFiles.begin(), data->pFilesToConvert->m_inputFiles.begin(), data->pFilesToConvert->m_inputFiles.end());
            data->pFilesToConvert->m_inputFiles.clear();
        }


        if (data->pFilesToConvert->m_inputFiles.empty())
        {
            data->pFilesToConvert->unlock();
            break;
        }

        const RcFile fileToConvert = data->pFilesToConvert->m_inputFiles.back();
        data->pFilesToConvert->m_inputFiles.pop_back();

        data->pFilesToConvert->unlock();

        const string sourceInnerPath = PathHelpers::GetDirectory(fileToConvert.m_sourceInnerPathAndName);
        const string sourceFullFileName = PathHelpers::Join(fileToConvert.m_sourceLeftPath, fileToConvert.m_sourceInnerPathAndName);

        enum EResult
        {
            eResult_Ok,
            eResult_Error,
            eResult_OutOfMemory,
            eResult_Exception
        };
        EResult eResult;

        try
        {
            if (data->rc->CompileFile(sourceFullFileName.c_str(), fileToConvert.m_targetLeftPath.c_str(), sourceInnerPath.c_str(), data->compiler))
            {
                eResult = eResult_Ok;
            }
            else
            {
                eResult = eResult_Error;
            }
        }
        catch (std::bad_alloc&)
        {
            eResult = eResult_OutOfMemory;
        }
        // Any other exception should be uncaught and allowed to go to the unhandled exception handler,
        // which will actually record useful data
        //catch (...)
        //{
        //  eResult = eResult_Exception;
        //}

        data->pFilesToConvert->lock();
        switch (eResult)
        {
        case eResult_Ok:
            data->pFilesToConvert->m_convertedFiles.push_back(fileToConvert);
            break;
        case eResult_Error:
            data->pFilesToConvert->m_failedFiles.push_back(fileToConvert);
            break;
        case eResult_OutOfMemory:
            data->rc->LogMemoryUsage(false);
            RCLogWarning("Run out of memory: \"%s\" \"%s\"", fileToConvert.m_sourceLeftPath.c_str(), fileToConvert.m_sourceInnerPathAndName.c_str());
            data->pFilesToConvert->m_outOfMemoryFiles.push_back(fileToConvert);
            break;
        case eResult_Exception:
            data->rc->LogMemoryUsage(false);
            RCLogError("Unknown failure: \"%s\" \"%s\"", fileToConvert.m_sourceLeftPath.c_str(), fileToConvert.m_sourceInnerPathAndName.c_str());
            data->pFilesToConvert->m_failedFiles.push_back(fileToConvert);
            break;
        default:
            assert(0);
            break;
        }
        data->pFilesToConvert->unlock();
    }

    data->compiler->EndProcessing();

    return 0;
}


const char* ResourceCompiler::GetExePath() const
{
    return m_exePath.c_str();
}

const char* ResourceCompiler::GetTmpPath() const
{
    return m_tempPath.c_str();
}

const char* ResourceCompiler::GetInitialCurrentDir() const
{
    return m_initialCurrentDir.c_str();
}

const char* ResourceCompiler::GetAppRoot() const
{
    return m_appRoot.c_str();
}


//////////////////////////////////////////////////////////////////////////
bool ResourceCompiler::CompileFile(
    const char* const sourceFullFileName,
    const char* const targetLeftPath,
    const char* const sourceInnerPath,
    ICompiler* compiler)
{
    {
        RcThreadData* const pThreadData = static_cast<RcThreadData*>(TLSGET(m_tlsIndex_pThreadData));

        if (!pThreadData)
        {
            RCLogError("Unexpected threading failure");
            exit(eRcExitCode_FatalError);
        }
        const bool bMemoryReportProblemsOnly = !pThreadData->bLogMemory;
        LogMemoryUsage(bMemoryReportProblemsOnly);
    }

    MultiplatformConfig localMultiConfig = m_multiConfig;
    const IConfig* const config = &m_multiConfig.getConfig();

    const string targetPath = PathHelpers::Join(targetLeftPath, sourceInnerPath);

    if (GetVerbosityLevel() >= 2)
    {
        RCLog("CompileFile():");
        RCLog("  sourceFullFileName: '%s'", sourceFullFileName);
        RCLog("  targetLeftPath: '%s'", targetLeftPath);
        RCLog("  sourceInnerPath: '%s'", sourceInnerPath);
        RCLog("targetPath: '%s'", targetPath);
    }

    // Setup conversion context.
    IConvertContext* const pCC = compiler->GetConvertContext();

    pCC->SetMultiplatformConfig(&localMultiConfig);
    pCC->SetRC(this);
    pCC->SetThreads(m_maxThreads);

    {
        bool bRefresh = config->GetAsBool("refresh", false, true);

        // Force "refresh" to be true if user asked for dialog - it helps a lot
        // when a command line used, because users very often forget to specify /refresh
        // in such cases. Also, some tools, including CryTif exporter, call RC without
        // providing /refresh
        if (config->GetAsBool("userdialog", false, true))
        {
            bRefresh = true;
        }

        pCC->SetForceRecompiling(bRefresh);
    }

    {
        const string sourceExtension = PathHelpers::FindExtension(sourceFullFileName);
        const string convertorExtension = config->GetAsString("overwriteextension", sourceExtension, sourceExtension);
        pCC->SetConvertorExtension(convertorExtension);
    }

    pCC->SetSourceFileNameOnly(PathHelpers::GetFilename(sourceFullFileName));
    pCC->SetSourceFolder(PathHelpers::GetDirectory(PathHelpers::GetAbsoluteAsciiPath(sourceFullFileName)));

    const string outputFolder = PathHelpers::GetAbsoluteAsciiPath(targetPath);
    pCC->SetOutputFolder(outputFolder);

    if (!FileUtil::EnsureDirectoryExists(outputFolder.c_str()))
    {
        RCLogError("Creating directory failed: %s", outputFolder.c_str());
        return false;
    }

    if (GetVerbosityLevel() >= 0)
    {
        RCLog("---------------------------------");
    }

    if (GetVerbosityLevel() >= 2)
    {
        RCLog("sourceFullFileName: '%s'", sourceFullFileName);
        RCLog("outputFolder: '%s'", outputFolder);
        RCLog("Path='%s'", PathHelpers::CanonicalizePath(sourceInnerPath).c_str());
        RCLog("File='%s'", PathHelpers::GetFilename(sourceFullFileName).c_str());
    }
    else if (GetVerbosityLevel() > 0)
    {
        RCLog("Path='%s'", PathHelpers::CanonicalizePath(sourceInnerPath).c_str());
        RCLog("File='%s'", PathHelpers::GetFilename(sourceFullFileName).c_str());
    }
    else if (GetVerbosityLevel() == 0)
    {
        const string sourceInnerPathAndName = PathHelpers::AddSeparator(sourceInnerPath) + PathHelpers::GetFilename(sourceFullFileName);
        RCLog("File='%s'", sourceInnerPathAndName.c_str());
    }

    // file name changed - print new header for warnings and errors
    {
        RcThreadData* const pThreadData = static_cast<RcThreadData*>(TLSGET(m_tlsIndex_pThreadData));

        if (!pThreadData)
        {
            RCLogError("Unexpected threading failure");
            exit(eRcExitCode_FatalError);
        }
        pThreadData->bWarningHeaderLine = false;
        pThreadData->bErrorHeaderLine = false;
        pThreadData->logHeaderLine = sourceFullFileName;
    }

    bool bRet;
    bool createJobs = this->GetMultiplatformConfig().getConfig().GetAsString("createjobs", "", "").empty() == false;

    if (createJobs)
    {
        bRet = compiler->CreateJobs();

        if (!bRet)
        {
            RCLogError("Failed to create jobs for file %s", sourceFullFileName);
        }
    }
    else
    {
        // Compile file(s)
        bRet = compiler->Process();

        if (!bRet)
        {
            RCLogError("Failed to convert file %s", sourceFullFileName);
        }
    }

    return bRet;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::AddInputOutputFilePair(const char* inputFilename, const char* outputFilename)
{
    assert(outputFilename && outputFilename[0]);
    assert(inputFilename && inputFilename[0]);

    AZStd::lock_guard<AZStd::mutex> lock(m_inputOutputFilesLock);

    m_inputOutputFileList.Add(inputFilename, outputFilename);
}

void ResourceCompiler::MarkOutputFileForRemoval(const char* outputFilename)
{
    assert(outputFilename && outputFilename[0]);

    AZStd::lock_guard<AZStd::mutex> lock(m_inputOutputFilesLock);

    // using empty input file name will force CleanTargetFolder(false) to delete the output file
    m_inputOutputFileList.Add("", outputFilename);

    if (GetVerbosityLevel() > 0)
    {
        RCLog("Marking file for removal: %s", outputFilename);
    }
}

void ResourceCompiler::InitializeThreadIds()
{
    TLSALLOC(&m_tlsIndex_pThreadData);

#if defined(AZ_PLATFORM_WINDOWS)
    if (m_tlsIndex_pThreadData == TLS_OUT_OF_INDEXES)
    {
        printf("RC Initialization error");
        exit(eRcExitCode_FatalError);
    }
#endif

    TLSSET(m_tlsIndex_pThreadData, static_cast<void *>(0));
}

void ResourceCompiler::LogLine(const IRCLog::EType eType, const char* szText)
{
    AZStd::lock_guard<AZStd::mutex> lock(m_logLock);

    if (eType == IRCLog::eType_Warning)
    {
        ++m_numWarnings;
    }
    else if (eType == IRCLog::eType_Error)
    {
        ++m_numErrors;
    }

    FILE* fLog = 0;
    if (!m_mainLogFileName.empty())
    {
        azfopen(&fLog, m_mainLogFileName.c_str(), "a+t");
    }


    if (m_bQuiet)
    {
        if (fLog)
        {
            fprintf(fLog, "%s\n", szText);
            fclose(fLog);
        }
        return;
    }

    RcThreadData* const pThreadData = static_cast<RcThreadData*>(TLSGET(m_tlsIndex_pThreadData));

    // if pThreadData is 0, then probably RC is just not running threads yet

    char threadString[10];
    threadString[0] = 0;
    if (pThreadData)
    {
        sprintf_s(threadString, "%d> ", pThreadData->threadId);
    }

    char timeString[20];
    timeString[0] = 0;
    if (m_bTimeLogging)
    {
        const int seconds = int(float((clock() - m_startTime) / CLOCKS_PER_SEC) + 0.5f);
        const int minutes = seconds / 60;
        sprintf_s(timeString, "%2d:%02d ", minutes, seconds - (minutes * 60));
    }

    const char* prefix = "";
    const char* additionalLogFileName = 0;
    bool* pbAdditionalLogHeaderLine = 0;

    switch (eType)
    {
    case IRCLog::eType_Info:
        prefix = "   ";
        break;

    case IRCLog::eType_Warning:
        prefix = "W: ";
        if (!m_warningLogFileName.empty())
        {
            additionalLogFileName = m_warningLogFileName.c_str();
            if (pThreadData)
            {
                pbAdditionalLogHeaderLine = &pThreadData->bWarningHeaderLine;
            }
        }
        break;

    case IRCLog::eType_Error:
        prefix = "E: ";
        if (!m_errorLogFileName.empty())
        {
            additionalLogFileName = m_errorLogFileName.c_str();
            if (pThreadData)
            {
                pbAdditionalLogHeaderLine = &pThreadData->bErrorHeaderLine;
            }
        }
        break;

    case IRCLog::eType_Context:
        prefix = "C: ";
        break;

    default:
        assert(0);
        break;
    }

    FILE* fAdditionalLog = 0;
    if (additionalLogFileName)
    {
        azfopen(&fAdditionalLog, additionalLogFileName, "a+t");
    }

    if (fAdditionalLog)
    {
        if (pbAdditionalLogHeaderLine && (*pbAdditionalLogHeaderLine == false))
        {
            fprintf(fAdditionalLog, "------------------------------------\n");
            fprintf(fAdditionalLog, "%s%s%s%s\n", prefix, threadString, timeString, pThreadData->logHeaderLine.c_str());
            *pbAdditionalLogHeaderLine = true;
        }
    }

    const char* line = szText;
    for (;; )
    {
        size_t lineLen = 0;
        while (line[lineLen] && line[lineLen] != '\n')
        {
            ++lineLen;
        }

        if (fAdditionalLog)
        {
            fprintf(fAdditionalLog, "%s%s%s%.*s\n", prefix, threadString, timeString, int(lineLen), line);
        }

        if (fLog)
        {
            fprintf(fLog, "%s%s%s%.*s\n", prefix, threadString, timeString, int(lineLen), line);
        }

        printf("%s%s%s%.*s\n", prefix, threadString, timeString, int(lineLen), line);
        fflush(stdout);

        line += lineLen;
        if (*line == '\0')
        {
            break;
        }
        ++line;
    }

    if (fAdditionalLog)
    {
        fclose(fAdditionalLog);
        fAdditionalLog = 0;
    }

    if (fLog)
    {
        fclose(fLog);
        fLog = 0;
    }

    if (m_bWarningsAsErrors && (eType == IRCLog::eType_Warning || eType == IRCLog::eType_Error))
    {
#if defined(AZ_PLATFORM_WINDOWS)
        ::MessageBoxA(NULL, szText, "RC Compilation Error", MB_OK | MB_ICONERROR);
#else
        //TODO:Implement alert box functionality with an 'ok' button for other platforms
#endif
        this->NotifyExitObservers();
        exit(eRcExitCode_Error);
    }
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::LogMultiLine(const char* szText)
{
    const char* p = szText;
    char szLine[80], * pLine = szLine;

    for (;; )
    {
        if (*p == '\n' || *p == 0 || (pLine - szLine) >= sizeof(szLine) - (5 + 2 + 1)) // 5 spaces +2 (W: or E:) +1 to avoid nextline jump
        {
            *pLine = 0;                                                     // zero termination
            RCLog("     %s", szLine);                                      // 5 spaces
            pLine = szLine;

            if (*p == '\n')
            {
                ++p;
                continue;
            }
        }

        if (*p == 0)
        {
            return;
        }

        *pLine++ = *p++;
    }
    while (*p)
    {
        ;
    }
}

SSystemGlobalEnvironment* ResourceCompiler::GetSystemEnvironment()
{
    return gEnv;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::ShowHelp(const bool bDetailed)
{
    RCLog("");
    RCLog("Usage: RC filespec /p=<platform> [/Key1=Value1] [/Key2=Value2] etc...");

    if (bDetailed)
    {
        RCLog("");

        std::map<string, string>::const_iterator it, end = m_KeyHelp.end();

        for (it = m_KeyHelp.begin(); it != end; ++it)
        {
            const string& rKey = it->first;
            const string& rHelp = it->second;

            // hide internal keys (keys starting from '_')
            if (!rKey.empty() && rKey[0] != '_')
            {
                RCLog("/%s", rKey.c_str());
                LogMultiLine(rHelp.c_str());
                RCLog("");
            }
        }
    }
    else
    {
        RCLog("       RC /help             // will list all usable keys with description");
        RCLog("       RC /help >file.txt   // output help text to file.txt");
        RCLog("");
    }
}

void ResourceCompiler::AddPluginDLL(HMODULE pluginDLL)
{
    if (pluginDLL)
    {
        m_loadedPlugins.push_back(pluginDLL);
    }
}

void ResourceCompiler::RemovePluginDLL(HMODULE pluginDLL)
{
    if (pluginDLL)
    {
        stl::find_and_erase_if(m_loadedPlugins, [pluginDLL](const HMODULE moduleToCheck) { return moduleToCheck == pluginDLL; });
    }
}

//////////////////////////////////////////////////////////////////////////
static bool RegisterConvertors(ResourceCompiler* pRc)
{
    const string strDir = pRc->GetExePath();

    AZ::IO::LocalFileIO localFile;
    bool foundOK = localFile.FindFiles(strDir.c_str(), CryLibraryDefName("ResourceCompiler*"), [&](const char* pluginFilename) -> bool
    {
#if defined(AZ_PLATFORM_WINDOWS)
        HMODULE hPlugin = CryLoadLibrary(pluginFilename);
#elif defined(AZ_PLATFORM_APPLE) || defined(AZ_PLATFORM_LINUX)
        HMODULE hPlugin = CryLoadLibrary(pluginFilename, false, false);
#endif
        if (!hPlugin)
        {
            const DWORD errCode = GetLastError();
            char messageBuffer[1024] = { '?', 0  };
#if defined(AZ_PLATFORM_WINDOWS)
            FormatMessageA(
                           FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL,
                           errCode,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           messageBuffer,
                           sizeof(messageBuffer) - 1,
                           NULL);
#endif
            RCLogError("Couldn't load plug-in module \"%s\"", pluginFilename);
            RCLogError("Error code: 0x%x = %s", errCode, messageBuffer);
            // this return controls whether to keep going on other converters or stop the entire process here.
            // it is NOT AN ERROR if one resource compiler dll fails to load
            // it might not be the DLL that is required for this particular compile.
            return true;
        }

        FnInitializeModule fnInitializeAzEnvironment =
            hPlugin ? (FnInitializeModule)CryGetProcAddress(hPlugin, "InitializeAzEnvironment") : NULL;
        if (fnInitializeAzEnvironment)
        {
            fnInitializeAzEnvironment(AZ::Environment::GetInstance());
        }

        FnRegisterConvertors fnRegister =
            hPlugin ? (FnRegisterConvertors)CryGetProcAddress(hPlugin, "RegisterConvertors") : NULL;
        if (!fnRegister)
        {
            RCLog("Error: plug-in module \"%s\" doesn't have RegisterConvertors function", pluginFilename);
            CryFreeLibrary(hPlugin);
            // this return controls whether to keep going on other converters or stop the entire process here.
            // it is NOT AN ERROR if one resource compiler dll fails to load
            // it might not be the DLL that is required for this particular compile.
            return true;
        }

        RCLog("  Loaded \"%s\"", pluginFilename);

        pRc->AddPluginDLL(hPlugin);

        const int oldErrorCount = pRc->GetNumErrors();
        fnRegister(pRc);
        const int newErrorCount = pRc->GetNumErrors();
        if (newErrorCount > oldErrorCount)
        {
            RCLog("Error: plug-in module \"%s\" emitted errors during register", pluginFilename);
            pRc->RemovePluginDLL(hPlugin);
            FnBeforeUnloadDLL fnBeforeUnload = (FnBeforeUnloadDLL)CryGetProcAddress(hPlugin, "BeforeUnloadDLL");
            if (fnBeforeUnload)
            {
                (*fnBeforeUnload)();
            }
            CryFreeLibrary(hPlugin);
            // this return controls whether to keep going on other converters or stop the entire process here.
            return true;
        }

        // Grab unit test function if the convertor has it
        FnRunUnitTests runUnitTestsFunction = hPlugin ? reinterpret_cast<FnRunUnitTests>(CryGetProcAddress(hPlugin, "RunUnitTests")) : nullptr;
        if (runUnitTestsFunction)
        {
            pRc->AddUnitTest(runUnitTestsFunction);
        }

        return true; // continue iterating to all plugins
    });

    return true;
}

void ResourceCompiler::InitPakManager()
{
    delete m_pPakManager;

    m_pPakManager = new PakManager(this);
    m_pPakManager->RegisterKeys(this);
}

static string GetResourceCompilerGenericInfo(const ResourceCompiler& rc, const string& newline)
{
    string s;
    const SFileVersion& v = rc.GetFileVersion();

#if defined(_WIN64)
    s += "ResourceCompiler  64-bit";
#else
    s += "ResourceCompiler  32-bit";
#endif
#if defined(_DEBUG)
    s += "  DEBUG";
#endif
    s += newline;

    s += "Platform support: PC";
#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_TOOLS_RESTRICTED_PLATFORM_EXPANSION(PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
    s += ", " #PublicAuxName3;
    AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#undef AZ_TOOLS_RESTRICTED_PLATFORM_EXPANSION
#endif
#if defined(TOOLS_SUPPORT_POWERVR)
    s += ", PowerVR";
#endif
#if defined(TOOLS_SUPPORT_ETC2COMP)
    s += ", etc2Comp";
#endif
    s += newline;

    s += StringHelpers::Format("Version %d.%d.%d.%d  %s %s", v.v[3], v.v[2], v.v[1], v.v[0], __DATE__, __TIME__);
    s += newline;

    s += newline;

    s += "Portions of this file Copyright (c) Amazon.com, Inc. or its affiliates. All Rights Reserved. Original file Copyright (c) Crytek GMBH. Used under license by Amazon.com, Inc. and its affiliates.";
    s += newline;

    s += newline;

    s += "Exe directory:";
    s += newline;
    s += StringHelpers::Format("  \"%s\"", rc.GetExePath());
    s += newline;
    s += "Temp directory:";
    s += newline;
    s += StringHelpers::Format("  \"%s\"", rc.GetTmpPath());
    s += newline;
    s += "Current directory:";
    s += newline;
    s += StringHelpers::Format("  \"%s\"", rc.GetInitialCurrentDir());
    s += newline;

    return s;
}


const void ResourceCompiler::GetGenericInfo(
    char* buffer,
    size_t bufferSize,
    const char* rowSeparator) const
{
    if (bufferSize <= 0 || buffer == 0)
    {
        return;
    }
    const string s = GetResourceCompilerGenericInfo(*this, string(rowSeparator));
    cry_strcpy(buffer, bufferSize, s.c_str());
}


#if defined(AZ_PLATFORM_WINDOWS)
static void CopyStringToClipboard(const string& s)
{
    if (OpenClipboard(NULL))
    {
        const HGLOBAL hAllocResult = GlobalAlloc(GMEM_MOVEABLE, s.length() + 1);
        if (hAllocResult)
        {
            const LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hAllocResult);
            memcpy(lptstrCopy, s.c_str(), s.length() + 1);
            GlobalUnlock(hAllocResult);

            EmptyClipboard();

            if (::SetClipboardData(CF_TEXT, hAllocResult) == NULL)
            {
                GlobalFree(hAllocResult);
            }
        }
        CloseClipboard();
    }
}
#endif


static void ShowAboutDialog(const ResourceCompiler& rc)
{
    const string newline("\r\n");

    const string s = GetResourceCompilerGenericInfo(rc, newline);

    string sSuffix;
    sSuffix += newline;
    sSuffix += newline;
    sSuffix += newline;
    sSuffix += "Use \"RC /help\" to list all available command-line options.";
    sSuffix += newline;
    sSuffix += newline;
    sSuffix += "Press [OK] to copy the info above to clipboard.";
#if defined(AZ_PLATFORM_WINDOWS)
    if (::MessageBoxA(NULL, (s + sSuffix).c_str(), "About", MB_OKCANCEL | MB_APPLMODAL | MB_SETFOREGROUND) == IDOK)
    {
        CopyStringToClipboard(s.c_str());
    }
#else
    if (CryMessageBox((s + sSuffix).c_str(), "About", 0) == 1)
    {
        //TODO: CopyStringToClipboard needs cross platform support! Throwing an assert for now.
        assert(0);
    }
#endif
}


static void ShowWaitDialog(const ResourceCompiler& rc, const string& action, const std::vector<string>& args, int originalArgc)
{
    const string newline("\r\n");

    const string title = string("RC is about to ") + action;

    string sPrefix;
    sPrefix += title;
    sPrefix += " (/wait was specified).";
    sPrefix += newline;
    sPrefix += newline;
    sPrefix += newline;

    string s;
    s += GetResourceCompilerGenericInfo(rc, newline);
    s += "Command line:";
    s += newline;
    for (size_t i = 0; i < args.size(); ++i)
    {
        s += "  \"";
        s += args[i];
        s += "\"";
        if ((int)i >= originalArgc)
        {
            s += "  (from ";
            s += rc.m_filenameOptions;
            s += ")";
        }
        s += newline;
    }

    string sSuffix;
    sSuffix += newline;
    sSuffix += "Do you want to copy the info above to clipboard?";

#if defined(AZ_PLATFORM_WINDOWS)
    if (::MessageBoxA(NULL, (sPrefix + s + sSuffix).c_str(), title.c_str(), MB_YESNO | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND) == IDYES)
    {
        CopyStringToClipboard(s);
    }
#else
    if (CryMessageBox((sPrefix + s + sSuffix).c_str(), title.c_str(), 0) == 1)
    {
        //TODO: CopyStringToClipboard needs cross platform support! Assert for now.
        assert(0);
    }
#endif
}


static string GetTimeAsString(const time_t tm)
{
    char* buffer = ctime(&tm);
    string str = buffer;
    while (StringHelpers::EndsWith(str, "\n"))
    {
        str = str.substr(0, str.length() - 1);
    }

    return str;
}


static void ShowResourceCompilerVersionInfo(const ResourceCompiler& rc)
{
    const string newline("\n");
    const string info = GetResourceCompilerGenericInfo(rc, newline);
    std::vector<string> rows;
    StringHelpers::Split(info, newline, true, rows);
    for (size_t i = 0; i < rows.size(); ++i)
    {
        RCLog("%s", rows[i].c_str());
    }
}


static void ShowResourceCompilerLaunchInfo(const std::vector<string>& args, int originalArgc, const ResourceCompiler& rc)
{
    ShowResourceCompilerVersionInfo(rc);

    RCLog("");
    RCLog("Command line:");
    for (size_t i = 0; i < args.size(); ++i)
    {
        if ((int)i < originalArgc)
        {
            RCLog("  \"%s\"", args[i].c_str());
        }
        else
        {
            RCLog("  \"%s\"  (from %s)", args[i].c_str(), rc.m_filenameOptions);
        }
    }
    RCLog("");

    RCLog("Platforms specified in %s:", rc.m_filenameRcIni);
    for (int i = 0; i < rc.GetPlatformCount(); ++i)
    {
        const PlatformInfo* const p = rc.GetPlatformInfo(i);
        RCLog("  %s (%s)",
            p->GetCommaSeparatedNames().c_str(),
            (p->bBigEndian ? "big-endian" : "little-endian"));
    }
    RCLog("");

    RCLog("Started at: %s", GetTimeAsString(time(0)).c_str());
}


static bool CheckCommandLineOptions(const IConfig& config, const std::vector<string>* keysToIgnore)
{
    std::vector<string> unknownKeys;
    config.GetUnknownKeys(unknownKeys);

    if (keysToIgnore)
    {
        for (size_t i = 0; i < unknownKeys.size(); ++i)
        {
            if (std::find(keysToIgnore->begin(), keysToIgnore->end(), StringHelpers::MakeLowerCase(unknownKeys[i])) != keysToIgnore->end())
            {
                unknownKeys.erase(unknownKeys.begin() + i);
                --i;
            }
        }
    }

    if (!unknownKeys.empty())
    {
        RCLogWarning("Unknown command-line options (use \"RC /help\"):");

        for (size_t i = 0; i < unknownKeys.size(); ++i)
        {
            RCLogWarning("    /%s", unknownKeys[i].c_str());
        }

        if (config.GetAsBool("failonwarnings", false, true))
        {
            return false;
        }
    }

    return true;
}

#if defined(AZ_PLATFORM_WINDOWS)
static void EnableCrtMemoryChecks()
{
    uint32 tmp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmp &= ~_CRTDBG_CHECK_ALWAYS_DF;
    tmp |= _CRTDBG_ALLOC_MEM_DF;
    tmp |= _CRTDBG_CHECK_CRT_DF;
    tmp |= _CRTDBG_DELAY_FREE_MEM_DF;
    tmp |= _CRTDBG_LEAK_CHECK_DF;    // used on exit
    // Set desired check frequency
    const uint32 eachX = 1;
    tmp = (tmp & 0xFFFF) | (eachX << 16);
    _CrtSetDbgFlag(tmp);
    // Check heap every
    //_CrtSetBreakAlloc(2031);
}
#endif

void GetCommandLineArguments(std::vector<string>& resArgs, int argc, char** argv)
{
    resArgs.clear();
    resArgs.reserve(argc);
    for (int i = 0; i < argc; ++i)
    {
        resArgs.push_back(string(argv[i]));
    }
}


void AddCommandLineArgumentsFromFile(std::vector<string>& args, const char* const pFilename)
{
    FILE* f = nullptr; 
    azfopen(&f, pFilename, "rt");
    if (!f)
    {
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), f) != 0)
    {
        if (line[0] == 0)
        {
            continue;
        }

        string sLine = line;
        sLine.Trim();
        if (sLine.empty())
        {
            continue;
        }

        args.push_back(sLine);
    }

    fclose(f);
}


static void GetFileSizeAndCrc32(int64& size, uint32& crc32, const char* pFilename)
{
    size = FileUtil::GetFileSize(pFilename);
    if (size <= 0)
    {
        return;
    }

    FILE* f = nullptr; 
    azfopen(&f, pFilename, "rb");
    if (!f)
    {
        size = -1;
        return;
    }

    int64 sizeLeft = size;
    char buffer[1024];

    CCrc32 c;

    while (sizeLeft > 0)
    {
        const int len = int((sizeLeft <= sizeof(buffer)) ? sizeLeft : sizeof(buffer));
        sizeLeft -= len;
        if (fread(buffer, 1, len, f) != len)
        {
            fclose(f);
            size = -1;
            return;
        }
        c.Add(buffer, len);
    }

    fclose(f);

    crc32 = c.Get();
}


std::unique_ptr<QCoreApplication> CreateQApplication(int &argc, char** argv)
{
#if defined(WIN32) || defined(WIN64)
    char modulePath[_MAX_PATH] = { 0 };
    GetModuleFileName(NULL, modulePath, _MAX_PATH);
    char drive[_MAX_DRIVE] = { 0 };
    char dir[_MAX_DIR] = { 0 };
    _splitpath_s(modulePath, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
    azstrcat(dir, R"(..\qtlibs\plugins)");
    char pluginPath[_MAX_PATH] = { 0 };
    _makepath_s(pluginPath, drive, dir, NULL, NULL);
    char pluginFullPath[_MAX_PATH] = { 0 };
    _fullpath(pluginFullPath, pluginPath, _MAX_PATH);
    QCoreApplication::addLibraryPath(pluginFullPath);  // relative to the executable
#else
    QString path;
    {
        QCoreApplication app(argc, argv);
        path = QDir(app.applicationDirPath()).absoluteFilePath("../qtlibs/plugins");
    }
    qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1");
    QCoreApplication::addLibraryPath(path);
#endif // #if defined(WIN32) || defined(WIN64)

    //we are not going to start a mesg loop. so exec will not be called on the qapp

    // special circumsance  - if 'userDialog' is present on the command line, we need an interactive app:
    AzFramework::CommandLine cmdLine;
    cmdLine.Parse(argc, argv);
    bool userDialog = cmdLine.HasSwitch("userdialog") &&
        ((cmdLine.GetNumSwitchValues("userdialog") == 0) || (cmdLine.GetSwitchValue("userdialog", 0) == "1"));
    
    std::unique_ptr<QCoreApplication> qApplication = userDialog? std::make_unique<QApplication>(argc, argv) : std::make_unique<QCoreApplication>(argc, argv);

    // now that QT is initialized, we can use its path manip to set the rest up:
    QDir appPath(qApp->applicationDirPath());
    while (!appPath.isRoot())
    {
        if (QFile::exists(appPath.filePath("engineroot.txt")))
        {
            QCoreApplication::addLibraryPath(appPath.absolutePath());
            break;
        }
        if (!appPath.cdUp())
        {
            break;
        }
    }
    return qApplication;
}

#ifdef AZ_TESTS_ENABLED
int AzMainUnitTests();
#endif // AZ_TESTS_ENABLED

//////////////////////////////////////////////////////////////////////////
// this is a wrapped version of main, so that we can freely create things on the stack like unique_ptrs
// and know that the actual main() can clean up memory since no objects will be in scope upon return of this main.

int __cdecl main_impl(int argc, char** argv, char** envp)
{
    std::unique_ptr<QCoreApplication> qApplication = CreateQApplication(argc, argv);

#if 0
    EnableCrtMemoryChecks();
#endif

    EnableFloatingPointExceptions();

    ResourceCompiler rc;

    rc.QueryVersionInfo();
    rc.InitPaths();

    if (argc <= 1)
    {
        ShowAboutDialog(rc);
        return eRcExitCode_Success;
    }

    std::vector<string> args;
    {
        GetCommandLineArguments(args, argc, argv);

        const string filename = string(rc.GetExePath()) + rc.m_filenameOptions;
        AddCommandLineArgumentsFromFile(args, filename.c_str());
    }

    rc.RegisterKey("_debug", "");   // hidden key for debug-related activities. parsing is module-dependent and subject to change without prior notice.

    rc.RegisterKey("wait",
        "wait for an user action on start and/or finish of RC:\n"
        "0-don't wait (default),\n"
        "1 or empty-wait for a key pressed on finish,\n"
        "2-pop up a message box and wait for the button pressed on finish,\n"
        "3-pop up a message box and wait for the button pressed on start,\n"
        "4-pop up a message box and wait for the button pressed on start and on finish\n");
    rc.RegisterKey("wx", "pause and display message box in case of warning or error");
    rc.RegisterKey("recursive", "traverse input directory with sub folders");
    rc.RegisterKey("refresh", "force recompilation of resources with up to date timestamp");
    rc.RegisterKey("p", "to specify platform (for supported names see [_platform] sections in rc.ini)");
    rc.RegisterKey("pi", "provides the platform id from the Asset Processor");
    rc.RegisterKey("statistics", "log statistics to rc_stats_* files");
    rc.RegisterKey("dependencies",
        "Use it to specify a file with dependencies to be written.\n"
        "Each line in the file will contain an input filename\n"
        "and an output filename for every file written by RC.");
    rc.RegisterKey("clean_targetroot", "When 'targetroot' switch specified will clean up this folder after rc runs, to delete all files that were not processed");
    rc.RegisterKey("verbose", "to control amount of information in logs: 0-default, 1-detailed, 2-very detailed, etc");
    rc.RegisterKey("quiet", "to suppress all printouts");
    rc.RegisterKey("skipmissing", "do not produce warnings about missing input files");
    rc.RegisterKey("logfiles", "to suppress generating log file rc_log.log");
    rc.RegisterKey("logprefix", "prepends this prefix to every log file name used (by default the prefix is the rc.exe's folder).");
    rc.RegisterKey("logtime", "logs time passed: 0=off, 1=on (default)");
    rc.RegisterKey("gameroot", "The root of the current game project.  Used to find files related to the current game.");
    rc.RegisterKey("watchfolder", "The watched root folder that this file is located in.  Used to produce the relative asset name.");
    rc.RegisterKey("nosourcecontrol", "Boolean - if true, disables initialization of source control.  Disabling Source Control in the editor automatically disables it here, too.");
    rc.RegisterKey("sourceroot", "list of source folders separated by semicolon");
    rc.RegisterKey("targetroot", "to define the destination folder. note: this folder and its subtrees will be excluded from the source files scanning process");
    rc.RegisterKey("targetnameformat",
        "Use it to specify format of the output filenames.\n"
        "syntax is /targetnameformat=\"<pair[;pair[;pair[...]]]>\" where\n"
        "<pair> is <mask>,<resultingname>.\n"
        "<mask> is a name consisting of normal and wildcard chars.\n"
        "<resultingname> is a name consisting of normal chars and special strings:\n"
        "{0} filename of a file matching the mask,\n"
        "{1} part of the filename matching the first wildcard of the mask,\n"
        "{2} part of the filename matching the second wildcard of the mask,\n"
        "and so on.\n"
        "A filename will be processed by first pair that has matching mask.\n"
        "If no any match for a filename found, then the filename stays\n"
        "unmodified.\n"
        "Example: /targetnameformat=\"*alfa*.txt,{1}beta{2}.txt\"");
    rc.RegisterKey("filesperprocess",
        "to specify number of files converted by one process in one step\n"
        "default is 100. this option is unused if /processes is 0.");
    rc.RegisterKey("threads",
        "use multiple threads. syntax is /threads=<expression>.\n"
        "<expression> is an arithmetical expression consisting of numbers,\n"
        "'cores', 'processors', '+' and '-'. 'cores' is the number of CPU\n"
        "cores; 'processors' is the number of logical processors.\n"
        "if expression is omitted, then /threads=cores is assumed.\n"
        "example: /threads=cores+2");
    rc.RegisterKey("failonwarnings", "return error code if warnings are encountered");
    rc.RegisterKey("p4_workspace", "Perforce workspace to use. Enables output of source control information for failed files");
    rc.RegisterKey("p4_user", "Perforce username to use. Default to current system username");
    rc.RegisterKey("p4_depotFilenames", "Input filelist is given in the Perforce file format");

    rc.RegisterKey("help", "lists all usable keys of the ResourceCompiler with description");
    rc.RegisterKey("version", "shows version and exits");
    rc.RegisterKey("overwriteextension", "ignore existing file extension and use specified convertor");
    rc.RegisterKey("overwritefilename", "use the filename for output file (folder part is not affected)");

    rc.RegisterKey("listfile", "Specify List file, List file can contain file lists from zip files like: @Levels\\Test\\level.pak|resourcelist.txt");
    rc.RegisterKey("listformat",
        "Specify format of the file name read from the list file. You may use special strings:\n"
        "{0} the file name from the file list,\n"
        "{1} text matching first wildcard from the input file mask,\n"
        "{2} text matching second wildcard from the input file mask,\n"
        "and so on.\n"
        "Also, you can use multiple format strings, separated by semicolons.\n"
        "In this case multiple filenames will be generated, one for\n"
        "each format string.");
    rc.RegisterKey("copyonly", "copy source files to target root without processing");
    rc.RegisterKey("copyonlynooverwrite", "copy source files to target root without processing, will not overwrite if target file exists");
    rc.RegisterKey("name_as_crc32", "When creating Pak File outputs target filename as the CRC32 code without the extension");
    rc.RegisterKey("exclude", "List of file exclusions for the command, separated by semicolon, may contain wildcard characters");
    rc.RegisterKey("exclude_listfile", "Specify a file which contains a list of files to be excluded from command input");

    rc.RegisterKey("validate", "When specified RC is running in a resource validation mode");
    rc.RegisterKey("MailServer", "SMTP Mail server used when RC needs to send an e-mail");
    rc.RegisterKey("MailErrors", "0=off 1=on When enabled sends an email to the user who checked in asset that failed validation");
    rc.RegisterKey("cc_email", "When sending mail this address will be added to CC, semicolon separates multiple addresses");
    rc.RegisterKey("job", "Process a job xml file");
    rc.RegisterKey("jobtarget", "Run only a job with specific name instead of whole job-file. Used only with /job option");
    rc.RegisterKey("unittest", "Run the unit tests for resource compiler and nothing else");
    rc.RegisterKey("gamesubdirectory", "The relative path to game folder from root from @devroot@.  Defines @devassets@ when concatenated with @devroot@.  Used to find files related to the this game.");
    rc.RegisterKey("unattended", "Prevents RC from opening any dialogs or message boxes");
    rc.RegisterKey("createjobs", "Instructs RC to read the specified input file (a CreateJobsRequest) and output a CreateJobsResponse");
    rc.RegisterKey("port", "Specifies the port that should be used to connect to the asset processor.  If not set, the default from the bootstrap cfg will be used instead");
    rc.RegisterKey("approot", "Specifies a custom directory for the engine root path. This path should contain bootstrap.cfg.");
    rc.RegisterKey("branchtoken", "Specifies a branchtoken that should be used by the RC to negotiate with the asset processor. if not set it will be set from the bootstrap file.");

    string fileSpec;
    bool bUnitTestMode = false;

    // Initialization, showing startup info, loading configs
    {
        Config mainConfig;
        mainConfig.SetConfigKeyRegistry(&rc);

        QSettings settings("HKEY_CURRENT_USER\\Software\\Amazon\\Lumberyard\\Settings", QSettings::NativeFormat);
        bool enableSourceControl = settings.value("RC_EnableSourceControl", true).toBool();
        mainConfig.SetKeyValue(eCP_PriorityCmdline, "nosourcecontrol", enableSourceControl ? "0" : "1");

        CmdLine::Parse(args, &mainConfig, fileSpec);

        // initialize rc (also initializes logs)
        rc.Init(mainConfig);

        AZ::Debug::Trace::HandleExceptions(true);
#if defined(AZ_PLATFORM_WINDOWS)
        s_crashHandler.SetDumpFile(rc.FormLogFileName(ResourceCompiler::m_filenameCrashDump));
#endif

        if (mainConfig.GetAsBool("version", false, true))
        {
            ShowResourceCompilerVersionInfo(rc);
            return eRcExitCode_Success;
        }

        bUnitTestMode = mainConfig.GetAsBool("unittest", false, true);

        switch (mainConfig.GetAsInt("wait", 0, 1))
        {
        case 3:
        case 4:
            ShowWaitDialog(rc, "start", args, argc);
            break;
        default:
            break;
        }

        ShowResourceCompilerLaunchInfo(args, argc, rc);

        rc.SetTimeLogging(mainConfig.GetAsBool("logtime", true, true));

        rc.LogMemoryUsage(false);

        RCLog("");

        if (!rc.LoadIniFile())
        {
            return eRcExitCode_FatalError;
        }

        // Make sure that rc.ini doesn't have obsolete settings
        for (int i = 0;; ++i)
        {
            const char* const pName = rc.GetIniFile()->GetSectionName(i);
            if (!pName)
            {
                break;
            }

            Config cfg;
            rc.GetIniFile()->CopySectionKeysToConfig(eCP_PriorityRcIni, i, 0, &cfg);

            if (cfg.HasKeyMatchingWildcards("srgb") || cfg.HasKeyMatchingWildcards("srgb:*"))
            {
                RCLogError("Obsolete setting 'srgb' found in %s", rc.m_filenameRcIni);
                RCLog(
                    "\n"
                    "Please replace all occurences of 'srgb' by corresponding\n"
                    "'colorspace' settings. Use the following table as the reference:\n"
                    "  srgb=0 -> colorspace=linear,linear\n"
                    "  srgb=1 -> colorspace=sRGB,auto\n"
                    "  srgb=2 -> colorspace=sRGB,sRGB\n"
                    "  srgb=3 -> colorspace=linear,sRGB\n"
                    "  srgb=4 -> colorspace=sRGB,linear");
                return eRcExitCode_FatalError;
            }
        }

        // Load list of platforms
        {
            for (int i = 0;; ++i)
            {
                const char* const pName = rc.GetIniFile()->GetSectionName(i);
                if (!pName)
                {
                    break;
                }
                if (!StringHelpers::Equals(pName, "_platform"))
                {
                    continue;
                }

                Config cfg;
                rc.GetIniFile()->CopySectionKeysToConfig(eCP_PriorityRcIni, i, "", &cfg);

                const string names = StringHelpers::MakeLowerCase(cfg.GetAsString("name", "", ""));
                const bool bBigEndian = cfg.GetAsBool("bigendian", false, true);
                const int pointerSize = cfg.GetAsInt("pointersize", 4, 0);

                if (!rc.AddPlatform(names, bBigEndian, pointerSize))
                {
                    RCLogError("Bad platform data in %s", rc.m_filenameRcIni);
                    return eRcExitCode_FatalError;
                }
            }

            if (rc.GetPlatformCount() <= 0)
            {
                RCLogError("Missing [_platform] in %s", rc.m_filenameRcIni);
                return eRcExitCode_FatalError;
            }
        }

        // Obtain target platform
        int platform;
        {
            string platformStr = mainConfig.GetAsString("p", "", "");
            if (platformStr.empty())
            {
                if (!mainConfig.GetAsBool("version", false, true))
                {
                    RCLog("Platform (/p) not specified, defaulting to 'pc'.");
                    RCLog("");
                }
                platformStr = "pc";
                mainConfig.SetKeyValue(eCP_PriorityCmdline, "p", platformStr.c_str());
            }

            platform = rc.FindPlatform(platformStr.c_str());
            if (platform < 0)
            {
                RCLogError("Unknown platform specified: '%s'", platformStr.c_str());
                return eRcExitCode_FatalError;
            }
        }

        // Load configs for every platform
        rc.GetMultiplatformConfig().init(rc.GetPlatformCount(), platform, &rc);
        for (int i = 0; i < rc.GetPlatformCount(); ++i)
        {
            IConfig& cfg = rc.GetMultiplatformConfig().getConfig(i);
            rc.GetIniFile()->CopySectionKeysToConfig(eCP_PriorityRcIni, 0, rc.GetPlatformInfo(i)->GetCommaSeparatedNames().c_str(), &cfg);
            cfg.AddConfig(&mainConfig);
        }
    }

    const IConfig& config = rc.GetMultiplatformConfig().getConfig();

    {
        RCLog("Initializing pak management");
        rc.InitPakManager();
        RCLog("");

        RCLog("Initializing System");

        string logDir = config.GetAsString("logprefix", "", "");

        if (logDir.empty())
        {
            logDir = "@log@/";
        }
        string logFile = logDir + "Log.txt";
        SSystemInitParams systemInitParams;

        const char* configSearchPaths[] =
        {
            ".",
            rc.GetExePath()
        };
        // If RC is running inside a .app (like it does on macOS inside the AssetProcessor.app bundle)
        // we need to look a few levels higher than the default of 3
        const int maxLevelsToSearchUp = 6;
        CEngineConfig cfg(configSearchPaths, sizeof(configSearchPaths) / sizeof(configSearchPaths[0]), maxLevelsToSearchUp);


        cfg.CopyToStartupParams(systemInitParams);

        systemInitParams.bDedicatedServer = true;
        systemInitParams.bEditor = false;
        systemInitParams.bExecuteCommandLine = false;
        systemInitParams.bMinimal = true;
        systemInitParams.bSkipPhysics = true;
        systemInitParams.waitForConnection = false;
        systemInitParams.bNoRandom = false;
        systemInitParams.bPreview = true;
        systemInitParams.bShaderCacheGen = false;
        systemInitParams.bSkipConsole = false;
        systemInitParams.bSkipFont = true;
        systemInitParams.bSkipNetwork = true;
        systemInitParams.bSkipRenderer = true;
        systemInitParams.bTesting = false;
        systemInitParams.bTestMode = false;
        systemInitParams.bSkipMovie = true;
        systemInitParams.bSkipAnimation = true;
        systemInitParams.bSkipScriptSystem = true;
        systemInitParams.bToolMode = true;
        systemInitParams.sLogFileName = logFile.c_str();
        systemInitParams.autoBackupLogs = false;
        systemInitParams.pSharedEnvironment = AZ::Environment::GetInstance();
        systemInitParams.bUnattendedMode = config.GetAsBool("unattended", false, false);

        //allow override from commandline
        string gameName = config.GetAsString("gamesubdirectory", "", "");
        if (!gameName.empty())
        {
            cfg.m_gameFolder = gameName;
        }

        rc.InitSystem(systemInitParams);

        // only after installing and setting those up, do we install our handler becuase perforce does this too...
#if defined(AZ_PLATFORM_WINDOWS)
        SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine, TRUE);
#elif defined(AZ_PLATFORM_APPLE) || defined(AZ_PLATFORM_LINUX)
        signal(SIGINT, CtrlHandlerRoutine);
#endif
        // and because we're a tool, add the tool folders:
        if ((gEnv) && (gEnv->pFileIO))
        {
            const char* engineRoot = nullptr;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(engineRoot, &AzToolsFramework::ToolsApplicationRequests::GetEngineRootPath);
            if (engineRoot != nullptr)
            {
                gEnv->pFileIO->SetAlias("@engroot@", engineRoot);
            }
            else
            {
                gEnv->pFileIO->SetAlias("@engroot@", cfg.m_rootFolder.c_str());
            }
            gEnv->pFileIO->SetAlias("@devroot@", cfg.m_rootFolder.c_str());

            string gamePath = config.GetAsString("gameroot", "", "");
            string devAssets = (!gamePath.empty()) ? gamePath : string(cfg.m_rootFolder + "/" + cfg.m_gameFolder);
            gEnv->pFileIO->SetAlias("@devassets@", devAssets.c_str());

            string appRootInput = config.GetAsString("approot", "", "");
            string appRoot = (!appRootInput.empty()) ? appRootInput : ResourceCompiler::GetAppRootPathFromGameRoot(devAssets);
            rc.SetAppRootPath(appRoot);
        }
        RCLog("");

        RCLog("Loading compiler plug-ins (ResourceCompiler*.dll)");

        // Force the current working directory to be the same as the executable so
        // that we can load any shared libraries that don't have run-time paths in
        // them (I'm looking at you AWS SDK libraries!).
        QString currentDir =  QDir::currentPath();
        QDir::setCurrent(QCoreApplication::applicationDirPath());

        if (!RegisterConvertors(&rc))
        {
            RCLogError("A fatal error occurred when loading plug-ins (see error message(s) above). RC cannot continue.");
            rc.UnregisterConvertors();
            return eRcExitCode_FatalError;
        }
        // Restore currentDir so that file paths will work that are relative to
        // where the user executed RC
        QDir::setCurrent(currentDir);
        RCLog("");

        RCLog("Loading zip & pak compiler module");
        rc.RegisterConvertor("zip & pak compiler", new ZipEncryptor(&rc));
        RCLog("");

        rc.LogMemoryUsage(false);
    }

    const bool bJobMode = config.HasKey("job");
    // Don't even bother setting up if we aren't going to do anything
    if (!bUnitTestMode && !bJobMode && !CheckCommandLineOptions(config, 0))
    {
        return eRcExitCode_Error;
    }

    bool bExitCodeIsReady = false;
    int exitCode = eRcExitCode_Success;

    bool bShowUsage = false;
    if (bUnitTestMode)
    {
        // Run unit tests
        exitCode = rc.RunUnitTests();

        // Set exit code
        bExitCodeIsReady = true;
        rc.PostBuild();      // e.g. writing statistics files
    }
    else if (bJobMode)
    {
        const int tmpResult = rc.ProcessJobFile();
        if (tmpResult)
        {
            exitCode = tmpResult;
            bExitCodeIsReady = true;
        }
        rc.PostBuild();      // e.g. writing statistics files
    }
    else if (!fileSpec.empty())
    {
        rc.RemoveOutputFiles();
        std::vector<RcFile> files;
        if (rc.CollectFilesToCompile(fileSpec, files) && !files.empty())
        {
            rc.SetupMaxThreads();
            rc.CompileFilesBySingleProcess(files);
        }
        rc.PostBuild();      // e.g. writing statistics files
    }
    else
    {
        bShowUsage = true;
    }

    rc.UnregisterConvertors();

    rc.SetTimeLogging(false);

    if (bShowUsage && !rc.m_bQuiet)
    {
        rc.ShowHelp(false);
    }

    if (config.GetAsBool("help", false, true))
    {
        rc.ShowHelp(true);
    }

    rc.LogMemoryUsage(false);

    RCLog("");
    RCLog("Finished at: %s", GetTimeAsString(time(0)).c_str());

    if (rc.GetNumErrors() || rc.GetNumWarnings())
    {
        RCLog("");
        RCLog("%d errors, %d warnings.", rc.GetNumErrors(), rc.GetNumWarnings());
    }

    if (!bExitCodeIsReady)
    {
        const bool bFail = rc.GetNumErrors() || (rc.GetNumWarnings() && config.GetAsBool("failonwarnings", false, true));
        exitCode = (bFail ? eRcExitCode_Error : eRcExitCode_Success);
        bExitCodeIsReady = true;
    }

    switch (config.GetAsInt("wait", 0, 1))
    {
    case 1:
        RCLog("");
        RCLog("    Press <RETURN> (/wait was specified)");
        getchar();
        break;
    case 2:
    case 4:
        ShowWaitDialog(rc, "finish", args, argc);
        break;
    default:
        break;
    }

    return exitCode;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::PostBuild()
{
    const IConfig* const config = &m_multiConfig.getConfig();

    // Save list of created files.
    {
        m_inputOutputFileList.RemoveDuplicates();
        m_inputOutputFileList.SaveOutputOnly(FormLogFileName(m_filenameCreatedFileList));
    }

    switch (config->GetAsInt("clean_targetroot", 0, 1))
    {
    case 1:
        CleanTargetFolder(false);
        break;
    case 2:
        CleanTargetFolder(true);
        break;
    default:
        break;
    }

    const string dependenciesFilename = m_multiConfig.getConfig().GetAsString("dependencies", "", "");
    if (!dependenciesFilename.empty())
    {
        m_inputOutputFileList.RemoveDuplicates();
        m_inputOutputFileList.Save(dependenciesFilename.c_str());
    }
}
void ResourceCompiler::SetAppRootPath(const string& appRootPath)
{
    m_appRoot = appRootPath;
}

string ResourceCompiler::GetAppRootPathFromGameRoot(const string& gameRootPath)
{
    // Since we cannot rely on the current exe path or engine path to determine the app root path since
    // the RC request could be coming in from an external game, we need to calculate it from the game root path.
    // Typically, the game root path is one level above the app root (the app root and the engine path is the same
    // if the game project resides in the engine folder).  If its external, we need the app root for that external
    // project in order to negotiate communications back to the AP with the right branch token
    //
    // Example:
    // Game root path        :   /LyEngine/MyGame/
    // Derived app root path :   /LyEngine

    size_t index = gameRootPath.length();
    if (index == 0)
    {
        // Empty string, skip
        return "";
    }

    char lastChar = gameRootPath.at(--index);
    // Skip over any initial path delimiters or spaces (Reduce all trailing '/' for example:  c:\foo\bar\\\ -> c:\foo\bar)
    while (((lastChar == AZ_CORRECT_FILESYSTEM_SEPARATOR) || (lastChar == AZ_WRONG_FILESYSTEM_SEPARATOR) || (lastChar == ' ')) && index>0)
    {
        lastChar = gameRootPath.at(--index);
    }
    // Invalid path (may have been '\\\\\\\\')
    if (index == 0)
    {
        return "";
    }

    // Go to the next path delimeter
    while (((lastChar != AZ_CORRECT_FILESYSTEM_SEPARATOR) && (lastChar != AZ_WRONG_FILESYSTEM_SEPARATOR)) && index>0)
    {
        lastChar = gameRootPath.at(--index);
    }

    // Invalid path (no path delimiter found after the first group, ie:  c:root\ would not resolve a parent )
    if (index == 0)
    {
        return "";
    }
    return gameRootPath.substr(0, index);
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::QueryVersionInfo()
{
#if defined(AZ_PLATFORM_WINDOWS)
    wchar_t moduleNameW[MAX_PATH];
    {
        const int bufferCharCount = sizeof(moduleNameW) / sizeof(moduleNameW[0]);
        const int charCount = GetModuleFileNameW(NULL, moduleNameW, bufferCharCount);
        if (charCount <= 0 || charCount >= bufferCharCount)
        {
            printf("RC QueryVersionInfo(): fatal error");
            exit(eRcExitCode_FatalError);
        }
        moduleNameW[charCount] = 0;
    }
    m_exePath = PathHelpers::GetAbsoluteAsciiPath(moduleNameW);
#elif defined(AZ_PLATFORM_APPLE)
    char moduleName[MAX_PATH];
    uint32_t bufferCharCount = sizeof(moduleName) / sizeof(moduleName[0]);
    if (_NSGetExecutablePath(moduleName, &bufferCharCount))
    {
        printf("RC QueryVersionInfo(): fatal error");
        exit(eRcExitCode_FatalError);
    }
    m_exePath = moduleName;
#endif


    if (m_exePath.empty())
    {
        printf("RC module name: fatal error");
        exit(eRcExitCode_FatalError);
    }
    m_exePath = PathHelpers::AddSeparator(PathHelpers::GetDirectory(m_exePath));

#if defined(AZ_PLATFORM_WINDOWS)
    DWORD handle;
    char ver[1024 * 8];
    const int verSize = GetFileVersionInfoSizeW(moduleNameW, &handle);
    if (verSize > 0 && verSize <= sizeof(ver))
    {
        GetFileVersionInfoW(moduleNameW, 0, sizeof(ver), ver);
        VS_FIXEDFILEINFO* vinfo;
        UINT len;
        VerQueryValue(ver, "\\", (void**)&vinfo, &len);

        m_fileVersion.v[0] = vinfo->dwFileVersionLS & 0xFFFF;
        m_fileVersion.v[1] = vinfo->dwFileVersionLS >> 16;
        m_fileVersion.v[2] = vinfo->dwFileVersionMS & 0xFFFF;
        m_fileVersion.v[3] = vinfo->dwFileVersionMS >> 16;
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::InitPaths()
{
    if (m_exePath.empty())
    {
        printf("RC InitPaths(): internal error");
        exit(eRcExitCode_FatalError);
    }
#if defined(AZ_PLATFORM_WINDOWS)
    // Get wide-character temp path from OS
    {
        static const DWORD bufferSize = AZ_MAX_PATH_LEN;
        wchar_t bufferWchars[bufferSize];

        DWORD resultLength = GetTempPathW(bufferSize, bufferWchars);
        if ((resultLength >= bufferSize) || (resultLength <= 0))
        {
            resultLength = 0;
        }
        bufferWchars[resultLength] = 0;

        m_tempPath = PathHelpers::GetAbsoluteAsciiPath(bufferWchars);
        if (m_tempPath.empty())
        {
            m_tempPath = m_exePath;
        }
        m_tempPath = PathHelpers::AddSeparator(m_tempPath);
    }
#elif defined(AZ_PLATFORM_APPLE) || defined(AZ_PLATFORM_LINUX)
    //The path supplied by the first environment variable found in the
    //list TMPDIR, TMP, TEMP, TEMPDIR. If none of these are found, "/tmp".
    //Another possibility is using QTemporaryDir for this work.
    char const *tmpDir = getenv("TMPDIR");
    if (tmpDir == 0)
    {
        tmpDir = getenv("TMP");
        if (tmpDir == 0)
        {
            tmpDir = getenv("TEMP");
            if (tmpDir == 0)
            {
                tmpDir = getenv("TEMPDIR");
                if (tmpDir == 0)
                {
                    tmpDir = "/tmp";
                }
            }
        }
    }
    m_tempPath = tmpDir;
#else
    #error Needs implementation!
#endif
    m_initialCurrentDir = PathHelpers::GetAbsoluteAsciiPath(".");
    if (m_initialCurrentDir.empty())
    {
        printf("RC InitPaths(): internal error");
        exit(eRcExitCode_FatalError);
    }
    m_initialCurrentDir = PathHelpers::AddSeparator(m_initialCurrentDir);

    // Prepend Bin64 (one level up from rc.exe) to the path, so child dlls
    // can find engine or dependency dlls. -> Prepend so that our directory gets searched first!
#if defined(AZ_PLATFORM_WINDOWS)
    char pathEnv[s_environmentBufferSize] = {'\0'};
    GetEnvironmentVariable("PATH", pathEnv, s_environmentBufferSize);

    string pathEnvNew = m_exePath;
    pathEnvNew.append("..\\;");
    pathEnvNew.append(pathEnv);

    SetEnvironmentVariable("PATH", pathEnvNew.c_str());

    // On Windows, force it to load dlls from the directory that the executable is in
    // and from the Bin64* directory one up, if it's there.
    // That way all of the dlls that RC uses don't have to be copied into the RC
    // sub-directory to work
    QDir tempDir(m_exePath.c_str());
    tempDir.cdUp();
    if (tempDir.dirName().contains("Bin64"))
    {
        // Calling this forces default dll loading to only occur from and in the following order:
        // 1) the directory containing the currently loading dll (in this case, dll dependent)
        // 2) the host exe's directory (Bin64./rc
        // 3) Directories registered with AddDllDirectory (Bin64*, added below)
        // 4) System32 (usually C:\Windows\System32)
        //
        // This should make it so that the current working directory no longer matters, and things
        // referenced on the environment PATH variable won't accidentally get loaded instead of
        // from our directories.
        // https://msdn.microsoft.com/en-us/library/windows/desktop/hh310515(v=vs.85).aspx
        SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        AddDllDirectory(tempDir.absolutePath().toStdWString().c_str());
    }
#elif defined(AZ_PLATFORM_APPLE) || defined(AZ_PLATFORM_LINUX)
    const char * pathEnv = getenv("PATH");
    string pathEnvNew = pathEnv;
    pathEnvNew.append(":");
    pathEnvNew.append(m_exePath);
    pathEnvNew.append("../");
    setenv("PATH", pathEnvNew.c_str(), 1);
#endif
}

//////////////////////////////////////////////////////////////////////////
bool ResourceCompiler::LoadIniFile()
{
#if defined(AZ_PLATFORM_WINDOWS)
    const string filename = PathHelpers::ToDosPath(m_exePath) + m_filenameRcIni;
#elif defined(AZ_PLATFORM_APPLE_OSX)
    // Handle the case where RC is inside an App Bundle (for the Editor or AssetProcessor)
    string filename = PathHelpers::ToUnixPath(m_exePath);
    string::size_type appBundleDirPosition = filename.rfind(".app");
    if (appBundleDirPosition != string::npos)
    {
        // ini file should be in the Resources directory. Doing the substr cuts
        // off the .app part so add it back in as well.
        filename = filename.substr(0, appBundleDirPosition);
        filename += ".app/Contents/Resources/";
    }
    filename = filename + m_filenameRcIni;
#else
    const string filename = PathHelpers::ToUnixPath(m_exePath) + m_filenameRcIni;
#endif

    RCLog("Loading \"%s\"", filename.c_str());

    if (!FileUtil::FileExists(filename.c_str()))
    {
        RCLogError("Resource compiler .ini file (%s) is missing.", filename.c_str());
        return false;
    }

    if (!m_iniFile.Load(filename))
    {
        RCLogError("Failed to load resource compiler .ini file (%s).", filename.c_str());
        return false;
    }

    RCLog("  Loaded \"%s\"", filename.c_str());
    RCLog("");

    return true;
}


void ResourceCompiler::InitSystem(SSystemInitParams& startupParams)
{
    m_pSystem = std::unique_ptr<ISystem>(startupParams.pSystem);


    if (!startupParams.pSystem)
    {
#if !defined(AZ_MONOLITHIC_BUILD)
#if defined(AZ_PLATFORM_APPLE_OSX)
        m_systemDll = CryLoadLibrary("../libCrySystem.dylib");
#elif defined(AZ_PLATFORM_LINUX)
        m_systemDll = CryLoadLibrary("../libCrySystem.so");
#else
        m_systemDll = CryLoadLibrary("../crysystem.dll");
#endif
        if (!m_systemDll)
        {
            return;
        }
        PFNCREATESYSTEMINTERFACE CreateSystemInterface =
            (PFNCREATESYSTEMINTERFACE)CryGetProcAddress(m_systemDll, "CreateSystemInterface");
#endif // !AZ_MONOLITHIC_BUILD

        // initialize the system
        m_pSystem = std::unique_ptr<ISystem>(CreateSystemInterface(startupParams));

        if (!m_pSystem)
        {
            return;
        }
    }

    ModuleInitISystem(m_pSystem.get(), "ResourceCompiler"); // Needed by GetISystem();
}


//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::Init(Config& config)
{
    m_bQuiet = config.GetAsBool("quiet", false, true);
    m_verbosityLevel = config.GetAsInt("verbose", 0, 1);

    m_bWarningsAsErrors = config.GetAsBool("wx", false, true);

    m_startTime = clock();
    m_bTimeLogging = false;

    InitLogs(config);
    SetRCLog(this);
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::UnregisterConvertors()
{
    m_extensionManager.UnregisterAll();

    for (HMODULE pluginDll : m_loadedPlugins)
    {
        FnBeforeUnloadDLL fnBeforeUnload = (FnBeforeUnloadDLL)CryGetProcAddress(pluginDll, "BeforeUnloadDLL");
        if (fnBeforeUnload)
        {
            (*fnBeforeUnload)();
        }
    }

    // let the before unload functions for all DLLs complete before you unload any of them
    for (HMODULE pluginDll : m_loadedPlugins)
    {
        CryFreeLibrary(pluginDll);
    }

    m_loadedPlugins.resize(0);
}

//////////////////////////////////////////////////////////////////////////
MultiplatformConfig& ResourceCompiler::GetMultiplatformConfig()
{
    return m_multiConfig;
}

//////////////////////////////////////////////////////////////////////////
static int ComputeExpression(
    const char* a_optionName,
    int a_processCpuCoreCount,
    int a_processLogicalProcessorCount,
    const string& a_expression,
    string& a_message)
{
    string expression = a_expression;
    a_message.clear();
    int expressionResult = 0;
    char op = 0;
    for (;; )
    {
        expression = StringHelpers::TrimLeft(expression);
        if (expression.empty())
        {
            break;
        }

        if ((expression[0] == '-') || (expression[0] == '+'))
        {
            if ((op == '+') || (op == '-'))
            {
                a_message.Format("/%s syntax error: '%s'.", a_optionName, a_expression.c_str());
                break;
            }

            op = expression[0];
            expression = expression.substr(1, expression.npos);
        }
        else
        {
            if (op == '?')
            {
                a_message.Format("/%s syntax error: '%s'.", a_optionName, a_expression.c_str());
                break;
            }

            const size_t pos = expression.find_first_of(" +-");
            const string token = expression.substr(0, pos);
            expression = (pos == expression.npos) ? "" : expression.substr(pos, expression.npos);

            int tokenInt = 0;
            if (StringHelpers::Equals(token, "cores"))
            {
                tokenInt = a_processCpuCoreCount;
            }
            else if (StringHelpers::Equals(token, "processors"))
            {
                tokenInt = a_processLogicalProcessorCount;
            }
            else if (token.find_first_not_of("0123456789") == token.npos)
            {
                expressionResult  += ((op == '-') ? -1 : 1) * atol(token.c_str());
            }
            else
            {
                a_message.Format("/%s contains unknown parameter '%s': '%s'.", a_optionName, token.c_str(), a_expression.c_str());
                break;
            }
            expressionResult += ((op == '-') ? -1 : 1) * tokenInt;
            op = '?';
        }
    }

    return expressionResult;
}

//////////////////////////////////////////////////////////////////////////
static int GetAParallelOption(
    const IConfig* a_config,
    const char* a_optionName,
    int a_processCpuCoreCount,
    int a_processLogicalProcessorCount,
    int a_valueIfNotSpecified,
    int a_valueIfEmpty,
    int a_minValue,
    int a_maxValue,
    string& message)
{
    int result = -1;

    const string optionValue = a_config->GetAsString(a_optionName, "?", "");

    if (optionValue.empty())
    {
        message.Format("/%s specified without value.", a_optionName);
        result = a_valueIfEmpty;
    }
    else if (optionValue[0] == '?')
    {
        message.Format("/%s was not specified.", a_optionName);
        result = a_valueIfNotSpecified;
    }
    else
    {
        const int expressionResult = ComputeExpression(a_optionName, a_processCpuCoreCount, a_processLogicalProcessorCount, optionValue, message);
        if (!message.empty())
        {
            result = a_valueIfNotSpecified;
        }
        else if (expressionResult < a_minValue)
        {
            message.Format("/%s specified with too small value %d: '%s'.", a_optionName, expressionResult, optionValue.c_str());
            result = a_minValue;
        }
        else if (expressionResult > a_maxValue)
        {
            message.Format("/%s specified with too big value %d: '%s'.", a_optionName, expressionResult, optionValue.c_str());
            result = a_maxValue;
        }
        else
        {
            message.Format("/%s specified with value %d: '%s'.", a_optionName, expressionResult, optionValue.c_str());
            result = expressionResult;
        }
    }

    if ((result != 1) && a_config->GetAsBool("validate", false, true))
    {
        message.Format("/%s forced to 1 because RC is in resource validation mode (see /validate).", a_optionName);
        result = 1;
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////
static unsigned int CountBitsSetTo1(const DWORD_PTR val)
{
    unsigned int result = 0;
    const size_t bitCount = sizeof(val) * 8;
    for (size_t i = 0; i < bitCount; ++i)
    {
        result += (val & (((DWORD_PTR)1) << i)) ? 1 : 0;
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::SetupMaxThreads()
{
    const IConfig* const config = &m_multiConfig.getConfig();

    {
        unsigned int numCoresInSystem = 0;
        unsigned int numCoresAvailableToProcess = 0;
#if defined(AZ_PLATFORM_WINDOWS)
        GetNumCPUCores(numCoresInSystem, numCoresAvailableToProcess);
#else
        //TODO: Needs cross platform support in order to use multithreading properly
#endif
        if (numCoresAvailableToProcess <= 0)
        {
            numCoresAvailableToProcess = 1;
        }

        unsigned int numLogicalProcessorsAvailableToProcess = 0;
        {
            DWORD_PTR processAffinity = 0;
            DWORD_PTR systemAffinity = 0;
#if defined(AZ_PLATFORM_WINDOWS)
            GetProcessAffinityMask(GetCurrentProcess(), &processAffinity, &systemAffinity);
#else
            //TODO: Needs cross platform support
#endif

            numLogicalProcessorsAvailableToProcess = CountBitsSetTo1(processAffinity);

            if (numLogicalProcessorsAvailableToProcess < numCoresAvailableToProcess)
            {
                numLogicalProcessorsAvailableToProcess = numCoresAvailableToProcess;
            }
        }

        unsigned int numLogicalProcessorsInSystem = 0;
        {
#if defined(AZ_PLATFORM_WINDOWS)
            SYSTEM_INFO si;
            GetSystemInfo(&si);
            numLogicalProcessorsInSystem = si.dwNumberOfProcessors;
#else
            //TODO: Needs cross platform support
#endif
        }

        m_systemCpuCoreCount = numCoresInSystem;
        m_processCpuCoreCount = numCoresAvailableToProcess;
        m_systemLogicalProcessorCount = numLogicalProcessorsInSystem;
        m_processLogicalProcessorCount = numLogicalProcessorsAvailableToProcess;
    }

    RCLog("");
    RCLog("CPU cores: %d available (%d in system).",
        m_processCpuCoreCount,
        m_systemCpuCoreCount);
    RCLog("Logical processors: %d available (%d in system).",
        m_processLogicalProcessorCount,
        m_systemLogicalProcessorCount);

    string message;

    m_maxThreads = GetAParallelOption(
            config,
            "threads",
            m_processCpuCoreCount,
            m_processLogicalProcessorCount,
            1,
            m_processCpuCoreCount,
            1,
            m_processLogicalProcessorCount,
            message);
    RCLog("%s Using up to %d thread%s.", message.c_str(), m_maxThreads, ((m_maxThreads > 1) ? "s" : ""));

    RCLog("");

    assert(m_maxThreads >= 1);

    m_pPakManager->SetMaxThreads(m_maxThreads);
}

//////////////////////////////////////////////////////////////////////////
int ResourceCompiler::GetMaxThreads() const
{
    return m_maxThreads;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::VerifyKeyRegistration(const char* szKey) const
{
    assert(szKey);
    const string sKey = StringHelpers::MakeLowerCase(string(szKey));

    if (m_KeyHelp.count(sKey) == 0)
    {
        RCLogWarning("Key '%s' was not registered, call RegisterKey() before using the key", szKey);
    }
}

//////////////////////////////////////////////////////////////////////////
bool ResourceCompiler::HasKeyRegistered(const char* szKey) const
{
    assert(szKey);
    const string sKey = StringHelpers::MakeLowerCase(string(szKey));

    return (m_KeyHelp.count(sKey) != 0);
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::RegisterKey(const char* key, const char* helptxt)
{
    const string sKey = StringHelpers::MakeLowerCase(string(key));

    assert(m_KeyHelp.count(sKey) == 0);       // registered twice

    m_KeyHelp[sKey] = helptxt;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::InitLogs(Config& config)
{
    m_logPrefix.clear();
    m_logPrefix = config.GetAsString("logprefix", "", "");
    if (m_logPrefix.empty())
    {
        m_logPrefix = m_exePath;
    }

    {
        const string logDir = PathHelpers::GetDirectory(m_logPrefix + "unused.name");
        if (!FileUtil::EnsureDirectoryExists(logDir.c_str()))
        {
            RCLogError("Creating directory failed: %s", logDir.c_str());
        }
    }

    m_mainLogFileName    = FormLogFileName(m_filenameLog);
    m_warningLogFileName = FormLogFileName(m_filenameLogWarnings);
    m_errorLogFileName   = FormLogFileName(m_filenameLogErrors);

    AZ::IO::LocalFileIO().Remove(m_mainLogFileName);
    AZ::IO::LocalFileIO().Remove(m_warningLogFileName);
    AZ::IO::LocalFileIO().Remove(m_errorLogFileName);

    //if logfiles is false, disable logging by setting the m_mainLofFileName to empty string
    if (config.GetAsBool("logfiles", false, true))
    {
        m_mainLogFileName = "";
    }
}

string ResourceCompiler::FormLogFileName(const char* suffix) const
{
    return (suffix && suffix[0]) ? m_logPrefix + suffix : string();
}

const string& ResourceCompiler::GetMainLogFileName() const
{
    return m_mainLogFileName;
}

const string& ResourceCompiler::GetErrorLogFileName() const
{
    return m_errorLogFileName;
}

//////////////////////////////////////////////////////////////////////////
clock_t ResourceCompiler::GetStartTime() const
{
    return m_startTime;
}

bool ResourceCompiler::GetTimeLogging() const
{
    return m_bTimeLogging;
}

void ResourceCompiler::SetTimeLogging(bool enable)
{
    m_bTimeLogging = enable;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::AddExitObserver(IResourceCompiler::IExitObserver* p)
{
    if (p == 0)
    {
        return;
    }

    AZStd::lock_guard<AZStd::mutex> lock(m_exitObserversLock);

    m_exitObservers.push_back(p);
}

void ResourceCompiler::RemoveExitObserver(IResourceCompiler::IExitObserver* p)
{
    if (p == 0)
    {
        return;
    }

    AZStd::lock_guard<AZStd::mutex> lock(m_exitObserversLock);

    for (size_t i = 0; i < m_exitObservers.size(); ++i)
    {
        if (m_exitObservers[i] == p)
        {
            m_exitObservers.erase(m_exitObservers.begin() + i);
            return;
        }
    }
}

void ResourceCompiler::NotifyExitObservers()
{
    AZStd::lock_guard<AZStd::mutex> lock(m_exitObserversLock);

    for (size_t i = 0; i < m_exitObservers.size(); ++i)
    {
        m_exitObservers[i]->OnExit();
    }

    m_exitObservers.clear();
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::LogV(const IRCLog::EType eType, const char* szFormat, va_list args)
{
    char str[s_internalBufferSize];
    azvsnprintf(str, s_internalBufferSize, szFormat, args);
    str[s_internalBufferSize - 1] = 0;

    // remove non-printable characters except newlines and tabs
    char* p = str;
    while (*p)
    {
        if ((*p < ' ') && (*p != '\n') && (*p != '\t'))
        {
            *p = ' ';
        }
        ++p;
    }

    LogLine(eType, str);
}

void ResourceCompiler::Log(const IRCLog::EType eType, const char* szMessage)
{
    char str[s_internalBufferSize];
    azstrcpy(str, s_internalBufferSize, szMessage);
    str[s_internalBufferSize - 1] = 0;

    // remove non-printable characters except newlines and tabs
    char* p = str;
    while (*p)
    {
        if ((*p < ' ') && (*p != '\n') && (*p != '\t'))
        {
            *p = ' ';
        }
        ++p;
    }

    LogLine(eType, str);
}

namespace
{
    struct CheckEmptySourceInnerPathAndName
    {
        bool operator()(const RcFile& file) const
        {
            return file.m_sourceInnerPathAndName.empty();
        }
    };
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::FilterExcludedFiles(std::vector<RcFile>& files)
{
    const IConfig* const config = &m_multiConfig.getConfig();

    const bool bVerbose = GetVerbosityLevel() > 0;

    std::vector<string> excludes;
    {
        string excludeStr = config->GetAsString("exclude", "", "", eCP_PriorityAll & ~eCP_PriorityJob);
        if (!excludeStr.empty())
        {
            excludeStr = PathHelpers::ToPlatformPath(excludeStr);
            StringHelpers::Split(excludeStr, ";", false, excludes);
        }

        excludeStr = config->GetAsString("exclude", "", "", eCP_PriorityJob);
        if (!excludeStr.empty())
        {
            excludeStr = PathHelpers::ToPlatformPath(excludeStr);
            StringHelpers::Split(excludeStr, ";", false, excludes);
        }

        if (bVerbose)
        {
            excludeStr.clear();
            for (size_t i = 0; i < excludes.size(); ++i)
            {
                if (i > 0)
                {
                    excludeStr += ";";
                }
                excludeStr += excludes[i];
            }
            RCLog("   Exclude: %s", excludeStr.c_str());
        }
    }

    std::set<const char*, stl::less_stricmp<const char*> > excludedFiles;
    std::vector<std::pair<string, string> > excludedStrings;
    const string excludeListFile = config->GetAsString("exclude_listfile", "", "");
    if (!excludeListFile.empty())
    {
        const string listFormat = config->GetAsString("listformat", "", "");
        CListFile(this).Process(excludeListFile, listFormat, "*", string(), excludedStrings);
        const size_t filenameCount = excludedStrings.size();
        for (size_t i = 0; i < filenameCount; ++i)
        {
            string& name = excludedStrings[i].second;
            name = PathHelpers::ToPlatformPath(name);
            excludedFiles.insert(name.c_str());
        }
    }

    if (excludes.empty() && excludedFiles.empty())
    {
        return;
    }

    string name;
    for (size_t nFile = 0; nFile < files.size(); ++nFile)
    {
        name = files[nFile].m_sourceInnerPathAndName;
        name = PathHelpers::ToPlatformPath(name);

        if (excludedFiles.find(name.c_str()) != excludedFiles.end())
        {
            if (bVerbose)
            {
                RCLog("    Excluding file %s by %s", name.c_str(), excludeListFile.c_str());
            }
            files[nFile].m_sourceInnerPathAndName.clear();
            continue;
        }

        for (size_t i = 0; i < excludes.size(); ++i)
        {
            if (StringHelpers::MatchesWildcardsIgnoreCase(name, excludes[i]))
            {
                if (bVerbose)
                {
                    RCLog("    Excluding file %s by %s", name.c_str(), excludes[i].c_str());
                }
                files[nFile].m_sourceInnerPathAndName.clear();
                break;
            }
        }
    }

    const size_t sizeBefore = files.size();
    files.erase(std::remove_if(files.begin(), files.end(), CheckEmptySourceInnerPathAndName()), files.end());

    RCLog("Files excluded: %i", sizeBefore - files.size());
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::CopyFiles(const std::vector<RcFile>& files, bool bNoOverwrite)
{
    const IConfig* const config = &m_multiConfig.getConfig();

    const bool bSkipMissing = config->GetAsBool("skipmissing", false, true);
    const int srcMaxSize = config->GetAsInt("sourcemaxsize", -1, -1);

    const size_t numFiles = files.size();
    size_t numFilesCopied = 0;
    size_t numFilesUpToDate = 0;
    size_t numFilesSkipped = 0;
    size_t numFilesMissing = 0;
    size_t numFilesFailed = 0;

    RCLog("Starting copying %" PRISIZE_T " files", numFiles);

    string progressString;
    progressString.Format("Copying % " PRISIZE_T " files", numFiles);
    StartProgress();

    const bool bRefresh = config->GetAsBool("refresh", false, true);

    NameConvertor nc;
    if (!nc.SetRules(config->GetAsString("targetnameformat", "", "")))
    {
        return;
    }

    for (size_t i = 0; i < numFiles; ++i)
    {
        if (g_gotCTRLBreakSignalFromOS)
        {
            // abort!
            return;
        }

        ShowProgress(progressString.c_str(), i, numFiles);

        const string srcFilename = PathHelpers::Join(files[i].m_sourceLeftPath, files[i].m_sourceInnerPathAndName);
        string trgFilename = PathHelpers::Join(files[i].m_targetLeftPath, files[i].m_sourceInnerPathAndName);
        if (nc.HasRules())
        {
            const string oldFilename = PathHelpers::GetFilename(trgFilename);
            const string newFilename = nc.GetConvertedName(oldFilename);
            if (newFilename.empty())
            {
                return;
            }
            if (!StringHelpers::EqualsIgnoreCase(oldFilename, newFilename))
            {
                if (GetVerbosityLevel() >= 2)
                {
                    RCLog("Target file name changed: %s -> %s", oldFilename.c_str(), newFilename.c_str());
                }
                trgFilename = PathHelpers::Join(PathHelpers::GetDirectory(trgFilename), newFilename);
            }
        }

        if (GetVerbosityLevel() >= 1)
        {
            RCLog("Copying %s to %s", srcFilename.c_str(), trgFilename.c_str());
        }

        const bool bSourceFileExists = FileUtil::FileExists(srcFilename.c_str());
        const bool bTargetFileExists = FileUtil::FileExists(trgFilename.c_str());

        if (!bSourceFileExists)
        {
            ++numFilesMissing;
            if (!bSkipMissing)
            {
                RCLog("Source file %s does not exist", srcFilename.c_str());
            }
            continue;
        }
        else
        {
            if (!FileUtil::EnsureDirectoryExists(PathHelpers::GetDirectory(trgFilename).c_str()))
            {
                RCLog("Failed creating directory for %s", trgFilename.c_str());
                ++numFilesFailed;
                RCLog("Failed to copy %s to %s", srcFilename.c_str(), trgFilename.c_str());
                continue;
            }

            //////////////////////////////////////////////////////////////////////////
            // Compare source and target files modify timestamps.
            if (bTargetFileExists && FileUtil::FileTimesAreEqual(srcFilename, trgFilename) && !bRefresh)
            {
                // Up to date file already exists in target folder
                ++numFilesUpToDate;
                AddInputOutputFilePair(srcFilename, trgFilename);
                continue;
            }
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // no overwrite
            if (bTargetFileExists && bNoOverwrite && !bRefresh)
            {
                // Up to date file already exists in target folder
                ++numFilesUpToDate;
                AddInputOutputFilePair(srcFilename, trgFilename);
                continue;
            }
            //////////////////////////////////////////////////////////////////////////

            if (srcMaxSize >= 0)
            {
                const __int64 fileSize = FileUtil::GetFileSize(srcFilename);
                if (fileSize > srcMaxSize)
                {
                    ++numFilesSkipped;
                    RCLog("Source file %s is bigger than %d bytes (size is %" PRId64 " ). Skipped.", srcFilename.c_str(), srcMaxSize, fileSize);
                    AddInputOutputFilePair(srcFilename, trgFilename);
                    continue;
                }
            }
#if defined(AZ_PLATFORM_WINDOWS)
            SetFileAttributes(trgFilename, FILE_ATTRIBUTE_ARCHIVE);
#endif
            AZ::IO::LocalFileIO localFileIO;
            const bool bCopied = (localFileIO.Copy(srcFilename, trgFilename) != 0);

            if (bCopied)
            {
                ++numFilesCopied;
#if defined(AZ_PLATFORM_WINDOWS)
                SetFileAttributes(trgFilename, FILE_ATTRIBUTE_ARCHIVE);
#endif
                FileUtil::SetFileTimes(srcFilename, trgFilename);
            }
            else
            {
                ++numFilesFailed;
                RCLog("Failed to copy %s to %s", srcFilename.c_str(), trgFilename.c_str());
            }
        }

        AddInputOutputFilePair(srcFilename, trgFilename);
    }

    RCLog("Finished copying %" PRISIZE_T " files: "
        "%" PRISIZE_T " copied, "
        "%" PRISIZE_T " up-to-date, "
        "%" PRISIZE_T " skipped, "
        "%" PRISIZE_T " missing, "
        "%" PRISIZE_T " failed",
        numFiles, numFilesCopied, numFilesUpToDate, numFilesSkipped, numFilesMissing, numFilesFailed);
}

//////////////////////////////////////////////////////////////////////////
string ResourceCompiler::FindSuitableSourceRoot(const std::vector<string>& sourceRootsReversed, const string& fileName)
{
    if (sourceRootsReversed.empty())
    {
        return string();
    }
    if (sourceRootsReversed.size() > 1)
    {
        for (size_t i = 0; i < sourceRootsReversed.size(); ++i)
        {
            const string& sourceRoot = sourceRootsReversed[i];
            const string fullPath = PathHelpers::Join(sourceRoot, fileName);
            const DWORD fileAttribs = GetFileAttributes(fullPath);
            if (fileAttribs == INVALID_FILE_ATTRIBUTES)
            {
                continue;
            }
            if ((fileAttribs & FILE_ATTRIBUTE_NORMAL) != 0)
            {
                return sourceRoot;
            }
        }
    }

    return sourceRootsReversed[0];
}

//////////////////////////////////////////////////////////////////////////
typedef std::set<const char*, stl::less_strcmp<const char*> > References;
static bool FindReference(const References& references, const char* filename)
{
    // FIXME: for .mtl we have paths with gamefolder included
    // that's why we first check with gamefolder in path...
    if (references.find(filename) != references.end())
    {
        return true;
    }

    // ...then trim gamefolder and check again
    filename = strchr(filename, '/');
    if (!filename)
    {
        return false;
    }

    // Trim slash
    ++filename;

    return references.find(filename) != references.end();
}

//////////////////////////////////////////////////////////////////////////
// Transforms *.$dds -> *.dds, *.$caf -> *.caf, etc.
static string FixAssetPath(const char* path)
{
    const char* const ext = PathUtil::GetExt(path);
    if (ext[0] == '$')
    {
        return PathUtil::ReplaceExtension(path, ext + 1);
    }
    else
    {
        return path;
    }
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::ScanForAssetReferences(std::vector<string>& outReferences, const string& refsRoot)
{
    const IConfig* const config = &m_multiConfig.getConfig();

    const char* const scanRoot = ".";

    RCLog("Scanning for asset references in \"%s\"", scanRoot);

    int numSources = 0;

    References references;

    std::list<string> strings; // storage for all strings
    std::vector<char*> lines;

    TextFileReader reader;

    std::vector<string> resourceListFiles;
    FileUtil::ScanDirectory(scanRoot, "auto_resource*.txt", resourceListFiles, true, string());
    FileUtil::ScanDirectory(scanRoot, "resourcelist.txt", resourceListFiles, true, string());
    for (size_t i = 0; i < resourceListFiles.size(); ++i)
    {
        const string& resourceListFile = resourceListFiles[i];
        if (reader.Load(resourceListFile.c_str(), lines))
        {
            for (size_t j = 0; j < lines.size(); ++j)
            {
                const char* const line = lines[j];
                if (references.find(line) == references.end())
                {
                    strings.push_back(line);
                    references.insert(strings.back().c_str());
                }
            }
            ++numSources;
        }
    }

    std::vector<string> levelPaks;
    FileUtil::ScanDirectory(scanRoot, "level.pak", levelPaks, true, string());
    for (size_t i = 0; i < levelPaks.size(); ++i)
    {
        const string path = PathHelpers::GetDirectory(levelPaks[i].c_str()) + "\\resourcelist.txt";
        if (reader.LoadFromPak(GetPakSystem(), path, lines))
        {
            for (size_t j = 0; j < lines.size(); ++j)
            {
                const char* const line = lines[j];
                if (references.find(line) == references.end())
                {
                    strings.push_back(line);
                    references.insert(strings.back().c_str());
                }
            }
            ++numSources;
        }
    }

    RCLog("Found %i unique references in %i sources", references.size(), numSources);
    RCLog("");

    if (refsRoot.empty())
    {
        outReferences.insert(outReferences.end(), references.begin(), references.end());
    }
    else
    {
        // include mipmaps together with referenced dds-textures
        const string ddsExt("dds");
        const string cgfExt("cgf");
        const string chrExt("chr");
        const string skinExt("skin");
        char extSuffix[16];

        for (References::iterator it = references.begin(); it != references.end(); ++it)
        {
            const string ext = PathHelpers::FindExtension(*it);
#if defined(AZ_PLATFORM_WINDOWS)
            const string dosPath = PathHelpers::ToDosPath(*it);
#else
            const string dosPath = PathHelpers::ToUnixPath(*it);
#endif

            if (StringHelpers::EqualsIgnoreCase(ext, ddsExt))
            {
                string fullPath;
                string mipPath;
                for (int mip = 0;; ++mip)
                {
                    azsnprintf(extSuffix, sizeof(extSuffix), ".%i", mip);
                    fullPath = PathHelpers::Join(refsRoot, dosPath + extSuffix);
                    if (!FileUtil::FileExists(fullPath.c_str()))
                    {
                        break;
                    }
                    outReferences.push_back(string(*it) + extSuffix);
                }
                for (int mip = 0;; ++mip)
                {
                    azsnprintf(extSuffix, sizeof(extSuffix), ".%ia", mip);
                    fullPath = PathHelpers::Join(refsRoot, dosPath + extSuffix);
                    if (!FileUtil::FileExists(fullPath.c_str()))
                    {
                        break;
                    }
                    outReferences.push_back(string(*it) + extSuffix);
                }
            }
            else if (StringHelpers::EqualsIgnoreCase(ext, cgfExt) || StringHelpers::EqualsIgnoreCase(ext, chrExt) || StringHelpers::EqualsIgnoreCase(ext, skinExt))
            {
                static const char* const cgfmext = "m";

                string fullPath;
                fullPath = PathHelpers::Join(refsRoot, dosPath + cgfmext);
                if (FileUtil::FileExists(fullPath.c_str()))
                {
                    outReferences.push_back(string(*it) + cgfmext);
                }
            }

            // we are interested only in existing files
            const string fullPath = PathHelpers::Join(refsRoot, dosPath);
            if (FileUtil::FileExists(fullPath))
            {
                outReferences.push_back(*it);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
static bool MatchesWildcardsSet(const string& str, const std::vector<string>& masks)
{
    const size_t numMasks = masks.size();

    for (size_t i = 0; i < numMasks; ++i)
    {
        if (StringHelpers::MatchesWildcardsIgnoreCase(str, masks[i]))
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::SaveAssetReferences(const std::vector<string>& references, const string& filename, const string& includeMasksStr, const string& excludeMasksStr)
{
    const IConfig* const config = &m_multiConfig.getConfig();

    std::vector<string> includeMasks;
    StringHelpers::Split(includeMasksStr, ";", false, includeMasks);

    std::vector<string> excludeMasks;
    StringHelpers::Split(excludeMasksStr, ";", false, excludeMasks);

    FILE* f = nullptr; 
    azfopen(&f, filename.c_str(), "wt");
    if (!f)
    {
        RCLogError("Unable to open %s for writing", filename.c_str());
        return;
    }

    for (std::vector<string>::const_iterator it = references.begin(); it != references.end(); ++it)
    {
        if (MatchesWildcardsSet(*it, excludeMasks))
        {
            continue;
        }

        if (!includeMasks.empty() && !MatchesWildcardsSet(*it, includeMasks))
        {
            continue;
        }
        fprintf(f, "%s\n", it->c_str());
    }
    fclose(f);
}
//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::CleanTargetFolder(bool bUseOnlyInputFiles)
{
    const IConfig* const config = &m_multiConfig.getConfig();

    {
        const string targetroot = PathHelpers::ToPlatformPath(PathHelpers::CanonicalizePath(config->GetAsString("targetroot", "", "")));
        if (targetroot.empty())
        {
            return;
        }
        RCLog("Cleaning target folder %s", targetroot.c_str());
    }

    CDependencyList inputOutputFileList(m_inputOutputFileList);

    // Look at the list of processed files.
    {
        inputOutputFileList.RemoveDuplicates();

        const string filename = FormLogFileName(m_filenameOutputFileList);

        if (FileUtil::FileExists(filename.c_str()))
        {
            const size_t oldCount = inputOutputFileList.GetCount();
            inputOutputFileList.Load(filename);
            const size_t loadedCount = inputOutputFileList.GetCount() - oldCount;
            inputOutputFileList.RemoveDuplicates();
            const size_t addedCount = inputOutputFileList.GetCount() - oldCount;
            RCLog("%u entries (%u unique) found in list of processed files '%s'", (uint)loadedCount, (uint)addedCount, filename.c_str());
        }
        else
        {
            RCLog("List of processed files '%s' is not found", filename.c_str());
        }

        RCLog("%u entries in list of processed files", (uint)inputOutputFileList.GetCount());
    }

    // Prepare lists of files that were deleted.
    std::vector<string> deletedSourceFiles;
    std::vector<string> deletedTargetFiles;

    if (bUseOnlyInputFiles)
    {
        for (size_t nInput = 0; nInput < m_inputFilesDeleted.size(); ++nInput)
        {
            const string& deletedInputFilename = m_inputFilesDeleted[nInput].m_sourceInnerPathAndName;
            deletedSourceFiles.push_back(deletedInputFilename);
            for (size_t i = 0; i < inputOutputFileList.GetCount(); ++i)
            {
                const CDependencyList::SFile& of = inputOutputFileList.GetElement(i);
                if (deletedInputFilename == of.inputFile)
                {
                    deletedTargetFiles.push_back(of.outputFile);
                }
            }
        }
    }
    else
    {
        string lastInputFile;
        bool bSrcFileExists = false;
        for (size_t i = 0; i < inputOutputFileList.GetCount(); ++i)
        {
            // Check if input file exists
            const CDependencyList::SFile& of = inputOutputFileList.GetElement(i);
            if (of.inputFile != lastInputFile)
            {
                lastInputFile = of.inputFile;
                if (FileUtil::FileExists(of.inputFile.c_str()))
                {
                    bSrcFileExists = true;
                }
                else
                {
                    RCLog("Source file deleted: \"%s\"", of.inputFile.c_str());
                    deletedSourceFiles.push_back(of.inputFile);
                    bSrcFileExists = false;
                }
            }
            if (!bSrcFileExists)
            {
                deletedTargetFiles.push_back(of.outputFile);
            }
        }
    }

    std::sort(deletedSourceFiles.begin(), deletedSourceFiles.end());
    std::sort(deletedTargetFiles.begin(), deletedTargetFiles.end());

    //////////////////////////////////////////////////////////////////////////
    // Remove records with matching input filenames
    inputOutputFileList.RemoveInputFiles(deletedSourceFiles);

    //////////////////////////////////////////////////////////////////////////
    // Delete target files from disk
    for (size_t i = 0; i < deletedTargetFiles.size(); ++i)
    {
        const string& filename = deletedTargetFiles[i];
        if (i <= 0 || filename != deletedTargetFiles[i - 1])
        {
            RCLog("Deleting file \"%s\"", filename.c_str());
            AZ::IO::LocalFileIO().Remove(filename.c_str());

        }
    }

    StartProgress();

    m_pPakManager->DeleteFilesFromPaks(config, deletedTargetFiles);

    {
        const string filename = FormLogFileName(m_filenameOutputFileList);
        RCLog("Saving %s", filename.c_str());
        inputOutputFileList.RemoveDuplicates();
        inputOutputFileList.Save(filename.c_str());
    }

    // store deleted files list.
    {
        const string filename = FormLogFileName(m_filenameDeletedFileList);
        RCLog("Saving %s", filename.c_str());

        FILE* f = nullptr; 
        azfopen(&f, filename.c_str(), "wt");
        if (f)
        {
            for (size_t i = 0; i < deletedTargetFiles.size(); ++i)
            {
                const string filename = CDependencyList::NormalizeFilename(deletedTargetFiles[i].c_str());
                fprintf(f, "%s\n", filename.c_str());
            }
            fclose(f);
        }
        else
        {
            RCLogError("Failed to write %s", filename.c_str());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
static ICryXML* LoadICryXML()
{
    HMODULE hXMLLibrary = CryLoadLibraryDefName("CryXML");
    if (NULL == hXMLLibrary)
    {
        RCLogError("Unable to load xml library (CryXML)");
        return 0;
    }
    FnGetICryXML pfnGetICryXML = (FnGetICryXML)CryGetProcAddress(hXMLLibrary, "GetICryXML");
    if (pfnGetICryXML == 0)
    {
        RCLogError("Unable to load xml library (CryXML) - cannot find exported function GetICryXML().");
        return 0;
    }
    return pfnGetICryXML();
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef ResourceCompiler::LoadXml(const char* filename)
{
    ICryXML* pCryXML = LoadICryXML();
    if (!pCryXML)
    {
        return 0;
    }

    // Get the xml serializer.
    IXMLSerializer* pSerializer = pCryXML->GetXMLSerializer();

    // Read in the input file.
    XmlNodeRef root;
    {
        const bool bRemoveNonessentialSpacesFromContent = false;
        char szErrorBuffer[1024];
        root = pSerializer->Read(FileXmlBufferSource(filename), false, sizeof(szErrorBuffer), szErrorBuffer);
        if (!root)
        {
            RCLogError("Failed to load XML file '%s': %s", filename, szErrorBuffer);
            return 0;
        }
    }
    return root;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef ResourceCompiler::CreateXml(const char* tag)
{
    ICryXML* pCryXML = LoadICryXML();
    if (!pCryXML)
    {
        return 0;
    }

    // Get the xml serializer.
    IXMLSerializer* pSerializer = pCryXML->GetXMLSerializer();

    XmlNodeRef root;
    {
        root = pSerializer->CreateNode(tag);
        if (!root)
        {
            RCLogError("Cannot create new XML node '%s'\n", tag);
            return 0;
        }
    }
    return root;
}

//////////////////////////////////////////////////////////////////////////
int ResourceCompiler::ProcessJobFile()
{
    const MultiplatformConfig savedConfig = m_multiConfig;
    const IConfig* const config = &m_multiConfig.getConfig();

    // Job file is an XML with multiple jobs for the RC
    const string jobFile = config->GetAsString("job", "", "");
    if (jobFile.empty())
    {
        RCLogError("No job file specified");
        m_multiConfig = savedConfig;
        return eRcExitCode_Error;
    }

    const string runJob = config->GetAsString("jobtarget", "", "");
    const bool runJobFromCommandLine = !runJob.empty();

    CPropertyVars properties(this);

    properties.SetProperty("_rc_exe_folder", GetExePath());
    properties.SetProperty("_rc_tmp_folder", GetTmpPath());

    if (GetVerbosityLevel() >= 0)
    {
        RCLog("Pre-defined job properties:");
        properties.PrintProperties();
    }

    config->CopyToPropertyVars(properties);

    XmlNodeRef root = LoadXml(jobFile.c_str());
    if (!root)
    {
        RCLogError("Failed to load job XML file %s", jobFile.c_str());
        m_multiConfig = savedConfig;
        return eRcExitCode_Error;
    }

    // Check command line with respect to known DefaultProperties
    {
        std::vector<string> defaultProperties;

        for (int i = 0; i < root->getChildCount(); ++i)
        {
            ExtractJobDefaultProperties(defaultProperties, root->getChild(i));
        }

        if (!CheckCommandLineOptions(*config, &defaultProperties))
        {
            return eRcExitCode_Error;
        }
    }

    int result = eRcExitCode_Success;

    for (int i = 0; i < root->getChildCount(); ++i)
    {
        XmlNodeRef jobNode = root->getChild(i);
        const bool runNodes = !runJobFromCommandLine;
        const int tmpResult = EvaluateJobXmlNode(properties, jobNode, runNodes);
        if (result == eRcExitCode_Success)
        {
            result = tmpResult;
        }
    }

    if (runJobFromCommandLine)
    {
        const int tmpResult = RunJobByName(properties, root, runJob);
        if (result == eRcExitCode_Success)
        {
            result = tmpResult;
        }
    }

    m_multiConfig = savedConfig;
    return result;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::ExtractJobDefaultProperties(std::vector<string>& properties, const XmlNodeRef& jobNode)
{
    IConfig* const config = &m_multiConfig.getConfig();

    if (jobNode->isTag("DefaultProperties"))
    {
        // Attributes are config modifiers.
        for (int attr = 0; attr < jobNode->getNumAttributes(); ++attr)
        {
            const char* key = "";
            const char* value = "";
            jobNode->getAttributeByIndex(attr, &key, &value);
            if (std::find(properties.begin(), properties.end(), key) == properties.end())
            {
                properties.push_back(StringHelpers::MakeLowerCase(key));
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
int ResourceCompiler::EvaluateJobXmlNode(CPropertyVars& properties, XmlNodeRef& jobNode, bool runJobs)
{
    IConfig* const config = &m_multiConfig.getConfig();

    if (g_gotCTRLBreakSignalFromOS)
    {
        return eRcExitCode_Error;
    }

    if (jobNode->isTag("Properties"))
    {
        // Attributes are config modifiers.
        for (int attr = 0; attr < jobNode->getNumAttributes(); attr++)
        {
            const char* key = "";
            const char* value = "";
            jobNode->getAttributeByIndex(attr, &key, &value);
            string strValue = value;
            properties.ExpandProperties(strValue);
            properties.SetProperty(key, strValue);
        }
        return eRcExitCode_Success;
    }

    // Default properties don't overwrite existing ones (e.g. specified from command line)
    if (jobNode->isTag("DefaultProperties"))
    {
        // Attributes are config modifiers.
        for (int attr = 0; attr < jobNode->getNumAttributes(); attr++)
        {
            const char* key = "";
            const char* value = "";
            jobNode->getAttributeByIndex(attr, &key, &value);
            if (!properties.HasProperty(key))
            {
                string strValue = value;
                properties.ExpandProperties(strValue);
                properties.SetProperty(key, strValue);
            }
        }
        return eRcExitCode_Success;
    }

    if (jobNode->isTag("Run"))
    {
        if (!runJobs)
        {
            return eRcExitCode_Success;
        }

        const char* jobListName = jobNode->getAttr("Job");
        if (strlen(jobListName) == 0)
        {
            return eRcExitCode_Success;
        }

        // Attributes are config modifiers.
        const CPropertyVars previousProperties = properties;
        for (int attr = 0; attr < jobNode->getNumAttributes(); attr++)
        {
            const char* key = "";
            const char* value = "";
            jobNode->getAttributeByIndex(attr, &key, &value);
            string strValue = value;
            properties.ExpandProperties(strValue);
            properties.SetProperty(key, strValue);
        }

        int result = RunJobByName(properties, jobNode, jobListName);
        properties = previousProperties;
        return result;
    }

    if (jobNode->isTag("Include"))
    {
        const char* includeFile = jobNode->getAttr("file");
        if (strlen(includeFile) == 0)
        {
            return eRcExitCode_Success;
        }

        const string jobFile = config->GetAsString("job", "", "");
        const string includePath = PathHelpers::AddSeparator(PathHelpers::GetDirectory(jobFile)) + includeFile;

        XmlNodeRef root = LoadXml(includePath);
        if (!root)
        {
            RCLogError("Failed to load included job XML file '%s'", includePath.c_str());
            return eRcExitCode_Error;
        }

        // Add include sub-nodes
        XmlNodeRef parent = jobNode->getParent();
        while (root->getChildCount() != 0)
        {
            XmlNodeRef subJobNode = root->getChild(0);
            root->removeChild(subJobNode);
            parent->addChild(subJobNode);
        }
        return eRcExitCode_Success;
    }

    // Condition node.
    if (jobNode->isTag("if") || jobNode->isTag("ifnot"))
    {
        bool bIf = false;
        for (int attr = 0; attr < jobNode->getNumAttributes(); attr++)
        {
            const char* key = "";
            const char* value = "";
            jobNode->getAttributeByIndex(attr, &key, &value);

            string propValue;
            properties.GetProperty(key, propValue);
            if (azstricmp(value, propValue.c_str()) == 0)
            {
                // match.
                bIf = true;
            }
        }
        if (jobNode->isTag("ifnot"))
        {
            bIf = !bIf; // Invert
        }
        int result = eRcExitCode_Success;
        if (bIf)
        {
            // Exec sub-nodes
            for (int i = 0; i < jobNode->getChildCount(); i++)
            {
                XmlNodeRef subJobNode = jobNode->getChild(i);
                const int tmpResult = EvaluateJobXmlNode(properties, subJobNode, true);
                if (result == eRcExitCode_Success)
                {
                    result = tmpResult;
                }
            }
        }
        return result;
    }

    if (jobNode->isTag("Job"))
    {
        RCLog("-------------------------------------------------------------------");
        string jobLog = "Job: ";
        // Delete all config entries from previous job.
        config->ClearPriorityUsage(eCP_PriorityJob);

        bool bCleanTargetRoot = false;
        string refsSaveFilename;
        string refsSaveInclude;
        string refsSaveExclude;
        string refsRoot;

        // Attributes are config modifiers.
        for (int attr = 0; attr < jobNode->getNumAttributes(); attr++)
        {
            const char* key = "";
            const char* value = "";
            jobNode->getAttributeByIndex(attr, &key, &value);

            string valueStr = value;
            properties.ExpandProperties(valueStr);

            if (azstricmp(key, "input") == 0)
            {
                jobLog += string("/") + key + "=" + valueStr + " ";
                continue;
            }
            else if (azstricmp(key, "clean_targetroot") == 0)
            {
                bCleanTargetRoot = true;
                continue;
            }
            else if (azstricmp(key, "refs_scan") == 0)
            {
                RCLogError("refs_scan is not supported anymore");
                return eRcExitCode_Error;
            }
            else if (azstricmp(key, "refs_save") == 0)
            {
                if (valueStr.empty())
                {
                    RCLogError("Missing filename in refs_save option");
                    return eRcExitCode_Error;
                }
                refsSaveFilename = valueStr;
                continue;
            }
            else if (azstricmp(key, "refs_root") == 0)
            {
                refsRoot = valueStr;
                continue;
            }
            else if (azstricmp(key, "refs_save_include") == 0)
            {
                refsSaveInclude = valueStr;
                continue;
            }
            else if (azstricmp(key, "refs_save_exclude") == 0)
            {
                refsSaveExclude = valueStr;
                continue;
            }

            config->SetKeyValue(eCP_PriorityJob, key, valueStr);
            jobLog += string("/") + key + "=" + valueStr + " (attribute) ";
        }

        // Apply properties from RCJob to config
        properties.Enumerate([config, &jobLog](const string& propName, const string& propVal)
            {
                if (config->HasKeyRegistered(propName) && !StringHelpers::EqualsIgnoreCase(propName, "job"))
                {
                    config->SetKeyValue(eCP_PriorityProperty, propName, propVal);
                    jobLog += string("/") + propName + "=" + propVal + " (property) ";
                }
            });

        // Check current platform property against start-up platform setting
        // Reason: This setting cannot be modified after start-up
        const char* pCurrentPlatform = NULL;
        if (config->GetKeyValue("p", pCurrentPlatform) && pCurrentPlatform)
        {
            int currentPlatformIndex = FindPlatform(pCurrentPlatform);
            if (GetMultiplatformConfig().getActivePlatform() != currentPlatformIndex)
            {
                RCLogWarning("The platform property '/p=%s' is ignored because it can only be specified on the command-line", pCurrentPlatform);
            }
        }

        string fileSpec = jobNode->getAttr("input");
        properties.ExpandProperties(fileSpec);
        if (!fileSpec.empty())
        {
            RCLog(jobLog);
            RemoveOutputFiles();
            std::vector<RcFile> files;
            if (CollectFilesToCompile(fileSpec, files) && !files.empty())
            {
                SetupMaxThreads();
                CompileFilesBySingleProcess(files);
            }
        }
        else
        {
            if (!refsSaveFilename.empty())
            {
                properties.ExpandProperties(refsSaveFilename);
                if (refsSaveFilename.empty())
                {
                    RCLogError("Empty filename specified in refs_save option");
                    return eRcExitCode_Error;
                }

                properties.ExpandProperties(refsRoot);

                std::vector<string> references;
                ScanForAssetReferences(references, refsRoot);
                SaveAssetReferences(references, refsSaveFilename, refsSaveInclude, refsSaveExclude);
            }
            if (bCleanTargetRoot)
            {
                CleanTargetFolder(false);
            }
        }
        // Delete all config entries from our job.
        config->ClearPriorityUsage(eCP_PriorityJob | eCP_PriorityProperty);
        return eRcExitCode_Success;
    }

    return eRcExitCode_Success;
}

//////////////////////////////////////////////////////////////////////////
int ResourceCompiler::RunJobByName(CPropertyVars& properties, XmlNodeRef& anyNode, const char* name)
{
    // Find root
    XmlNodeRef root = anyNode;
    while (root->getParent())
    {
        root = root->getParent();
    }

    // Find JobList.
    XmlNodeRef jobListNode = root->findChild(name);
    if (!jobListNode)
    {
        RCLogError("Unable to find job \"%s\"", name);
        return eRcExitCode_Error;
    }
    {
        // Execute Job sub nodes.
        int result = eRcExitCode_Success;
        for (int i = 0; i < jobListNode->getChildCount(); i++)
        {
            XmlNodeRef subJobNode = jobListNode->getChild(i);
            const int tmpResult = EvaluateJobXmlNode(properties, subJobNode, true);
            if (result == eRcExitCode_Success)
            {
                result = tmpResult;
            }
        }
        return result;
    }
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::LogMemoryUsage(bool bReportProblemsOnly)
{
#if defined(AZ_PLATFORM_WINDOWS)
    PROCESS_MEMORY_COUNTERS p;
    ZeroMemory(&p, sizeof(p));
    p.cb = sizeof(p);


    if (!GetProcessMemoryInfo(GetCurrentProcess(), &p, sizeof(p)))
    {
        RCLogError("Cannot obtain memory info");
        return;
    }

    static const float megabyte = 1024 * 1024;
    const float peakSizeMb = p.PeakWorkingSetSize / megabyte;
#if defined(WIN64)
    static const float warningMemorySizePeakMb =  7500.0f;
    static const float errorMemorySizePeakMb   = 15500.0f;
#else
    static const float warningMemorySizePeakMb =  3100.0f;
    static const float errorMemorySizePeakMb   =  3600.0f;
#endif

    bool bReportProblem = false;
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_memorySizeLock);
        if (peakSizeMb > m_memorySizePeakMb)
        {
            m_memorySizePeakMb = peakSizeMb;
            bReportProblem = (peakSizeMb >= warningMemorySizePeakMb);
        }
    }

    if (bReportProblem || !bReportProblemsOnly)
    {
        if (peakSizeMb >= warningMemorySizePeakMb)
        {
            ((peakSizeMb >= errorMemorySizePeakMb) ? RCLogError : RCLogWarning)(
                "Memory: working set %.1fMb (peak %.1fMb - DANGER!), pagefile %.1fMb (peak %.1fMb)",
                p.WorkingSetSize / megabyte, p.PeakWorkingSetSize / megabyte,
                p.PagefileUsage / megabyte, p.PeakPagefileUsage / megabyte);
        }
        else
        {
            RCLog(
                "Memory: working set %.1fMb (peak %.1fMb), pagefile %.1fMb (peak %.1fMb)",
                p.WorkingSetSize / megabyte, p.PeakWorkingSetSize / megabyte,
                p.PagefileUsage / megabyte, p.PeakPagefileUsage / megabyte);
        }
    }
#endif
}

void ResourceCompiler::AddUnitTest(FnRunUnitTests unitTestFunction)
{
    if (!unitTestFunction)
    {
        RCLogWarning("AddUnitTest called with null function pointer");
        return;
    }

    m_unitTestFunctions.push_back(unitTestFunction);
}

namespace ResourceCompilerTests
{
    void Run(UnitTestHelper* pUnitTestHelper);
}

int ResourceCompiler::RunUnitTests()
{
    RCLog("Executing unit tests...");
    UnitTestHelper unitTestHelper;

    ResourceCompilerTests::Run(&unitTestHelper);

    for (auto unitTestFunction : m_unitTestFunctions)
    {
        unitTestFunction(&unitTestHelper);
    }
    RCLog("Unit testing complete. %u out of %u passed", unitTestHelper.GetTestsSucceededCount(), unitTestHelper.GetTestsPerformedCount());
    return unitTestHelper.AllUnitTestsPassed() ? eRcExitCode_Success : eRcExitCode_Error;
}

// here we wrap main(...) so that we can absolutely ensure any objects created on the stack during the actual main are gone by the time
// we leave it and memory can be freed.
int __cdecl main(int argc, char** argv, char** envp)
{
#ifdef AZ_TESTS_ENABLED
    if (argc == 2 && 0 == azstricmp(argv[1], "--unittest"))
    {
        return AzMainUnitTests();
    }
#endif // AZ_TESTS_ENABLED

    // here we are wrapping main to handle memory management around it.
    if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
    }

    int exitCode = main_impl(argc, argv, envp);

    if (AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }

    return exitCode;
}
