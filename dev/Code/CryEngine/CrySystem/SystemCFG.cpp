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

// Description : handles system cfg


#include "StdAfx.h"
#include "System.h"
#include <time.h>
#include "XConsole.h"
#include "CryFile.h"
#include <AzCore/std/any.h>

#include <IScriptSystem.h>
#include "SystemCFG.h"
#if defined(LINUX) || defined(APPLE)
#include <Version.h>
#include "ILog.h"

#endif

#ifndef EXE_VERSION_INFO_0
#define EXE_VERSION_INFO_0 1
#endif

#ifndef EXE_VERSION_INFO_1
#define EXE_VERSION_INFO_1 0
#endif

#ifndef EXE_VERSION_INFO_2
#define EXE_VERSION_INFO_2 0
#endif

#ifndef EXE_VERSION_INFO_3
#define EXE_VERSION_INFO_3 1
#endif


//////////////////////////////////////////////////////////////////////////
const SFileVersion& CSystem::GetFileVersion()
{
    return m_fileVersion;
}

//////////////////////////////////////////////////////////////////////////
const SFileVersion& CSystem::GetProductVersion()
{
    return m_productVersion;
}

//////////////////////////////////////////////////////////////////////////
const SFileVersion& CSystem::GetBuildVersion()
{
    return m_buildVersion;
}

//////////////////////////////////////////////////////////////////////////
#ifndef _RELEASE
void CSystem::SystemVersionChanged(ICVar* pCVar)
{
    if (CSystem* pThis = static_cast<CSystem*>(gEnv->pSystem))
    {
        pThis->SetVersionInfo(pCVar->GetString());
    }
}

void CSystem::SetVersionInfo(const char* const szVersion)
{
    m_fileVersion.Set(szVersion);
    m_productVersion.Set(szVersion);
    m_buildVersion.Set(szVersion);
    CryLog("SetVersionInfo '%s'", szVersion);
    CryLog("FileVersion: %d.%d.%d.%d", m_fileVersion.v[3], m_fileVersion.v[2], m_fileVersion.v[1], m_fileVersion.v[0]);
    CryLog("ProductVersion: %d.%d.%d.%d", m_productVersion.v[3], m_productVersion.v[2], m_productVersion.v[1], m_productVersion.v[0]);
    CryLog("BuildVersion: %d.%d.%d.%d", m_buildVersion.v[3], m_buildVersion.v[2], m_buildVersion.v[1], m_buildVersion.v[0]);
}
#endif // #ifndef _RELEASE

//////////////////////////////////////////////////////////////////////////
void CSystem::QueryVersionInfo()
{
#ifndef WIN32
    //do we need some other values here?
    m_fileVersion.v[0] = m_productVersion.v[0] = EXE_VERSION_INFO_3;
    m_fileVersion.v[1] = m_productVersion.v[1] = EXE_VERSION_INFO_2;
    m_fileVersion.v[2] = m_productVersion.v[2] = EXE_VERSION_INFO_1;
    m_fileVersion.v[3] = m_productVersion.v[3] = EXE_VERSION_INFO_0;
    m_buildVersion = m_fileVersion;
#else  //WIN32
    char moduleName[_MAX_PATH];
    DWORD dwHandle;
    UINT len;

    char ver[1024 * 8];

    GetModuleFileName(NULL, moduleName, _MAX_PATH);  //retrieves the PATH for the current module

#ifdef AZ_MONOLITHIC_BUILD
    GetModuleFileName(NULL, moduleName, _MAX_PATH);  //retrieves the PATH for the current module
#else // AZ_MONOLITHIC_BUILD
    strcpy(moduleName, "CrySystem.dll"); // we want to version from the system dll
#endif // AZ_MONOLITHIC_BUILD

    int verSize = GetFileVersionInfoSize(moduleName, &dwHandle);
    if (verSize > 0)
    {
        GetFileVersionInfo(moduleName, dwHandle, 1024 * 8, ver);
        VS_FIXEDFILEINFO* vinfo;
        VerQueryValue(ver, "\\", (void**)&vinfo, &len);

        const uint32 verIndices[4] = {0, 1, 2, 3};
        m_fileVersion.v[verIndices[0]] = m_productVersion.v[verIndices[0]] = vinfo->dwFileVersionLS & 0xFFFF;
        m_fileVersion.v[verIndices[1]] = m_productVersion.v[verIndices[1]] = vinfo->dwFileVersionLS >> 16;
        m_fileVersion.v[verIndices[2]] = m_productVersion.v[verIndices[2]] = vinfo->dwFileVersionMS & 0xFFFF;
        m_fileVersion.v[verIndices[3]] = m_productVersion.v[verIndices[3]] = vinfo->dwFileVersionMS >> 16;
        m_buildVersion = m_fileVersion;

        struct LANGANDCODEPAGE
        {
            WORD wLanguage;
            WORD wCodePage;
        }* lpTranslate;

        UINT count = 0;
        char path[256];
        char* version = NULL;

        VerQueryValue(ver, "\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &count);
        if (lpTranslate != NULL)
        {
            _snprintf(path, sizeof(path), "\\StringFileInfo\\%04x%04x\\InternalName", lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
            VerQueryValue(ver, path, (LPVOID*)&version, &count);
            if (version)
            {
                m_buildVersion.Set(version);
            }
        }
    }
#endif //WIN32
}

//////////////////////////////////////////////////////////////////////////
void CSystem::LogVersion()
{
    // Get time.
    time_t ltime;
    time(&ltime);
    tm* today = localtime(&ltime);

    char s[1024];


    strftime(s, 128, "%d %b %y (%H %M %S)", today);

    const SFileVersion& ver = GetFileVersion();

    CryLogAlways("BackupNameAttachment=\" Build(%d) %s\"  -- used by backup system\n", ver.v[0], s);          // read by CreateBackupFile()

    // Use strftime to build a customized time string.
    strftime(s, 128, "Log Started at %c", today);
    CryLogAlways(s);

    CryLogAlways("Built on " __DATE__ " " __TIME__);

#if   defined(ANDROID)
    CryLogAlways("Running 32 bit Android version API VER:%d", __ANDROID_API__);
#elif defined(IOS)
    CryLogAlways("Running 64 bit iOS version");
#elif defined(APPLETV)
    CryLogAlways("Running 64 bit AppleTV version");
#elif defined(WIN64)
    CryLogAlways("Running 64 bit Windows version");
#elif defined(WIN32)
    CryLogAlways("Running 32 bit Windows version");
#elif defined(LINUX64)
    CryLogAlways("Running 64 bit Linux version");
#elif defined(LINUX32)
    CryLogAlways("Running 32 bit Linux version");
#elif defined(MAC)
    CryLogAlways("Running 64 bit Mac version");
#endif
#if AZ_LEGACY_CRYSYSTEM_TRAIT_SYSTEMCFG_MODULENAME
    GetModuleFileName(NULL, s, sizeof(s));
    CryLogAlways("Executable: %s", s);
#endif

    CryLogAlways("FileVersion: %d.%d.%d.%d", m_fileVersion.v[3], m_fileVersion.v[2], m_fileVersion.v[1], m_fileVersion.v[0]);
#if defined(LY_BUILD)
    CryLogAlways("ProductVersion: %d.%d.%d.%d - Build %d", m_productVersion.v[3], m_productVersion.v[2], m_productVersion.v[1], m_productVersion.v[0], LY_BUILD);
#else // defined(LY_BUILD)
    CryLogAlways("ProductVersion: %d.%d.%d.%d", m_productVersion.v[3], m_productVersion.v[2], m_productVersion.v[1], m_productVersion.v[0]);
#endif // defined(LY_BUILD)



#if   defined(_MSC_VER)
    CryLogAlways("Using Microsoft (tm) C++ Standard Library implementation\n");
#elif defined(__clang__)
    CryLogAlways("Using CLANG C++ Standard Library implementation\n");
#elif defined(__GNUC__)
    CryLogAlways("Using GNU C++ Standard Library implementation\n");
#else
#error "Please specify C++ STL library"
#endif
}

//////////////////////////////////////////////////////////////////////////
void CSystem::LogBuildInfo()
{
    ICVar* pGameName = m_env.pConsole->GetCVar("sys_game_name");
    if (pGameName)
    {
        CryLogAlways("GameName: %s", pGameName->GetString());
    }
    else
    {
        CryLogAlways("Couldn't find game name in cvar sys_game_name");
    }
    CryLogAlways("BuildTime: " __DATE__ " " __TIME__);
}



//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
class CCVarSaveDump
    : public ICVarDumpSink
{
public:

    CCVarSaveDump(FILE* pFile)
    {
        m_pFile = pFile;
    }

    virtual void OnElementFound(ICVar* pCVar)
    {
        if (!pCVar)
        {
            return;
        }
        int nFlags = pCVar->GetFlags();
        if (((nFlags & VF_DUMPTODISK) && (nFlags & VF_MODIFIED)) || (nFlags & VF_WASINCONFIG))
        {
            string szValue = pCVar->GetString();
            int pos;

            pos = 1;
            for (;; )
            {
                pos = szValue.find_first_of("\\", pos);

                if (pos == string::npos)
                {
                    break;
                }

                szValue.replace(pos, 1, "\\\\", 2);
                pos += 2;
            }

            // replace " with \"
            pos = 1;
            for (;; )
            {
                pos = szValue.find_first_of("\"", pos);

                if (pos == string::npos)
                {
                    break;
                }

                szValue.replace(pos, 1, "\\\"", 2);
                pos += 2;
            }

            string szLine = pCVar->GetName();

            if (pCVar->GetType() == CVAR_STRING)
            {
                szLine += " = \"" + szValue + "\"\r\n";
            }
            else
            {
                szLine += " = " + szValue + "\r\n";
            }

            if (pCVar->GetFlags() & VF_WARNING_NOTUSED)
            {
                fputs("-- REMARK: the following was not assigned to a console variable\r\n", m_pFile);
            }

            fputs(szLine.c_str(), m_pFile);
        }
    }

private: // --------------------------------------------------------

    FILE*              m_pFile;                     //
};

//////////////////////////////////////////////////////////////////////////
void CSystem::SaveConfiguration()
{
}

//////////////////////////////////////////////////////////////////////////
// system cfg
//////////////////////////////////////////////////////////////////////////
CSystemConfiguration::CSystemConfiguration(const string& strSysConfigFilePath, CSystem* pSystem, ILoadConfigurationEntrySink* pSink, AZStd::unordered_map<AZStd::string, CVarInfo>* editorMap, bool warnIfMissing)
    : m_strSysConfigFilePath(strSysConfigFilePath)
    , m_bError(false)
    , m_pSink(pSink)
    , m_editorMap(editorMap)
    , m_warnIfMissing(warnIfMissing)
{
    assert(pSink);

    m_pSystem = pSystem;
    m_bError = !ParseSystemConfig();
}

//////////////////////////////////////////////////////////////////////////
CSystemConfiguration::~CSystemConfiguration()
{
}

//////////////////////////////////////////////////////////////////////////
void CSystemConfiguration::AddCVarToMap(const string& filename, const string& strKey, const string& strValue, const string& strGroup)
{
    AZStd::string key = strKey.c_str();
    ICVar* cvar = gEnv->pConsole->GetCVar(strKey);

    if (cvar)
    {
        AZStd::transform(key.begin(), key.end(), key.begin(), tolower);
        if (azstricmp(strKey, "sys_spec_full") == 0 || strKey.find("sys_spec_") == strKey.npos)
        {
            int type = cvar->GetType();
            AZStd::any val;
            if (type == CVAR_INT)
            {
                val = atoi(strValue);
            }
            else if (type == CVAR_FLOAT)
            {
                val = static_cast<float>(atof(strValue));
            }
            else
            {
                val = AZStd::string(strValue.c_str());
            }

            // Platform cfg file (ex. pc_veryhigh.cfg)
            if (strGroup.empty())
            {
                // New cvar loaded into map
                if (m_editorMap->find(key) == m_editorMap->end())
                {
                    (*m_editorMap)[key].type = type;
                    (*m_editorMap)[key].cvarGroup = "miscellaneous";
                    AZStd::any empty;
                    if (type == CVAR_INT)
                    {
                        empty = 0;
                    }
                    else if (type == CVAR_FLOAT)
                    {
                        empty = 0.0f;
                    }
                    else
                    {
                        empty = AZStd::string("");
                    }
                    (*m_editorMap)[key].fileVals.resize(NUM_SPEC_LEVELS, CVarFileStatus(empty, empty, empty));
                }

                int specIndex = 0;

                // Subtracting one since index is one less than enum value
                if (filename.find("veryhigh.cfg") != filename.npos)
                {
                    specIndex = CONFIG_VERYHIGH_SPEC - 1;
                }
                else if (filename.find("high.cfg") != filename.npos)
                {
                    specIndex = CONFIG_HIGH_SPEC - 1;
                }
                else if (filename.find("medium.cfg") != filename.npos)
                {
                    specIndex = CONFIG_MEDIUM_SPEC - 1;
                }
                else // Low spec / no spec level
                {
                    specIndex = CONFIG_LOW_SPEC - 1;
                }

                (*m_editorMap)[key].fileVals[specIndex].editedValue = val;
                (*m_editorMap)[key].fileVals[specIndex].overwrittenValue = val;
            }
            // default group in sys_spec cfg file
            else if (azstricmp(strGroup, "default") == 0)
            {
                // New cvar loaded into map
                if (m_editorMap->find(key) == m_editorMap->end())
                {
                    CVarInfo* currentCVar = &(*m_editorMap)[key];
                    currentCVar->type = cvar->GetType();
                    CVarFileStatus defaultVal(val, val, val);
                    currentCVar->fileVals.resize(NUM_SPEC_LEVELS, defaultVal);
                }
                // Overwrite miscellaneous if mentioned in platform config file
                (*m_editorMap)[key].cvarGroup = filename;
            }
            // specific index in sys_spec cfg file
            else
            {
                int group = 0;

                if (sscanf(strGroup, "%d", &group) == 1)
                {
                    CVarFileStatus indexAssignment(val, val, val);
                    for (int specLevel = 0; specLevel < NUM_SPEC_LEVELS; ++specLevel)
                    {
                        // Only apply cvar change to configurations with sys_spec_Full matching the index
                        int overwrittenValue;
                        if (AZStd::any_numeric_cast<int>(&(*m_editorMap)["sys_spec_full"].fileVals[specLevel].overwrittenValue, overwrittenValue) && group == overwrittenValue)
                        {
                            (*m_editorMap)[key].fileVals[specLevel] = indexAssignment;
                        }
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CSystemConfiguration::ParseSystemConfig()
{
    string filename = m_strSysConfigFilePath;
    if (strlen(PathUtil::GetExt(filename)) == 0)
    {
        filename = PathUtil::ReplaceExtension(filename, "cfg");
    }

    CCryFile file;
    string filenameLog;
    {
        int flags = ICryPak::FOPEN_HINT_QUIET | ICryPak::FOPEN_ONDISK;

        if (filename[0] == '@')
        {
            // this is used when theres a very specific file to read, like @user@/game.cfg which is read
            // IN ADDITION to the one in the game folder, and afterwards to override values in it.
            // if the file is missing and its already prefixed with an alias, there is no need to look any further.
            if (!(file.Open(filename, "rb", flags)))
            {
                if (m_warnIfMissing)
                {
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Config file %s not found!", filename.c_str());
                }
                return false;
            }
        }
        else
        {
            // otherwise, if the file isn't prefixed with an alias, then its likely one of the convenience mappings
            // to either root or assets/config.  this is done so that code can just request a simple file name and get its data
            if (
                !(file.Open(filename, "rb", flags)) &&
                !(file.Open(string("@root@/") + filename, "rb", flags)) &&
                !(file.Open(string("@assets@/") + filename, "rb", flags)) &&
                !(file.Open(string("@assets@/config/") + filename, "rb", flags)) &&
                !(file.Open(string("@assets@/config/spec/") + filename, "rb", flags))
                )
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Config file %s not found!", filename.c_str());
                return false;
            }
        }

        filenameLog = file.GetAdjustedFilename();
    }

    INDENT_LOG_DURING_SCOPE();

    int nLen = file.GetLength();
    if (nLen == 0)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Couldn't get length for Config file %s", filename.c_str());
        return false;
    }
    char* sAllText = new char [nLen + 16];
    if (file.ReadRaw(sAllText, nLen) < (size_t)nLen)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Couldn't read Config file %s", filename.c_str());
        return false;
    }
    sAllText[nLen] = '\0';
    sAllText[nLen + 1] = '\0';

    string strGroup;            // current group e.g. "[General]"

    char* strLast = sAllText + nLen;
    char* str = sAllText;
    while (str < strLast)
    {
        char* s = str;
        while (str < strLast && *str != '\n' && *str != '\r')
        {
            str++;
        }
        *str = '\0';
        str++;
        while (str < strLast && (*str == '\n' || *str == '\r'))
        {
            str++;
        }

        string strLine = s;

        // detect groups e.g. "[General]"   should set strGroup="General"
        {
            string strTrimmedLine(RemoveWhiteSpaces(strLine));
            size_t size = strTrimmedLine.size();

            if (size >= 3)
            {
                if (strTrimmedLine[0] == '[' && strTrimmedLine[size - 1] == ']')       // currently no comments are allowed to be behind groups
                {
                    strGroup = &strTrimmedLine[1];
                    strGroup.resize(size - 2);                                  // remove [ and ]
                    continue;                                       // next line
                }
            }
        }

        //trim all whitespace characters at the beginning and the end of the current line and store its size
        strLine.Trim();
        size_t strLineSize = strLine.size();

        //skip comments, comments start with ";" or "--" but may have preceding whitespace characters
        if (strLineSize > 0)
        {
            if (strLine[0] == ';')
            {
                continue;
            }
            else if (strLine.find("--") == 0)
            {
                continue;
            }
        }
        //skip empty lines
        else
        {
            continue;
        }

        //if line contains a '=' try to read and assign console variable
        string::size_type posEq(strLine.find("=", 0));
        if (string::npos != posEq)
        {
            string stemp(strLine, 0, posEq);
            string strKey(RemoveWhiteSpaces(stemp));

            //                if (!strKey.empty())
            {
                // extract value
                string::size_type posValueStart(strLine.find("\"", posEq + 1) + 1);
                // string::size_type posValueEnd( strLine.find( "\"", posValueStart ) );
                string::size_type posValueEnd(strLine.rfind('\"'));

                string strValue;

                if (string::npos != posValueStart && string::npos != posValueEnd)
                {
                    strValue = string(strLine, posValueStart, posValueEnd - posValueStart);
                }
                else
                {
                    string strTmp(strLine, posEq + 1, strLine.size() - (posEq + 1));
                    strValue = RemoveWhiteSpaces(strTmp);
                }

                {
                    // replace '\\\\' with '\\' and '\\\"' with '\"'
                    strValue.replace("\\\\", "\\");
                    strValue.replace("\\\"", "\"");

                    //                      m_pSystem->GetILog()->Log("Setting %s to %s",strKey.c_str(),strValue.c_str());
                    
                    // Check if running Graphics Settings Dialog
                    if (m_editorMap != nullptr)
                    {
                        AddCVarToMap(filename, strKey, strValue, strGroup);
                    }
                    else // Loading / registering cvar
                    {
                        m_pSink->OnLoadConfigurationEntry(strKey, strValue, strGroup);
                    }
                }
            }
        }
        else
        {
            gEnv->pLog->LogWithType(ILog::eWarning, "%s -> invalid configuration line: %s", filename.c_str(), strLine.c_str());
        }
    }

    delete []sAllText;

    CryLog("Loading Config file %s (%s)", filename.c_str(), filenameLog.c_str());

    m_pSink->OnLoadConfigurationEntry_End();

    return true;
}


//////////////////////////////////////////////////////////////////////////
void CSystem::OnLoadConfigurationEntry(const char* szKey, const char* szValue, const char* szGroup)
{
    if (!gEnv->pConsole)
    {
        return;
    }

    if (*szKey != 0)
    {
        gEnv->pConsole->LoadConfigVar(szKey, szValue);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::LoadConfiguration(const char* sFilename, ILoadConfigurationEntrySink* pSink, bool warnIfMissing)
{
    if (sFilename && strlen(sFilename) > 0)
    {
        if (!pSink)
        {
            pSink = this;
        }

        CSystemConfiguration tempConfig(sFilename, this, pSink, gEnv->pSystem->GetGraphicsSettingsMap(), warnIfMissing);
    }
}

