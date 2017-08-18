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

#if defined(CRY_ENABLE_RC_HELPER)

#include "SettingsManagerHelpers.h"
#include "EngineSettingsManager.h"

#if defined(AZ_PLATFORM_WINDOWS)
#define KDAB_MAC_PORT 1
#endif

#ifdef KDAB_MAC_PORT
#include <windows.h>
#endif // KDAB_MAC_PORT

#ifdef QT_VERSION
#include <QProcess>
#else
#include <shellapi.h> //ShellExecuteW()

#pragma comment(lib, "Shell32.lib")
#endif

bool SettingsManagerHelpers::Utf16ContainsAsciiOnly(const wchar_t* wstr)
{
    while (*wstr)
    {
        if (*wstr > 127 || *wstr < 0)
        {
            return false;
        }
        ++wstr;
    }
    return true;
}


void SettingsManagerHelpers::ConvertUtf16ToUtf8(const wchar_t* src, CCharBuffer dst)
{
    if (dst.getSizeInElements() <= 0)
    {
        return;
    }

    if (src[0] == 0)
    {
        dst[0] = 0;
    }
#ifdef KDAB_MAC_PORT
    else
    {
        const int srclen = int(wcslen(src));
        const int dstsize = int(dst.getSizeInBytes());
        const int byteCount = WideCharToMultiByte(
                CP_UTF8,
                0,
                src,
                srclen,
                dst.getPtr(), // output buffer
                dstsize - 1, // size of the output buffer in bytes
                NULL,
                NULL);
        if (byteCount <= 0 || byteCount >= dstsize)
        {
            dst[0] = 0;
        }
        else
        {
            dst[byteCount] = 0;
        }
    }
#endif // KDAB_MAC_PORT
}


void SettingsManagerHelpers::ConvertUtf8ToUtf16(const char* src, CWCharBuffer dst)
{
    if (dst.getSizeInElements() <= 0)
    {
        return;
    }

    if (src[0] == 0)
    {
        dst[0] = 0;
    }
#ifdef KDAB_MAC_PORT
    else
    {
        const int srclen = int(strlen(src));
        const int dstsize = int(dst.getSizeInElements());
        const int charCount = MultiByteToWideChar(
                CP_UTF8,
                0,
                src,
                srclen,
                dst.getPtr(), // output buffer
                dstsize - 1); // size of the output buffer in characters
        if (charCount <= 0 || charCount >= dstsize)
        {
            dst[0] = 0;
        }
        else
        {
            dst[charCount] = 0;
        }
    }
#endif // KDAB_MAC_PORT
}


void SettingsManagerHelpers::GetAsciiFilename(const wchar_t* wfilename, CCharBuffer buffer)
{
    if (buffer.getSizeInElements() <= 0)
    {
        return;
    }

    if (wfilename[0] == 0)
    {
        buffer[0] = 0;
        return;
    }

    if (Utf16ContainsAsciiOnly(wfilename))
    {
        ConvertUtf16ToUtf8(wfilename, buffer);
        return;
    }

#ifdef KDAB_MAC_PORT
    // The path is non-ASCII unicode, so let's resort to short filenames (they are always ASCII-only, I hope)
    wchar_t shortW[MAX_PATH];
    const int bufferCharCount = sizeof(shortW) / sizeof(shortW[0]);
    const int charCount = GetShortPathNameW(wfilename, shortW, bufferCharCount);
    if (charCount <= 0 || charCount >= bufferCharCount)
    {
        buffer[0] = 0;
        return;
    }

    shortW[charCount] = 0;
    if (!Utf16ContainsAsciiOnly(shortW))
    {
        buffer[0] = 0;
        return;
    }

    ConvertUtf16ToUtf8(shortW, buffer);
#endif // KDAB_MAC_PORT
}


//////////////////////////////////////////////////////////////////////////
CSettingsManagerTools::CSettingsManagerTools(const wchar_t* szModuleName)
{
    m_pSettingsManager = new CEngineSettingsManager(szModuleName);
}

//////////////////////////////////////////////////////////////////////////
CSettingsManagerTools::~CSettingsManagerTools()
{
    delete m_pSettingsManager;
}

//////////////////////////////////////////////////////////////////////////
void CSettingsManagerTools::GetRootPathUtf16(bool pullFromRegistry, SettingsManagerHelpers::CWCharBuffer wbuffer)
{
    if (pullFromRegistry)
    {
        m_pSettingsManager->GetRootPathUtf16(wbuffer);
    }
    else
    {
        m_pSettingsManager->GetValueByRef("ENG_RootPath", wbuffer);
    }
}


void CSettingsManagerTools::GetRootPathAscii(bool pullFromRegistry, SettingsManagerHelpers::CCharBuffer buffer)
{
    wchar_t wbuffer[MAX_PATH];

    GetRootPathUtf16(pullFromRegistry, SettingsManagerHelpers::CWCharBuffer(wbuffer, sizeof(wbuffer)));

    SettingsManagerHelpers::GetAsciiFilename(wbuffer, buffer);
}

//////////////////////////////////////////////////////////////////////////
void CSettingsManagerTools::GetBinFolderUtf16(bool pullFromRegistry, SettingsManagerHelpers::CWCharBuffer wbuffer)
{
    if (pullFromRegistry)
    {
        m_pSettingsManager->GetBinFolderUtf16(wbuffer);
    }
    else
    {
        m_pSettingsManager->GetValueByRef("ENG_BinFolder", wbuffer);
    }
}


void CSettingsManagerTools::GetBinFolderAscii(bool pullFromRegistry, SettingsManagerHelpers::CCharBuffer buffer)
{
    wchar_t wbuffer[MAX_PATH];

    GetBinFolderUtf16(pullFromRegistry, SettingsManagerHelpers::CWCharBuffer(wbuffer, sizeof(wbuffer)));

    SettingsManagerHelpers::GetAsciiFilename(wbuffer, buffer);
}



//////////////////////////////////////////////////////////////////////////
bool CSettingsManagerTools::GetInstalledBuildPathUtf16(const int index, SettingsManagerHelpers::CWCharBuffer name, SettingsManagerHelpers::CWCharBuffer path)
{
    return m_pSettingsManager->GetInstalledBuildRootPathUtf16(index, name, path);
}


bool CSettingsManagerTools::GetInstalledBuildPathAscii(const int index, SettingsManagerHelpers::CCharBuffer name, SettingsManagerHelpers::CCharBuffer path)
{
    wchar_t wName[MAX_PATH];
    wchar_t wPath[MAX_PATH];
    if (GetInstalledBuildPathUtf16(index, SettingsManagerHelpers::CWCharBuffer(wName, sizeof(wName)), SettingsManagerHelpers::CWCharBuffer(wPath, sizeof(wPath))))
    {
        SettingsManagerHelpers::GetAsciiFilename(wName, name);
        SettingsManagerHelpers::GetAsciiFilename(wPath, path);
        return true;
    }
    return false;
}



//////////////////////////////////////////////////////////////////////////
static bool FileExists(const wchar_t* filename)
{
#ifdef KDAB_MAC_PORT
    const DWORD dwAttrib = GetFileAttributesW(filename);
    return dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
#else
    return false;
#endif // KDAB_MAC_PORT
}


//////////////////////////////////////////////////////////////////////////
void CSettingsManagerTools::GetEditorExecutable(SettingsManagerHelpers::CWCharBuffer wbuffer)
{
    if (wbuffer.getSizeInElements() <= 0)
    {
        return;
    }

    m_pSettingsManager->GetRootPathUtf16(wbuffer);

    SettingsManagerHelpers::CFixedString<wchar_t, 1024> editorExe;
    editorExe = wbuffer.getPtr();

    if (editorExe.length() <= 0)
    {
        wbuffer[0] = 0;
        return;
    }

    bool bFound = false;
    if (Is64bitWindows())
    {
        const size_t len = editorExe.length();
        editorExe.appendAscii("/" BINFOLDER_NAME "/Editor.exe");
        bFound = FileExists(editorExe.c_str());
        if (!bFound)
        {
            editorExe.setLength(len);
        }
    }
    if (!bFound)
    {
        editorExe.appendAscii("/Bin32/Editor.exe");
        bFound = FileExists(editorExe.c_str());
    }

    const size_t sizeToCopy = (editorExe.length() + 1) * sizeof(wbuffer[0]);
    if (!bFound || sizeToCopy > wbuffer.getSizeInBytes())
    {
        wbuffer[0] = 0;
    }
    else
    {
        memcpy(wbuffer.getPtr(), editorExe.c_str(), sizeToCopy);
    }
}


//////////////////////////////////////////////////////////////////////////
bool CSettingsManagerTools::CallEditor(void** pEditorWindow, void* hParent, const char* pWindowName, const char* pFlag)
{
#ifdef KDAB_MAC_PORT
    HWND window = ::FindWindowA(NULL, pWindowName);
    if (window)
    {
        *pEditorWindow = window;
        return true;
    }
    else
#endif // KDAB_MAC_PORT
    {
        *pEditorWindow = 0;

        wchar_t buffer[512] = { L'\0' };
        GetEditorExecutable(SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer)));

        SettingsManagerHelpers::CFixedString<wchar_t, 256> wFlags;
        SettingsManagerHelpers::ConvertUtf8ToUtf16(pFlag, wFlags.getBuffer());
        wFlags.setLength(wcslen(wFlags.c_str()));

        if (buffer[0] != '\0')
        {
#ifdef QT_VERSION
            if (QProcess::startDetached(QString::fromWCharArray(buffer), { QString::fromUtf8(pFlag) }))
#else
            INT_PTR hIns = (INT_PTR)ShellExecuteW(NULL, L"open", buffer, wFlags.c_str(), NULL, SW_SHOWNORMAL);
            if (hIns > 32)
#endif
            {
                return true;
            }
            else
            {
#ifdef KDAB_MAC_PORT
                MessageBoxA(0, "Editor.exe was not found.\n\nPlease verify CryENGINE root path.", "Error", MB_ICONERROR | MB_OK);
#endif // KDAB_MAC_PORT
            }
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
// Modified version of
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms684139(v=vs.85).aspx
bool CSettingsManagerTools::Is64bitWindows()
{
#if defined(_WIN64)
    // 64-bit programs run only on 64-bit Windows
    return true;
#elif !defined(KDAB_MAC_PORT)
    return false;
#else
    // 32-bit programs run on both 32-bit and 64-bit Windows
    static bool bWin64 = false;
    static bool bOnce = true;
    if (bOnce)
    {
        typedef BOOL (WINAPI * LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
        LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandleA("kernel32"), "IsWow64Process");
        if (fnIsWow64Process != NULL)
        {
            BOOL itIsWow64Process = FALSE;
            if (fnIsWow64Process(GetCurrentProcess(), &itIsWow64Process))
            {
                bWin64 = (itIsWow64Process == TRUE);
            }
        }
        bOnce = false;
    }
    return bWin64;
#endif
}

#endif // #if defined(CRY_ENABLE_RC_HELPER)

// eof
