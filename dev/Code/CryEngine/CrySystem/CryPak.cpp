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

// Description : Implementation of the Crytek package files management


#include "StdAfx.h"
#include "CryPak.h"
#include <ILog.h>
#include <ISystem.h>
#include <IPlatformOS.h>
#include <IConsole.h>
#include <ITimer.h>
#include <IPerfHud.h>
#include <IStatoscope.h>
#include <IStreamEngine.h>
#include <IGameStartup.h>
#include "CryCrc32.h"
#include "CryZlib.h"
#include <md5.h>
#include "System.h"
#include "CustomMemoryHeap.h"
#include "CryArchive.h"
#include "StringUtils.h"
#include "ZipEncrypt.h"

#include <AzCore/base.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/functional.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzFramework/IO/FileOperations.h>
#include <LoadScreenBus.h>

#include "System.h"

#if AZ_LEGACY_CRYSYSTEM_TRAIT_CRYPAK_POSIX
#include <unistd.h>
#include <sys/stat.h>                   // fstat, fileno
//#define fileno(X) ((X)->_Handle)
#endif

#include <IDiskProfiler.h>

#if defined(AZ_PLATFORM_APPLE)
#include "SystemUtilsApple.h"
#endif

typedef CryStackStringT<char, 1024> TPathStackString;
/////////////////////////////////////////////////////

#define EDITOR_DATA_FOLDER "editor"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif //WIN32

extern CMTSafeHeap* g_pPakHeap;

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((unsigned int)-1)
#endif

#if defined(PURE_CLIENT)

#pragma warning( disable : 6240 ) // (<expression> && <non-zero constant>) always evaluates to the result of <expression>

#define IsModPath(xxx) (false)                                          // Removes functionality for PC client release and consoles. ANTI TAMPER

#else

inline bool IsModPath(const char* originalPath)
{
    // create copy of path and replace slashes
    CryPathString path = originalPath;
    path.replace('\\', '/');

    // check if field lies inside of mods/ folder
    CryStackStringT<char, 32> modsStr("m");
    modsStr += "o";
    modsStr += "d";
    modsStr += "s";
    modsStr += "/";
    const size_t modsStrLen = modsStr.length();

    if (_strnicmp(path.c_str(), modsStr.c_str(), modsStrLen) == 0)
    {
        return true;
    }

    // check for custom SP/MP levels inside of gamecrysis2/levels/ folder
    CryStackStringT<char, 32> levelsStr("c");
    levelsStr += "3";
    levelsStr += "/";
    levelsStr += "l";
    levelsStr += "e";
    levelsStr += "v";
    levelsStr += "e";
    levelsStr += "l";
    levelsStr += "s";
    levelsStr += "/";
    const size_t levelsStrLen = levelsStr.length();
    const bool startsWithLevels = _strnicmp(path.c_str(), levelsStr.c_str(), levelsStrLen) == 0;
    if (startsWithLevels)
    {
        return true;
    }

    return false;
}
#endif

namespace
{
    // Utility function to de-alias pak file opening and file-within-pak opening
    // if the file specified was an absolute path but it points at one of the aliases, de-alias it and replace it with that alias.
    // this works around problems where the level editor is in control but still mounts asset packs (ie, level.pak mounted as @assets@)
    // it is assumed that the path is already converted to forward slashes, lowcased, and normalized
    string ConvertAbsolutePathToAliasedPath(const char* sourcePath, const char* aliasToLookFor = "@devassets@", const char* aliasToReplaceWith = "@assets@")
    {
        if ((aliasToLookFor) && (aliasToReplaceWith) && (sourcePath) && (AZ::IO::FileIOBase::GetDirectInstance()))
        {
            char unaliasedPath[AZ_MAX_PATH_LEN] = { 0 };

            if (sourcePath[0] != '@')
            {
                const char* alias = AZ::IO::FileIOBase::GetDirectInstance()->GetAlias(aliasToLookFor);
                if ((alias) && (azstrnicmp(sourcePath, alias, strlen(alias)) == 0)) // check to see if it starts with the absolute path that the alias resolves to
                {
                    azstrcpy(unaliasedPath, AZ_MAX_PATH_LEN, aliasToReplaceWith);
                    azstrcat(unaliasedPath, AZ_MAX_PATH_LEN, CRY_NATIVE_PATH_SEPSTR);

                    if (strlen(sourcePath) == strlen(alias))
                    {
                        return unaliasedPath;
                    }

                    azstrcat(unaliasedPath, AZ_MAX_PATH_LEN, sourcePath + (strlen(alias) + 1));
                    return unaliasedPath;
                }
            }
            else if (azstrnicmp(sourcePath, aliasToLookFor, strlen(aliasToLookFor)) == 0)  // we also check to see if it starts with the alias instead of its absolute path
            {
                azstrcpy(unaliasedPath, AZ_MAX_PATH_LEN, aliasToReplaceWith);
                azstrcat(unaliasedPath, AZ_MAX_PATH_LEN, CRY_NATIVE_PATH_SEPSTR);

                if (strlen(sourcePath) == strlen(aliasToLookFor))
                {
                    return unaliasedPath;
                }

                azstrcat(unaliasedPath, AZ_MAX_PATH_LEN, sourcePath + (strlen(aliasToLookFor) + 1));
                return unaliasedPath;
            }
        }
        return sourcePath;
    }
}

//////////////////////////////////////////////////////////////////////////
// IResourceList implementation class.
//////////////////////////////////////////////////////////////////////////
class CResourceList
    : public IResourceList
{
public:
    CResourceList() { m_iter = m_set.end(); };
    ~CResourceList() {};

    static stack_string UnifyFilename(const char* sResourceFile)
    {
        if (!sResourceFile)
        {
            return ".";
        }
        stack_string filename = sResourceFile;
        // Ideally the convert to alias call would be unnecessary - if we use aliases in all cases coming into our file open operations.
        // This isn't currently the case, so we'll remove any that manage to get through to this point
        AZ::IO::FileIOBase::GetInstance()->ConvertToAlias(filename.m_strBuf, filename.capacity());
        
        // And convert from @devassets@ to @assets@
        filename = ConvertAbsolutePathToAliasedPath(filename.c_str());
        filename.replace('\\', '/');
#if !defined(AZ_PLATFORM_APPLE)
        filename.MakeLower();
#endif
        return filename;
    }

    virtual void Add(const char* sResourceFile)
    {
        stack_string filename = UnifyFilename(sResourceFile);

        CryAutoLock<CryCriticalSection> lock(m_lock);
        m_set.insert(filename);
    }
    virtual void Clear()
    {
        CryAutoLock<CryCriticalSection> lock(m_lock);
        m_set.clear();
        m_iter = m_set.begin();
    }
    virtual bool IsExist(const char* sResourceFile)
    {
        stack_string filename = UnifyFilename(sResourceFile);

        CryAutoLock<CryCriticalSection> lock(m_lock);
        if (m_set.find(CONST_TEMP_STRING(filename.c_str())) != m_set.end())
        {
            return true;
        }
        return false;
    }
    virtual bool Load(const char* sResourceListFilename)
    {
        Clear();
        CCryFile file;
        if (file.Open(sResourceListFilename, "rb"))
        {
            CryAutoLock<CryCriticalSection> lock(m_lock);

            int nLen = file.GetLength();
            _smart_ptr<IMemoryBlock> pMemBlock = gEnv->pCryPak->PoolAllocMemoryBlock(nLen + 1, "ResourceList"); // Allocate 1 character more for Null termination.
            char* buf = (char*)pMemBlock->GetData();
            buf[nLen] = 0; // Force null terminate.
            file.ReadRaw(buf, nLen);

            // Parse file, every line in a file represents a resource filename.
            char seps[] = "\r\n";
            char* nextToken = nullptr;
            char* token = azstrtok(buf, 0, seps, &nextToken);
            while (token != NULL)
            {
                Add(token);
                token = azstrtok(NULL, 0, seps, &nextToken);
            }
            return true;
        }
        return false;
    }
    virtual const char* GetFirst()
    {
        m_lock.Lock();
        m_iter = m_set.begin();
        if (m_iter != m_set.end())
        {
            return *m_iter;
        }
        m_lock.Unlock();
        return NULL;
    }
    virtual const char* GetNext()
    {
        if (m_iter != m_set.end())
        {
            m_iter++;
            if (m_iter != m_set.end())
            {
                return *m_iter;
            }
        }
        m_lock.Unlock();
        return NULL;
    }

    void GetMemoryStatistics(ICrySizer* pSizer)
    {
        CryAutoLock<CryCriticalSection> lock(m_lock);

        int nSize = sizeof(*this);
        for (ResourceSet::const_iterator it = m_set.begin(); it != m_set.end(); ++it)
        {
            // Count size of all strings in the set.
            nSize += it->GetAllocatedMemory();
        }
        pSizer->AddObject(this, nSize);
        pSizer->AddObject(m_set);
    }

private:
    typedef std::set<string> ResourceSet;
    CryCriticalSection m_lock;
    ResourceSet m_set;
    ResourceSet::iterator m_iter;
};

//////////////////////////////////////////////////////////////////////////
class CNextLevelResourceList
    : public IResourceList
{
public:
    CNextLevelResourceList() {};
    ~CNextLevelResourceList() {};

    const char* UnifyFilename(const char* sResourceFile)
    {
        static char sFile[256];
        int len = min((int)strlen(sResourceFile), (int)sizeof(sFile) - 1);
        int i;
        for (i = 0; i < len; i++)
        {
            if (sResourceFile[i] != '\\')
            {
                sFile[i] = sResourceFile[i];
            }
            else
            {
                sFile[i] = '/';
            }
        }
        sFile[i] = 0;
        azstrlwr(sFile, AZ_ARRAY_SIZE(sFile));
        return sFile;
    }

    uint32 GetFilenameHash(const char* sResourceFile)
    {
        uint32 code = (uint32)crc32(0L, (unsigned char*)sResourceFile, strlen(sResourceFile));
        return code;
    }

    virtual void Add(const char* sResourceFile)
    {
        assert(0); // Not implemented
    }
    virtual void Clear()
    {
        stl::free_container(m_resources_crc32);
    }
    virtual bool IsExist(const char* sResourceFile)
    {
        uint32 nHash = GetFilenameHash(UnifyFilename(sResourceFile));
        if (stl::binary_find(m_resources_crc32.begin(), m_resources_crc32.end(), nHash) != m_resources_crc32.end())
        {
            return true;
        }
        return false;
    }
    virtual bool Load(const char* sResourceListFilename)
    {
        bool bOk = false;
        m_resources_crc32.reserve(1000);
        CCryFile file;
        if (file.Open(sResourceListFilename, "rb"))
        {
            int nFileLen = file.GetLength();
            char* buf = new char[nFileLen + 16];
            file.ReadRaw(buf, nFileLen);
            buf[nFileLen] = '\0';

            // Parse file, every line in a file represents a resource filename.
            char seps[] = "\r\n";
            char* nextToken = nullptr;
            char* token = azstrtok(buf, 0, seps, &nextToken);
            while (token != NULL)
            {
                uint32 nHash = GetFilenameHash(token);
                m_resources_crc32.push_back(nHash);
                token = azstrtok(NULL, 0, seps, &nextToken);
            }
            delete[]buf;
            bOk = true;
        }
        std::sort(m_resources_crc32.begin(), m_resources_crc32.end());
        return bOk;
    }
    virtual const char* GetFirst()
    {
        assert(0); // Not implemented
        return NULL;
    }
    virtual const char* GetNext()
    {
        assert(0); // Not implemented
        return NULL;
    }
    void GetMemoryStatistics(ICrySizer* pSizer)
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_resources_crc32);
    }

private:
    std::vector<uint32> m_resources_crc32;
};

#define COLLECT_TIME_STATISTICS
//////////////////////////////////////////////////////////////////////////
// Automatically calculate time taken by file operations.
//////////////////////////////////////////////////////////////////////////
struct SAutoCollectFileAcessTime
{
    SAutoCollectFileAcessTime(CCryPak* pPak)
    {
#ifdef COLLECT_TIME_STATISTICS
        m_pPak = pPak;
        m_fTime = m_pPak->m_pITimer->GetAsyncCurTime();
#endif
    }
    ~SAutoCollectFileAcessTime()
    {
#ifdef COLLECT_TIME_STATISTICS
        m_fTime = m_pPak->m_pITimer->GetAsyncCurTime() - m_fTime;
        m_pPak->m_fFileAcessTime += m_fTime;
#endif
    }
private:
    CCryPak* m_pPak;
    float m_fTime;
};

static void fileAccessMessage(int threadIndex, const char* inName)
{
    static volatile bool s_threadAndRecursionGuard = false;

    if (s_threadAndRecursionGuard == false)
    {
        s_threadAndRecursionGuard = true;

        const char* name = strchr(inName, ':');
        name = name ? (name + 2) : inName;

        const char* threadName = (threadIndex == 0) ? "main" : "render";
        CryFixedStringT<2048> msg;
        const char* funcs[32];
        int nCount = 32;

        gEnv->pSystem->debug_GetCallStack(funcs, nCount);

        msg.Format("File opened on %s thread:\n\n%s\n\n --- Callstack ---\n",
            threadName, name);

        CryFixedStringT<256> temp;

        for (int i = 1; i < nCount; i++)
        {
            temp.Format("%02d) %s\n", i, funcs[i]);
            msg.append(temp);
        }

        CryLog("%s", msg.c_str());

        IPlatformOS::EMsgBoxResult result;

        IPlatformOS* pOS = gEnv->pSystem->GetPlatformOS();
        result = pOS->DebugMessageBox(msg.c_str(), "TRC/TCR Fail: Syncronous File Access");

        if (result == IPlatformOS::eMsgBox_Cancel)
        {
            DebugBreak();
        }
        else
        {
            Sleep(33);
        }
        s_threadAndRecursionGuard = false;
    }
}

/////////////////////////////////////////////////////
// Initializes the crypak system;
//   pVarPakPriority points to the variable, which is, when set to 1,
//   signals that the files from pak should have higher priority than filesystem files
CCryPak::CCryPak(IMiniLog* pLog, PakVars* pPakVars, const bool bLvlRes, const IGameStartup* pGameStartup)
    : m_pLog(pLog)
    , m_eRecordFileOpenList(RFOM_Disabled)
    , m_pPakVars(pPakVars ? pPakVars : &g_cvars.pakVars)
    , m_fFileAcessTime(0.f)
    , m_bLvlRes(bLvlRes)
    , m_renderThreadId(0)
    , m_pWidget(NULL)
{
    LOADING_TIME_PROFILE_SECTION;

#if defined(LINUX) || defined(APPLE)
    m_HandleSource = 0;
#endif
    m_pITimer = gEnv->pTimer;

    m_bGameFolderWritable = true;
    m_disableRuntimeFileAccess[0] = m_disableRuntimeFileAccess[1] = false;

    // Default game folder
    m_strDataRoot = "@assets@";
    m_strDataRootWithSlash = m_strDataRoot + (char)g_cNativeSlash;

    m_pEngineStartupResourceList = new CResourceList;
    m_pLevelResourceList = new CResourceList;
    m_pNextLevelResourceList = new CNextLevelResourceList;
    m_arrArchives.reserve(64);

    m_bInstalledToHDD = true;

    m_mainThreadId = GetCurrentThreadId();

    gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

#ifdef INCLUDE_LIBTOMCRYPT
    if (pGameStartup)
    {
        uint32 keyLen;
        const uint8* pKeyData = pGameStartup->GetRSAKey(&keyLen);
        if (pKeyData && keyLen > 0)
        {
            ZipEncrypt::Init(pKeyData, keyLen);
        }
    }
#endif
}

bool CCryPak::CheckFileAccessDisabled(const char* name, const char* mode)
{
#if defined(ENABLE_PROFILING_CODE)
    /*
    if (gEnv->IsEditor())
    return true;
    */
    if (gEnv->IsEditing())
    {
        // Access always allowed when in edit mode
        return false;
    }

    int logInvalidFileAccess = m_pPakVars->nLogInvalidFileAccess;
    int msgInvalidFileAccess = m_pPakVars->nMessageInvalidFileAccess;

    if (logInvalidFileAccess | msgInvalidFileAccess)
    {
        if (gEnv->pSystem->IsSerializingFile())
        {
            return true;
        }

        threadID currentThreadId = GetCurrentThreadId(); // potentially expensive call
        int threadIndex = -1;

        if (currentThreadId == m_mainThreadId)
        {
            threadIndex = 0;
        }
        else if (currentThreadId == m_renderThreadId)
        {
            threadIndex = 1;
        }
        if (threadIndex >= 0 && m_disableRuntimeFileAccess[threadIndex])
        {
            if (msgInvalidFileAccess && name)
            {
                fileAccessMessage(threadIndex, name);
            }
            if (logInvalidFileAccess && name)
            {
                CDebugAllowFileAccess afa;
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "File: %s opened when runtime file access is disabled (mode %s)", name, mode);

                //strip off dir to reduce warning size
                const char* nameShort = strrchr(name, '/');

                if (!nameShort)
                {
                    nameShort = strrchr(name, '\\');
                }

                if (nameShort)
                {
                    nameShort = nameShort + 1;
                }
                else
                {
                    //no slashes in name
                    nameShort = name;
                }

                if (logInvalidFileAccess == 2)
                {
                    LogFileAccessCallStack(nameShort, name, mode);
                }

                {
                    char acTmp[2048];
                    sprintf_s(acTmp, 2048, "Invalid File Access: %s '%s'", nameShort, mode);
                    gEnv->pSystem->DisplayErrorMessage(acTmp, 5.f);
                }

                CryPerfHUDWarning(5.f, "File Access: %s '%s'", nameShort, mode);
            }
#if ENABLE_STATOSCOPE
            if (gEnv->pStatoscope && name)
            {
                const char* sThreadName = CryThreadGetName(currentThreadId);
                char sThreadNameBuf[11]; // 0x 12345678 \0 => 2+8+1=11
                if (!sThreadName || !sThreadName[0])
                {
                    azsnprintf(sThreadNameBuf, sizeof(sThreadNameBuf), "%" PRI_THREADID "", currentThreadId);
                    sThreadName = sThreadNameBuf;
                }

                string path = "FileAccess/";
                path += sThreadName;
                gEnv->pStatoscope->AddUserMarker(path, name);
            }
#endif // ENABLE_STATOSCOPE
            return true;
        }
    }
#endif
    return false;
}

void CCryPak::LogFileAccessCallStack(const char* name, const char* nameFull, const char* mode)
{
    //static std::vector<string> s_InvalidFileNames;
    static bool s_Init = false;
    static int s_AccessCount = 0;

    if (!s_Init)
    {
        MakeDir("FileAccess");
        s_Init = true;
    }

    /*
    // don't output the same file twice to reduce the amount if logging
    std::vector<string>::iterator itFindRes = std::find(s_InvalidFileNames.begin(), s_InvalidFileNames.end(), name);
    if (itFindRes != s_InvalidFileNames.end())
    return;
    s_InvalidFileNames.push_back(name);
    */

    string n(name);
    int idx = n.find('.');
    string filename;

    if (idx != -1)
    {
        n.replace(idx, n.length() - idx, 1, '\0'); //truncate extension

        filename.Format("FileAccess/%s_%s_%d.log", name + idx + 1, n.c_str(), ++s_AccessCount);
    }
    else
    {
        filename.Format("FileAccess/%s_%d.log", n.c_str(), ++s_AccessCount);
    }

    char tempPath[MAX_PATH];
    const char* szPath = AdjustFileName(filename.c_str(), tempPath, AZ_ARRAY_SIZE(tempPath), FLAGS_PATH_REAL | FLAGS_FOR_WRITING);

    // Print call stack for each find.
    const char* funcs[32];
    int nCount = 32;

    CryLogAlways("LogFileAccessCallStack() - name=%s; nameFull=%s; mode=%s", name, nameFull, mode);
    CryLogAlways("    ----- CallStack () -----");
    gEnv->pSystem->debug_GetCallStack(funcs, nCount);
    for (int i = 1; i < nCount; i++) // start from 1 to skip this function.
    {
        CryLogAlways("    %02d) %s", i, funcs[i]);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCryPak::SetGameFolderWritable(bool bWritable)
{
    m_bGameFolderWritable = bWritable;
}

//////////////////////////////////////////////////////////////////////////
void CCryPak::AddMod(const char* szMod)
{
    // remember the prefix to use to convert the file names
    CryPathString strPrepend = szMod;
    strPrepend.replace(g_cNativeSlash, g_cNonNativeSlash);

#if !defined(AZ_PLATFORM_APPLE)
    // Don't force paths to lower on Apple platforms that are case-sensitive
    strPrepend.MakeLower();
#endif

    std::vector<string>::iterator strit;
    for (strit = m_arrMods.begin(); strit != m_arrMods.end(); ++strit)
    {
        string& sMOD = *strit;
        if (azstricmp(sMOD.c_str(), strPrepend.c_str()) == 0)
        {
            return; // already added
        }
    }
    m_arrMods.push_back(strPrepend);
}

//////////////////////////////////////////////////////////////////////////
void CCryPak::RemoveMod(const char* szMod)
{
    CryPathString strPrepend = szMod;
    strPrepend.replace(g_cNativeSlash, g_cNonNativeSlash);
#if !defined(AZ_PLATFORM_APPLE)
    strPrepend.MakeLower();
#endif

    std::vector<string>::iterator it;
    for (it = m_arrMods.begin(); it != m_arrMods.end(); ++it)
    {
        string& sMOD = *it;
        if (azstricmp(sMOD.c_str(), strPrepend.c_str()) == 0)
        {
            m_arrMods.erase(it);
            break;
        }
    } //it
}

//////////////////////////////////////////////////////////////////////////
const char* CCryPak::GetMod(int index)
{
    return index >= 0 && index < (int)m_arrMods.size() ? m_arrMods[index].c_str() : NULL;
}

//////////////////////////////////////////////////////////////////////////

bool CCryPak::IsInstalledToHDD(const char* acFilePath) const
{
    if (m_bInstalledToHDD || g_cvars.sys_force_installtohdd_mode)
    {
        return true;
    }

    if (acFilePath)
    {
        const ICryArchive* pArchive = FindArchive(acFilePath);
        if (pArchive && pArchive->GetFlags() & ICryArchive::FLAGS_ON_HDD)
        {
            return true;
        }
    }

    return false;
}

void CCryPak::SetInstalledToHDD(bool bValue)
{
    m_bInstalledToHDD = bValue;

    IStreamEngine* pStreamEngine = gEnv->pSystem->GetStreamEngine();
    if (m_bInstalledToHDD && pStreamEngine)
    {
        // inform streaming system that data is available on HDD
        pStreamEngine->SetStreamDataOnHDD(true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCryPak::ParseAliases(const char* szCommandLine)
{
    const char* szVal = szCommandLine;
    for (;; )
    {
        // this is a list of pairs separated by commas, i.e. Folder1,FolderNew,Textures,TestBuildTextures etc.
        const char* const szSep = strchr(szVal, ',');
        if (!szSep)
        {
            // bogus string passed
            break;
        }

        char szName[256];
        char szAlias[256];

        // get old folder name
        cry_strcpy(szName, szVal, (size_t)(szSep - szVal));

        // find next pair
        const char* szSepNext = strchr(szSep + 1, ',');

        // get alias name
        if (!szSepNext)
        {
            // we may receive whole command line, not just alias pairs. so we must
            // check if there are other commands in the command line and skip them.
            const char* const szTail = strchr(szSep + 1, ' ');
            if (szTail)
            {
                cry_strcpy(szAlias, szSep + 1, (size_t)(szTail - (szSep + 1)));
            }
            else
            {
                cry_strcpy(szAlias, szSep + 1);
            }
        }
        else
        {
            cry_strcpy(szAlias, szSep + 1, (size_t)(szSepNext - (szSep + 1)));
        }

        // inform the pak system
        SetAlias(szName, szAlias, true);

        CryLogAlways("PAK ALIAS:%s,%s\n", szName, szAlias);

        if (!szSepNext)
        {
            // no more aliases
            break;
        }

        // move over to the next pair
        szVal = szSepNext + 1;
    }
}

//////////////////////////////////////////////////////////////////////////
//! if bReturnSame==true, it will return the input name if an alias doesn't exist. Otherwise returns NULL
const char* CCryPak::GetAlias(const char* szName, bool bReturnSame)
{
    const char* dest = AZ::IO::FileIOBase::GetDirectInstance()->GetAlias(szName);
    return (bReturnSame && !dest) ? szName : dest;
}

const char* CCryPak::GetDirectoryDelimiter() const
{
    return CRY_NATIVE_PATH_SEPSTR;
}

//////////////////////////////////////////////////////////////////////////
void CCryPak::SetLocalizationFolder(char const* const sLocalizationFolder)
{
    // Get the localization folder
    m_sLocalizationFolder = sLocalizationFolder;
    m_sLocalizationFolder += CRY_NATIVE_PATH_SEPSTR;
}

//////////////////////////////////////////////////////////////////////////
void CCryPak::SetAlias(const char* szName, const char* szAlias, bool bAdd)
{
    if (bAdd)
    {
        AZ::IO::FileIOBase::GetDirectInstance()->SetAlias(szName, szAlias);
    }
    else
    {
        AZ::IO::FileIOBase::GetDirectInstance()->ClearAlias(szName);
    }
}

//////////////////////////////////////////////////////////////////////////
CCryPak::~CCryPak()
{
    gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

    m_arrZips.clear();

    unsigned numFilesForcedToClose = 0;
    // scan through all open files and close them
    {
        AUTO_MODIFYLOCK(m_csOpenFiles);
        for (ZipPseudoFileArray::iterator itFile = m_arrOpenFiles.begin(); itFile != m_arrOpenFiles.end(); ++itFile)
        {
            if (itFile->GetFile())
            {
                itFile->Destruct();
                ++numFilesForcedToClose;
            }
        }
    }

    if (numFilesForcedToClose)
    {
        m_pLog->LogWarning("%u files were forced to close", numFilesForcedToClose);
    }

    {
        AUTO_MODIFYLOCK(m_csCachedFiles);
        size_t numDatasForcedToDestruct = m_setCachedFiles.size();
        for (size_t i = 0; i < numDatasForcedToDestruct; ++i)
        {
            if (m_setCachedFiles.empty())
            {
                assert(0);
            }
            else
            {
                delete *m_setCachedFiles.begin();
            }
        }
        if (numDatasForcedToDestruct)
        {
            m_pLog->LogWarning("%u cached file data blocks were forced to destruct; they still have references on them, crash possible", (unsigned int)numDatasForcedToDestruct);
        }
    }

    if (!m_arrArchives.empty())
    {
        m_pLog->LogError("There are %d external references to archive objects: they have dangling pointers and will either lead to memory leaks or crashes", (int)m_arrArchives.size());
    }

    if (!m_mapMissingFiles.empty())
    {
        AZ::IO::HandleType fileHandle;
        if (AZ::IO::FileIOBase::GetDirectInstance()->Open("Missing Files Report.txt", AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText, fileHandle))
        {
            AUTO_LOCK_CS(m_csMissingFiles);
            for (MissingFileMap::iterator it = m_mapMissingFiles.begin(); it != m_mapMissingFiles.end(); ++it)
            {
                AZ::IO::Print(fileHandle, "%d\t%s\n", it->second, it->first.c_str());
            }
            AZ::IO::FileIOBase::GetDirectInstance()->Close(fileHandle);
        }
    }

    SAFE_DELETE(m_pWidget);

    CRY_ASSERT_MESSAGE(m_cachedFileRawDataSet.empty(), "All PakFile cached raw data instances not closed");
}


// makes the path lower-case and removes the duplicate and non native slashes
// may make some other fool-proof stuff
// may NOT write beyond the string buffer (may not make it longer)
// returns: the pointer to the ending terminator \0
char* CCryPak::BeautifyPath(char* dst, bool bMakeLowercase)
{
    // make the path lower-letters and with native slashes
    char* p, * q;
    // there's a special case: two slashes at the beginning mean UNC filepath
    p = q = dst;
    if (*p == g_cNonNativeSlash || *p == g_cNativeSlash)
    {
        // start normalization/beautifications from the second symbol; if it's a slash, we'll add it, too
        ++p;
        ++q;
    }

    bool inToken = false;

    while (*p)
    {
        if (*p == g_cNonNativeSlash || *p == g_cNativeSlash)
        {
            *q = g_cNativeSlash;
            ++p;
            ++q;
            while (*p == g_cNonNativeSlash || *p == g_cNativeSlash)
            {
                ++p; // skip the extra slashes
            }
        }
        else
        {
            // Avoid making tokens like @user@ lowercase by toggling inToken.
            if (*p == '@')
            {
                inToken = !inToken;
            }

            if (!inToken && bMakeLowercase)
            {
                *q = tolower(*p);
            }
            else
            {
                *q = *p;
            }
            ++q;
            ++p;
        }
    }
    *q = '\0';
    return q;
}

// remove all '%s/..' or '.' parts from the path (needs beautified path - only single native slashes)
// e.g. Game/Scripts/AI/../Entities/foo -> Game/Scripts/Entities/foo
// e.g. Game/Scripts/./Entities/foo -> Game/Scripts/Entities/foo
void CCryPak::RemoveRelativeParts(char* dst)
{
    char* q = dst;
    char* p = NULL;

    PREFAST_ASSUME(q);

    // replace all '/./' with '/'
    const char slashDotSlashString[4] = { g_cNativeSlash, '.', g_cNativeSlash, 0 };

    while (p = strstr(q, slashDotSlashString))
    {
        //Move the string and the null terminator
        memmove(p, p + 2, strlen(p + 2) + 1);
    }

    // replace all '/%s/../' with '/'
    const char slashDotDotSlashString[5] = { g_cNativeSlash, '.', '.', g_cNativeSlash, 0 };

    while (p = strstr(q, slashDotDotSlashString))
    {
        if (p != q) // only remove if not in front of a path
        {
            int i = 4;
            bool bSpecial = true;
            while (*(--p) != g_cNativeSlash && p != q)
            {
                if (*p != '.')
                {
                    bSpecial = false;
                }
                i++;
            }
            if (!bSpecial)
            {
                memmove(p, p + i, strlen(p + i) + 1);
                continue;
            }
        }
        q += 3;
    }
}

namespace filehelpers
{
    //////////////////////////////////////////////////////////////////////////
    inline bool CheckPrefix(const char* str, const char* prefix)
    {
        //this should rather be a case insensitive check here, so strnicmp is used instead of strncmp
        return (_strnicmp(str, prefix, strlen(prefix)) == 0);
    }

    //////////////////////////////////////////////////////////////////////////
    inline ICryPak::SignedFileSize GetFileSizeOnDisk(const char* filename)
    {
        return gEnv->pCryPak->GetFileSizeOnDisk(filename);
    }

    //////////////////////////////////////////////////////////////////////////
    inline bool CheckFileExistOnDisk(const char* filename)
    {
        return AZ::IO::FileIOBase::GetDirectInstance()->Exists(filename);
    }
};

//////////////////////////////////////////////////////////////////////////

const char* CCryPak::AdjustFileNameImpl(const char* src, char* dst, size_t dstSize, unsigned nFlags, bool bSkipMods)
{
    if (gEnv->pConsole)
    {
        ICVar* pCaseSensitivityCVar = gEnv->pConsole->GetCVar("sys_FilesystemCaseSensitivity");
        if (pCaseSensitivityCVar && pCaseSensitivityCVar->GetIVal() > 0)
        {
            nFlags |= FLAGS_NO_LOWCASE;
        }
    }



    if (!bSkipMods &&
        ((nFlags & FLAGS_PATH_REAL) == 0) &&
        ((nFlags & FLAGS_FOR_WRITING) == 0) &&
        (!m_arrMods.empty()) &&
        (*src != '@') &&                                // If path starts from % it is a special alias.
        ((m_pPakVars->nPriority != ePakPriorityPakOnly) || (nFlags & FLAGS_NEVER_IN_PAK)))    // When priority is Pak only, we only check Mods directories if we're looking for a file that can't be in a pak
    {
        char adjustedSource[g_nMaxPath];
        const char* adjustedSourceResult = AdjustFileNameInternal(src, adjustedSource, g_nMaxPath, nFlags | FLAGS_PATH_REAL);
        // Scan mod folders
        std::vector<string>::reverse_iterator it;
        for (it = m_arrMods.rbegin(); it != m_arrMods.rend(); ++it)
        {
            CryPathString modPath = (*it).c_str();
            modPath.append(1, '/');

            // If the source already includes the modpath, don't bother adding it again.
            const char* szFinalPath = AdjustFileNameInternal(modPath, dst, dstSize, nFlags | FLAGS_PATH_REAL);

            if (strncmp(adjustedSourceResult, szFinalPath, strlen(szFinalPath)) == 0)
            {
                szFinalPath = src;
            }
            else
            {
                modPath += src;
                szFinalPath = AdjustFileNameInternal(modPath, dst, dstSize, nFlags | FLAGS_PATH_REAL);
            }

            if (m_pPakVars->nPriority == ePakPriorityFileFirstModsOnly && !IsModPath(szFinalPath))
            {
                continue;
            }

            if (nFlags & FLAGS_NEVER_IN_PAK)
            {
                // only check the filesystem
                if (filehelpers::CheckFileExistOnDisk(szFinalPath))
                {
                    azstrcpy(dst, g_nMaxPath, szFinalPath);
                    return szFinalPath;
                }
            }
            else
            {
                //look for regular files depending on the pak priority
                switch (m_pPakVars->nPriority)
                {
                case ePakPriorityFileFirstModsOnly:
                case ePakPriorityFileFirst:
                {
                    if (filehelpers::CheckFileExistOnDisk(szFinalPath))
                    {
                        azstrcpy(dst, g_nMaxPath, szFinalPath);
                        return szFinalPath;
                    }
                    if (FindPakFileEntry(szFinalPath))
                    {
                        azstrcpy(dst, g_nMaxPath, szFinalPath);
                        return szFinalPath;
                    }
                }
                break;
                case ePakPriorityPakFirst:
                {
                    if (FindPakFileEntry(szFinalPath))
                    {
                        azstrcpy(dst, g_nMaxPath, szFinalPath);
                        return szFinalPath;
                    }
                    if (filehelpers::CheckFileExistOnDisk(szFinalPath))
                    {
                        azstrcpy(dst, g_nMaxPath, szFinalPath);
                        return szFinalPath;
                    }
                }
                break;
                }
            }
        } //it
    }

    return AdjustFileNameInternal(src, dst, dstSize, nFlags);
}

#if defined(LINUX)
extern char* fopenwrapper_basedir;
#endif

void CCryPak::GetCachedPakCDROffsetSize(const char* szName, uint32& offset, uint32& size)
{
    offset = 0;         // play it safe
    size = 0;

    char szFullPathBuf[g_nMaxPath];

    const char* szFullPath = AdjustFileName(szName, szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), FOPEN_HINT_QUIET | FLAGS_PATH_REAL);

    CryLog("CRC:Looking for a zip called %s, fullpath %s", szName, szFullPath);

    // scan through registered pak files and try to find this file
    for (ZipArray::reverse_iterator itZip = m_arrZips.rbegin(); itZip != m_arrZips.rend(); ++itZip)
    {
        if (itZip->pArchive->GetFlags() & ICryArchive::FLAGS_DISABLE_PAK)
        {
            continue;
        }

        const char* pathToZip = itZip->pZip->GetFilePath();
        CryLog("CRC: comparing zip path %s", pathToZip);
        if (azstricmp(pathToZip, szFullPath) == 0)
        {
            itZip->pZip->GetCDROffsetSize(offset, size);
            CryLog("CRC: Match. offset %u, size %u", offset, size);
            return;                 // May as well early out
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// adjust a file name to something the underlying fileIO system will work
// note that this does not make a full path, just something that fileIO can eat
// such as @assets@/whatever.dds
const char* CCryPak::AdjustFileNameInternal(const char* src, char* dst, size_t dstSize, unsigned nFlags)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_SYSTEM);
    // in many cases, the path will not be long, so there's no need to allocate so much..
    // I'd use _alloca, but I don't like non-portable solutions. besides, it tends to confuse new developers. So I'm just using a big enough array
    char szNewSrc[g_nMaxPath];
    cry_strcpy(szNewSrc, src);

    BeautifyPath(szNewSrc, (nFlags & FLAGS_NO_LOWCASE) == 0);
    RemoveRelativeParts(szNewSrc);

    // do not muddle with the path if an alias is present, FileIO will handle it
    bool bAliasWasUsed = szNewSrc[0] == '@';
    if (nFlags & FLAGS_NO_FULL_PATH || bAliasWasUsed)
    {
        azstrcpy(dst, dstSize, szNewSrc);
        return dst;
    }

    bool isAbsolutePath = IsAbsPath(szNewSrc);

    // remove the 'current folder' specifier (./), it is illegal
    if (filehelpers::CheckPrefix(szNewSrc, "./") || filehelpers::CheckPrefix(szNewSrc, ".\\"))
    {
        const int nLen = min(sizeof(szNewSrc), strlen(szNewSrc) - 2);
        memmove(szNewSrc, szNewSrc + 2, nLen);
        szNewSrc[nLen] = 0;
    }

    if (!isAbsolutePath)
    {
        // This is a relative filename, and its missing any aliases.
        // 4) textures/something.dds
        // if this is the case, it must be treated as an ASSET
        // since if it were not an ASSET, it would have been specified, like @log@ something or @root@ something (for cfg files)

        // if no alias was specified, add data folder prefix:
        memmove(szNewSrc + m_strDataRootWithSlash.length(), szNewSrc, strlen(szNewSrc) + 1);
        memcpy(szNewSrc, m_strDataRootWithSlash.c_str(), m_strDataRootWithSlash.length());
    }

    azstrcpy(dst, dstSize, szNewSrc);
    const int dstLen = strlen(dst);

    char* pEnd = dst + dstLen;

#if defined(LINUX) || defined(AZ_PLATFORM_APPLE)
    if ((nFlags & FLAGS_ADD_TRAILING_SLASH) && pEnd > dst && (pEnd[-1] != g_cNativeSlash && pEnd[-1] != g_cNonNativeSlash))
#else
    // p now points to the end of string
    if ((nFlags & FLAGS_ADD_TRAILING_SLASH) && pEnd > dst && pEnd[-1] != g_cNativeSlash)
#endif
    {
        *pEnd = g_cNativeSlash;
        *++pEnd = '\0';
    }

    return dst;
}

//////////////////////////////////////////////////////////////////////////
bool CCryPak::CopyFileOnDisk(const char* source, const char* dest, bool bFailIfExist)
{
#ifdef WIN32
    return ::CopyFile((LPCSTR)source, (LPCSTR)dest, bFailIfExist) == TRUE;
#else
    if (bFailIfExist && IsFileExist(dest, eFileLocation_OnDisk))
    {
        return false;
    }

    AZ::IO::HandleType src = FOpen(source, "rb");
    AZ::IO::HandleType dst = FOpen(dest, "wb");

    if (!src || !dst)
    {
        return false;
    }

#define CHUNK_SIZE 64 * 1024

    char* buf = new char[CHUNK_SIZE];
    size_t readBytes = 0;
    size_t writtenBytes = 0;

    while (!FEof(src))
    {
        readBytes = FReadRaw(buf, sizeof(char), CHUNK_SIZE, src);
        writtenBytes = FWrite(buf, sizeof(char), readBytes, dst);

        if (readBytes != writtenBytes)
        {
            FClose(src);
            FClose(dst);

            delete[] buf;
            return false;
        }
    }

    delete[] buf;

#undef CHUNK_SIZE

    FClose(src);
    FClose(dst);

    return true;
#endif
}

//////////////////////////////////////////////////////////////////////////
bool CCryPak::IsFileExist(const char* sFilename, EFileSearchLocation fileLocation)
{
    // lock-less check
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_SYSTEM);

    char szFullPathBuf[g_nMaxPath];

    const int nVarPakPriority = m_pPakVars->nPriority;

    const char* szFullPath = AdjustFileName(sFilename, szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), FOPEN_HINT_QUIET);
    if (!szFullPath)
    {
        return false;
    }

    if (fileLocation == eFileLocation_InPak)
    {
        if (FindPakFileEntry(szFullPath))   // try to find the pseudo-file in one of the zips
        {
            return true;
        }
        return false;
    }

    if (nVarPakPriority == ePakPriorityFileFirst ||
        (nVarPakPriority == ePakPriorityFileFirstModsOnly && IsModPath(szFullPath))) // if the file system files have priority now..
    {
        if (filehelpers::CheckFileExistOnDisk(szFullPath))
        {
            return true;
        }
        else if (fileLocation == eFileLocation_OnDisk)
        {
            return false;
        }
    }

    if (FindPakFileEntry(szFullPath))   // try to find the pseudo-file in one of the zips
    {
        return true;
    }

    if (nVarPakPriority == ePakPriorityPakFirst || eFileLocation_OnDisk == fileLocation) // if the pak files had more priority, we didn't attempt fopen before- try it now
    {
        if (filehelpers::CheckFileExistOnDisk(szFullPath))
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CCryPak::IsFolder(const char* sPath)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_SYSTEM);

    char szFullPathBuf[g_nMaxPath];
    const char* szFullPath = AdjustFileName(sPath, szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), FOPEN_HINT_QUIET);
    if (!szFullPath)
    {
        return false;
    }

    char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
    AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(szFullPathBuf, resolvedPath, AZ_MAX_PATH_LEN);

    DWORD attrs = CryGetFileAttributes(resolvedPath);

    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        // non-existent things are not directories.
        return false;
    }

    if ((attrs & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
        return true;
    }

    return false;
}


ICryPak::SignedFileSize CCryPak::GetFileSizeOnDisk(const char* filename)
{
    AZ::u64 fileSize = 0;
    if (AZ::IO::FileIOBase::GetDirectInstance()->Size(filename, fileSize))
    {
        return static_cast<ICryPak::SignedFileSize>(fileSize);
    }
    return ICryPak::FILE_NOT_PRESENT;
}

//////////////////////////////////////////////////////////////////////////
bool CCryPak::IsFileCompressed(const char* filename)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_SYSTEM);

    char fullPathBuf[g_nMaxPath];
    const char* pFullPath = AdjustFileName(filename, fullPathBuf, AZ_ARRAY_SIZE(fullPathBuf), FOPEN_HINT_QUIET);
    if (!pFullPath)
    {
        return false;
    }

    if (m_pPakVars->nPriority == ePakPriorityFileFirst ||
        (m_pPakVars->nPriority == ePakPriorityFileFirstModsOnly && IsModPath(pFullPath)))
    {
        if (filehelpers::CheckFileExistOnDisk(pFullPath))
        {
            return false;
        }
    }

    ZipDir::FileEntry* pFileEntry = FindPakFileEntry(pFullPath);
    if (pFileEntry)
    {
        return pFileEntry->IsCompressed();
    }

    return false;
}


//////////////////////////////////////////////////////////////////////////
AZ::IO::HandleType CCryPak::FOpen(const char* pName, const char* szMode, char* szFileGamePath, int nLen)
{
    LOADING_TIME_PROFILE_SECTION;

    SAutoCollectFileAcessTime accessTime(this);

    PROFILE_DISK_OPEN;

    char szFullPathBuf[g_nMaxPath];
    const char* szFullPath = AdjustFileName(pName, szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), 0);
    if (nLen > g_nMaxPath)
    {
        nLen = g_nMaxPath;
    }
    azstrncpy(szFileGamePath, nLen, szFullPath, nLen);
    AZ::IO::HandleType fileHandle;
    AZ::IO::FileIOBase::GetDirectInstance()->Open(szFullPath, AZ::IO::GetOpenModeFromStringMode(szMode), fileHandle);

    CheckFileAccessDisabled(pName, szMode);

    if (fileHandle != AZ::IO::InvalidHandle)
    {
        RecordFile(fileHandle, pName);
    }
    else
    {
        OnMissingFile(pName);
    }
    return fileHandle;
}

//////////////////////////////////////////////////////////////////////////
AZ::IO::HandleType CCryPak::FOpen(const char* pName, const char* szMode, unsigned nInputFlags)
{
    LOADING_TIME_PROFILE_SECTION;

    const size_t pathLen = strlen(pName);
    if (pathLen == 0 || pathLen >= g_nMaxPath)
    {
        return AZ::IO::InvalidHandle;
    }

    PROFILE_DISK_OPEN;
    SAutoCollectFileAcessTime accessTime(this);

    AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
    char szFullPathBuf[g_nMaxPath];

    bool bFileCanBeOnDisk = 0 != (nInputFlags & FOPEN_ONDISK);

    // get the priority into local variable to avoid it changing in the course of
    // this function execution (?)
    int nVarPakPriority = m_pPakVars->nPriority;

    // Remove unknown to CRT 'x' parameter.
    char smode[16];
    cry_strcpy(smode, szMode);
    for (char* s = smode; *s; s++)
    {
        if (*s == 'x' || *s == 'X')
        {
            *s = ' ';
        }
        ;
    }

#if defined(LINUX) || defined(APPLE)
    unsigned nOSFlags = _O_RDONLY;
#else
    unsigned nOSFlags = _O_BINARY | _O_RDONLY;
#endif

    //Timur, Try direct zip operation always.
    nInputFlags |= FOPEN_HINT_DIRECT_OPERATION;

    int nAdjustFlags = 0;

    // check the szMode
    for (const char* pModeChar = szMode; *pModeChar; ++pModeChar)
    {
        switch (*pModeChar)
        {
        case 'r':
            nOSFlags &= ~(_O_WRONLY | _O_RDWR);
            // read mode is the only mode we can open the file in
            break;
        case 'w':
            nOSFlags |= _O_WRONLY;
            nAdjustFlags |= FLAGS_FOR_WRITING;
            break;
        case 'a':
            nOSFlags |= _O_RDWR;
            nAdjustFlags |= FLAGS_FOR_WRITING;
            break;
        case '+':
            nOSFlags |= _O_RDWR;
            nAdjustFlags |= FLAGS_FOR_WRITING;
            break;

        case 'b':
            nOSFlags &= ~_O_TEXT;
            nOSFlags |= _O_BINARY;
            break;
        case 't':
            nOSFlags &= ~_O_BINARY;
            nOSFlags |= _O_TEXT;
            break;

        case 'c':
        case 'C':
            nOSFlags |= (uint32)CZipPseudoFile::_O_COMMIT_FLUSH_MODE;
            break;
        case 'n':
        case 'N':
            nOSFlags &= ~CZipPseudoFile::_O_COMMIT_FLUSH_MODE;
            break;

        case 'S':
            nOSFlags |= _O_SEQUENTIAL;
            break;

        case 'R':
            nOSFlags |= _O_RANDOM;
            break;

        case 'T':
            nOSFlags |= _O_SHORT_LIVED;
            break;

        case 'D':
            nOSFlags |= _O_TEMPORARY;
            break;

        case 'x':
        case 'X':
            nInputFlags |= FOPEN_HINT_DIRECT_OPERATION;
            break;
        }
    }

    if (nInputFlags & FLAGS_PATH_REAL)
    {
        nAdjustFlags |= FLAGS_PATH_REAL;
    }

    const char* szFullPath = AdjustFileName(pName, szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), nAdjustFlags);

    if (nOSFlags & (_O_WRONLY | _O_RDWR))
    {
        CheckFileAccessDisabled(szFullPath, szMode);

        // we need to open the file for writing, but we failed to do so.
        // the only reason that can be is that there are no directories for that file.
        // now create those dirs
        if (!MakeDir(PathUtil::GetParentDirectory(string(szFullPath)).c_str()))
        {
            return AZ::IO::InvalidHandle;
        }

        if (AZ::IO::FileIOBase::GetDirectInstance()->Open(szFullPath, AZ::IO::GetOpenModeFromStringMode(smode), fileHandle))
        {
#if !defined(_RELEASE)
            if (g_cvars.pakVars.nLogAllFileAccess)
            {
                CryLog("<PAK LOG FILE ACCESS> CCryPak::FOpen() has directly opened requested file %s for writing", szFullPath);
            }
#endif
#if AZ_LOADSCREENCOMPONENT_ENABLED
            if (GetISystem() && GetISystem()->GetGlobalEnvironment() && GetISystem()->GetGlobalEnvironment()->mMainThreadId == CryGetCurrentThreadId())
            {
                EBUS_EVENT(LoadScreenBus, UpdateAndRender);
            }
#endif // defined(AZ_LOADSCREENCOMPONENT_ENABLED)

            return fileHandle;
        }

        return AZ::IO::InvalidHandle;
    }

    if (nVarPakPriority == ePakPriorityFileFirst ||
        (nVarPakPriority == ePakPriorityFileFirstModsOnly && IsModPath(szFullPath))) // if the file system files have priority now..
    {
        if (AZ::IO::FileIOBase::GetDirectInstance()->Open(szFullPath, AZ::IO::GetOpenModeFromStringMode(smode), fileHandle))
        {
            CheckFileAccessDisabled(szFullPath, szMode);

#if !defined(_RELEASE)
            if (g_cvars.pakVars.nLogAllFileAccess)
            {
                CryLog("<PAK LOG FILE ACCESS> CCryPak::FOpen() has directly opened requested file %s with FileFirst priority", szFullPath);
            }
#endif
            RecordFile(fileHandle, pName);

#if AZ_LOADSCREENCOMPONENT_ENABLED
            if (GetISystem() && GetISystem()->GetGlobalEnvironment() && GetISystem()->GetGlobalEnvironment()->mMainThreadId == CryGetCurrentThreadId())
            {
                EBUS_EVENT(LoadScreenBus, UpdateAndRender);
            }
#endif // defined(AZ_LOADSCREENCOMPONENT_ENABLED)

            return fileHandle;
        }
    }

    unsigned int archiveFlags;
    CCachedFileData_AutoPtr pFileData = GetFileData(szFullPath, archiveFlags);
    if (pFileData)
    {
#if !defined(_RELEASE)
        if (g_cvars.pakVars.nLogAllFileAccess)
        {
            bool logged = false;
            ZipDir::Cache* pZip = pFileData->GetZip();
            if (pZip)
            {
                const char* pZipFilePath = pZip->GetFilePath();
                if (pZipFilePath && pZipFilePath[0])
                {
                    CryLog("<PAK LOG FILE ACCESS> CCryPak::FOpen() has opened requested file %s from %s pak %s, disk offset %i", szFullPath, pZip->IsInMemory() ? "memory" : "disk", pZipFilePath, pFileData->GetFileEntry()->nFileDataOffset);
                    logged = true;
                }
            }

            if (!logged)
            {
                CryLog("<PAK LOG FILE ACCESS> CCryPak::FOpen() has opened requested file %s from a pak file who's path isn't known", szFullPath);
            }
        }
#endif
    }
    else
    {
        if (nVarPakPriority != ePakPriorityPakOnly || bFileCanBeOnDisk) // if the pak files had more priority, we didn't attempt fopen before- try it now
        {
            if (AZ::IO::FileIOBase::GetDirectInstance()->Open(szFullPath, AZ::IO::GetOpenModeFromStringMode(smode), fileHandle))
            {
                CheckFileAccessDisabled(szFullPath, szMode);

#if !defined(_RELEASE)
                if (g_cvars.pakVars.nLogAllFileAccess)
                {
                    CryLog("<PAK LOG FILE ACCESS> CCryPak::FOpen() has directly opened requested file %s after failing to open from paks", szFullPath);
                }
#endif

#if !defined (_RELEASE)
                RecordFile(fileHandle, pName);
#endif
                return fileHandle;
            }
        }
#if !defined (_RELEASE)
        if (!(nInputFlags & FOPEN_HINT_QUIET))
        {
            OnMissingFile(pName);
        }
#endif
        return AZ::IO::InvalidHandle; // we can't find such file in the pack files
    }

    CheckFileAccessDisabled(szFullPath, szMode);

    // try to open the pseudofile from one of the zips, make sure there is no user alias
    AUTO_MODIFYLOCK(m_csOpenFiles);

    size_t nFile;
    // find the empty slot and open the file there; return the handle
    {
        for (nFile = 0; nFile < m_arrOpenFiles.size() && m_arrOpenFiles[nFile].GetFile(); ++nFile)
        {
            continue;
        }
        if (nFile == m_arrOpenFiles.size())
        {
            ScopedSwitchToGlobalHeap globalHeap;
            m_arrOpenFiles.resize(nFile + 1);
        }
        if (pFileData != NULL && (nInputFlags & FOPEN_HINT_DIRECT_OPERATION) && !pFileData->m_pZip->IsInMemory())
        {
            nOSFlags |= CZipPseudoFile::_O_DIRECT_OPERATION;
        }
        CZipPseudoFile& rZipFile = m_arrOpenFiles[nFile];
        nOSFlags |= (archiveFlags & FLAGS_REDIRECT_TO_DISC);
        rZipFile.Construct(pFileData, nOSFlags);
    }

    AZ::IO::HandleType ret = (AZ::IO::HandleType)(nFile + g_nPseudoFileIdxOffset);

#if !defined (_RELEASE)
    RecordFile(ret, pName);
#endif

    return ret; // the handle to the file
}

//////////////////////////////////////////////////////////////////////////
// given the file name, searches for the file entry among the zip files.
// if there's such file in one of the zips, then creates (or used cached)
// CCachedFileData instance and returns it.
// The file data object may be created in this function,
// and it's important that the autoptr is returned: another thread may release the existing
// cached data before the function returns
// the path must be absolute normalized lower-case with forward-slashes
CCachedFileDataPtr CCryPak::GetFileData(const char* szName, unsigned int& nArchiveFlags)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_SYSTEM);

    CCachedFileData* pResult = 0;

    ZipDir::CachePtr pZip = 0;
    ZipDir::FileEntry* pFileEntry = FindPakFileEntry(szName, nArchiveFlags, &pZip);
    if (pFileEntry)
    {
        pResult = new CCachedFileData(this, pZip, nArchiveFlags, pFileEntry, szName);
    }
    return pResult;
}

//////////////////////////////////////////////////////////////////////////
CCachedFileDataPtr CCryPak::GetOpenedFileDataInZip(AZ::IO::HandleType fileHandle)
{
    AUTO_READLOCK(m_csOpenFiles);

    INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
    if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
    {
        return m_arrOpenFiles[nPseudoFile].GetFile();
    }
    return 0;
}

bool CCryPak::WillOpenFromPak(const char* szPath)
{
    int nVarPakPriority = m_pPakVars->nPriority;

    if (nVarPakPriority == ePakPriorityFileFirst ||
        (nVarPakPriority == ePakPriorityFileFirstModsOnly && IsModPath(szPath))) // if the file system files have priority now..
    {
        if (filehelpers::CheckFileExistOnDisk(szPath))
        {
            return false;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
// tests if the given file path refers to an existing file inside registered (opened) packs
// the path must be absolute normalized lower-case with forward-slashes
ZipDir::FileEntry* CCryPak::FindPakFileEntry(const char* szPath, unsigned int& nArchiveFlags, ZipDir::CachePtr* pZip, bool bSkipInMemoryPaks)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_SYSTEM);
    LOADING_TIME_PROFILE_SECTION;


    string unaliasedPath = ConvertAbsolutePathToAliasedPath(szPath);

    unsigned nNameLen = static_cast<unsigned>(unaliasedPath.size());
    AUTO_READLOCK(m_csZips);
    // scan through registered pak files and try to find this file
    for (ZipArray::reverse_iterator itZip = m_arrZips.rbegin(); itZip != m_arrZips.rend(); ++itZip)
    {
        if (bSkipInMemoryPaks && itZip->pArchive->GetFlags() & ICryArchive::FLAGS_IN_MEMORY_MASK)
        {
            continue;
        }

        if (itZip->pArchive->GetFlags() & ICryArchive::FLAGS_DISABLE_PAK)
        {
            continue;
        }

        size_t nBindRootLen = itZip->strBindRoot.length();

        size_t nRootCompLength = itZip->strBindRoot.length();
        const char* const cpRoot = itZip->strBindRoot.c_str();

        if (nNameLen > nRootCompLength  && !memcmp(cpRoot, unaliasedPath.c_str(), nRootCompLength))
        {
            nBindRootLen = nRootCompLength;

            ZipDir::FileEntry* pFileEntry = itZip->pZip->FindFile(unaliasedPath.c_str() + nBindRootLen);
            if (pFileEntry)
            {
                if (pZip)
                {
                    *pZip = itZip->pZip;
                }

                //if (pZip)
                //CryLog( "Zip [%s] %s",itZip->pZip->GetFilePath(),unaliasedPath.c_str() );
                nArchiveFlags = itZip->pArchive->GetFlags();
                return pFileEntry;
            }
        }
    }
    nArchiveFlags = 0;
    return NULL;
}

long CCryPak::FTell(AZ::IO::HandleType fileHandle)
{
    AUTO_READLOCK(m_csOpenFiles);
    INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
    if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
    {
        return m_arrOpenFiles[nPseudoFile].FTell();
    }
    else
    {
        AZ::u64 returnValue = 0;
        AZ::IO::FileIOBase::GetDirectInstance()->Tell(fileHandle, returnValue);
        return static_cast<long>(returnValue);
    }
}

// returns the path to the archive in which the file was opened
const char* CCryPak::GetFileArchivePath(AZ::IO::HandleType fileHandle)
{
    AUTO_READLOCK(m_csOpenFiles);
    INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
    if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
    {
        return m_arrOpenFiles[nPseudoFile].GetArchivePath();
    }
    else
    {
        return nullptr;
    }
}

#ifndef Int32x32To64
#define Int32x32To64(a, b) ((uint64)((uint64)(a)) * (uint64)((uint64)(b)))
#endif

// returns the file modification time
uint64 CCryPak::GetModificationTime(AZ::IO::HandleType fileHandle)
{
    INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
    {
        AUTO_READLOCK(m_csOpenFiles);
        if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
        {
            return m_arrOpenFiles[nPseudoFile].GetModificationTime();
        }
    }

    return AZ::IO::FileIOBase::GetDirectInstance()->ModificationTime(fileHandle);
}

size_t CCryPak::FGetSize(AZ::IO::HandleType fileHandle)
{
    INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
    {
        AUTO_READLOCK(m_csOpenFiles);
        if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
        {
            return m_arrOpenFiles[nPseudoFile].GetFileSize();
        }
    }

    AZ::u64 fileSize = 0;
    AZ::IO::FileIOBase::GetDirectInstance()->Size(fileHandle, fileSize);
    return fileSize;
}

//////////////////////////////////////////////////////////////////////////
size_t CCryPak::FGetSize(const char* sFilename, bool bAllowUseFileSystem)
{
    // lock-less GetSize
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_SYSTEM);

    char szFullPathBuf[g_nMaxPath];
    const char* szFullPath = AdjustFileName(sFilename, szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), FOPEN_HINT_QUIET);
    if (!szFullPath)
    {
        return 0;
    }

    if (m_pPakVars->nPriority == ePakPriorityFileFirst ||
        (m_pPakVars->nPriority == ePakPriorityFileFirstModsOnly && IsModPath(szFullPath))) // if the file system files have priority now..
    {
        ICryPak::SignedFileSize nFileSize = filehelpers::GetFileSizeOnDisk(szFullPath);
        if (nFileSize != ICryPak::FILE_NOT_PRESENT)
        {
            return (size_t)nFileSize;
        }
    }

    ZipDir::FileEntry* pFileEntry = FindPakFileEntry(szFullPath);
    if (pFileEntry) // try to find the pseudo-file in one of the zips
    {
        return pFileEntry->desc.lSizeUncompressed;
    }

    if (bAllowUseFileSystem || m_pPakVars->nPriority == ePakPriorityPakFirst) // if the pak files had more priority, we didn't attempt fopen before- try it now
    {
        ICryPak::SignedFileSize nFileSize = filehelpers::GetFileSizeOnDisk(szFullPath);
        if (nFileSize != ICryPak::FILE_NOT_PRESENT)
        {
            return (size_t)nFileSize;
        }
    }

    return 0;
}

int CCryPak::FFlush(AZ::IO::HandleType fileHandle)
{
    SAutoCollectFileAcessTime accessTime(this);
    INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
    {
        AUTO_READLOCK(m_csOpenFiles);
        if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
        {
            return 0;
        }
    }

    if (AZ::IO::FileIOBase::GetDirectInstance()->Flush(fileHandle))
    {
        return 0;
    }
    return 1;
}

size_t CCryPak::FSeek(AZ::IO::HandleType fileHandle, long seek, int mode)
{
    SAutoCollectFileAcessTime accessTime(this);

    {
        AUTO_READLOCK(m_csOpenFiles);
        INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
        if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
        {
            return m_arrOpenFiles[nPseudoFile].FSeek(seek, mode);
        }
    }

    if (AZ::IO::FileIOBase::GetDirectInstance()->Seek(fileHandle, static_cast<uint64_t>(seek), AZ::IO::GetSeekTypeFromFSeekMode(mode)))
    {
        return 0;
    }

    return 1;
}

size_t CCryPak::FWrite(const void* data, size_t length, size_t elems, AZ::IO::HandleType fileHandle)
{
    SAutoCollectFileAcessTime accessTime(this);

    INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
    {
        AUTO_READLOCK(m_csOpenFiles);
        if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
        {
            return 0;
        }
    }

    CRY_ASSERT(fileHandle);
    PROFILE_DISK_WRITE;
    if (AZ::IO::FileIOBase::GetDirectInstance()->Write(fileHandle, data, length * elems))
    {
        return elems;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
size_t CCryPak::FReadRaw(void* pData, size_t nSize, size_t nCount, AZ::IO::HandleType fileHandle)
{
    LOADING_TIME_PROFILE_SECTION;

    SAutoCollectFileAcessTime accessTime(this);

    {
        AUTO_READLOCK(m_csOpenFiles);
        INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
        if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
        {
            return m_arrOpenFiles[nPseudoFile].FRead(pData, nSize, nCount, fileHandle);
        }
    }

    AZ::u64 bytesRead = 0;
    AZ::IO::FileIOBase::GetDirectInstance()->Read(fileHandle, pData, nSize * nCount, false, &bytesRead);
    return static_cast<size_t>(bytesRead / nSize);
}

//////////////////////////////////////////////////////////////////////////
size_t CCryPak::FReadRawAll(void* pData, size_t nFileSize, AZ::IO::HandleType fileHandle)
{
    LOADING_TIME_PROFILE_SECTION;

    SAutoCollectFileAcessTime accessTime(this);
    {
        AUTO_READLOCK(m_csOpenFiles);
        INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
        if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
        {
            return m_arrOpenFiles[nPseudoFile].FReadAll(pData, nFileSize, fileHandle);
        }
    }

    AZ::IO::FileIOBase::GetDirectInstance()->Seek(fileHandle, 0, AZ::IO::SeekType::SeekFromStart);
    AZ::u64 bytesRead = 0;
    AZ::IO::FileIOBase::GetDirectInstance()->Read(fileHandle, pData, nFileSize, false, &bytesRead);
    return static_cast<size_t>(bytesRead);
}

//////////////////////////////////////////////////////////////////////////
void* CCryPak::FGetCachedFileData(AZ::IO::HandleType fileHandle, size_t& nFileSize)
{
    LOADING_TIME_PROFILE_SECTION;

    SAutoCollectFileAcessTime accessTime(this);
    {
        AUTO_READLOCK(m_csOpenFiles);
        INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
        if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
        {
            return m_arrOpenFiles[nPseudoFile].GetFileData(nFileSize, fileHandle);
        }
    }

    // Cached lookup
    {
        RawDataCacheLockGuard lock(m_cachedFileRawDataMutex);
        auto itr = m_cachedFileRawDataSet.find(fileHandle);
        if (itr != m_cachedFileRawDataSet.end())
        {
            CachedRawDataEntry& entry = itr->second;
            nFileSize = entry.m_fileSize;
            return entry.m_data->m_pCachedData;
        }
    }

    // Cache miss, now read the file
    nFileSize = FGetSize(fileHandle);

    AZStd::unique_ptr<CCachedFileRawData> pCachedFileRawData(new CCachedFileRawData(nFileSize));

    AZ::IO::FileIOBase::GetDirectInstance()->Seek(fileHandle, 0, AZ::IO::SeekType::SeekFromStart);
    if (!AZ::IO::FileIOBase::GetDirectInstance()->Read(fileHandle, pCachedFileRawData->m_pCachedData, nFileSize))
    {
        return nullptr;
    }

    // Add to the cache
    void* pCachedData = nullptr;
    {
        RawDataCacheLockGuard lock(m_cachedFileRawDataMutex);

        CachedRawDataEntry& entry = m_cachedFileRawDataSet[fileHandle];
        if (!entry.m_data)
        {
            entry.m_data = std::move(pCachedFileRawData);
            entry.m_fileSize = nFileSize;
        }
        else
        {
#ifndef _RELEASE
            char fileNameBuffer[MAX_PATH];
            const char* fileName = AZ::IO::FileIOBase::GetDirectInstance()->GetFilename(fileHandle, fileNameBuffer, AZ_ARRAY_SIZE(fileNameBuffer)) ? fileNameBuffer : "unknown";
            CryLog("Perf Warning: First call to FGetCachedFileData for file \"%s\" made from multiple threads concurrently", fileName);
#endif
            CRY_ASSERT(entry.m_fileSize == nFileSize);
        }

        pCachedData = entry.m_data->m_pCachedData;
    }

    return pCachedData;
}

//////////////////////////////////////////////////////////////////////////
int CCryPak::FClose(AZ::IO::HandleType fileHandle)
{
    // Free cached data (not all files have raw cached data)
    {
        RawDataCacheLockGuard lock(m_cachedFileRawDataMutex);
        m_cachedFileRawDataSet.erase(fileHandle);
    }

    SAutoCollectFileAcessTime accessTime(this);
    INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
    AUTO_MODIFYLOCK(m_csOpenFiles);
    if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
    {
        m_arrOpenFiles[nPseudoFile].Destruct();
        return 0;
    }
    else
    {
        if (AZ::IO::FileIOBase::GetDirectInstance()->Close(fileHandle))
        {
            return 0;
        }
        return 1;
    }
}

bool CCryPak::IsInPak(AZ::IO::HandleType fileHandle)
{
    AUTO_READLOCK(m_csOpenFiles);
    INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
    return (UINT_PTR)nPseudoFile < m_arrOpenFiles.size();
}

int CCryPak::FEof(AZ::IO::HandleType fileHandle)
{
    SAutoCollectFileAcessTime accessTime(this);
    {
        AUTO_READLOCK(m_csOpenFiles);
        INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
        if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
        {
            return m_arrOpenFiles[nPseudoFile].FEof();
        }
    }

    return AZ::IO::FileIOBase::GetDirectInstance()->Eof(fileHandle);
}


int CCryPak::FPrintf(AZ::IO::HandleType fileHandle, const char* szFormat, ...)
{
    SAutoCollectFileAcessTime accessTime(this);
    {
        AUTO_READLOCK(m_csOpenFiles);
        INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
        if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
        {
            return 0; // we don't support it now
        }
    }


    va_list arglist;
    int rv;
    va_start(arglist, szFormat);
    rv = static_cast<int>(AZ::IO::PrintV(fileHandle, szFormat, arglist));
    va_end(arglist);
    return rv;
}

char* CCryPak::FGets(char* str, int n, AZ::IO::HandleType fileHandle)
{
    SAutoCollectFileAcessTime accessTime(this);
    {
        AUTO_READLOCK(m_csOpenFiles);
        INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
        if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
        {
            return m_arrOpenFiles[nPseudoFile].FGets(str, n);
        }
    }

    PROFILE_DISK_READ(n);

    return AZ::IO::FGetS(str, n, fileHandle);
}

int CCryPak::Getc(AZ::IO::HandleType fileHandle)
{
    SAutoCollectFileAcessTime accessTime(this);
    {
        AUTO_READLOCK(m_csOpenFiles);
        INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
        if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
        {
            return m_arrOpenFiles[nPseudoFile].Getc();
        }
    }

    PROFILE_DISK_READ(1);
    return AZ::IO::GetC(fileHandle);
}

int CCryPak::Ungetc(int c, AZ::IO::HandleType fileHandle)
{
    SAutoCollectFileAcessTime accessTime(this);
    {
        AUTO_READLOCK(m_csOpenFiles);
        INT_PTR nPseudoFile = ((INT_PTR)fileHandle) - g_nPseudoFileIdxOffset;
        if ((UINT_PTR)nPseudoFile < m_arrOpenFiles.size())
        {
            return m_arrOpenFiles[nPseudoFile].Ungetc(c);
        }
    }

    return AZ::IO::UnGetC(c, fileHandle);
}

const char* GetExtension(const char* in)
{
    while (*in)
    {
        if (*in == '.')
        {
            return in;
        }
        in++;
    }
    return NULL;
}


//////////////////////////////////////////////////////////////////////////
intptr_t CCryPak::FindFirst(const char* pDir, _finddata_t* fd,
    unsigned int nPathFlags, bool bAllOwUseFileSystem)
{
    char szFullPathBuf[g_nMaxPath];

    //m_pLog->Log("Scanning %s",pDir);
    //const char *szFullPath = AdjustFileName(pDir, szFullPathBuf, 0);
    const char* szFullPath = AdjustFileName(pDir, szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), nPathFlags);
    CCryPakFindData_AutoPtr pFindData = CreateFindData();
    pFindData->Scan(this, szFullPath, bAllOwUseFileSystem);
    if (pFindData->empty())
    {
        if (m_arrMods.empty())
        {
            return (-1); // no mods and no files found
        }
    }

    if (m_pPakVars->nPriority != ePakPriorityPakOnly)
    {
        // now scan mod folders as well

        std::vector<string>::reverse_iterator it;
        for (it = m_arrMods.rbegin(); it != m_arrMods.rend(); ++it)
        {
            if (m_pPakVars->nPriority == ePakPriorityFileFirstModsOnly && !IsModPath(it->c_str()))
            {
                continue;
            }
            CryPathString modPath = (*it).c_str();
            modPath.append(1, '/');
            modPath += pDir;
            const char* szFullModPath = AdjustFileName(modPath, szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), nPathFlags | FLAGS_PATH_REAL);
            if (szFullModPath)
            {
                pFindData->Scan(this, szFullModPath);
            }
        } //it
    }


    if (pFindData->empty())
    {
        return (-1);
    }

    AUTO_MODIFYLOCK(m_csFindData);
    m_setFindData.insert(pFindData);

    pFindData->Fetch(fd);

#if defined(LINUX) || defined(APPLE)
    //[K01]: you can't just cast a pointer...if it's negative, we'll think it failed!
    m_HandleSource++;
    if (m_HandleSource < 0)
    {
        m_HandleSource = 1;
    }
    assert(m_Handles.find(m_HandleSource) == m_Handles.end());
    m_Handles.insert(std::pair<intptr_t, CCryPakFindData*>(m_HandleSource, pFindData));
    return m_HandleSource;
#else
    return (intptr_t)(CCryPakFindData*)pFindData;
#endif
}

//////////////////////////////////////////////////////////////////////////
int CCryPak::FindNext(intptr_t handle, struct _finddata_t* fd)
{
    AUTO_READLOCK(m_csFindData);
    //if (m_setFindData.find ((CCryPakFindData*)handle) == m_setFindData.end())
    //  return -1; // invalid handle

#if defined(LINUX) || defined(APPLE)
    //[K01]: linux fixes
    std::map<intptr_t, CCryPakFindData*>::iterator lookup = m_Handles.find(handle);
    if (lookup == m_Handles.end())
    {
        return -1;
    }

    CCryPakFindData* CryFinder = lookup->second;
    if (CryFinder->Fetch(fd))
    {
        return 0;
    }
    else
    {
        return -1;
    }
#else
    if (((CCryPakFindData*)handle)->Fetch(fd))
    {
        return 0;
    }
    else
    {
        return -1;
    }
#endif
}

int CCryPak::FindClose(intptr_t handle)
{
    AUTO_MODIFYLOCK(m_csFindData);

#if defined(LINUX) || defined(APPLE)
    std::map<intptr_t, CCryPakFindData*>::iterator lookup = m_Handles.find(handle);
    if (lookup == m_Handles.end())
    {
        return -1;
    }
    CCryPakFindData* CryFinder = lookup->second;
    m_Handles.erase(lookup);

    m_setFindData.erase (CryFinder);
#else
    m_setFindData.erase((CCryPakFindData*)handle);
#endif
    return 0;
}

//////////////////////////////////////////////////////////////////////////
bool CCryPak::LoadPakToMemory(const char* pName, ICryPak::EInMemoryPakLocation nLoadPakToMemory, IMemoryBlock* pMemoryBlock)
{
    LOADING_TIME_PROFILE_SECTION_ARGS(pName);
    MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "Load Pak To Memory: %s", pName);

    CryPathString pakFile = pName;
    pakFile.MakeLower();

    pakFile = ConvertAbsolutePathToAliasedPath(pakFile.c_str());
    unsigned int index = 0;
    if (pakFile[index] != '@') // if unaliased, we need to alias it, since paks are using 'full paths' which are in fact aliased
    {
        pakFile = CryPathString("@assets@\\") + pakFile;
    }

    pakFile.replace(g_cNonNativeSlash, g_cNativeSlash);

    AUTO_MODIFYLOCK(m_csZips);
    for (ZipArray::iterator it = m_arrZips.begin(); it != m_arrZips.end(); ++it)
    {
        const char* fullPath = it->GetFullPath();
        if (0 != strstr(fullPath, pakFile.c_str()))
        {
            if (nLoadPakToMemory != eInMemoryPakLocale_Unload)
            {
                it->pZip->PreloadToMemory(pMemoryBlock);
            }
            else
            {
                it->pZip->UnloadFromMemory();
            }
            return true;
        }
    }
    // this could indicate a problem so at least log it!
    CryLog("Asked to load pak '%s' to memory, but did not find it in the list of available paks.", pName);
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CCryPak::LoadPaksToMemory(int nMaxPakSize, bool bLoadToMemory)
{
    LOADING_TIME_PROFILE_SECTION;
    MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Load Paks To Memory");
    AUTO_MODIFYLOCK(m_csZips);
    for (ZipArray::reverse_iterator itZip = m_arrZips.rbegin(); itZip != m_arrZips.rend(); ++itZip)
    {
        if (bLoadToMemory)
        {
            if ((int)itZip->pZip->GetZipFileSize() <= nMaxPakSize)
            {
                itZip->pZip->PreloadToMemory();
            }
        }
        else
        {
            // Unload
            itZip->pZip->UnloadFromMemory();
        }
    }
}

//======================================================================
bool CCryPak::OpenPack(const char* szBindRootIn, const char* szPath, unsigned nFlags, IMemoryBlock* pData, CryFixedStringT<g_nMaxPath>* pFullPath)
{
    assert(szBindRootIn);
    char szFullPathBuf[g_nMaxPath];

    const char* szFullPath = AdjustFileName(szPath, szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), nFlags);

    char szBindRootBuf[g_nMaxPath];
    const char* szBindRoot = AdjustFileName(szBindRootIn, szBindRootBuf, AZ_ARRAY_SIZE(szBindRootBuf), FLAGS_ADD_TRAILING_SLASH | FLAGS_PATH_REAL);

    bool result = OpenPackCommon(szBindRoot, szFullPath, nFlags, pData);

    if (pFullPath)
    {
        pFullPath->assign(szFullPath);
    }

    return result;
}

bool CCryPak::OpenPack(const char* szPath, unsigned nFlags, IMemoryBlock* pData, CryFixedStringT<g_nMaxPath>* pFullPath)
{
    char szFullPathBuf[g_nMaxPath];

    const char* szFullPath = AdjustFileName(szPath, szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), nFlags);
    string strBindRoot;
    const char* pLastSlash = strrchr(szFullPath, g_cNativeSlash);
#if AZ_LEGACY_CRYSYSTEM_TRAIT_CRYPAK_POSIX
    if (!pLastSlash)
    {
        pLastSlash = strrchr(szFullPath, g_cNonNativeSlash);
    }
#endif
    if (pLastSlash)
    {
        strBindRoot.assign(szFullPath, pLastSlash - szFullPath + 1);
    }
    else
    {
        m_pLog->LogError("Pak file %s has absolute path %s, which is strange", szPath, szFullPath);
        //      desc.strFileName = szZipPath;
    }

    bool result = OpenPackCommon(strBindRoot.c_str(), szFullPath, nFlags, pData);

    if (pFullPath)
    {
        pFullPath->assign(szFullPath);
    }

    return result;
}

#ifdef USE_LIBTOMCRYPT
extern rsa_key g_rsa_key_public_for_sign;
#endif

bool CCryPak::OpenPackCommon(const char* szBindRoot, const char* szFullPath, unsigned int nPakFlags, IMemoryBlock* pData)
{
    // setup PackDesc before the duplicate test
    PackDesc desc;
    desc.strBindRoot = szBindRoot;
    desc.strFileName = szFullPath;

    if (desc.strBindRoot.empty())
    {
        desc.strBindRoot = "@assets@" CRY_NATIVE_PATH_SEPSTR;
    }
    else if (desc.strBindRoot[desc.strBindRoot.size() - 1] != CRY_NATIVE_PATH_SEPSTR[0])
    {
        desc.strBindRoot.append(CRY_NATIVE_PATH_SEPSTR);
    }

    // Note this will replace @devassets@ with @assets@ to provide a proper bind root for the paks
    desc.strBindRoot = ConvertAbsolutePathToAliasedPath(desc.strBindRoot.c_str());

    // hold the lock from the point we query the zip array,
    // so we don't end up adding a given pak twice
    AUTO_MODIFYLOCK(m_csZips);

    {
        // try to find this - maybe the pack has already been opened
        for (ZipArray::iterator it = m_arrZips.begin(); it != m_arrZips.end(); ++it)
        {
            const char* pFilePath = it->pZip->GetFilePath();
            if (!azstricmp(pFilePath, desc.strFileName.c_str())
                && !azstricmp(it->strBindRoot.c_str(), desc.strBindRoot.c_str()))
            {
                // load to mem if open has requested in-mem
                if ((nPakFlags & (FLAGS_PAK_IN_MEMORY | FLAGS_PAK_IN_MEMORY_CPU)) && !it->pZip->IsInMemory())
                {
                    it->pZip->PreloadToMemory(NULL);
                }

                return true; // already opened
            }
        }
    }

    int flags = ICryArchive::FLAGS_OPTIMIZED_READ_ONLY | ICryArchive::FLAGS_ABSOLUTE_PATHS;
    if ((nPakFlags & FLAGS_PAK_IN_MEMORY) != 0)
    {
        flags |= ICryArchive::FLAGS_IN_MEMORY;
    }
    if ((nPakFlags & FLAGS_PAK_IN_MEMORY_CPU) != 0)
    {
        flags |= ICryArchive::FLAGS_IN_MEMORY_CPU;
    }
    if ((nPakFlags & FLAGS_FILENAMES_AS_CRC32) != 0)
    {
        flags |= ICryArchive::FLAGS_FILENAMES_AS_CRC32;
    }
    if ((nPakFlags & FLAGS_REDIRECT_TO_DISC) != 0)
    {
        flags |= FLAGS_REDIRECT_TO_DISC;
    }
    if ((nPakFlags& ICryArchive::FLAGS_OVERRIDE_PAK) != 0)
    {
        flags |= ICryArchive::FLAGS_OVERRIDE_PAK;
    }
    if ((nPakFlags& FLAGS_NO_LOWCASE) != 0)
    {
        flags |= FLAGS_NO_LOWCASE;
    }
    desc.pArchive = OpenArchive(szFullPath, szBindRoot, flags, pData);
    if (!desc.pArchive)
    {
        return false; // couldn't open the archive
    }
    if (m_filesCachedOnHDD.size())
    {
        uint32 crc = CCrc32::ComputeLowercase(szFullPath);
        if (m_filesCachedOnHDD.find(crc) != m_filesCachedOnHDD.end())
        {
            unsigned int eFlags = desc.pArchive->GetFlags();
            desc.pArchive->SetFlags(eFlags | ICryArchive::FLAGS_ON_HDD);
            //printf("%s is installed on the HDD\n", szFullPath);
        }
    }

    CryComment("Opening pak file %s", szFullPath);

    if (desc.pArchive->GetClassId() == CryArchive::gClassId)
    {
        m_pLog->LogWithType(IMiniLog::eComment, "Opening pak file %s to %s", szFullPath, szBindRoot ? szBindRoot : "<NIL>");
        desc.pZip = static_cast<CryArchive*>((ICryArchive*)desc.pArchive)->GetCache();

        //Append the pak to the end but before any override paks
        ZipArray::reverse_iterator revItZip = m_arrZips.rbegin();
        if ((nPakFlags& ICryArchive::FLAGS_OVERRIDE_PAK) == 0)
        {
            for (; revItZip != m_arrZips.rend(); ++revItZip)
            {
                if ((revItZip->pArchive->GetFlags() & ICryArchive::FLAGS_OVERRIDE_PAK) == 0)
                {
                    break;
                }
            }
        }
        ZipArray::iterator itZipPlace = revItZip.base();
        m_arrZips.insert(itZipPlace, desc);

#if 0
        CryLog("---START Pack List: OpenPackCommon '%s' 0x%X---", szFullPath, nPakFlags);
        for (ZipArray::iterator it = m_arrZips.begin(); it != m_arrZips.end(); ++it)
        {
            CryLog("Pack '%s' Bind '%s' 0x%X %d", it->GetFullPath(), it->strBindRoot.c_str(), it->pArchive->GetFlags(), it->pArchive->GetFlags() & ICryArchive::FLAGS_OVERRIDE_PAK);
        }
        CryLog("---END Pack List---");
#endif // #if 0

        return true;
    }
    else
    {
        return false; // don't support such objects yet
    }
}


//int gg=1;
// after this call, the file will be unlocked and closed, and its contents won't be used to search for files
bool CCryPak::ClosePack(const char* pName, unsigned nFlags)
{
    char szZipPathBuf[g_nMaxPath];
    const char* szZipPath = AdjustFileName(pName, szZipPathBuf, AZ_ARRAY_SIZE(szZipPathBuf), nFlags);

    //if (strstr(szZipPath,"huggy_tweak_scripts"))
    //  gg=0;

    AUTO_MODIFYLOCK(m_csZips);
    for (ZipArray::iterator it = m_arrZips.begin(); it != m_arrZips.end(); ++it)
    {
        if (!azstricmp(szZipPath, it->GetFullPath()))
        {
            // this is the pack with the given name - remove it, and if possible it will be deleted
            // the zip is referenced from the archive and *it; the archive is referenced only from *it
            //
            // the pZip (cache) can be referenced from stream engine and pseudo-files.
            // the archive can be referenced from outside
            bool bResult = (it->pZip->NumRefs() == 2) && it->pArchive->NumRefs() == 1;
            if (bResult)
            {
                m_arrZips.erase(it);
            }
#if 0
            CryLog("---START Pack List: ClosePack '%s' 0x%X---", pName, nFlags);
            for (ZipArray::iterator it = m_arrZips.begin(); it != m_arrZips.end(); ++it)
            {
                CryLog("Pack '%s' Bind '%s' 0x%X %d", it->GetFullPath(), it->strBindRoot.c_str(), it->pArchive->GetFlags(), it->pArchive->GetFlags() & ICryArchive::FLAGS_OVERRIDE_PAK);
            }
            CryLog("---END Pack List---");
#endif // #if 0
            return bResult;
        }
    }
    return true;
}

bool CCryPak::FindPacks(const char* pWildcardIn)
{
    char cWorkBuf[g_nMaxPath];
    AdjustFileName(pWildcardIn, cWorkBuf, AZ_ARRAY_SIZE(cWorkBuf), ICryPak::FLAGS_PATH_REAL | ICryArchive::FLAGS_OVERRIDE_PAK | FLAGS_COPY_DEST_ALWAYS);
    bool foundMatchingPackFile = false;
    AZ::IO::FileIOBase::GetDirectInstance()->FindFiles(PathUtil::GetPath(cWorkBuf), PathUtil::GetFile(cWorkBuf), [&](const char* filePath) -> bool
        {
            // Even one invocation here means we found a matching file
            foundMatchingPackFile = true;
            // Don't bother getting any more
            return false;
        });
    return foundMatchingPackFile;
}

bool CCryPak::OpenPacks(const char* pWildcardIn, unsigned nFlags, std::vector< CryFixedStringT<g_nMaxPath> >* pFullPaths)
{
    char cWorkBuf[g_nMaxPath];
    AdjustFileName(pWildcardIn, cWorkBuf, AZ_ARRAY_SIZE(cWorkBuf), nFlags | FLAGS_COPY_DEST_ALWAYS);
    string strBindRoot = PathUtil::GetParentDirectory(cWorkBuf);
    strBindRoot += g_cNativeSlash;
    return OpenPacksCommon(strBindRoot.c_str(), pWildcardIn, cWorkBuf, nFlags, pFullPaths);
}

bool CCryPak::OpenPacks(const char* szBindRoot, const char* pWildcardIn, unsigned nFlags, std::vector< CryFixedStringT<g_nMaxPath> >* pFullPaths)
{
    char cWorkBuf[g_nMaxPath];
    AdjustFileName(pWildcardIn, cWorkBuf, AZ_ARRAY_SIZE(cWorkBuf), nFlags | FLAGS_COPY_DEST_ALWAYS);

    char cBindRootBuf[g_nMaxPath];
    const char* pBindRoot = AdjustFileName(szBindRoot, cBindRootBuf, AZ_ARRAY_SIZE(cBindRootBuf), FLAGS_ADD_TRAILING_SLASH | FLAGS_PATH_REAL);

    return OpenPacksCommon(pBindRoot, pWildcardIn, cWorkBuf, nFlags, pFullPaths);
}

bool CCryPak::OpenPacksCommon(const char* szDir, const char* pWildcardIn, char* cWork, int nPakFlags, std::vector< CryFixedStringT<g_nMaxPath> >* pFullPaths)
{
    if (!strchr(cWork, '*') && !strchr(cWork, '?'))
    {
        // No wildcards, just open pack
        if (OpenPackCommon(szDir, cWork, nPakFlags))
        {
            if (pFullPaths)
            {
                pFullPaths->push_back(cWork);
            }
        }
        return true;
    }

    // Note this code suffers from a common FindFirstFile problem - a search
    // for *.pak will also find *.pak? (since the short filename version of *.pakx,
    // for example, is *.pak). For more information, see
    // http://blogs.msdn.com/oldnewthing/archive/2005/07/20/440918.aspx
    // Therefore this code performs an additional check to make sure that the
    // found filenames match the spec.
    _finddata_t fd;
    intptr_t h = FindFirst(cWork, &fd, 0, /*bAllOwUseFileSystem =*/ true);

    char cWildcardFullPath[MAX_PATH];
    sprintf_s(cWildcardFullPath, "*.%s", PathUtil::GetExt(pWildcardIn));

    // where to copy the filenames to form the path in cWork
    char* pDestName = strrchr(cWork, g_cNativeSlash);
#if AZ_LEGACY_CRYSYSTEM_TRAIT_CRYPAK_POSIX
    if (!pDestName)
    {
        pDestName = strrchr (cWork, g_cNonNativeSlash);
    }
#endif
    size_t cWorkBufferSize = g_nMaxPath;
    if (!pDestName)
    {
        pDestName = cWork;
    }
    else
    {
        ++pDestName;
        // account for new buffer size.
        cWorkBufferSize -= (pDestName-cWork);
    }
    if (h != -1)
    {
        std::vector<string> files;
        do
        {
            azstrcpy(pDestName, cWorkBufferSize, fd.name);
            if (PathUtil::MatchWildcard(cWork, cWildcardFullPath))
            {
#pragma warning( push )
#pragma warning(disable: 4996)
                files.push_back(strlwr(cWork));
#pragma warning( pop )
            }
        } while (FindNext(h, &fd) >= 0);

        // Open files in alphabet order.
        std::sort(files.begin(), files.end());
        bool bAllOk = true;
        for (int i = 0; i < (int)files.size(); i++)
        {
            bAllOk = OpenPackCommon(szDir, files[i].c_str(), nPakFlags) && bAllOk;

            if (pFullPaths)
            {
                pFullPaths->push_back(files[i].c_str());
            }
        }

        FindClose(h);
        return bAllOk;
    }

    return false;
}


bool CCryPak::ClosePacks(const char* pWildcardIn, unsigned nFlags)
{
    char cWorkBuf[g_nMaxPath];
    const char* cWork = AdjustFileName(pWildcardIn, cWorkBuf, AZ_ARRAY_SIZE(cWorkBuf), nFlags);
    string searchDirectory = PathUtil::GetPath(cWork);
    string pattern = PathUtil::GetFile(cWork);
    return AZ::IO::FileIOBase::GetDirectInstance()->FindFiles(searchDirectory, pattern, [&](const char* filePath) -> bool
        {
            ClosePack(filePath, nFlags);
            return true;
        });
}

bool CCryPak::ReOpenPack(const char* pPath)
{
    AUTO_MODIFYLOCK(m_csZips);

    ZipArray::iterator end = m_arrZips.end();
    for (ZipArray::iterator it = m_arrZips.begin(); it != end; ++it)
    {
        ZipDir::Cache* pZip = it->pZip;
        const char* curPath = pZip->GetFilePath();

        if (strstr(curPath, pPath))
        {
            pZip->ReOpen(curPath);

            //printf("[CCryPak] ReOpen: %s\n", curPath);

            ICryArchive* pArchive = it->pArchive;
            if (pArchive)
            {
                unsigned int eFlags = pArchive->GetFlags();
                pArchive->SetFlags(eFlags | ICryArchive::FLAGS_ON_HDD);
            }
        }
    }

    uint32 crc = CCrc32::ComputeLowercase(pPath);
    m_filesCachedOnHDD.insert(crc);
    //printf("[HDD Cache] %s : %u\n", pPath, crc);

    return true;
}

int CCryPak::RemountPacks(DynArray<AZ::IO::HandleType>& outHandlesToClose, CBShouldPackReOpen pShouldPackReOpen)
{
    AUTO_MODIFYLOCK(m_csZips);

    outHandlesToClose.reserve(m_arrZips.size());

    int numRemounted = 0;
    ZipArray::iterator end = m_arrZips.end();
    for (ZipArray::iterator it = m_arrZips.begin(); it != end; ++it)
    {
        if (!(it->pArchive->GetFlags() & ICryArchive::FLAGS_IN_MEMORY_MASK))
        {
            ZipDir::Cache* pZip = it->pZip;
            const char* curPath = pZip->GetFilePath();

            if (pShouldPackReOpen(curPath))
            {
                outHandlesToClose.push_back(pZip->GetFileHandle());
                bool okay = pZip->ReOpen(curPath);

                ICryArchive* pArchive = it->pArchive;
                if (pArchive)
                {
                    unsigned int eFlags = pArchive->GetFlags();
                    pArchive->SetFlags(eFlags | ICryArchive::FLAGS_ON_HDD);
                }

                if (!okay)
                {
                    CryFatalError("RemountPacks: Failed to reopen pak %s\n", curPath);
                }
                numRemounted++;
            }
        }
    }
    return numRemounted;
}


bool CCryPak::InitPack(const char* szBasePath, unsigned nFlags)
{
    return true;
}

/////////////////////////////////////////////////////
bool CCryPak::Init(const char* szBasePath)
{
    return InitPack(szBasePath);
}

void CCryPak::Release()
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CCachedFileRawData::CCachedFileRawData(int nAlloc)
{
    m_pCachedData = 0;

    m_pCachedData = g_pPakHeap->TempAlloc(nAlloc, "CCachedFileRawData::CCachedFileRawData");
}

//////////////////////////////////////////////////////////////////////////
CCachedFileRawData::~CCachedFileRawData()
{
    if (m_pCachedData)
    {
        g_pPakHeap->FreeTemporary(m_pCachedData);
    }
    m_pCachedData = 0;
}


//////////////////////////////////////////////////////////////////////////
// this object must be constructed before usage
// nFlags is a combination of _O_... flags
void CZipPseudoFile::Construct(CCachedFileData* pFileData, unsigned nFlags)
{
    m_pFileData = pFileData;
    m_nFlags = nFlags;
    m_nCurSeek = 0;
}

//////////////////////////////////////////////////////////////////////////
// this object needs to be freed manually when the CryPak shuts down..
void CZipPseudoFile::Destruct()
{
    assert(m_pFileData);
    // mark it free, and deallocate the pseudo file memory
    m_pFileData = NULL;
}

//////////////////////////////////////////////////////////////////////////
int CZipPseudoFile::FSeek(long nOffset, int nMode)
{
    if (!m_pFileData)
    {
        return -1;
    }

    switch (nMode)
    {
    case SEEK_SET:
        m_nCurSeek = nOffset;
        break;
    case SEEK_CUR:
        m_nCurSeek += nOffset;
        break;
    case SEEK_END:
        m_nCurSeek = GetFileSize() + nOffset;
        break;
    default:
        assert(0);
        return -1;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
size_t CZipPseudoFile::FRead(void* pDest, size_t nSize, size_t nCount, AZ::IO::HandleType fileHandle)
{
    LOADING_TIME_PROFILE_SECTION;

    if (!GetFile())
    {
        return 0;
    }

    size_t nTotal = nSize * nCount;
    if (!nTotal || (unsigned)m_nCurSeek >= GetFileSize())
    {
        return 0;
    }

    if (nTotal > GetFileSize() - m_nCurSeek)
    {
        nTotal = GetFileSize() - m_nCurSeek;
        if (nTotal < nSize)
        {
            return 0;
        }
        nTotal -= nTotal % nSize;
    }

    int64 nReadBytes = GetFile()->ReadData(pDest, m_nCurSeek, nTotal);
    if (nReadBytes == -1)
    {
        return 0;
    }

    if (nReadBytes != nTotal)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR_DBGBRK, "FRead did not read expected number of byte from file, only %" PRISIZE_T " of %lld bytes read", nTotal, nReadBytes);
        nTotal = (size_t)nReadBytes;
    }
    m_nCurSeek += nTotal;
    return nTotal / nSize;
}

//////////////////////////////////////////////////////////////////////////
size_t CZipPseudoFile::FReadAll(void* pDest, size_t nFileSize, AZ::IO::HandleType fileHandle)
{
    LOADING_TIME_PROFILE_SECTION;

    if (!GetFile())
    {
        return 0;
    }

    if (nFileSize != GetFileSize())
    {
        assert(0); // Bad call
        return 0;
    }

    if (!GetFile()->ReadData(pDest, 0, nFileSize))
    {
        return 0;
    }

    m_nCurSeek = nFileSize;

    return nFileSize;
}

//////////////////////////////////////////////////////////////////////////
void* CZipPseudoFile::GetFileData(size_t& nFileSize, AZ::IO::HandleType fileHandle)
{
    LOADING_TIME_PROFILE_SECTION;

    if (!GetFile())
    {
        return 0;
    }

    nFileSize = GetFileSize();

    void* pData = GetFile()->GetData();
    m_nCurSeek = nFileSize;
    return pData;
}

//////////////////////////////////////////////////////////////////////////
int CZipPseudoFile::FEof()
{
    return (unsigned)m_nCurSeek >= GetFileSize();
}

char* CZipPseudoFile::FGets(char* pBuf, int n)
{
    if (!GetFile())
    {
        return NULL;
    }

    char* pData = (char*)GetFile()->GetData();
    if (!pData)
    {
        return NULL;
    }
    int nn = 0;
    int i;
    for (i = 0; i < n; i++)
    {
        if (i + m_nCurSeek == GetFileSize())
        {
            break;
        }
        char c = pData[i + m_nCurSeek];
        if (c == 0xa || c == 0)
        {
            pBuf[nn++] = c;
            i++;
            break;
        }
        else
        if (c == 0xd)
        {
            continue;
        }
        pBuf[nn++] = c;
    }
    pBuf[nn] = 0;
    m_nCurSeek += i;

    if (m_nCurSeek == GetFileSize())
    {
        return NULL;
    }
    return pBuf;
}

int CZipPseudoFile::Getc()
{
    if (!GetFile())
    {
        return EOF;
    }
    char* pData = (char*)GetFile()->GetData();
    if (!pData)
    {
        return EOF;
    }
    int c = EOF;
    int i;
    for (i = 0; i < 1; i++)
    {
        if (i + m_nCurSeek == GetFileSize())
        {
            return c;
        }
        c = pData[i + m_nCurSeek];
        break;
    }
    m_nCurSeek += i + 1;
    return c;
}

int CZipPseudoFile::Ungetc(int c)
{
    if (m_nCurSeek <= 0)
    {
        return EOF;
    }
    m_nCurSeek--;
    return c;
}

CCachedFileData::CCachedFileData(class CCryPak* pPak, ZipDir::Cache* pZip, unsigned int nArchiveFlags, ZipDir::FileEntry* pFileEntry, const char* szFilename)
{
    m_pPak = pPak;
    m_nArchiveFlags = nArchiveFlags;
    m_pFileData = NULL;
    m_pZip = pZip;
    m_pFileEntry = pFileEntry;

#ifdef _DEBUG
    m_sDebugFilename = szFilename;
#endif // _DEBUG
       //m_filename = szFilename;

    if (pPak)
    {
        pPak->Register(this);
    }
}

CCachedFileData::~CCachedFileData()
{
    if (m_pPak)
    {
        m_pPak->Unregister(this);
    }

    // forced destruction
    if (m_pFileData)
    {
        g_pPakHeap->FreeTemporary(m_pFileData);
        m_pFileData = NULL;
    }

    m_pZip = NULL;
    m_pFileEntry = NULL;
}

//////////////////////////////////////////////////////////////////////////
bool CCachedFileData::GetDataTo(void* pFileData, int nDataSize, bool bDecompress)
{
    assert(m_pZip);
    assert(m_pFileEntry && m_pZip->IsOwnerOf(m_pFileEntry));

    if (nDataSize != m_pFileEntry->desc.lSizeUncompressed && bDecompress)
    {
        return false;
    }
    else if (nDataSize != m_pFileEntry->desc.lSizeCompressed && !bDecompress)
    {
        return false;
    }

    if (!m_pFileData)
    {
        AUTO_LOCK_CS(m_csDecompressDecryptLock);
        if (!m_pFileData)
        {
            if (ZipDir::ZD_ERROR_SUCCESS != m_pZip->ReadFile(m_pFileEntry, NULL, pFileData, bDecompress))
            {
                return false;
            }
        }
        else
        {
            memcpy(pFileData, m_pFileData, nDataSize);
        }
    }
    else
    {
        memcpy(pFileData, m_pFileData, nDataSize);
    }
    return true;
}

// return the data in the file, or NULL if error
void* CCachedFileData::GetData(bool bRefreshCache, bool decompress, bool decrypt)
{
    // first, do a "dirty" fast check without locking the critical section
    // in most cases, the data's going to be already there, and if it's there,
    // nobody's going to release it until this object is destructed.
    if (bRefreshCache && !m_pFileData)
    {
        assert(m_pZip);
        assert(m_pFileEntry && m_pZip->IsOwnerOf(m_pFileEntry));
        // Then, lock it and check whether the data is still not there.
        // if it's not, allocate memory and unpack the file
        AUTO_LOCK_CS(m_csDecompressDecryptLock);
        if (!m_pFileData)
        {
            // don't try to decompress or decrypt if its not actually compressed or encrypted
            decompress = decompress && m_pFileEntry->IsCompressed();
            decrypt = decrypt && m_pFileEntry->IsEncrypted();

            // if we are going to decompress into the buffer, we MUST allocate enough for it!
            // if we are either requesting decompressed data, or we are already decompressed, then we will need enough room for the
            // decompressed data
            bool allocateForDecompressed = decompress || (!m_pFileEntry->IsCompressed());
            uint32 nTempBufferSize = (allocateForDecompressed) ? m_pFileEntry->desc.lSizeUncompressed : m_pFileEntry->desc.lSizeCompressed;
            void* fileData = g_pPakHeap->TempAlloc(nTempBufferSize, "CCachedFileData::GetData");

            ZipDir::ErrorEnum result = m_pZip->ReadFile(m_pFileEntry, NULL, fileData, decompress, 0, -1, decrypt);

            if (result != ZipDir::ZD_ERROR_SUCCESS)
            {
                CryLogAlways("[ERROR] ReadFile returned %d\n", result);
                g_pPakHeap->FreeTemporary(fileData);
            }
            else
            {
                m_pFileData = fileData;
            }
        }
    }
    return m_pFileData;
}

//////////////////////////////////////////////////////////////////////////
int64 CCachedFileData::ReadData(void* pBuffer, int64 nFileOffset, int64 nReadSize)
{
    if (!m_pFileEntry)
    {
        return -1;
    }

    int64 nFileSize = m_pFileEntry->desc.lSizeUncompressed;
    if (nFileOffset + nReadSize > nFileSize)
    {
        nReadSize = nFileSize - nFileOffset;
    }
    if (nReadSize < 0)
    {
        return -1;
    }

    if (nReadSize == 0)
    {
        return 0;
    }

    if (m_pFileEntry->nMethod == ZipFile::METHOD_STORE) //Can't use this technique for METHOD_STORE_AND_STREAMCIPHER_KEYTABLE as seeking with encryption performs poorly
    {
        AUTO_LOCK_CS(m_csDecompressDecryptLock);
        // Uncompressed read.
        if (ZipDir::ZD_ERROR_SUCCESS != m_pZip->ReadFile(m_pFileEntry, NULL, pBuffer, false, nFileOffset, nReadSize))
        {
            return -1;
        }
    }
    else
    {
        uint8* pSrcBuffer = (uint8*)GetData();

        if (pSrcBuffer)
        {
            pSrcBuffer += nFileOffset;
            memcpy(pBuffer, pSrcBuffer, (size_t)nReadSize);
        }
        else
        {
            return -1;
        }
    }

    return nReadSize;
}


//////////////////////////////////////////////////////////////////////////
void CCachedFileData::AddRef()
{
    CryInterlockedIncrement(&m_nRefCounter);
}

//////////////////////////////////////////////////////////////////////////
void CCachedFileData::Release()
{
    const int nCount = CryInterlockedDecrement(&m_nRefCounter);
    assert(nCount >= 0);
    if (nCount == 0)
    {
        AUTO_MODIFYLOCK(m_pPak->m_csCachedFiles);
        if (m_nRefCounter == 0)
        {
            delete this;
        }
    }
    else if (nCount < 0)
    {
        assert(0);
        CryFatalError("Deleting Reference Counted Object Twice");
    }
}

//////////////////////////////////////////////////////////////////////////
void CCryPakFindData::Scan(class CCryPak* pPak, const char* szDir, bool bAllowUseFS)
{
    // get the priority into local variable to avoid it changing in the course of
    // this function execution
    int nVarPakPriority = pPak->m_pPakVars->nPriority;

    if (nVarPakPriority == ePakPriorityFileFirst)
    {
        // first, find the file system files
        ScanFS(pPak, szDir);
        ScanZips(pPak, szDir);
    }
    else
    {
        // first, find the zip files
        ScanZips(pPak, szDir);
        if (bAllowUseFS ||
            (nVarPakPriority != ePakPriorityPakOnly && (nVarPakPriority != ePakPriorityFileFirstModsOnly || IsModPath(szDir))))
        {
            ScanFS(pPak, szDir);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CCryPakFindData::CCryPakFindData()
{
}

//////////////////////////////////////////////////////////////////////////
void CCryPakFindData::ScanFS(CCryPak* pPak, const char* szDirIn)
{
    (void)pPak;

    string searchDirectory = PathUtil::GetPath(szDirIn);
    string pattern = PathUtil::GetFile(szDirIn);

    AZ::IO::FileIOBase::GetDirectInstance()->FindFiles(searchDirectory, pattern, [&](const char* filePath) -> bool
        {
            __finddata64_t fd;
            memset(&fd, 0, sizeof(fd));

            azstrcpy(fd.name, AZ_ARRAY_SIZE(fd.name), PathUtil::GetFile(filePath));

            if (AZ::IO::FileIOBase::GetDirectInstance()->IsDirectory(filePath))
            {
                fd.attrib |= _A_SUBDIR;
            }
            if (AZ::IO::FileIOBase::GetDirectInstance()->IsReadOnly(filePath))
            {
                fd.attrib |= _A_RDONLY;
            }
            AZ::u64 fileSize = 0;
            AZ::IO::FileIOBase::GetDirectInstance()->Size(filePath, fileSize);
            fd.size = fileSize;
            fd.time_write = AZ::IO::FileIOBase::GetDirectInstance()->ModificationTime(filePath);

            // These times are not supported by our file interface
            fd.time_access = fd.time_write;
            fd.time_create = fd.time_write;
            m_mapFiles.insert(FileMap::value_type(fd.name, FileDesc(&fd)));

            return true;
        });
}

//////////////////////////////////////////////////////////////////////////
void CCryPakFindData::ScanZips(CCryPak* pPak, const char* szDir)
{
    AUTO_READLOCK(pPak->m_csZips);


    char remainder[AZ_MAX_PATH_LEN] = { 0 };
    char mountPointRemainder[AZ_MAX_PATH_LEN] = { 0 };

    size_t nLen = strlen(szDir);
    for (CCryPak::ZipArray::iterator it = pPak->m_arrZips.begin(); it != pPak->m_arrZips.end(); ++it)
    {
        // filter out the stuff which does not match.

        // the problem here is that szDir might be something line "@assets@/levels/*.*"
        // but our pak might be mounted at the root, or at some other folder at like "@assets@" or "@assets@/levels/mylevel"
        // so there's really no way to filter out opening the pack and looking at the files inside.
        // however, the bind root is not part of the inner zip entry name either
        // and the ZipDir::FindFile actually expects just the chopped off piece.
        // we have to find whats in common between them and check that:

        size_t commonPoint = 0;
        while ((commonPoint < nLen) && (commonPoint < it->strBindRoot.size()) && (it->strBindRoot[commonPoint] == szDir[commonPoint]))
        {
            ++commonPoint;
        }

        if (commonPoint == 0)
        {
            continue; // if we have nothing in common we aren't eligible.
        }

        // commonPoint now contains the offset of the first differing character

        azstrncpy(remainder, AZ_MAX_PATH_LEN, szDir + commonPoint, strlen(szDir) - commonPoint + 1); // +1 to copy the null-terminator
        azstrncpy(mountPointRemainder, AZ_MAX_PATH_LEN, it->strBindRoot.c_str() + commonPoint, it->strBindRoot.size() - commonPoint);

        // Example:
        // "@assets@\\levels\\*.*" <--- szDir
        // "@assets@\\" <--- mount point
        // ~~~~~~~~~~~ Common part
        // "levels\\*.*" <---- remainder that is not in common
        // "" <--- mount point remainder.  In this case, we should scan the contents of the pak for the remainder

        // Example:
        // "@assets@\\levels\\*.*" <--- szDir
        // "@assets@\\levels\\mylevel\\" <--- mount point (its level.pak)
        //  ~~~~~~~~~~~~~~~~~~ common part
        // "*.*" <----  remainder that is not in common
        // "mylevel\\" <--- mount point remainder.

        // example:
        // "@assets@\\levels\\otherlevel\\*.*" <--- szDir
        // "@assets@\\levels\\mylevel\\" <--- mount point (its level.pak)
        // "otherlevel\\*.*" <----  remainder
        // "mylevel\\" <--- mount point remainder.

        // the general strategy here is that IF there is a mount point remainder
        // then it means that the pack's mount point itself might be a return value, not the files inside the pack
        // in that case, we compare the mount point remainder itself with the search filter

        if (strlen(mountPointRemainder) > 0)
        {
            // note that this is NOT equivalent to removing the slash - we are actually only returning the first component of the path,
            // even if it contains a string like "blah1\\blah2\\blah3" and we'd like to do this in-place without allocating anything
            char* endPos = strchr(mountPointRemainder, '/');
            if (!endPos)
            {
                endPos = strchr(mountPointRemainder, '\\');
            }

            if (endPos)
            {
                *endPos = 0; // terminate there - we only want the first component!
            }

            if (AZ::IO::NameMatchesFilter(mountPointRemainder, remainder))
            {
                m_mapFiles.insert(FileMap::value_type(PathUtil::RemoveSlash(mountPointRemainder).c_str(), FileDesc()));
            }
            *mountPointRemainder = 0; // Set back to an empty string for the next iteration of the loop
            continue;
        }

        // if we get here, it means that the search pattern's root and the mount point for this pack are identical
        // which means we may search inside the pack.
        {
            ZipDir::FindFile fd(it->pZip);
            for (fd.FindFirst(remainder); fd.GetFileEntry(); fd.FindNext())
            {
                const char* fname = fd.GetFileName();
                if (fname[0] == '\0')
                {
                    CryFatalError("Empty filename within zip file: '%s'", it->pZip->GetFilePath());
                }
                m_mapFiles.insert(FileMap::value_type(fname, FileDesc(fd.GetFileEntry())));
            }
        }

        {
            ZipDir::FindDir fd(it->pZip);
            for (fd.FindFirst(remainder); fd.GetDirEntry(); fd.FindNext())
            {
                const char* fname = fd.GetDirName();
                if (fname[0] == '\0')
                {
                    CryFatalError("Empty directory name within zip file: '%s'", it->pZip->GetFilePath());
                }
                m_mapFiles.insert(FileMap::value_type(fname, FileDesc()));
            }
        }
    }
}

bool CCryPakFindData::empty() const
{
    return m_mapFiles.empty();
}

bool CCryPakFindData::Fetch(_finddata_t* pfd)
{
    if (m_mapFiles.empty())
    {
        return false;
    }

    FileMap::iterator it = m_mapFiles.begin();
    memcpy(pfd->name, it->first.c_str(), min((uint32)sizeof(pfd->name), (uint32)(it->first.length() + 1)));
    pfd->attrib = it->second.nAttrib;
    pfd->size = it->second.nSize;
    pfd->time_access = it->second.tAccess;
    pfd->time_create = it->second.tCreate;
    pfd->time_write = it->second.tWrite;

    m_mapFiles.erase(it);
    return true;
}

CCryPakFindData::FileDesc::FileDesc(struct _finddata_t* fd)
{
    nSize = fd->size;
    nAttrib = fd->attrib;
    tAccess = fd->time_access;
    tCreate = fd->time_create;
    tWrite = fd->time_write;
}

// the conversions in this function imply that we don't support
// 64-bit file sizes or 64-bit time values
CCryPakFindData::FileDesc::FileDesc(struct __finddata64_t* fd)
{
    nSize = (unsigned)fd->size;
    nAttrib = fd->attrib;
    tAccess = (time_t)fd->time_access;
    tCreate = (time_t)fd->time_create;
    tWrite = (time_t)fd->time_write;
}


CCryPakFindData::FileDesc::FileDesc(ZipDir::FileEntry* fe)
{
    nSize = fe->desc.lSizeUncompressed;
#if defined(LINUX) || defined(APPLE)
    nAttrib = _A_IN_CRYPAK; // files in zip are read-only, and
#else
    nAttrib = _A_RDONLY | _A_IN_CRYPAK; // files in zip are read-only, and
#endif
    tAccess = -1;
    tCreate = -1;
    tWrite = fe->GetModificationTime();
}

CCryPakFindData::FileDesc::FileDesc()
{
    nSize = 0;
#if defined(LINUX) || defined(APPLE)
    nAttrib = _A_SUBDIR;
#else
    nAttrib = _A_SUBDIR | _A_RDONLY;
#endif
    tAccess = -1;
    tCreate = -1;
    tWrite = -1;
}

//! Puts the memory statistics into the given sizer object
//! According to the specifications in interface ICrySizer
void CCryPak::GetMemoryStatistics(ICrySizer* pSizer)
{
    {
        AUTO_READLOCK(m_csZips);
        SIZER_SUBCOMPONENT_NAME(pSizer, "Zips");
        pSizer->AddObject(m_arrZips);
    }

    {
        AUTO_READLOCK(m_csFindData);
        SIZER_SUBCOMPONENT_NAME(pSizer, "FindData");
        pSizer->AddObject(m_setFindData);
    }

    {
        AUTO_READLOCK(m_csCachedFiles);
        SIZER_SUBCOMPONENT_NAME(pSizer, "CachedFiles");
        pSizer->AddObject(m_setCachedFiles);
    }

    {
        AUTO_READLOCK(m_csOpenFiles);
        SIZER_SUBCOMPONENT_NAME(pSizer, "OpenFiles");
        pSizer->AddObject(m_arrOpenFiles);
    }

    {
        SIZER_SUBCOMPONENT_NAME(pSizer, "Resource Lists");

        if (m_pEngineStartupResourceList)
        {
            m_pEngineStartupResourceList->GetMemoryStatistics(pSizer);
        }

        if (m_pLevelResourceList)
        {
            m_pLevelResourceList->GetMemoryStatistics(pSizer);
        }

        if (m_pNextLevelResourceList)
        {
            m_pNextLevelResourceList->GetMemoryStatistics(pSizer);
        }
    }
}

size_t CCryPakFindData::sizeofThis() const
{
    size_t nSize = sizeof(*this);
    for (FileMap::const_iterator it = m_mapFiles.begin(); it != m_mapFiles.end(); ++it)
    {
        nSize += sizeof(FileMap::value_type);
        nSize += it->first.capacity();
    }
    return nSize;
}

void CCryPakFindData::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(m_mapFiles);
}

bool CCryPak::MakeDir(const char* szPathIn, bool bGamePathMapping)
{
    stack_string pathStr = szPathIn;
    // Determine if there is a period ('.') after the last slash to determine if the path contains a file.
    // This used to be a strchr on the whole path which could contain a period in a path, such as network domain paths (domain.user).
    bool bPathContainsFile = false;
    size_t findDotFromPos = pathStr.rfind(g_cNativeSlash);
    if (findDotFromPos == stack_string::npos)
    {
        findDotFromPos = pathStr.rfind(g_cNonNativeSlash);
        if (findDotFromPos == stack_string::npos)
        {
            findDotFromPos = 0;
        }
    }
    size_t dotPos = pathStr.find('.', findDotFromPos);
    if (dotPos != stack_string::npos)
    {
        pathStr = PathUtil::GetPath(stack_string(szPathIn));
    }
    else
    {
        pathStr = szPathIn;
    }

    const char* szPath = pathStr;
    if (0 == szPath[0])
    {
        return true;
    }

    char tempPath[MAX_PATH];
    int nFlagsAdd = (!bGamePathMapping) ? FLAGS_PATH_REAL : 0;
    szPath = AdjustFileName(szPath, tempPath, AZ_ARRAY_SIZE(tempPath), FLAGS_FOR_WRITING | nFlagsAdd);

    char newPath[MAX_PATH];
    char* q = newPath;

    memset(newPath, 0, sizeof(newPath));

    const char* p = szPath;
    bool bUNCPath = false;
    // Check for UNC path.
    if (szPath[0] == g_cNativeSlash && szPath[1] == g_cNativeSlash)
    {
        // UNC path given, Skip first 2 slashes.
        *q++ = *p++;
        *q++ = *p++;
    }

    for (; *p; )
    {
        while (*p != g_cNonNativeSlash && *p != g_cNativeSlash && *p)
        {
            *q++ = *p++;
        }
        // If empty string, nothing to create
        if (*newPath != 0)
        {
            AZ::IO::FileIOBase::GetDirectInstance()->CreatePath(newPath);
        }
        if (*p)
        {
            if (*p != g_cNonNativeSlash)
            {
                *q++ = *p++;
            }
            else
            {
                *q++ = g_cNativeSlash;
                p++;
            }
        }
    }

    return AZ::IO::FileIOBase::GetDirectInstance()->CreatePath(szPath);
}

//////////////////////////////////////////////////////////////////////////
// open the physical archive file - creates if it doesn't exist
// returns NULL if it's invalid or can't open the file
ICryArchive* CCryPak::OpenArchive(
    const char* szPath, const char* bindRoot, unsigned int nFlags, IMemoryBlock* pData)
{
    LOADING_TIME_PROFILE_SECTION_ARGS(szPath);
    PROFILE_DISK_OPEN;
    MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "CryPak (%s)", szPath);

    char szFullPathBuf[CCryPak::g_nMaxPath];

    const char* szFullPath = AdjustFileName(szPath, szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), FLAGS_PATH_REAL | nFlags);

    // if it's simple and read-only, it's assumed it's read-only
    if (nFlags & ICryArchive::FLAGS_OPTIMIZED_READ_ONLY)
    {
        nFlags |= ICryArchive::FLAGS_READ_ONLY;
    }

    unsigned nFactoryFlags = 0;

    if (nFlags & ICryArchive::FLAGS_IN_MEMORY)
    {
        nFactoryFlags |= ZipDir::CacheFactory::FLAGS_IN_MEMORY;
    }

    if (nFlags & ICryArchive::FLAGS_IN_MEMORY_CPU)
    {
        nFactoryFlags |= ZipDir::CacheFactory::FLAGS_IN_MEMORY_CPU;
    }

    if (nFlags & ICryArchive::FLAGS_DONT_COMPACT)
    {
        nFactoryFlags |= ZipDir::CacheFactory::FLAGS_DONT_COMPACT;
    }

    if (nFlags & ICryArchive::FLAGS_READ_ONLY)
    {
        nFactoryFlags |= ZipDir::CacheFactory::FLAGS_READ_ONLY;
    }

    if (nFlags & ICryArchive::FLAGS_CREATE_NEW)
    {
        nFactoryFlags |= ZipDir::CacheFactory::FLAGS_CREATE_NEW;
    }

    ICryArchive* pArchive = FindArchive(szFullPath);
    if (pArchive)
    {
        // check for compatibility
        if (!(nFlags& ICryArchive::FLAGS_RELATIVE_PATHS_ONLY) && (pArchive->GetFlags() & ICryArchive::FLAGS_RELATIVE_PATHS_ONLY))
        {
            pArchive->ResetFlags(ICryArchive::FLAGS_RELATIVE_PATHS_ONLY);
        }

        // we found one
        if (nFlags & ICryArchive::FLAGS_OPTIMIZED_READ_ONLY)
        {
            if (pArchive->GetClassId() == CryArchive::gClassId)
            {
                return pArchive; // we can return an optimized archive
            }
            //if (!(pArchive->GetFlags() & ICryArchive::FLAGS_READ_ONLY))
            return NULL; // we can't let it open read-only optimized while it's open for RW access
        }
        else
        {
            if (!(nFlags& ICryArchive::FLAGS_READ_ONLY) && (pArchive->GetFlags() & ICryArchive::FLAGS_READ_ONLY))
            {
                // we don't support upgrading from ReadOnly to ReadWrite
                return NULL;
            }

            return pArchive;
        }

        return NULL;
    }

    string strBindRoot;

    // if no bind root is specified, compute one:
    if (!bindRoot)
    {
        strBindRoot = PathUtil::GetParentDirectory(szFullPath);
    }
    else
    {
        strBindRoot = bindRoot;
    }

    // Check if file on disk exist.
    bool bFileExists = false;

    // if valid data is given and read only, then don't check if file excists
    if (pData && nFlags & ICryArchive::FLAGS_OPTIMIZED_READ_ONLY)
    {
        bFileExists = true;
    }
    else
    {
        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        if (AZ::IO::FileIOBase::GetDirectInstance()->Open(szFullPath, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, fileHandle))
        {
            AZ::IO::FileIOBase::GetDirectInstance()->Close(fileHandle);
            bFileExists = true;
        }
    }

    if (!bFileExists && (nFactoryFlags & ZipDir::CacheFactory::FLAGS_READ_ONLY))
    {
        // Pak file not found.
        if (m_pPakVars->nWarnOnPakAccessFails != 0)
        {
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Cannot open Pak file %s", szFullPath);
        }
        else
        {
            CryLog("Cannot open Pak file %s", szFullPath);
        }
        return NULL;
    }

    ZipDir::CacheFactory factory(g_pPakHeap, ZipDir::ZD_INIT_FAST, nFactoryFlags);
    if (nFlags & ICryArchive::FLAGS_OPTIMIZED_READ_ONLY)
    {
        ZipDir::CachePtr cache = 0;
        if (pData)
        {
            cache = factory.New(szFullPath, pData);
        }
        else
        {
            EInMemoryPakLocation eMemLocale = eInMemoryPakLocale_Unload;
            if (nFactoryFlags & ZipDir::CacheFactory::FLAGS_IN_MEMORY_CPU)
            {
                eMemLocale = eInMemoryPakLocale_CPU;
            }
            if (nFactoryFlags & ZipDir::CacheFactory::FLAGS_IN_MEMORY)
            {
                eMemLocale = eInMemoryPakLocale_GPU;
            }
            cache = factory.New(szFullPath, eMemLocale);
        }

        if (cache)
        {
            return new CryArchive(this, strBindRoot, cache, nFlags);
        }
    }
    else
    {
#ifndef OPTIMIZED_READONLY_ZIP_ENTRY
        ZipDir::CacheRWPtr cache = factory.NewRW(szFullPath);
        if (cache)
        {
            return new CryArchiveRW(this, strBindRoot, cache, nFlags);
        }
#else
        return 0;
#endif
    }

    return 0;
}

uint32 CCryPak::ComputeCRC(const char* szPath, uint32 nFileOpenFlags)
{
    assert(szPath);

    uint32 dwCRC = 0;

    // generate crc32
    {
        // avoid heap allocation by working in 8k chunks
        const uint32 dwChunkSize = 1024 * 8;

        // note that the actual CRC algorithm can work on various sized words but operates on individual words
        // so there's little difference between feeding it 8k and 8mb, except you might save yourself some io calls.

        uint8 pMem[dwChunkSize];

        uint32 dwSize = 0;
        PROFILE_DISK_OPEN;

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            return ZipDir::ZD_ERROR_INVALID_CALL;
        }

        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;

        if (!fileIO->Open(szPath, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, fileHandle))
        {
            return ZipDir::ZD_ERROR_INVALID_PATH;
        }
        // load whole file in chunks and compute CRC
        while (true)
        {
            AZ::u64 bytesRead = 0;
            fileIO->Read(fileHandle, pMem, dwChunkSize, false, &bytesRead); // read up to ChunkSize bytes and put the actual number of bytes read into bytesRead.

            if (bytesRead)
            {
                dwCRC = crc32(dwCRC, pMem, aznumeric_caster(bytesRead));
            }
            else
            {
                break;
            }
        }

        gEnv->pCryPak->FClose(fileHandle);
    }

    return dwCRC;
}

bool CCryPak::ComputeMD5(const char* szPath, unsigned char* md5, uint32 nFileOpenFlags, bool useDirectFileAccess)
{
    if (!szPath || !md5)
    {
        return false;
    }

    MD5Context context;
    MD5Init(&context);

    // generate checksum
    {
        const AZ::u64 dwChunkSize = 1024 * 1024;     // 1MB chunks
        uint8* pMem = new uint8[dwChunkSize];

        if (!pMem)
        {
            return false;
        }

        AZ::u64 dwSize = 0;

        PROFILE_DISK_OPEN;
        if(useDirectFileAccess)
        {
            AZ::IO::FileIOBase::GetDirectInstance()->Size(szPath, dwSize);
        }
        else
        {
            AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(szPath, "rb", nFileOpenFlags);

            if (fileHandle != AZ::IO::InvalidHandle)
            {
                dwSize = gEnv->pCryPak->FGetSize(fileHandle);
                gEnv->pCryPak->FClose(fileHandle);
            }
        }

        // rbx open flags, x is a hint to not cache whole file in memory.
        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        if (useDirectFileAccess)
        {
            AZ::IO::FileIOBase::GetDirectInstance()->Open(szPath, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, fileHandle);
        }
        else
        {
            fileHandle = gEnv->pCryPak->FOpen(szPath, "rbx", nFileOpenFlags);
        }

        if (fileHandle == AZ::IO::InvalidHandle)
        {
            delete[]pMem;
            return false;
        }

        // load whole file in chunks and compute Md5
        while (dwSize > 0)
        {
            uint32 dwLocalSize = min(dwSize, dwChunkSize);

            AZ::u64 read{ 0 };
            if (useDirectFileAccess)
            {
                AZ::IO::FileIOBase::GetDirectInstance()->Read(fileHandle, pMem, dwLocalSize, false, &read);
            }
            else
            {
                read = gEnv->pCryPak->FReadRaw(pMem, 1, dwLocalSize, fileHandle);
            }
            assert(read == dwLocalSize);

            MD5Update(&context, pMem, dwLocalSize);
            dwSize -= dwLocalSize;
        }

        delete[]pMem;

        if (useDirectFileAccess)
        {
            AZ::IO::FileIOBase::GetDirectInstance()->Close(fileHandle);
        }
        else
        {
            gEnv->pCryPak->FClose(fileHandle);
        }
    }

    MD5Final(md5, &context);
    return true;
}

int CCryPak::ComputeCachedPakCDR_CRC(const char* filename, bool useCryFile /*=true*/, IMemoryBlock* pData /*=NULL*/)
{
    uint32 offset = 0;
    uint32 size = 0;
    int retval = 0;

    //get Central Directory Record position and length
    gEnv->pCryPak->GetCachedPakCDROffsetSize(filename, offset, size);

    CCryFile file;
    AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;

    uint32 dwCRC = 0;

    if (pData)
    {
        const uint8* pMem = static_cast<const uint8*>(pData->GetData());
        CRY_ASSERT(pMem);
        const uint8* pCDR = &pMem[offset];
        dwCRC = crc32(dwCRC, pCDR, size);
    }
    else
    {
        if (useCryFile)
        {
            file.Open(filename, "rb", ICryPak::FOPEN_ONDISK);
            file.Seek(offset, 0);

            CryLog("CRC: Used CryFile method, fileHandle is 0x%u", file.GetHandle());
        }
        else
        {
            AZ::IO::FileIOBase::GetDirectInstance()->Open(filename, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, fileHandle);
            AZ::IO::FileIOBase::GetDirectInstance()->Seek(fileHandle, offset, AZ::IO::SeekType::SeekFromStart);

            CryLog("CRC: Used CIO method, fileHandle is 0x%u", fileHandle);
        }


        const uint32 dwChunkSize = 1024 * 1024;     // 1MB chunks
        uint8* pMem = new uint8[dwChunkSize];

        if (!pMem)
        {
            return ZipDir::ZD_ERROR_NO_MEMORY;
        }

        uint32 dwSize = size;

        // load Central Directory Record in chunks and compute CRC
        while (dwSize > 0)
        {
            uint32 dwLocalSize = min(dwSize, dwChunkSize);

            size_t read = 0;
            if (useCryFile)
            {
                read = file.ReadRaw(pMem, dwLocalSize);
            }
            else
            {
                AZ::u64 bytesRead = 0;
                AZ::IO::FileIOBase::GetDirectInstance()->Read(fileHandle, pMem, dwLocalSize, false, &bytesRead);
                read = static_cast<size_t>(bytesRead);
            }

            assert(read == dwLocalSize);

            dwCRC = crc32(dwCRC, pMem, dwLocalSize);
            dwSize -= dwLocalSize;
        }


        delete[]pMem;

        if (useCryFile)
        {
            file.Close();
        }
        else
        {
            AZ::IO::FileIOBase::GetDirectInstance()->Close(fileHandle);
        }
    }

    return dwCRC;
}

void CCryPak::Register(ICryArchive* pArchive)
{
    AUTO_MODIFYLOCK(m_csZips);
    ArchiveArray::iterator it = std::lower_bound(m_arrArchives.begin(), m_arrArchives.end(), pArchive, CryArchiveSortByName());
    m_arrArchives.insert(it, pArchive);
}

void CCryPak::Unregister(ICryArchive* pArchive)
{
    AUTO_MODIFYLOCK(m_csZips);
    if (pArchive)
    {
        CryComment("Closing PAK file: %s", pArchive->GetFullPath());
    }
    ArchiveArray::iterator it;
    if (m_arrArchives.size() < 16)
    {
        // for small array sizes, we'll use linear search
        it = std::find(m_arrArchives.begin(), m_arrArchives.end(), pArchive);
    }
    else
    {
        it = std::lower_bound(m_arrArchives.begin(), m_arrArchives.end(), pArchive, CryArchiveSortByName());
    }

    if (it != m_arrArchives.end() && *it == pArchive)
    {
        m_arrArchives.erase(it);
    }
    else
    {
        assert(0); // unregistration of unregistered archive
    }
}

ICryArchive* CCryPak::FindArchive(const char* szFullPath)
{
    AUTO_READLOCK(m_csZips);
    ArchiveArray::iterator it = std::lower_bound(m_arrArchives.begin(), m_arrArchives.end(), szFullPath, CryArchiveSortByName());
    if (it != m_arrArchives.end() && !azstricmp(szFullPath, (*it)->GetFullPath()))
    {
        return *it;
    }
    else
    {
        return NULL;
    }
}

const ICryArchive* CCryPak::FindArchive(const char* szFullPath) const
{
    AUTO_READLOCK(m_csZips);
    ArchiveArray::const_iterator it = std::lower_bound(m_arrArchives.begin(), m_arrArchives.end(), szFullPath, CryArchiveSortByName());
    if (it != m_arrArchives.end() && !azstricmp(szFullPath, (*it)->GetFullPath()))
    {
        return *it;
    }
    else
    {
        return NULL;
    }
}


// compresses the raw data into raw data. The buffer for compressed data itself with the heap passed. Uses method 8 (deflate)
// returns one of the Z_* errors (Z_OK upon success)
// MT-safe
int CCryPak::RawCompress(const void* pUncompressed, unsigned long* pDestSize, void* pCompressed, unsigned long nSrcSize, int nLevel)
{
    return ZipDir::ZipRawCompress(g_pPakHeap, pUncompressed, pDestSize, pCompressed, nSrcSize, nLevel);
}

// Uncompresses raw (without wrapping) data that is compressed with method 8 (deflated) in the Zip file
// returns one of the Z_* errors (Z_OK upon success)
// This function just mimics the standard uncompress (with modification taken from unzReadCurrentFile)
// with 2 differences: there are no 16-bit checks, and
// it initializes the inflation to start without waiting for compression method byte, as this is the
// way it's stored into zip file
int CCryPak::RawUncompress(void* pUncompressed, unsigned long* pDestSize, const void* pCompressed, unsigned long nSrcSize)
{
    return ZipDir::ZipRawUncompress(g_pPakHeap, pUncompressed, pDestSize, pCompressed, nSrcSize);
}


// returns the current game directory, with trailing slash (or empty string if it's right in MasterCD)
// this is used to support Resource Compiler which doesn't have access to this interface:
// in case all the contents is located in a subdirectory of MasterCD, this string is the subdirectory name with slash
/*const char* CCryPak::GetGameDir()
{
return m_strPrepend.c_str();
}
*/

//////////////////////////////////////////////////////////////////////////
void CCryPak::RecordFileOpen(ERecordFileOpenList eList)
{
    m_eRecordFileOpenList = eList;

    //CryLog("RecordFileOpen(%d)",(int)eList);

    switch (m_eRecordFileOpenList)
    {
    case RFOM_Disabled:
    case RFOM_EngineStartup:
    case RFOM_Level:
        break;

    case RFOM_NextLevel:
    default:
        assert(0);
    }
}

//////////////////////////////////////////////////////////////////////////
ICryPak::ERecordFileOpenList CCryPak::GetRecordFileOpenList()
{
    return m_eRecordFileOpenList;
}

//////////////////////////////////////////////////////////////////////////
IResourceList* CCryPak::GetResourceList(ERecordFileOpenList eList)
{
    switch (eList)
    {
    case RFOM_EngineStartup:
        return m_pEngineStartupResourceList;
    case RFOM_Level:
        return m_pLevelResourceList;
    case RFOM_NextLevel:
        return m_pNextLevelResourceList;

    case RFOM_Disabled:
    default:
        assert(0);
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CCryPak::SetResourceList(ERecordFileOpenList eList, IResourceList* pResourceList)
{
    switch (eList)
    {
    case RFOM_EngineStartup:
        m_pEngineStartupResourceList = pResourceList;
        break;
    case RFOM_Level:
        m_pLevelResourceList = pResourceList;
        break;
    case RFOM_NextLevel:
        m_pNextLevelResourceList = pResourceList;
        break;

    case RFOM_Disabled:
    default:
        assert(0);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCryPak::RecordFile(AZ::IO::HandleType inFileHandle, const char* szFilename)
{
    //if (m_pLog)
    //      CryComment( "File open: %s",szFilename );

    if (m_eRecordFileOpenList != ICryPak::RFOM_Disabled)
    {
        // we only want to record ASSET access
        // assets are identified as things which start with no alias, or with the @assets@ alias
        stack_string assetPath = szFilename;
        AZ::IO::FileIOBase::GetInstance()->ConvertToAlias(assetPath.m_str, AZ_MAX_PATH_LEN);
        if ((assetPath.c_str()[0] != '@') || (_strnicmp(assetPath.c_str(), "@assets@", 8) == 0))
        {
            IResourceList* pList = GetResourceList(m_eRecordFileOpenList);

            if (pList)
            {
                pList->Add(szFilename);
            }
        }
    }

    std::vector<ICryPakFileAcesssSink*>::iterator it, end = m_FileAccessSinks.end();

    for (it = m_FileAccessSinks.begin(); it != end; ++it)
    {
        ICryPakFileAcesssSink* pSink = *it;

        pSink->ReportFileOpen(inFileHandle, szFilename);
    }
}


void CCryPak::OnMissingFile(const char* szPath)
{
    AUTO_LOCK_CS(m_csMissingFiles);
    if (m_pPakVars->nLogMissingFiles)
    {
        std::pair<MissingFileMap::iterator, bool> insertion = m_mapMissingFiles.insert(MissingFileMap::value_type(szPath, 1));
        if (m_pPakVars->nLogMissingFiles >= 2 && (insertion.second || m_pPakVars->nLogMissingFiles >= 3))
        {
            // insertion occured
            char szFileName[64];
            sprintf_s(szFileName, "MissingFiles%d.log", m_pPakVars->nLogMissingFiles);
            AZ::IO::HandleType fileHandle;
            if (AZ::IO::FileIOBase::GetDirectInstance()->Open(szFileName, AZ::IO::OpenMode::ModeAppend, fileHandle))
            {
                AZ::IO::Print(fileHandle, "%s\n", szPath);
                AZ::IO::FileIOBase::GetDirectInstance()->Close(fileHandle);
            }
        }
        else
        {
            ++insertion.first->second;  // increase the count of missing files
        }
    }
}

bool CCryPak::DisableRuntimeFileAccess(bool status, threadID threadId)
{
    bool prev = false;
    if (threadId == m_mainThreadId)
    {
        prev = m_disableRuntimeFileAccess[0];
        m_disableRuntimeFileAccess[0] = status;
    }
    else if (threadId == m_renderThreadId)
    {
        prev = m_disableRuntimeFileAccess[1];
        m_disableRuntimeFileAccess[1] = status;
    }
    return prev;
}

static char* cry_strdup(const char* szSource)
{
    size_t len = strlen(szSource);
    char* szResult = (char*)malloc(len + 1);
    memcpy(szResult, szSource, len + 1);
    return szResult;
}


ICryPak::PakInfo* CCryPak::GetPakInfo()
{
    AUTO_READLOCK(m_csZips);
    PakInfo* pResult = (PakInfo*)malloc(sizeof(PakInfo) + sizeof(PakInfo::Pak) * m_arrZips.size());
    assert(pResult);
    pResult->numOpenPaks = m_arrZips.size();
    for (unsigned i = 0; i < m_arrZips.size(); ++i)
    {
        pResult->arrPaks[i].szBindRoot = cry_strdup(m_arrZips[i].strBindRoot.c_str());
        pResult->arrPaks[i].szFilePath = cry_strdup(m_arrZips[i].GetFullPath());
        pResult->arrPaks[i].nUsedMem = m_arrZips[i].sizeofThis();
    }
    return pResult;
}

//////////////////////////////////////////////////////////////////////////
void CCryPak::FreePakInfo(PakInfo* pPakInfo)
{
    for (unsigned i = 0; i < pPakInfo->numOpenPaks; ++i)
    {
        free((void*)pPakInfo->arrPaks[i].szBindRoot);
        free((void*)pPakInfo->arrPaks[i].szFilePath);
    }
    free(pPakInfo);
}

//////////////////////////////////////////////////////////////////////////
void CCryPak::RegisterFileAccessSink(ICryPakFileAcesssSink* pSink)
{
    assert(pSink);

    if (std::find(m_FileAccessSinks.begin(), m_FileAccessSinks.end(), pSink) != m_FileAccessSinks.end())
    {
        // was already registered
        assert(0);
        return;
    }

    m_FileAccessSinks.push_back(pSink);
}


//////////////////////////////////////////////////////////////////////////
void CCryPak::UnregisterFileAccessSink(ICryPakFileAcesssSink* pSink)
{
    assert(pSink);

    std::vector<ICryPakFileAcesssSink*>::iterator it = std::find(m_FileAccessSinks.begin(), m_FileAccessSinks.end(), pSink);

    if (it == m_FileAccessSinks.end())
    {
        // was not there
        assert(0);
        return;
    }

    m_FileAccessSinks.erase(it);
}

//////////////////////////////////////////////////////////////////////////
bool CCryPak::RemoveFile(const char* pName)
{
    char szFullPathBuf[g_nMaxPath];
    const char* szFullPath = AdjustFileName(pName, szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), 0);

    char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
    AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(szFullPathBuf, resolvedPath, AZ_MAX_PATH_LEN);

    uint32 dwAttr = CryGetFileAttributes(resolvedPath);
    bool ok = false;
    if (dwAttr != INVALID_FILE_ATTRIBUTES && (dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
#ifdef WIN32
        SetFileAttributes(resolvedPath, FILE_ATTRIBUTE_NORMAL);
#endif
        ok = AZ::IO::FileIOBase::GetDirectInstance()->Remove(szFullPath) == AZ::IO::ResultCode::Success;
    }
    return ok;
}

//////////////////////////////////////////////////////////////////////////
bool CCryPak::RemoveDir(const char* pName)
{
    char szFullPathBuf[g_nMaxPath];
    const char* szFullPath = AdjustFileName(pName, szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), 0);

    if (AZ::IO::FileIOBase::GetDirectInstance()->IsDirectory(szFullPath))
    {
        AZ::IO::FileIOBase::GetDirectInstance()->DestroyPath(szFullPath);
        return true;
    }
    return false;
}

ILINE bool IsDirSep(const char c)
{
    return (c == CCryPak::g_cNativeSlash || c == CCryPak::g_cNonNativeSlash);
}

//////////////////////////////////////////////////////////////////////////
bool CCryPak::IsAbsPath(const char* pPath)
{
    return (pPath && ((pPath[0] && pPath[1] == ':' && IsDirSep(pPath[2]))
                      || IsDirSep(pPath[0])
                      )
            );
}

CCryPakFindData* CCryPak::CreateFindData()
{
    return new CCryPakFindData();
}

//////////////////////////////////////////////////////////////////////////
void CCryPak::SetLog(IMiniLog* pLog)
{
    m_pLog = pLog;


    std::vector<string>::iterator stringVecIt;
    for (stringVecIt = m_arrMods.begin(); stringVecIt != m_arrMods.end(); ++stringVecIt)
    {
        string& sMOD = *stringVecIt;
        m_pLog->Log("Added MOD directory <%s> to CryPak", sMOD.c_str());
    }
}

void* CCryPak::PoolMalloc(size_t size)
{
    return g_pPakHeap->TempAlloc(size, "CCryPak::GetPoolRealloc");
}

void CCryPak::PoolFree(void* p)
{
    g_pPakHeap->FreeTemporary(p);
}

void CCryPak::Lock()
{
    m_csMain.LockModify();
}

void CCryPak::Unlock()
{
    m_csMain.UnlockModify();
}

// gets the current pak priority
int CCryPak::GetPakPriority()
{
    return m_pPakVars->nPriority;
}

//////////////////////////////////////////////////////////////////////////
class CFilePoolMemoryBlock
    : public IMemoryBlock
{
public:
    virtual void* GetData() { return m_pData; }
    virtual int GetSize() { return m_nSize; }

    virtual ~CFilePoolMemoryBlock()
    {
        if (m_pData)
        {
            g_pPakHeap->FreeTemporary(m_pData);
        }
    }

    CFilePoolMemoryBlock(size_t nSize, const char* sUsage, size_t nAlign)
    {
        m_sUsage = sUsage;
        m_pData = g_pPakHeap->TempAlloc(nSize, m_sUsage.c_str(), (uint32)nAlign);
        m_nSize = nSize;
    }

private:
    string m_sUsage;
    void* m_pData;
    size_t m_nSize;
};

IMemoryBlock* CCryPak::PoolAllocMemoryBlock(size_t nSize, const char* sUsage, size_t nAlign)
{
    return new CFilePoolMemoryBlock(nSize, sUsage, nAlign);
}

//////////////////////////////////////////////////////////////////////////
void CCryPak::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_LEVEL_LOAD_START:
        m_fFileAcessTime = 0;
        break;
    case ESYSTEM_EVENT_LEVEL_LOAD_END:
    {
        // Log used time.
        CryLog("File access time during level loading: %.2f seconds", m_fFileAcessTime);
        m_fFileAcessTime = 0;
    }
    break;

    case ESYSTEM_EVENT_GAME_POST_INIT:
        break;

    case ESYSTEM_EVENT_LEVEL_LOAD_PREPARE:
        break;

    case ESYSTEM_EVENT_LEVEL_PRECACHE_START:
        break;

    case ESYSTEM_EVENT_LEVEL_PRECACHE_END:
        break;
    }
}

// return offset in pak file (ideally has to return offset on DVD)
uint64 CCryPak::GetFileOffsetOnMedia(const char* sFilename)
{
    char szFullPathBuf[g_nMaxPath];
    const char* szFullPath = AdjustFileName(sFilename, szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), FOPEN_HINT_QUIET);

    assert(szFullPath);
    if (!szFullPath)
    {
        return 0;
    }

    ZipDir::CachePtr pZip = 0;
    unsigned int nArchiveFlags;
    ZipDir::FileEntry* pFileEntry = FindPakFileEntry(szFullPath, nArchiveFlags, &pZip, false);
    if (!pFileEntry)
    {
        return 0;
    }
    return pZip->GetPakFileOffsetOnMedia() + (uint64)pFileEntry->nFileDataOffset;
}

EStreamSourceMediaType CCryPak::GetFileMediaType(const char* szName)
{
    char szFullPathBuf[g_nMaxPath];
    const char* szFullPath = gEnv->pCryPak->AdjustFileName(szName, szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), ICryPak::FOPEN_HINT_QUIET);

    ZipDir::CachePtr pZip = 0;
    unsigned int archFlags;
    ZipDir::FileEntry* pFileEntry = FindPakFileEntry(szFullPath, archFlags, &pZip, false);

    return GetMediaType(pZip, archFlags);
}

EStreamSourceMediaType CCryPak::GetMediaType(ZipDir::Cache* pZip, unsigned int nArchiveFlags)
{
    EStreamSourceMediaType mediaType = eStreamSourceTypeDisc;

    // Highest priority for files that are in memory already and can be loaded very fast.
    if (pZip)
    {
        if (pZip->IsInMemory())
        {
            mediaType = eStreamSourceTypeMemory;
        }
        else if (IsInstalledToHDD() || (nArchiveFlags & ICryArchive::FLAGS_ON_HDD))
        {
            mediaType = eStreamSourceTypeHDD;
        }
    }
    else
    {
        if (IsInstalledToHDD())
        {
            mediaType = eStreamSourceTypeHDD;
        }
    }

    return mediaType;
}

//////////////////////////////////////////////////////////////////////////
ICustomMemoryHeap* CCryPak::GetInMemoryPakHeap()
{
    if (!m_pInMemoryPaksCPUHeap)
    {
        m_pInMemoryPaksCPUHeap = new CCustomMemoryHeap(IMemoryManager::eapPageMapped);
    }

    return m_pInMemoryPaksCPUHeap;
}

bool CCryPak::SetPacksAccessible(bool bAccessible, const char* pWildcard, unsigned nFlags)
{
    char cWorkBuf[g_nMaxPath];
    const char* cWork = AdjustFileName(pWildcard, cWorkBuf, AZ_ARRAY_SIZE(cWorkBuf), nFlags);
    string searchDirectory = PathUtil::GetPath(cWork);
    string pattern = PathUtil::GetFile(cWork);

    return AZ::IO::FileIOBase::GetDirectInstance()->FindFiles(searchDirectory, pattern, [&](const char* filePath) -> bool
        {
            SetPackAccessible(bAccessible, filePath, nFlags);
            return true;
        });
}

bool CCryPak::SetPackAccessible(bool bAccessible, const char* pName, unsigned nFlags)
{
    char szZipPathBuf[g_nMaxPath];
    const char* szZipPath = AdjustFileName(pName, szZipPathBuf, AZ_ARRAY_SIZE(szZipPathBuf), nFlags);

    AUTO_MODIFYLOCK(m_csZips);
    for (ZipArray::iterator it = m_arrZips.begin(); it != m_arrZips.end(); ++it)
    {
        if (!azstricmp(szZipPath, it->GetFullPath()))
        {
            return it->pArchive->SetPackAccessible(bAccessible);
        }
    }

    return true;
}

void CCryPak::SetPacksAccessibleForLevel(const char* sLevelName)
{
    LOADING_TIME_PROFILE_SECTION;
    IPlatformOS* pPlatform = gEnv->pSystem->GetPlatformOS();

    AUTO_MODIFYLOCK(m_csZips);
    for (ZipArray::iterator it = m_arrZips.begin(); it != m_arrZips.end(); ++it)
    {
        const bool bRequired = m_pPakVars->nDisableNonLevelRelatedPaks ? pPlatform->IsPakRequiredForLevel(it->GetFullPath(), sLevelName) : true;
        it->pArchive->SetPackAccessible(bRequired);
    }
}

void CCryPak::CreatePerfHUDWidget()
{
    if (m_pWidget == NULL)
    {
        ICryPerfHUD* pPerfHUD = gEnv->pSystem->GetPerfHUD();

        if (pPerfHUD)
        {
            minigui::IMiniCtrl* pRenderMenu = pPerfHUD->GetMenu("System");

            if (pRenderMenu)
            {
                m_pWidget = new CPakFileWidget(pRenderMenu, pPerfHUD, this);
            }
        }
    }
}

//PerfHUD widget for pak file info
CCryPak::CPakFileWidget::CPakFileWidget(minigui::IMiniCtrl* pParentMenu, ICryPerfHUD* pPerfHud, CCryPak* pPak)
    : ICryPerfHUDWidget(eWidget_PakFile)
{
    m_pPak = pPak;

    m_pTable = pPerfHud->CreateTableMenuItem(pParentMenu, "Pak Files");

    if (m_pTable)
    {
        m_pTable->AddColumn("Path");
        m_pTable->AddColumn("BindRoot");
        m_pTable->AddColumn("In Mem");
        m_pTable->AddColumn("Override");
        m_pTable->Hide(true);
    }
    pPerfHud->AddWidget(this);
}

void CCryPak::CPakFileWidget::Update()
{
    const ColorB colNormal(255, 255, 255, 255);
    const ColorB colDisabled(128, 128, 128, 255);
    m_pTable->ClearTable();

    // try to find this - maybe the pack has already been opened
    for (ZipArray::reverse_iterator it = m_pPak->m_arrZips.rbegin(); it != m_pPak->m_arrZips.rend(); ++it)
    {
        const ColorB& col = (it->pArchive->GetFlags() & ICryArchive::FLAGS_DISABLE_PAK) ? colDisabled : colNormal;

        m_pTable->AddData(0, col, it->pZip->GetFilePath());
        m_pTable->AddData(1, col, it->strBindRoot.c_str());
        m_pTable->AddData(2, col, it->pZip->IsInMemory() ? "true" : "false");
        m_pTable->AddData(3, col, (it->pArchive->GetFlags() & ICryArchive::FLAGS_OVERRIDE_PAK) ? "true" : "false");
    }
}



