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

#include "StdAfx.h"

#include "EngineSettingsManager.h"

#if defined(CRY_ENABLE_RC_HELPER)

#include <assert.h>                                     // assert()

#if defined(AZ_PLATFORM_WINDOWS)
#define KDAB_MAC_PORT 1
#endif

#ifdef KDAB_MAC_PORT
#pragma comment(lib, "Ole32.lib")
#include <windows.h>
#endif // KDAB_MAC_PORT

#include <stdio.h>

#define REG_SOFTWARE            L"Software\\"
#define REG_COMPANY_NAME        L"Amazon\\"
#define REG_PRODUCT_NAME        L"Lumberyard\\"
#define REG_SETTING             L"Settings\\"
#define REG_BASE_SETTING_KEY  REG_SOFTWARE REG_COMPANY_NAME REG_PRODUCT_NAME REG_SETTING

// pseudo-variable that represents the DOS header of the module
#ifdef KDAB_MAC_PORT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#endif // KDAB_MAC_PORT


#define INFOTEXT L"Please specify the directory of your CryENGINE installation (RootPath):"


using namespace SettingsManagerHelpers;


static bool g_bWindowQuit;
static CEngineSettingsManager* g_pThis = 0;
static const unsigned int IDC_hEditRootPath = 100;
static const unsigned int IDC_hBtnBrowse = 101;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
namespace
{
    class RegKey
    {
    public:
        RegKey(const wchar_t* key, bool writeable);
        ~RegKey();
        void* pKey;
    };
}

//////////////////////////////////////////////////////////////////////////
RegKey::RegKey(const wchar_t* key, bool writeable)
{
#ifdef KDAB_MAC_PORT
    HKEY  hKey;
    LONG result;
    if (writeable)
    {
        result = RegCreateKeyExW(HKEY_CURRENT_USER, key, 0, 0, 0, KEY_WRITE, 0, &hKey, 0);
    }
    else
    {
        result = RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_READ, &hKey);
    }
    pKey = hKey;
#endif // KDAB_MAC_PORT
}

//////////////////////////////////////////////////////////////////////////
RegKey::~RegKey()
{
#ifdef KDAB_MAC_PORT
    RegCloseKey((HKEY)pKey);
#endif // KDAB_MAC_PORT
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CEngineSettingsManager::CEngineSettingsManager(const wchar_t* moduleName, const wchar_t* iniFileName)
    : m_hWndParent(0)
{
    m_sModuleName.clear();

    // std initialization
    RestoreDefaults();

    // try to load content from INI file
    if (moduleName != NULL)
    {
        m_sModuleName = moduleName;

        if (iniFileName == NULL)
        {
            // find INI filename located in module path
#ifdef KDAB_MAC_PORT
            HMODULE hInstance = GetModuleHandleW(moduleName);
            wchar_t szFilename[_MAX_PATH];
            GetModuleFileNameW((HINSTANCE)&__ImageBase, szFilename, _MAX_PATH);
            wchar_t drive[_MAX_DRIVE];
            wchar_t dir[_MAX_DIR];
            wchar_t fname[_MAX_FNAME];
            wchar_t ext[1] = L"";
            _wsplitpath_s(szFilename, drive, dir, fname, ext);
            _wmakepath_s(szFilename, drive, dir, fname, L"ini");
            m_sModuleFileName = szFilename;
#endif // KDAB_MAC_PORT
        }
        else
        {
            m_sModuleFileName = iniFileName;
        }

        if (LoadValuesFromConfigFile(m_sModuleFileName.c_str()))
        {
            m_bGetDataFromRegistry = false;
            return;
        }
    }

    m_bGetDataFromRegistry = true;

    // load basic content from registry
    LoadEngineSettingsFromRegistry();
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::RestoreDefaults()
{
    // Engine
    SetKey("ENG_RootPath", L"");

    // Engine Bin Path
    SetKey("ENG_BinFolder", L"");

    // RC
    SetKey("RC_ShowWindow", false);
    SetKey("RC_HideCustom", false);
    SetKey("RC_Parameters", L"");
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetModuleSpecificStringEntryUtf16(const char* key, SettingsManagerHelpers::CWCharBuffer wbuffer)
{
    if (wbuffer.getSizeInElements() <= 0)
    {
        return false;
    }

    if (!m_bGetDataFromRegistry)
    {
        if (!HasKey(key))
        {
            wbuffer[0] = 0;
            return false;
        }
        if (!GetValueByRef(key, wbuffer))
        {
            wbuffer[0] = 0;
            return false;
        }
    }
    else
    {
        CFixedString<wchar_t, 256> s = REG_BASE_SETTING_KEY;
        s.append(m_sModuleName.c_str());
        RegKey superKey(s.c_str(), false);
        if (!superKey.pKey)
        {
            wbuffer[0] = 0;
            return false;
        }
        if (!GetRegValue(superKey.pKey, key, wbuffer))
        {
            wbuffer[0] = 0;
            return false;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetModuleSpecificStringEntryUtf8(const char* key, SettingsManagerHelpers::CCharBuffer buffer)
{
    if (buffer.getSizeInElements() <= 0)
    {
        return false;
    }

    wchar_t wBuffer[1024];

    if (!GetModuleSpecificStringEntryUtf16(key, SettingsManagerHelpers::CWCharBuffer(wBuffer, sizeof(wBuffer))))
    {
        buffer[0] = 0;
        return false;
    }

    SettingsManagerHelpers::ConvertUtf16ToUtf8(wBuffer, buffer);

    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetModuleSpecificIntEntry(const char* key, int& value)
{
    value = 0;

    if (!m_bGetDataFromRegistry)
    {
        if (!HasKey(key))
        {
            return false;
        }
        if (!GetValueByRef(key, value))
        {
            return false;
        }
    }
    else
    {
        CFixedString<wchar_t, 256> s = REG_BASE_SETTING_KEY;
        s.append(m_sModuleName.c_str());
        RegKey superKey(s.c_str(), false);
        if (!superKey.pKey)
        {
            return false;
        }
        if (!GetRegValue(superKey.pKey, key, value))
        {
            value = 0;
            return false;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetModuleSpecificBoolEntry(const char* key, bool& value)
{
    value = false;

    if (!m_bGetDataFromRegistry)
    {
        if (!HasKey(key))
        {
            return false;
        }
        if (!GetValueByRef(key, value))
        {
            return false;
        }
    }
    else
    {
        CFixedString<wchar_t, 256> s = REG_BASE_SETTING_KEY;
        s.append(m_sModuleName.c_str());
        RegKey superKey(s.c_str(), false);
        if (!superKey.pKey)
        {
            return false;
        }
        if (!GetRegValue(superKey.pKey, key, value))
        {
            value = false;
            return false;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::SetModuleSpecificStringEntryUtf16(const char* key, const wchar_t* str)
{
    SetKey(key, str);
    if (!m_bGetDataFromRegistry)
    {
        return StoreData();
    }

    CFixedString<wchar_t, 256> s = REG_BASE_SETTING_KEY;
    s.append(m_sModuleName.c_str());
    RegKey superKey(s.c_str(), true);
    if (superKey.pKey)
    {
        return SetRegValue(superKey.pKey, key, str);
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::SetModuleSpecificStringEntryUtf8(const char* key, const char* str)
{
    wchar_t wbuffer[512];
    SettingsManagerHelpers::ConvertUtf8ToUtf16(str, SettingsManagerHelpers::CWCharBuffer(wbuffer, sizeof(wbuffer)));

    return SetModuleSpecificStringEntryUtf16(key, wbuffer);
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::SetModuleSpecificIntEntry(const char* key, const int& value)
{
    SetKey(key, value);
    if (!m_bGetDataFromRegistry)
    {
        return StoreData();
    }

    CFixedString<wchar_t, 256> s = REG_BASE_SETTING_KEY;
    s.append(m_sModuleName.c_str());
    RegKey superKey(s.c_str(), true);
    if (superKey.pKey)
    {
        return SetRegValue(superKey.pKey, key, value);
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::SetModuleSpecificBoolEntry(const char* key, const bool& value)
{
    SetKey(key, value);
    if (!m_bGetDataFromRegistry)
    {
        return StoreData();
    }

    CFixedString<wchar_t, 256> s = REG_BASE_SETTING_KEY;
    s.append(m_sModuleName.c_str());
    RegKey superKey(s.c_str(), true);
    if (superKey.pKey)
    {
        return SetRegValue(superKey.pKey, key, value);
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::SetRootPath(const wchar_t* szRootPath)
{
    CFixedString<wchar_t, MAX_PATH> path = szRootPath;
    size_t const len = path.length();

    if ((len > 0) && ((path[len - 1] == '\\') || (path[len - 1] == '/')))
    {
        path.setLength(len - 1);
    }

    SetKey("ENG_RootPath", path.c_str());
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::HasKey(const char* key)
{
    return m_keyValueArray.find(key) != 0;
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::SetKey(const char* key, const wchar_t* value)
{
    m_keyValueArray.set(key, value);
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::SetKey(const char* key, bool value)
{
    m_keyValueArray.set(key, (value ? L"true" : L"false"));
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::SetKey(const char* key, int value)
{
#ifdef KDAB_MAC_PORT
    wchar_t buf[40];
    swprintf_s(buf, L"%d", value);
    m_keyValueArray.set(key, buf);
#endif // KDAB_MAC_PORT
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::GetRootPathUtf16(SettingsManagerHelpers::CWCharBuffer wbuffer)
{
    LoadEngineSettingsFromRegistry();
    GetValueByRef("ENG_RootPath", wbuffer);
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::GetRootPathAscii(SettingsManagerHelpers::CCharBuffer buffer)
{
    wchar_t wbuffer[MAX_PATH];

    LoadEngineSettingsFromRegistry();
    GetValueByRef("ENG_RootPath", SettingsManagerHelpers::CWCharBuffer(wbuffer, sizeof(wbuffer)));

    SettingsManagerHelpers::GetAsciiFilename(wbuffer, buffer);
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::GetBinFolderUtf16(SettingsManagerHelpers::CWCharBuffer wbuffer)
{
    LoadEngineSettingsFromRegistry();
    GetValueByRef("ENG_BinFolder", wbuffer);
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::GetBinFolderAscii(SettingsManagerHelpers::CCharBuffer buffer)
{
    wchar_t wbuffer[MAX_PATH];

    LoadEngineSettingsFromRegistry();
    GetValueByRef("ENG_BinFolder", SettingsManagerHelpers::CWCharBuffer(wbuffer, sizeof(wbuffer)));

    SettingsManagerHelpers::GetAsciiFilename(wbuffer, buffer);
}


bool CEngineSettingsManager::GetInstalledBuildRootPathUtf16(const int index, SettingsManagerHelpers::CWCharBuffer name, SettingsManagerHelpers::CWCharBuffer path)
{
    RegKey key(REG_BASE_SETTING_KEY L"LumberyardExport\\ProjectBuilds", false);
    if (key.pKey)
    {
        DWORD type;
        DWORD nameSizeInBytes = DWORD(name.getSizeInBytes());
        DWORD pathSizeInBytes = DWORD(path.getSizeInBytes());
#ifdef KDAB_MAC_PORT
        LONG result = RegEnumValueW((HKEY)key.pKey, index, name.getPtr(), &nameSizeInBytes, NULL, &type, (BYTE*)path.getPtr(), &pathSizeInBytes);
        if (result == ERROR_SUCCESS)
        {
            return true;
        }
#endif // KDAB_MAC_PORT
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::SetParentDialog(unsigned long window)
{
    m_hWndParent = window;
}


//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::StoreData()
{
    if (m_bGetDataFromRegistry)
    {
        bool res = StoreEngineSettingsToRegistry();

        if (!res)
        {
#ifdef KDAB_MAC_PORT
            MessageBoxA((HWND)m_hWndParent, "Could not store data to registry.", "Error", MB_OK | MB_ICONERROR);
#endif // KDAB_MAC_PORT
        }
        return res;
    }

    // store data to INI file

    FILE* file;
#ifdef KDAB_MAC_PORT
    _wfopen_s(&file, m_sModuleFileName.c_str(), L"wb");
    if (file == NULL)
#endif // KDAB_MAC_PORT
    {
        return false;
    }

    char buffer[2048];

    for (size_t i = 0; i < m_keyValueArray.size(); ++i)
    {
        const SKeyValue& kv = m_keyValueArray[i];

        fprintf_s(file, kv.key.c_str());
        fprintf_s(file, " = ");

        if (kv.value.length() > 0)
        {
            SettingsManagerHelpers::ConvertUtf16ToUtf8(kv.value.c_str(), SettingsManagerHelpers::CCharBuffer(buffer, sizeof(buffer)));
            fprintf_s(file, "%s", buffer);
        }

        fprintf_s(file, "\r\n");
    }

    fclose(file);

    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::LoadValuesFromConfigFile(const wchar_t* szFileName)
{
#ifdef KDAB_MAC_PORT
    m_keyValueArray.clear();

    // read file to memory

    FILE* file;
#ifdef KDAB_MAC_PORT
    _wfopen_s(&file, szFileName, L"rb");
    if (file == NULL)
#endif // KDAB_MAC_PORT
    {
        return false;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* data = new char[size + 1];
    fread_s(data, size, 1, size, file);
    fclose(file);

    wchar_t wBuffer[1024];

    // parse file for root path

    int start = 0, end = 0;
    while (end < size)
    {
        while (end < size && data[end] != '\n')
        {
            end++;
        }

        memcpy(data, &data[start], end - start);
        data[end - start] = 0;
        start = end = end + 1;

        CFixedString<char, 2048> line(data);
        size_t equalsOfs;
        for (equalsOfs = 0; equalsOfs < line.length(); ++equalsOfs)
        {
            if (line[equalsOfs] == '=')
            {
                break;
            }
        }
        if (equalsOfs < line.length())
        {
            CFixedString<char, 256> key;
            CFixedString<wchar_t, 1024> value;

            key.appendAscii(line.c_str(), equalsOfs);
            key.trim();

            SettingsManagerHelpers::ConvertUtf8ToUtf16(line.c_str() + equalsOfs + 1, SettingsManagerHelpers::CWCharBuffer(wBuffer, sizeof(wBuffer)));
            value.append(wBuffer);
            value.trim();

            // Stay compatible to deprecated rootpath key
            if (key.equals("RootPath"))
            {
                key = "ENG_RootPath";
                if (value[value.length() - 1] == '\\' || value[value.length() - 1] == '/')
                {
                    value.set(value.c_str(), value.length() - 1);
                }
            }

            m_keyValueArray.set(key.c_str(), value.c_str());
        }
    }
    delete[] data;

    return true;
#else
    return false;
#endif // KDAB_MAC_PORT
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::SetRegValue(void* key, const char* valueName, const wchar_t* value)
{
    SettingsManagerHelpers::CFixedString<wchar_t, 256> name;
    name.appendAscii(valueName);

    size_t const sizeInBytes = (wcslen(value) + 1) * sizeof(value[0]);
#ifdef KDAB_MAC_PORT
    return (ERROR_SUCCESS == RegSetValueExW((HKEY)key, name.c_str(), 0, REG_SZ, (BYTE*)value, DWORD(sizeInBytes)));
#else
    return false;
#endif // KDAB_MAC_PORT
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::SetRegValue(void* key, const char* valueName, bool value)
{
    SettingsManagerHelpers::CFixedString<wchar_t, 256> name;
    name.appendAscii(valueName);

    DWORD dwVal = value;
#ifdef KDAB_MAC_PORT
    return (ERROR_SUCCESS == RegSetValueExW((HKEY)key, name.c_str(), 0, REG_DWORD, (BYTE*)&dwVal, sizeof(dwVal)));
#else
    return false;
#endif // KDAB_MAC_PORT
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::SetRegValue(void* key, const char* valueName, int value)
{
    SettingsManagerHelpers::CFixedString<wchar_t, 256> name;
    name.appendAscii(valueName);

    DWORD dwVal = value;
#ifdef KDAB_MAC_PORT
    return (ERROR_SUCCESS == RegSetValueExW((HKEY)key, name.c_str(), 0, REG_DWORD, (BYTE*)&dwVal, sizeof(dwVal)));
#else
    return false;
#endif // KDAB_MAC_PORT
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetRegValue(void* key, const char* valueName, SettingsManagerHelpers::CWCharBuffer wbuffer)
{
    if (wbuffer.getSizeInElements() <= 0)
    {
        return false;
    }

    SettingsManagerHelpers::CFixedString<wchar_t, 256> name;
    name.appendAscii(valueName);

    DWORD type;
    DWORD sizeInBytes = DWORD(wbuffer.getSizeInBytes());
#ifdef KDAB_MAC_PORT
    if (ERROR_SUCCESS != RegQueryValueExW((HKEY)key, name.c_str(), NULL, &type, (BYTE*)wbuffer.getPtr(), &sizeInBytes))
#endif // KDAB_MAC_PORT
    {
        wbuffer[0] = 0;
        return false;
    }

    const size_t sizeInElements = sizeInBytes / sizeof(wbuffer[0]);
    if (sizeInElements > wbuffer.getSizeInElements()) // paranoid check
    {
        wbuffer[0] = 0;
        return false;
    }

    // According to MSDN documentation for RegQueryValueEx(), strings returned by the function
    // are not zero-terminated sometimes, so we need to terminate them by ourselves.
    if (wbuffer[sizeInElements - 1] != 0)
    {
        if (sizeInElements >= wbuffer.getSizeInElements())
        {
            // No space left to put terminating zero character
            wbuffer[0] = 0;
            return false;
        }
        wbuffer[sizeInElements] = 0;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetRegValue(void* key, const char* valueName, bool& value)
{
    SettingsManagerHelpers::CFixedString<wchar_t, 256> name;
    name.appendAscii(valueName);

    // Open the appropriate registry key
    DWORD type, dwVal = 0, size = sizeof(dwVal);
#ifdef KDAB_MAC_PORT
    bool res = (ERROR_SUCCESS == RegQueryValueExW((HKEY)key, name.c_str(), NULL, &type, (BYTE*)&dwVal, &size));
#else
    bool res = false;
#endif // KDAB_MAC_PORT
    if (res)
    {
        value = (dwVal != 0);
    }
    else
    {
        wchar_t buffer[100];
        res = GetRegValue(key, valueName, SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer)));
        if (res)
        {
            value = (wcscmp(buffer, L"true") == 0);
        }
    }
    return res;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetRegValue(void* key, const char* valueName, int& value)
{
    SettingsManagerHelpers::CFixedString<wchar_t, 256> name;
    name.appendAscii(valueName);

    // Open the appropriate registry key
    DWORD type, dwVal = 0, size = sizeof(dwVal);

#ifdef KDAB_MAC_PORT
    bool res = (ERROR_SUCCESS == RegQueryValueExW((HKEY)key, name.c_str(), NULL, &type, (BYTE*)&dwVal, &size));
#else
    bool res = false;
#endif // KDAB_MAC_PORT
    if (res)
    {
        value = dwVal;
    }

    return res;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::StoreEngineSettingsToRegistry()
{
    if (!m_bGetDataFromRegistry)
    {
        return true;
    }

    // make sure the path in registry exists
    {
        RegKey key0(REG_SOFTWARE REG_COMPANY_NAME, true);
        if (!key0.pKey)
        {
            RegKey software(REG_SOFTWARE, true);
            HKEY hKey;
#ifdef KDAB_MAC_PORT
            RegCreateKeyW((HKEY)software.pKey, REG_COMPANY_NAME, &hKey);
            if (!hKey)
#endif // KDAB_MAC_PORT
            {
                return false;
            }
        }

        RegKey key1(REG_SOFTWARE REG_COMPANY_NAME REG_PRODUCT_NAME, true);
        if (!key1.pKey)
        {
            RegKey softwareCompany(REG_SOFTWARE REG_COMPANY_NAME, true);
            HKEY hKey;
#ifdef KDAB_MAC_PORT
            RegCreateKeyW((HKEY)softwareCompany.pKey, REG_COMPANY_NAME, &hKey);
            if (!hKey)
#endif // KDAB_MAC_PORT
            {
                return false;
            }
        }

        RegKey key2(REG_BASE_SETTING_KEY, true);
        if (!key2.pKey)
        {
            RegKey softwareCompanyProduct(REG_SOFTWARE REG_COMPANY_NAME REG_PRODUCT_NAME, true);
            HKEY hKey;
#ifdef KDAB_MAC_PORT
            RegCreateKeyW((HKEY)key2.pKey, REG_SETTING, &hKey);
            if (!hKey)
#endif // KDAB_MAC_PORT
            {
                return false;
            }
        }
    }

    bool bRet = true;

    RegKey key(REG_BASE_SETTING_KEY, true);
    if (!key.pKey)
    {
        bRet = false;
    }
    else
    {
        wchar_t buffer[1024];

        // Engine Specific
        if (GetValueByRef("ENG_RootPath", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            bRet &= SetRegValue(key.pKey, "ENG_RootPath", buffer);
        }
        else
        {
            bRet = false;
        }

        if (GetValueByRef("ENG_BinFolder", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            bRet &= SetRegValue(key.pKey, "ENG_BinFolder", buffer);
        }
        else
        {
            bRet = false;
        }
        // ResourceCompiler Specific

        if (GetValueByRef("RC_ShowWindow", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            const bool b = wcscmp(buffer, L"true") == 0;
            SetRegValue(key.pKey, "RC_ShowWindow", b);
        }

        if (GetValueByRef("RC_HideCustom", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            const bool b = wcscmp(buffer, L"true") == 0;
            SetRegValue(key.pKey, "RC_HideCustom", b);
        }

        if (GetValueByRef("RC_Parameters", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            SetRegValue(key.pKey, "RC_Parameters", buffer);
        }

        if (GetValueByRef("RC_EnableSourceControl", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            const bool b = wcscmp(buffer, L"true") == 0;
            SetRegValue(key.pKey, "RC_EnableSourceControl", b);
        }
    }

    return bRet;
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::LoadEngineSettingsFromRegistry()
{
    if (!m_bGetDataFromRegistry)
    {
        return;
    }

    wchar_t buffer[1024];

    bool bResult;

    // Engine Specific (Deprecated value)
    RegKey key(REG_BASE_SETTING_KEY, false);
    if (key.pKey)
    {
        if (GetRegValue(key.pKey, "RootPath", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            SetKey("ENG_RootPath", buffer);
        }

        // Engine Specific
        if (GetRegValue(key.pKey, "ENG_RootPath", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            SetKey("ENG_RootPath", buffer);
        }
        if (GetRegValue(key.pKey, "ENG_BinFolder", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            SetKey("ENG_BinFolder", buffer);
        }

        // ResourceCompiler Specific
        if (GetRegValue(key.pKey, "RC_ShowWindow", bResult))
        {
            SetKey("RC_ShowWindow", bResult);
        }
        if (GetRegValue(key.pKey, "RC_HideCustom", bResult))
        {
            SetKey("RC_HideCustom", bResult);
        }
        if (GetRegValue(key.pKey, "RC_Parameters", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            SetKey("RC_Parameters", buffer);
        }
        if (GetRegValue(key.pKey, "RC_EnableSourceControl", bResult))
        {
            SetKey("RC_EnableSourceControl", bResult);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetValueByRef(const char* key, SettingsManagerHelpers::CWCharBuffer wbuffer) const
{
    if (wbuffer.getSizeInElements() <= 0)
    {
        return false;
    }

    const SKeyValue* p = m_keyValueArray.find(key);
    if (!p || (p->value.length() + 1) > wbuffer.getSizeInElements())
    {
        wbuffer[0] = 0;
        return false;
    }
    wcscpy(wbuffer.getPtr(), p->value.c_str());
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetValueByRef(const char* key, bool& value) const
{
    wchar_t buffer[100];
    if (!GetValueByRef(key, SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
    {
        return false;
    }
    value = (wcscmp(buffer, L"true") == 0);
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetValueByRef(const char* key, int& value) const
{
    wchar_t buffer[100];
    if (!GetValueByRef(key, SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
    {
        return false;
    }
    value = wcstol(buffer, 0, 10);
    return true;
}

#endif //(CRY_ENABLE_RC_HELPER)
